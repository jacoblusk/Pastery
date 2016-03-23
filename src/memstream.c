#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memstream.h"

struct memstream {
	char **buffer;
	size_t *size;
};

int handle_write(void *cookie, const char *data, int size); 

FILE *open_memstream(char **buffer, size_t *size) {
	struct memstream *stream;
	
	stream = malloc(sizeof(struct memstream));
	if(stream == NULL)
		return NULL;

	stream->size = size;
	*buffer = NULL;
	stream->buffer = buffer;
	*size = 0;

	return fwopen(stream, handle_write);
}

int handle_write(void *cookie, const char *data, int size) {
	struct memstream *stream;
	size_t new_size;

	stream = cookie;
	if(size > 0 && *stream->size > SIZE_MAX - (size_t)size)
		return -1;
	new_size = sizeof(char) * size + *stream->size;
	char *new_buffer = realloc(*stream->buffer, new_size);
	memcpy(new_buffer + *stream->size, data, size); 
	if(new_buffer == NULL)
		return -1;
	*stream->buffer = new_buffer;
	*stream->size = new_size;
	return size;
}
