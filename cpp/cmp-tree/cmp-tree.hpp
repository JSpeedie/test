#ifndef CMP_TREE_HPP
#define CMP_TREE_HPP

/* C++ includes */
#include <filesystem>

namespace fs = std::filesystem;

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
	fs::file_type first_ft;
	fs::file_type second_ft;
}PartialFileComparison;

typedef struct full_file_cmp {
	PartialFileComparison partial_cmp;
	fs::path first_path;
	fs::path second_path;
}FullFileComparison;

#endif
