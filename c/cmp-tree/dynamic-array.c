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
 * \param '*copy_function' a function pointer that the dynamic array will use
 *     to copy a value into the dynamic array.
 * \param '*destroy_function' a function pointer that the dynamic array will
 *     use to destroy a value in the dynamic array.
 * \return 0 on success, -1 on failure.
 */
int dynamic_array_init(DynamicArray *da, size_t initial_capacity, \
	void * (*copy_function)(void *), int (*compare_function)(void *, void *), \
	void (*destroy_function)(void *)) {

	/* {{{ */
	da->length = 0;
	da->capacity = initial_capacity;
	da->copy_function = copy_function;
	da->compare_function = compare_function;
	da->destroy_function = destroy_function;
	da->array = malloc(sizeof(void *) * initial_capacity);
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
int dynamic_array_push(DynamicArray *da, void *new_element) {
	/* {{{ */
	/* If the dynamic array has the capacity to simply insert the new element
	 * without resizing */
	if (da->length + 1 <= da->capacity) {
		/* Insert the new element at the end of the array */
		da->array[da->length] = da->copy_function(new_element);
		da->length += 1;

		return 0;
	/* If the dynamic array does NOT have the capacity to insert a new
	 * element without resizing */
	} else {
		/* Grow the array */
		da->capacity = (2 * da->capacity) + 1; /* 2n + 1 resizing */
		da->array = realloc(da->array, sizeof(void *) * da->capacity);
		if (da->array == NULL) return -1;

		/* Then insert the new element at the end of the array */
		da->array[da->length] = da->copy_function(new_element);
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
 * \param '*base' a pointer to a DynamicArray representing the dynamic array
 *     which will have the second dynamic array appended to it.
 * \param '*extra' a pointer to a second DynamicArray which will be appended
 *     to the first one.
 * \return 0 if the value was successfully appended or -1 if there was an
 *     error.
 */
int dynamic_array_concat(DynamicArray *base, DynamicArray *extra) {
	/* {{{ */
	/* If the dynamic array has the capacity to simply append the second array
	 * without resizing */
	if (base->length + extra->length <= base->capacity) {
		/* Copy each element in '*extra' to the end of '*base' */
		for (int i = 0; i < extra->length; i++) {
			base->array[base->length] = \
				base->copy_function(extra->array[i]);
			base->length += 1;
		}

		return 0;
	/* If the dynamic array does NOT have the capacity to append the '*extra'
	 * dynamic array without resizing */
	} else {
		/* Grow the array */
		base->capacity = base->length + extra->length;
		base->array = \
			realloc(base->array, sizeof(void *) * base->capacity);
		if (base->array == NULL) return -1;

		/* Copy each element in '*extra' to the end of '*base' */
		for (int i = 0; i < extra->length; i++) {
			base->array[base->length] = \
				base->copy_function(extra->array[i]);
			base->length += 1;
		}

		return 0;
	}
	/* }}} */
}


/** Takes a DynamicArray and produces a slice of it which includes only
 * only the elements found in the original from indices 'start' to 'finish',
 * inclusive. Note that this array will not dupl
 *
 * \param '*slice' a pointer to the DynamicArraySlice this function will
 *     initialize.
 * \param '*da' a pointer to a DynamicArray representing the dynamic array
 *     which will be sliced.
 * \param 'start' an index in '*da' where our slice will start.
 * \param 'finish' an index in '*da' where our slice will end.
 * \return 0 if the dynamic array was successfully sliced, and -1 if
 *     there was an error.
 */
void dynamic_array_slice_init(DynamicArraySlice *slice, DynamicArray *da, \
	size_t start, size_t finish) {
	/* {{{ */
	slice->da = da;
	slice->start = start;
	slice->end = finish;
	/* }}} */
}


/** Takes a slice of a dynamic array and merge sorts it in non-decreasing
 * order.
 *
 * \param '*slice' a pointer to a slice of a DynamicArray representing the
 *     part of the dynamic array which will be merge sorted.
 * \return 0 if the dynamic array slice was successfully merge sorted, and -1
 *     if there was an error.
 */
int _dynamic_array_merge_sort(DynamicArraySlice *slice) {
	/* {{{ */
	size_t slice_len = slice->end - slice->start + 1;
	/* BASE CASE 1: If there are 0 elements or only 1 element in the slice */
	if (slice_len <= 1) {
		return 0;
	/* BASE CASE 2: If there are only 2 elements in the slice */
	} else if (slice_len == 2) {
		void ** slice_da_arr = slice->da->array;
		/* If they are already in non-decreasing order */
		// TODO: some problem with this function call
		if (0 >= slice->da->compare_function(slice_da_arr[slice->start], \
			slice_da_arr[slice->end]) ) {

			return 0;
		} else {
			/* Swap the 2 elements */
			void * temp = slice_da_arr[slice->start];
			slice_da_arr[slice->start] = slice_da_arr[slice->end];
			slice_da_arr[slice->end] = temp;

			return 0;
		}
	/* If there are more than 2 elements in the slice */
	} else {
		/* Recurse and merge sort the two halfs of the slice */
		DynamicArraySlice first_half;
		DynamicArraySlice second_half;
		dynamic_array_slice_init(&first_half, slice->da, \
			slice->start, slice->start + (slice_len / 2));
		dynamic_array_slice_init(&second_half, slice->da, \
			slice->start + (slice_len / 2) + 1, slice->end);
		_dynamic_array_merge_sort(&first_half);
		_dynamic_array_merge_sort(&second_half);

		void ** slice_da_arr = slice->da->array;

		/* Zip together the two sorted sections */
		size_t l = 0;
		size_t r = slice_len / 2;
		void * temp[slice_len];

		/* Zip together the two sorted sections into a temporary array */
		for (int i = 0; i < slice_len; i++) {
			if (r >= slice_len) {
				temp[i] = slice_da_arr[l];
				l++;
				continue;
			}
			if (l >= slice_len / 2) {
				temp[i] = slice_da_arr[r];
				r++;
				continue;
			}
			/* If slice_da_arr[l] <= slice_da_arr[r] */
			if (0 >= slice->da->compare_function(slice_da_arr[l], slice_da_arr[r])) {
				temp[i] = slice_da_arr[l];
				l++;
			} else {
				temp[i] = slice_da_arr[r];
				r++;
			}
		}

		/* Copy the contents of the sorted temporary array to the original
		 * array */
		for (int i = 0; i < slice_len; i++) {
			slice_da_arr[i] = temp[i];
		}
		
		return 0;
	}
	/* }}} */
}


/** Takes a dynamic array and sorts it in non-decreasing order.
 *
 * \param '*da' a pointer to a DynamicArray representing the dynamic array
 *     which will be sorted.
 * \return 0 if the dynamic array was successfully sorted, and -1 if there was
 *     an error.
 */
int dynamic_array_sort(DynamicArray *da) {
	/* {{{ */
	DynamicArraySlice slice;
	dynamic_array_slice_init(&slice, da, 0, da->length - 1);

	return _dynamic_array_merge_sort(&slice);
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
		da->destroy_function(da->array[i]);
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


// TODO: Plan:
// 1. Create a dynamic_array_merge_sort function for sorting dynamic arrays
//     a. Make dynamic arrays take a comparison function that returns 0 when
//        the two items compared and -1, 1 if one is greater than the other
//     b. Use that comparison function to implementat a type-indepedent merge
//        sort function
// 2. Create a dynamic_array_unique function that removes adjacent repeated
//    items.
//     a. This will rely on that comparison function returning 0 if the compared
//        items are equal.
