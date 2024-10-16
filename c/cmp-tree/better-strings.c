#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "better-strings.h"


String * create_string(char *s) {
	String *ret = malloc(sizeof(String));
	ret->length = strlen(s);
	ret->capacity = ret->length + 1; // + 1 for the null terminator
	ret->data = malloc(ret->capacity);
	memcpy(ret->data, s, ret->capacity);

	return ret;
}


void create_string_in_place(String *dst, char *s) {
	dst->length = strlen(s);
	dst->capacity = dst->length + 1; // + 1 for the null terminator
	dst->data = malloc(dst->capacity);
	memcpy(dst->data, s, dst->capacity);
}


void duplicate_string(String *dst, String *src) {
	dst->length = src->length;
	dst->capacity = src->capacity;
	dst->data = malloc(dst->capacity);
	memcpy(dst->data, src->data, src->capacity);
}


void destroy_string(String *s) {
	free(s->data);
}
