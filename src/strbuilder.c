#include <string.h>
#include <stdlib.h>
#include "strbuilder.h"
#include "error.h"

struct strbuilder *strbuilder_create() {
	struct strbuilder *builder = malloc(sizeof(struct strbuilder));
	if(builder == NULL)
		handle_error("malloc: NULL\n");
	builder->length = 0;
	builder->capacity = 0;
	builder->string = NULL;
	return builder;
}

void strbuilder_destroy(struct strbuilder *builder) {
	free(builder->string);
	free(builder);
}

void strbuilder_append_str(struct strbuilder *builder, const char *str) {
	size_t length = strlen(str);
	if((builder->length + length) > builder->capacity) {
		size_t new_capacity = builder->capacity + DEFAULT_CAPACITY + length;
		builder->string = realloc(builder->string, new_capacity);
		if(builder->string == NULL)
			handle_error("realloc: NULL\n");
		builder->capacity = new_capacity;
	}

	memcpy(builder->string + builder->length, str, length);
	builder->length += length;
}

void strbuilder_append_char(struct strbuilder *builder, const char c) {
	if((builder->length + 1) > builder->capacity) {
		size_t new_capacity = builder->capacity + DEFAULT_CAPACITY + 1;
		builder->string = realloc(builder->string, new_capacity);
		if(builder->string == NULL)
			handle_error("realloc: NULL\n");
		builder->capacity = new_capacity;
	}

	builder->string[builder->length] = c;
	builder->length++;
}

char *strbuilder_build(struct strbuilder *builder) {
	char *string = malloc(sizeof(char) * (builder->length + 1));
	if(string == NULL)
		handle_error("malloc: NULL\n");
	memcpy(string, builder->string, builder->length);
	string[builder->length + 1] = '\0';
	return string;
}
