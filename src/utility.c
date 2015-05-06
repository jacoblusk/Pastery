#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utility.h"
#include "strbuilder.h"
#include "error.h"

char *html_encode(const char *str) {
	struct strbuilder *builder;
	char *encoded_string;
	size_t length, pos;

	length = strlen(str);
	builder = strbuilder_create();
	for(pos = 0; pos <= length; pos++) {
		switch(str[pos]) {
			case '&':  strbuilder_append_str(builder, "&amp;");    break;
			case '\"': strbuilder_append_str(builder, "&quot;");   break;
			case '\'': strbuilder_append_str(builder, "&apos;");   break;
			case '<':  strbuilder_append_str(builder, "&lt;");     break;
			case '>':  strbuilder_append_str(builder, "&gt;");     break;
			default:   strbuilder_append_char(builder, str[pos]);  break;
		}
	}

	encoded_string = strbuilder_build(builder);
	strbuilder_destroy(builder);
	return encoded_string;
}

char *url_decode(const char *src) {
	char *dst, *ret_dst;
	char a, b;

	ret_dst = malloc(sizeof(char) * (strlen(src) + 1));
	if(ret_dst == NULL)
		handle_perror("malloc");
	dst = ret_dst;
	while (*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(isxdigit(a) && isxdigit(b))) {
			if (a >= 'a')
				a -= 'a'-'A';
			if (a >= 'A')
				a -= ('A' - 10);
			else
				a -= '0';
			if (b >= 'a')
				b -= 'a'-'A';
			if (b >= 'A')
				b -= ('A' - 10);
			else
				b -= '0';
			*dst++ = 16*a+b;
			src+=3;
		} else {
			if(*src == '+') {
				*dst++ = ' ';
				src++;
			} else
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
	return ret_dst;
}

char **parse_document_uri(FCGX_Request *request) {
	char *document_uri, **parts;
	char *ptr;
	size_t count, length;

	count = 0;
	document_uri = FCGX_GetParam("DOCUMENT_URI", request->envp);
	if(document_uri == NULL)
		return NULL;

	ptr = document_uri;
	while(*(ptr++) != '\0') {
		if(*ptr == '/') count++;
	}
	length = ptr - document_uri;
	printf("count: %d\n", count);
	return NULL;
}

char *read_file(const char *filename) {
	char *source;
	FILE *file;
	int buf_size;
	size_t amt_read;

	file = fopen(filename, "r");
	if(file == NULL)
		handle_perror("fopen");
	if(fseek(file, 0, SEEK_END) != 0)
		handle_perror("fseek");
	buf_size = ftell(file);
	if(buf_size == -1)
		handle_perror("ftell");
	source = malloc(sizeof(char) * (buf_size + 1));
	if(fseek(file, 0, SEEK_SET) != 0)
		handle_perror("fseek");
	amt_read = fread(source, sizeof(char), buf_size, file);
	if(amt_read == 0)
		handle_perror("fread");
	source[amt_read + 1] = '\0';
	fclose(file);
	return source;
}

char *read_body(FCGX_Request *request, size_t *length) {
	char *content_length_s;
	int content_length, amt_read;
	char *body;

	content_length_s = FCGX_GetParam("CONTENT_LENGTH", request->envp);
	if(content_length_s == NULL)
		return NULL;
	content_length = strtol(content_length_s, NULL, 10);
	if(content_length <= 0)
		return NULL;
	body = malloc(sizeof(char) * (content_length + 1));
	if(body == NULL)
		handle_perror("malloc");
	amt_read = FCGX_GetStr(body, content_length, request->in);
	if(amt_read != content_length)
		handle_warn("content length and amount read do not match!\n");
	if(length != NULL)
		*length = amt_read;
	body[content_length] = '\0';
	return body;
}
