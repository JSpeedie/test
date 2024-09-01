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

#endif
