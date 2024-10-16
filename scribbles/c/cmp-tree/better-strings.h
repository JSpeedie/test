#ifndef BETTER_STRINGS_H
#define BETTER_STRINGS_H

#include <stddef.h>


typedef struct string {
	char *data;
	size_t length;
	size_t capacity;
}String;


String * create_string(char *s);
void duplicate_string(String *dst, String *src);
void create_string_in_place(String *dst, char *s);
void destroy_string(String *s);

#endif
