#define NO_FCGI_DEFINES

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcgiapp.h>
#include <fcgi_stdio.h>
#include <ctemplate.h>
#include <hiredis/hiredis.h>

#include "utility.h"
#include "error.h"
#include "generator.h"
#include "memstream.h"
#include "pygments.h"

#define TITLE "Pastery"
#define PATH_SOCKET "/tmp/pastery.sock"
#define TEMPLATE_FILE "templates/index.tmpl"
#define KEY_LENGTH 7

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

void handle_get(FCGX_Request *request, FILE *out);
void handle_post(FCGX_Request *request, FILE *out);
char *get_value_from_redis(const char *value); 
char *get_language(char *postid); 

static redisContext *redis_context;

int main(int argc, char **argv) {
	int socket, result;
	FCGX_Request request;

	(void)argc, (void)argv;
	pygments_init();
	generator_init();
	struct timeval timeout = { 1, 500000 };
	redis_context = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
	if ( !redis_context || redis_context->err) {
		if(redis_context) {
			printf("Connection error: %s\n", redis_context->errstr);
			redisFree(redis_context);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		handle_error("unable to connect to redis");
	}

	socket = FCGX_OpenSocket(PATH_SOCKET, 10);
	if(socket == -1)
		handle_perror("FCGX_OpenSocket");
	result = chmod(PATH_SOCKET, 0777);
	if(result == -1)
		handle_perror("chmod");
	result = FCGX_Init();
	if(result != 0)
		handle_error("FCGX_Init: failed with %d\n", result);
	result = FCGX_InitRequest(&request, socket, 0);
	if(result != 0)
		handle_error("FCGX_InitRequest failed with %d\n", result);

	while(FCGX_Accept_r(&request) == 0) {
		FILE *out;
		char *out_buf, *method;
		size_t out_buf_size;

		out = open_memstream(&out_buf, &out_buf_size);
		if(out == NULL)
			handle_perror("open_memstream");

		method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		printf("method %s\n", method);
		if(method != NULL) {
			if(strcmp(method, "GET") == 0) {
				handle_get(&request, out);
			} else if(strcmp(method, "POST") == 0) {
				handle_post(&request, out);
			} else {
				fprintf(out, "Status-Code: 400\r\n\r\n\r\n");
			}
		}

		fclose(out);
		FCGX_PutStr(out_buf, out_buf_size, request.out);
		FCGX_Finish_r(&request);
		free(out_buf);
	}
	
	return EXIT_SUCCESS;
}

void handle_get(FCGX_Request *request, FILE *out) {
	TMPL_varlist *tmpl_var_list;
	char *tmpl_source;
	redisReply *reply;
	char **parts = NULL;
	size_t parts_length;
	char *highlighted = NULL;

	tmpl_source = read_file(TEMPLATE_FILE);
	tmpl_var_list = TMPL_add_var(NULL, "title", TITLE, NULL);
	parts = parse_document_uri(request, &parts_length);
	if(parts != NULL && parts_length >= 2) {
		char *postid = parts[1];
		char *language = get_language(postid);
		if(language != NULL) {
			printf("%s & %s\n", postid, language); 
		}
		char *reply = get_value_from_redis(postid);
		if(reply == NULL) {
			handle_error("error with redis command");
		}
		printf("reply: %s\n", reply);
		highlighted = pygmentize(reply, language);
		tmpl_var_list = TMPL_add_var(tmpl_var_list, "body",
			highlighted, NULL);
		tmpl_var_list = TMPL_add_var(tmpl_var_list, "postid", postid, NULL);
		tmpl_var_list = TMPL_add_var(tmpl_var_list, "mode", "view", NULL);
		free(reply);
	}
	fprintf(out, "Content-Type: text/html\r\n\r\n");
	TMPL_write(NULL, tmpl_source, NULL, tmpl_var_list, out, stderr);
	TMPL_free_varlist(tmpl_var_list);
	free(highlighted);
	fprintf(out, "\r\n");
	printf("test");
	free_parsed_document(parts);
}

void handle_post(FCGX_Request *request, FILE *out) {
	char *request_body = NULL, *key = NULL;
	struct pair *post_params = NULL;
	int sent_response = 0;
	redisReply *reply = NULL;

	request_body = read_body(request, NULL);
	do {
		if(request_body == NULL)
			break;
		post_params = parse_pairs(request_body);
		if(post_params == NULL)
			break;
		char *body = get_pair_value(post_params, "text");
		char *postid = get_pair_value(post_params, "postid");
		if(postid != NULL && body == NULL) {
			char *tmpl_source = NULL;
			printf("value: %s\n", postid);
			const char *value = get_value_from_redis(postid);
			if(value == NULL)
				break;
			TMPL_varlist *tmpl_var_list = NULL;
			tmpl_source = read_file(TEMPLATE_FILE);
			tmpl_var_list = TMPL_add_var(NULL, "title", TITLE, NULL);
			tmpl_var_list = TMPL_add_var(tmpl_var_list, "body",
					value, NULL);
			fprintf(out, "Content-Type: text/html\r\n\r\n");
			TMPL_write(NULL, tmpl_source, NULL, tmpl_var_list, out, stderr);
			TMPL_free_varlist(tmpl_var_list);
			break;
		}

		if(body == NULL)
			break;
		//Find an unused key
		for(;;) {
			key = generator_generate(KEY_LENGTH);
			reply = redisCommand(redis_context, "EXISTS %s", key);
			if(reply != NULL && reply->integer) {
				freeReplyObject(reply);
				free(key);
			}
			else if(reply == NULL) {
				handle_error("redis error");
			}
			else break;
		}

		sent_response = 1;
		reply = redisCommand(redis_context, "SET %s %s", key, body);
		fprintf(out, "Location: /fcgi/%s\r\n\r\n\r\n", key);
		printf("key: %s\n", key);
	} while(0);

	if(!sent_response) {
		fprintf(out, "Location: /fcgi/\r\n\r\n\r\n");
	}

	freeReplyObject(reply);
	free(key);
	free(request_body);
	free_pairs(post_params);
}

char *get_value_from_redis(const char *value) {
	redisReply *reply = NULL;
	char *reply_copy = NULL;
	reply = redisCommand(redis_context, "GET %s", value);
	if(reply == NULL || reply->str == NULL)
		return NULL;
	
	reply_copy = strdup(reply->str);
	freeReplyObject(reply);
	return reply_copy;
}

char *get_language(char *postid) {
	size_t length = 0;
	char *period;

	length = strlen(postid);
	period = strchr(postid, '.');
	if(period == NULL) {
		return NULL;
	}

	*period = '\0';
	return (period + 1);
}
