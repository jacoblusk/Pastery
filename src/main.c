#define NO_FCGI_DEFINES

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcgiapp.h>
#include <fcgi_stdio.h>
#include <ctemplate.h>

#include "utility.h"
#include "error.h"

#define TITLE "Pastery"
#define PATH_SOCKET "/tmp/pastery.sock"
#define TEMPLATE_FILE "templates/index.tmpl"

void handle_get(FCGX_Request *request, FILE *out);
void handle_post(FCGX_Request *request, FILE *out);

int main(int argc, char **argv) {
	int socket, result;
	FCGX_Request request;

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
		FCGX_FPrintF(request.out, out_buf);
		FCGX_Finish_r(&request);
		free(out_buf);
	}
	
	return EXIT_SUCCESS;
}

void handle_get(FCGX_Request *request, FILE *out) {
	TMPL_varlist *tmpl_var_list;
	char *tmpl_source;

	tmpl_source = read_file(TEMPLATE_FILE);
	tmpl_var_list = TMPL_add_var(NULL, "title", TITLE, NULL);
	fprintf(out, "Content-Type: text/html\r\n\r\n");
	TMPL_write(NULL, tmpl_source, NULL, tmpl_var_list, out, stderr);
	TMPL_free_varlist(tmpl_var_list);
	fprintf(out, "\r\n");
}

void handle_post(FCGX_Request *request, FILE *out) {
	char *request_body, *decoded_body;

	fprintf(out, "Location: /test\r\n\r\n\r\n");
	request_body = read_body(request, NULL);
	if(request_body != NULL) {
		decoded_body = url_decode(request_body);
		printf("body: %s => %s\n", request_body, decoded_body);
	}
	parse_document_uri(request);

	free(request_body);
	free(decoded_body);
}
