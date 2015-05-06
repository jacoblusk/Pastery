#ifndef STRBUILDER_H
#define STRBUILDER_H

#include <stdlib.h>

#define DEFAULT_CAPACITY 256

struct strbuilder {
	char *string;
	size_t length;
	size_t capacity;
};

struct strbuilder *strbuilder_create(); 
void strbuilder_destroy(struct strbuilder *builder);
void strbuilder_append_str(struct strbuilder *builder, const char *str); 
void strbuilder_append_char(struct strbuilder *builder, const char c);
char *strbuilder_build(struct strbuilder *builder); 

#endif
