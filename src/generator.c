#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

const char VOWELS[] = "aeiou";
const char CONSONANTS[] = "bcdfghjkmnpqrstvwxyz";

void generator_init() {
	srand(time(NULL));
}

char *generator_generate(int length) {
	char *key = malloc(sizeof(char) * (length + 1));
	if(key == NULL)
		handle_error("malloc");

	int start = rand() % 2;
	for(int i = 0; i < length; i++) {
		key[i] = (i % 2 == start) ?
			VOWELS[rand() % (sizeof(VOWELS) - 1)] : CONSONANTS[rand() % (sizeof(CONSONANTS) - 1)];
	}
	key[length] = '\0';
	return key;
}


