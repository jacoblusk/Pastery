#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define handle_error(...) do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(EXIT_FAILURE); } \
	while(0)

#define handle_perror(msg) do { \
	perror(msg); \
	exit(EXIT_FAILURE); } \
	while(0)

#define handle_warn(...) do { \
	fprintf(stderr, __VA_ARGS__); } \
	while(0)

#endif
