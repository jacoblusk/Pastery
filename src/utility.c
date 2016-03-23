#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
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

struct pair *parse_pairs(const char *data) {
	struct pair *pairs = malloc(sizeof(struct pair));
	if(!pairs) {
		goto fail;  // allocation failure
	}
	pairs->name = NULL;
	pairs->value = NULL;
	if(!data) {
		return pairs;
	}
	size_t pairs_used = 1;
	size_t pairs_cap = 1;
	while(*data) {
		const char *pair_end = strchr(data, '&');
		if(pair_end == NULL) {
			pair_end = data + strlen(data);
		}
		if(pairs_used == SIZE_MAX) {
			return NULL;  // impossible overflow
		}
		pairs_used++;
		while(pairs_used > pairs_cap) {
			if(pairs_cap * 2 < pairs_cap) {
				goto fail;  // overflow
			}
			pairs_cap *= 2;
			if(pairs_cap >= SIZE_MAX / sizeof(struct pair)) {
				goto fail;  // overflow
			}
			struct pair *new_pairs = realloc(pairs,
				pairs_cap * sizeof(struct pair));
			if(new_pairs == NULL) {
				goto fail;  // allocation failure
			}
			pairs = new_pairs;
		}
		struct pair *pair = &pairs[pairs_used - 2];
		pair[1].name = NULL;
		pair[1].value = NULL;
		const char *equals = memchr(data, '=', pair_end - data);
		const char *name_end;
		const char *value_start;
		if(equals == NULL) {
			name_end = pair_end;
			value_start = NULL;
		}else{
			name_end = equals;
			value_start = equals + 1;
		}
		char *undec_name_copy = malloc(name_end - data + 1);
		if(undec_name_copy == NULL) {
			goto fail;  // allocation failure
		}
		memcpy(undec_name_copy, data, name_end - data);
		undec_name_copy[name_end - data] = '\0';
		char *name_copy = url_decode(undec_name_copy);
		if(name_copy == NULL) {
			// decode failure
			free(undec_name_copy), undec_name_copy = NULL;  // not pair-owned
			goto fail;
		}
		free(undec_name_copy), undec_name_copy = NULL;
		pair->name = name_copy;  // name now owned by pairs
		if(value_start) {
			char *undec_value_copy = malloc(pair_end - value_start + 1);
			if(undec_value_copy == NULL) {
				goto fail;  // allocation failure
			}
			memcpy(undec_value_copy, value_start, pair_end - value_start);
			undec_value_copy[pair_end - value_start] = '\0';
			char *value_copy = url_decode(undec_value_copy);
			if(value_copy == NULL) {
				// decode failure
				free(undec_value_copy), undec_value_copy = NULL;  // not pair-owned
				goto fail;
			}
			pair->value = value_copy;
		}
		data = *pair_end == '&' ? pair_end + 1 : pair_end;
	}
	return pairs;
fail:
	free_pairs(pairs);
	return NULL;
}

void free_pairs(struct pair *pairs) {
	if(!pairs) {
		return;
	}
	struct pair *orig = pairs;
	while(pairs->name) {
		free(pairs->name);
		free(pairs->value);
		pairs++;
	}
	free(orig);
}

char *get_pair_value(struct pair *pairs, const char *name) {
	for(; pairs->name; pairs++) {
		if(strcmp(pairs->name, name) == 0) {
			return pairs->value;
		}
	}
	return NULL;
}

void free_parsed_document(char **parts) {
	//TODO: fix bug here
	char **orig = parts;
	while(*parts) {
		free(*parts), *parts++ = NULL;
	}
	free(orig);
}

char **parse_document_uri(FCGX_Request *request, size_t *parts_length) {
	char *document_uri, *const_uri, **parts;
	char *ptr;
	size_t count, length, index;

	count = 0;
	const_uri = FCGX_GetParam("DOCUMENT_URI", request->envp);
	document_uri = malloc(strlen(const_uri) + 1);
	if(document_uri == NULL)
		return NULL;

	strcpy(document_uri, const_uri);
	ptr = document_uri;
	while(*ptr != '\0') {
		if(*(ptr++) == '/') count++;
	}

	length = ptr - document_uri - 1;
	if(document_uri[length] == '/') { 
		document_uri[length] = '\0';
		count--;
	}

	if(count == 0) 
		return NULL;
	if((count + 1) > SIZE_MAX / sizeof(char *)) {
		handle_warn("count would overflow");
		return NULL;
	}

	parts = malloc(sizeof(char *) * (count + 1));
	ptr = strtok(document_uri + 1, "/");
	parts[count] = NULL;
	index = 0;
	while(ptr != NULL && index <= count) {
		int strlength = strlen(ptr);
		if(strlength > 0) {
			parts[index] = malloc(sizeof(char) * strlength + 1);
			memcpy(parts[index], ptr, strlength);
			parts[index][strlength] = '\0';
			ptr = strtok(NULL, "/");
			index++;
		}
	}
	free(document_uri);
	*parts_length = count;
	return parts;
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
	source[amt_read] = '\0';
	fclose(file);
	return source;
}

char *read_body(FCGX_Request *request, size_t *length) {
	char *content_length_s;
	long content_length_l;
	size_t content_length, amt_read;
	char *body;

	content_length_s = FCGX_GetParam("CONTENT_LENGTH", request->envp);
	if(content_length_s == NULL)
		return NULL;
	content_length_l = strtol(content_length_s, NULL, 10);
	if(content_length_l <= 0)
		return NULL;
	content_length = content_length_l;
	if(content_length > SIZE_MAX / sizeof(char) - 1) {
		handle_warn("content length would overflow");
		return NULL;
	}
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
