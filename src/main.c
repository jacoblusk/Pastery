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

#define PATH_SOCKET "/tmp/pastery.sock"
#define TEMPLATE_FILE "templates/index.tmpl"

int main(int argc, char **argv) {
	int socket, result;
	FCGX_Request request;
	TMPL_varlist *tmpl_list;
	char *tmpl_source;

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

	tmpl_source = read_file(TEMPLATE_FILE);
	tmpl_list = TMPL_add_var(NULL, "title", "Hello, world!", "body", "Hello, world!", NULL);

	while(FCGX_Accept_r(&request) == 0) {
		FILE *out;
		char *out_buf, *request_body, *decoded_body;
		size_t out_buf_size;

		out = open_memstream(&out_buf, &out_buf_size);
		if(out == NULL)
			handle_perror("open_memstream");

		fprintf(out, "Content-Type: text/html\r\n\r\n");
		TMPL_write(NULL, tmpl_source, NULL, tmpl_list, out, stderr); 
		fprintf(out, "\r\n");
		request_body = read_body(&request, NULL);
		if(request_body != NULL) {
			decoded_body = url_decode(request_body);
			printf("body: %s => %s\n", request_body, decoded_body);
		}
		parse_document_uri(&request);
		fclose(out);
		FCGX_FPrintF(request.out, out_buf);

		FCGX_Finish_r(&request);
		free(request_body);
		free(decoded_body);
		free(out_buf);
	}
	
	TMPL_free_varlist(tmpl_list);
	return EXIT_SUCCESS;
}
