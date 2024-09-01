#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dynamic-array.h"


/** Initializes the dynamic array with an initial capacity of
 * 'initial_capacity'.
 *
 * \param '*da' a pointer to a DynamicArray representing the dynamic array to
 *     be initialized.
 * \param 'initial_capacity' the number of elements the dynamic array should
 *     have space for.
 * \return 0 on success, -1 on failure.
 */
int dynamic_array_init(DynamicArray *da, size_t initial_capacity) {

	/* {{{ */
	da->length = 0;
	da->capacity = initial_capacity;
	da->array = malloc(sizeof(String) * initial_capacity);
	if (da->array == NULL) return -1;

	return 0;
	/* }}} */
}


/** Takes a new value and appends it to the dynamic array, resizing the array
 * if necessary.
 *
 * \param '*da' a pointer to a DynamicArray representing the dynamic array
 *     which will have the new element appended to it.
 * \param '*new_element' a new element to be added to the dynamic array.
 * \return 0 if the value was successfully appended or -1 if there was an
 *     error.
 */
int dynamic_array_push(DynamicArray *da, char *new_element) {
	/* {{{ */
	/* If the dynamic array has the capacity to simply insert the new element
	 * without resizing */
	if (da->length + 1 <= da->capacity) {
		/* Insert the new element at the end of the array */
		create_string_in_place(&da->array[da->length], new_element);
		da->length += 1;

		return 0;
	/* If the dynamic array does NOT have the capacity to insert a new
	 * element without resizing */
	} else {
		/* Grow the array */
		da->capacity = (2 * da->capacity) + 1; /* 2n + 1 resizing */
		da->array = realloc(da->array, sizeof(String) * da->capacity);
		if (da->array == NULL) return -1;

		/* Then insert the new element at the end of the array */
		create_string_in_place(&da->array[da->length], new_element);
		da->length += 1;

		return 0;
	}
	/* }}} */
}


// /** Takes a new value and inserts it at the given index in the dynamic array,
//  * growing the array if necessary.
//  *
//  * \param '*da' a pointer to a DynamicArray representing the dynamic array
//  *     which will have the new element inserted into it.
//  * \param '*value' a pointer to the value to be added to the dynamic array.
//  * \param 'index' the index in the dynamic array at which 'value' will be
//  *     inserted. Valid values are 0 <= 'index' <= 'da->length'.
//  * \return 0 if the value was successfully inserted or -1 if there was an
//  *     error.
//  */
// int dynamic_array_insert(DynamicArray *da, void * value, size_t index) {
// 	/* {{{ */
// 	/* Limit check 'index' */
// 	if (index < 0 || index > da->length) return -1;
// 
// 	/* If the dynamic array has the capacity to simply insert the new value
// 	 * without resizing */
// 	if (da->length + 1 <= da->capacity) {
// 		/* Shift to the right by 1 all the elements that come after the index
// 		 * for insertion */
// 		for (size_t i = index; i < da->length; i++) {
// 			memcpy(&da->array[da->length - i], \
// 				&da->array[da->length - i - 1], \
// 				da->element_size);
// 		}
// 		/* Insert the new value at the end of the array */
// 		memcpy(&da->array[index], value, da->element_size);
// 		da->length += 1;
// 
// 		return 0;
// 	/* If the dynamic array does NOT have the capacity to insert a new
// 	 * element without resizing */
// 	} else {
// 		/* Grow the array */
// 		da->capacity = (2 * da->capacity) + 1; /* 2n + 1 resizing */
// 		da->array = realloc(da->array, da->element_size * da->capacity);
// 		if (da->array == NULL) return -1;
// 
// 		/* Shift to the right by 1 all the elements that come after the index
// 		 * for insertion */
// 		for (size_t i = index; i < da->length; i++) {
// 			memcpy(&da->array[da->length - i], \
// 				&da->array[da->length - i - 1], \
// 				da->element_size);
// 		}
// 		/* Insert the new value at the end of the array */
// 		memcpy(&da->array[index], value, da->element_size);
// 		da->length += 1;
// 
// 		return 0;
// 	}
// 	/* }}} */
// }


/** Takes two dynamic arrays and appends the second one to the first one,
 * resizing if necessary.
 *
 * \param '*target' a pointer to a DynamicArray representing the dynamic array
 *     which will have the second dynamic array appended to it.
 * \param '*extra' a pointer to a second DynamicArray which will be appended
 *     to the first one.
 * \return 0 if the value was successfully appended or -1 if there was an
 *     error.
 */
