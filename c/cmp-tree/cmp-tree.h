#ifndef CMP_TREE_H
#define CMP_TREE_H

#include <stdbool.h>

#include "better-strings.h"


typedef struct partial_file_cmp {
	bool cmp;
	unsigned int first_fm;
	unsigned int second_fm;
}PartialFileComparison;

typedef struct full_file_cmp {
	PartialFileComparison partial_cmp;
	String first_path;
	String second_path;
}FullFileComparison;


void * copy_function_String(void *s);
int compare_function_String(void *s1, void *s2);
void destroy_function_String(void *s);
void * copy_function_FullFileComparison(void *ffc);
int compare_function_FullFileComparison(void *ffc1, void *ffc2);
void destroy_function_FullFileComparison(void *ffc);

#endif
