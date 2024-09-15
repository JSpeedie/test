#ifndef CMP_TREE_H
#define CMP_TREE_H

#include <stdbool.h>

#include "better-strings.h"


#define MIN_COMPARISONS_PER_THREAD 60


enum FileCmp {
	/* For when the two files (understood in the broad sense) match. For regular
	 * files, this indicates that the two files are byte-for-byte identical.
	 * For directories, this mean they both exist. */
	MATCH,
	/* For when the two files (understood in the broad sense) mismatch in their
	 * type (e.g. one is a directory, one is a regular file). */
	MISMATCH_TYPE,
	/* For when the two files (understood in the broad sense) match in their
	 * type, but mismatch in their content (e.g. both are regular files, but
	 * they are not byte-for-byte identical). */
	MISMATCH_CONTENT,
	/* For when neither of the two files (understood in the broad sense)
	 * exist. */
	MISMATCH_NEITHER_EXISTS,
	/* For when only the first of the two files (understood in the broad sense)
	 * exists. */
	MISMATCH_ONLY_FIRST_EXISTS,
	/* For when only the second of the two files (understood in the broad sense)
	 * exists. */
	MISMATCH_ONLY_SECOND_EXISTS,
};


typedef struct partial_file_cmp {
	enum FileCmp file_cmp;
	unsigned int first_fm;
	unsigned int second_fm;
}PartialFileComparison;

typedef struct full_file_cmp {
	PartialFileComparison partial_cmp;
	String first_path;
	String second_path;
}FullFileComparison;

typedef struct cdt_thread_args {
	String * first_root;
	String * second_root;
	void ** rel_paths;
	size_t start;
	size_t end;
	void ** ret_ffcs;
}CDTThreadArgs;


void * copy_function_String(void *s);
int compare_function_String(void *s1, void *s2);
void destroy_function_String(void *s);
void * copy_function_FullFileComparison(void *ffc);
int compare_function_FullFileComparison(void *ffc1, void *ffc2);
void destroy_function_FullFileComparison(void *ffc);

#endif