int dynamic_array_concat(DynamicArray *target, DynamicArray *extra) {
	/* {{{ */
	/* If the dynamic array has the capacity to simply append the second array
	 * without resizing */
	if (target->length + extra->length <= target->capacity) {
		/* Copy each element in '*extra' to the end of '*target' */
		for (int i = 0; i < extra->length; i++) {
			duplicate_string(&target->array[target->length], &extra->array[i]);
			target->length += 1;
		}

		return 0;
	/* If the dynamic array does NOT have the capacity to append the '*extra'
	 * dynamic array without resizing */
	} else {
		/* Grow the array */
		target->capacity = target->length + extra->length;
		target->array = realloc(target->array, sizeof(String) * target->capacity);
		if (target->array == NULL) return -1;

		/* Copy each element in '*extra' to the end of '*target' */
		for (int i = 0; i < extra->length; i++) {
			duplicate_string(&target->array[target->length], &extra->array[i]);
			target->length += 1;
		}

		return 0;
	}
	/* }}} */
}


// /** Takes an index and removes the item at the index, shifting the elements
//  * that come after to the left and possibly shrinking the capacity of the
//  * array.
//  *
//  * \param '*da' a pointer to a DynamicArray representing the dynamic array
//  *     which will have an element removed from it.
//  * \param 'index' the index in the dynamic array at which a value will be
//  *     removed. Valid values are 0 <= 'index' < 'da->length'.
//  * \return 0 if the value was successfully removed or -1 if there was an
//  *     error.
//  */
// int dynamic_array_remove(DynamicArray *da, size_t index) {
// 	/* {{{ */
// 	/* Limit check 'index' */
// 	if (index < 0 || index >= da->length) return -1;
// 
// 	/* Shift to the left by 1 all the elements that come after the index for
// 	 * insertion */
// 	for (size_t i = index; i < da->length - 1; i++) {
// 		memcpy(&da->array[i], &da->array[i + 1], da->element_size);
// 	}
// 	/* Chop off the last element */
// 	da->length -= 1;
// 
// 	/* If the number of array contents could fit in the array 2 resizes
// 	 * smaller */
// 	if (da->length <= (((da->capacity - 1) / 2) - 1) / 2) {
// 		/* Then shrink the array by 1 resize */
// 		da->capacity = (da->capacity - 1) / 2; /* 2n + 1 resizing */
// 		da->array = realloc(da->array, da->element_size * da->capacity);
// 		if (da->array == NULL) return -1;
// 	}
// 
// 	return 0;
// 	/* }}} */
// }


/** Free all heap-allocated memory associated with a dynamic array.
 *
 * \param '*da' a pointer to a DynamicArray representing the dynamic array to
 *     be destroyed.
 * \return void.
 */
void dynamic_array_destroy(DynamicArray *da) {
	/* {{{ */
	/* Destroy each String */
	for (int i = 0; i < da->length; i++) {
		destroy_string(&da->array[i]);
	}
	/* Free the heap-allocated array 'da->array', destroying the dynamic
	 * array */
	free(da->array);
	/* }}} */
}


// /** Print all the values in the dynamic array '*da'.
//  *
//  * \param '*da' a pointer to a DynamicArray representing the dynamic array that
//  *     will be printed.
//  * \return void.
//  */
// void print_dynamic_array_simple(DynamicArray *da) {
// 	/* {{{ */
// 	fprintf(stdout, "[");
// 
// 	for (int i = 0; i < da->length; i++) {
// 		if (i < da->length - 1) {
// 			fprintf(stdout, "%ld, ", da->array[i]);
// 		} else {
// 			fprintf(stdout, "%ld", da->array[i]);
// 		}
// 	}
// 
// 	fprintf(stdout, "]\n");
// 	/* }}} */
// }


// /** Print all the values in the dynamic array '*da' as well as all the empty
//  * indexes so we can see the capacity of the array.
//  *
//  * \param '*da' a pointer to a DynamicArray representing the dynamic array that
//  *     will be printed.
//  * \return void.
//  */
// void print_dynamic_array_verbose(DynamicArray *da) {
// 	/* {{{ */
// 	fprintf(stdout, "[");
// 
// 	for (int i = 0; i < da->length; i++) {
// 		fprintf(stdout, "%ld", da->array[i]);
// 
// 		/* If this is not the last index to be printed */
// 		if (i < da->capacity - 1) {
// 			/* Append a ", " */
// 			fprintf(stdout, ", ");
// 		}
// 	}
// 
// 	for (int i = 0; i < da->capacity - da->length; i++) {
// 		fprintf(stdout, "_");
// 
// 		/* If this is not the last index to be printed */
// 		if (i < da->capacity - da->length - 1) {
// 			/* Append a ", " */
// 			fprintf(stdout, ", ");
// 		}
// 	}
// 
// 	fprintf(stdout, "]\n");
// 	/* }}} */
// }
