#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "better-strings.h"


typedef struct dynamic_array {
	size_t capacity;
	size_t length;
	String *array;
}DynamicArray;


int dynamic_array_init(DynamicArray *da, size_t initial_capacity);
int dynamic_array_push(DynamicArray *da, char *new_element);
int dynamic_array_concat(DynamicArray *target, DynamicArray *extra);
void dynamic_array_destroy(DynamicArray *da);

#endif
