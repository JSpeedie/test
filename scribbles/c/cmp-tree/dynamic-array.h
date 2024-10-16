#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/** The idea here is that the array is an array of 'void *'s and that the user
 * of this array must:
 * 1. Cast an element from '*array' to the correct type when retrieving an
 *    element (using knowledge that the programmer knows and which cannot be
 *    corrected by the compiler since C does not (really) support generics. So
 *    while we can use the DynamicArray to store any type, any given
 *    DynamicArray does not explicitly state what type it stores and the
 *    programmer must be careful not to mix up two DynamicArrays that store
 *    different types of data.
 * 2. ...
 */
typedef struct dynamic_array {
	size_t capacity;
	size_t length;
	/* An array of 'void *'s */
	void **array;
	/* A provided copy function needs to take a pointer to an object the
	 * type of which the dynamic array stores and returns a new pointer to
	 * a duplicate of the object, malloc'd on the heap */
	void * (*copy_function)(void *);
	/* A provided compare function needs to take two pointers to two objects the
	 * type of which the dynamic array stores and returns 0 if they are equal,
	 * -1 if the first object is lesser and 1 if the second object is greater. */
	int (*compare_function)(void *, void *);
	/* A provided destroy function needs to take a pointer to an object (the
	 * type of which the dynamic array stores) and must deallocate all
	 * heap-allocated memory associated with the object NOT including the pointer
	 * to the object itself. */
	void (*destroy_function)(void *);
}DynamicArray;

typedef struct dynamic_array_slice {
	/* A pointer to the DynamicArray this slice is associated with */
	DynamicArray *da;
	/* The first index this slice includes */
	size_t start;
	/* The last index this slice includes */
	size_t end;
}DynamicArraySlice;


int dynamic_array_init(DynamicArray *da, size_t initial_capacity, \
	void * (*copy_function)(void *), int (*compare_function)(void *, void *), \
	void (*destroy_function)(void *));
int dynamic_array_grow(DynamicArray *da);
int dynamic_array_push(DynamicArray *da, void *new_element);
int dynamic_array_concat(DynamicArray *target, DynamicArray *extra);
void dynamic_array_slice_init(DynamicArraySlice *slice, DynamicArray *da, \
	size_t start, size_t finish);
int dynamic_array_sort(DynamicArray *da);
int dynamic_array_unique(DynamicArray *da);
void dynamic_array_destroy(DynamicArray *da);

#endif
