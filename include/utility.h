#ifndef UTILITY_H
#define UTILITY_H

#include <fcgiapp.h>

struct pair { char *name; char *value; };

char *html_encode(const char *str);
char *url_decode(const char *str);
struct pair *parse_pairs(const char *data);
void free_pairs(struct pair *pairs);
char *get_pair_value(struct pair *pairs, const char *name);
char *read_file(const char *filename);
char *read_body(FCGX_Request *request, size_t *length); 
char **parse_document_uri(FCGX_Request *request, size_t *length); 
void free_parsed_document(char **parts);

#endif
