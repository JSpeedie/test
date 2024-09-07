#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmp-tree.h"
#include "better-strings.h"
#include "dynamic-array.h"


/* For printing coloured output */
char NOTHING[] = { "" };
char BOLD[] = { "\x1B[1m" };
char NORMAL[] = { "\x1B[0m" };
char RED[] = { "\x1B[31m" };
char GREEN[] = { "\x1B[32m" };
char YELLOW[] = { "\x1B[33m" };
char BLUE[] = { "\x1B[34m" };
char MAGENTA[] = { "\x1B[35m" };
char CYAN[] = { "\x1B[36m" };
char WHITE[] = { "\x1B[37m" };


/* duplicate/destroy PartialFileComparison functions {{{ */
void duplicate_partial_file_comparison(PartialFileComparison *dst, \
	PartialFileComparison *src) {

	dst->cmp = src->cmp;
	dst->first_fm = src->first_fm;
	dst->second_fm = src->second_fm;
}


void destroy_partial_file_comparison(PartialFileComparison *fc) {
	return;
}
/* }}} */


/* duplicate/destroy FullFileComparison functions {{{ */
void duplicate_full_file_comparison(FullFileComparison *dst, \
	FullFileComparison *src) {

	duplicate_partial_file_comparison(&dst->partial_cmp, &src->partial_cmp);
	duplicate_string(&dst->first_path, &src->first_path);
	duplicate_string(&dst->second_path, &src->second_path);
}


void destroy_full_file_comparison(FullFileComparison *fc) {
	destroy_partial_file_comparison(&fc->partial_cmp);
	destroy_string(&fc->first_path);
	destroy_string(&fc->second_path);
}
/* }}} */


/* DynamicArray helper copy/compare/destroy String functions {{{ */
void * copy_function_String(void *s) {
	String *ret = malloc(sizeof(String));
	String *src = (String *) s;
	duplicate_string(ret, src);

	return (void *) ret;
}


int compare_function_String(void *s1, void *s2) {
	String *string1 = (String *) s1;
	String *string2 = (String *) s2;

	size_t longest_strlen = strlen(string1->data);
	if (strlen(string2->data) > longest_strlen) 
		longest_strlen = strlen(string2->data);

	return strncmp(string1->data, string2->data, longest_strlen);
}


void destroy_function_String(void *s) {
	String *src = (String *) s;
	destroy_string(src);
}
/* }}} */


/* DynamicArray helper copy/compare/destroy FullFileComparison functions {{{ */
void * copy_function_FullFileComparison(void *ffc) {
	FullFileComparison *ret = malloc(sizeof(FullFileComparison));
	FullFileComparison *src = (FullFileComparison *) ffc;
	duplicate_full_file_comparison(ret, src);

	return (void *) ret;
}


int compare_function_FullFileComparison(void *ffc1, void *ffc2) {
	/* FullFileComparison *first_ffc = (FullFileComparison *) ffc1; */
	/* FullFileComparison *second_ffc = (FullFileComparison *) ffc2; */

	return 0;
}


void destroy_function_FullFileComparison(void *ffc) {
	FullFileComparison *src = (FullFileComparison *) ffc;
	destroy_full_file_comparison(src);
}
/* }}} */


int str_eq(char *s1, char *s2) {
	/* {{{ */
	if (strlen(s1) == strlen(s2) && 0 == strncmp(s1, s2, strlen(s1))) {
		return 0;
	} else {
		return -1;
	}
	/* }}} */
}


String *path_extend(String *root, String *extension) {
	/* {{{ */
	String *ret = malloc(sizeof(String));

	/* If the root is empty, return a copy of '*extension' */
	if (0 == root->length) {
		duplicate_string(ret, extension);
		return ret;
	}

	/* If the extension is empty, return a copy of '*root' */
	if (0 == extension->length) {
		duplicate_string(ret, root);
		return ret;
	}

	/* Return the extended path, inserting a '/' if '*root' does not already
	 * include one */
	if (root->data[root->length] == '/') {
		ret->data = malloc(root->length + extension->length + 1);
		ret->capacity = root->length + extension->length + 1;
		memcpy(&ret->data[0], root->data, root->length);
		memcpy(&ret->data[root->length], extension->data, extension->capacity);
		ret->length = strlen(ret->data);
	} else {
		ret->data = malloc(root->length + extension->length + 2);
		ret->capacity = root->length + extension->length + 2;
		memcpy(&ret->data[0], root->data, root->length);
		ret->data[root->length] = '/';
		memcpy(&ret->data[root->length + 1], \
			extension->data, \
			extension->capacity);
		ret->length = strlen(ret->data);
	}

	return ret;
	/* }}} */
}


/** Returns an int representing whether the given file path points to a
 * directory or not.
 *
 * \param '*file_path' the file path to the (possible) directory to be checked.
 * \return negative int if there was an error, 0 if the file path leads to a
 *     directory, 1 if it does not.
 */
int is_dir(char *file_path) {
	/* {{{ */
	struct stat file_info;

	if(stat(file_path, &file_info) != 0) {
		/* stat() failed, return -1 */
		return -1;
	}

	if (S_ISDIR(file_info.st_mode)) {
		return 0;
	}

	/* If we make it to this line, '*file_path' does NOT point to a dir */
	return 1;
	/* }}} */
}


/** Returns an int representing the mode (or type) of the file pointed to by
 * the file path '*file_path'.
 *
 * \param '*file_path' a file path which points to the file we wish to get the
 *     file mode of.
 * \param '*ret' a return variable which (on success) will be modified to
 *     contain an unsigned integer representing the mode (or type) of the file.
 *     This value can be compared to the 'S_IFDIR', 'S_IFREG', etc. constants
 *     provided by <sys/stat.h> to confirm which type the file is.
 * \return negative int if there was an error, 0 on success.
 */
int get_file_mode(char *file_path, unsigned int *ret) {
	/* {{{ */
	struct stat file_info;

	if(stat(file_path, &file_info) != 0) {
		/* stat() failed, return -1 */
		return -1;
	}

	*ret =  file_info.st_mode & S_IFMT;
	return 0;
	/* }}} */
}


/** Returns an unsorted vector list of file names for all files (including
 * hidden files) in a directory tree rooted at the directory pointed to by
 * '&dir_path'. Paths will be missing their dirname so as to facilitate
 * appending these relative filepaths to both the first directory tree root
 * and the second directory tree root.
 *
 * \param '*root' the file path to the directory tree root.
 * \param '*extension' a relative file path that when added to the '*root'
 *    file path (using 'path_extend()') creates a full file path to the current
 *    directory which will have its contents added to the return array.
 * \return an unsorted vector list of the relative file paths for all the files
 *     in the directory tree rooted at the file path created by extending
 *     '*root' with '*extension'.
 */
DynamicArray relative_files_in_tree(String *root, String *extension) {
	/* {{{ */
	/* DynamicArray<String> */
	DynamicArray ret;
	dynamic_array_init(&ret, 2, &copy_function_String, \
		&compare_function_String, &destroy_function_String);
	String *dir_path = path_extend(root, extension);

	DIR *dir;
	/* If we are able to open the directory successfully */
	if ((dir = opendir(dir_path->data)) != NULL) {
		struct dirent *dir_entry;
		while ((dir_entry = readdir(dir)) != NULL) {
			String file_name;
			create_string_in_place(&file_name, dir_entry->d_name);
			/* Only includes files that are not the special "." and ".."
			 * entries */
			if (0 != str_eq(file_name.data, ".") \
					&& 0 != str_eq(file_name.data, "..")) {

				String *file_fp = path_extend(dir_path, &file_name);
				String *file_rp = path_extend(extension, &file_name);
				dynamic_array_push(&ret, file_rp);

				/* If the current element is a directory... */
				if (0 == is_dir(file_fp->data)) {
					/* Recurse and append the sub directory relative file
					 * paths */
					DynamicArray sub_ret = \
						relative_files_in_tree(root, file_rp);
					dynamic_array_concat(&ret, &sub_ret);
				}
			}
		}
		closedir(dir);
	/* If we are NOT able to open the directory successfully */
	} else {
		fprintf(stderr, "Was not able to open the directory \"%s\"\n", \
			dir_path->data);
	}

	return ret;
	/* }}} */
}


/** Returns an unsorted vector list file paths for all the files (in the broad
 * sense of the word, including links and directories, as well as hidden
 * regular files) in a directory tree rooted at the directory pointed to by
 * '&root'.
 *
 * \param '&dir_path' the file path to the directory for which we wish to get
 *     a list of all the files in the directory tree.
 * \return an unsorted vector list of the relative file paths for all the files
 *     in the directory tree rooted at '&root'.
 */
DynamicArray files_in_tree(String *root) {
	/* {{{ */
	String *extension = create_string("");
	return relative_files_in_tree(root, extension);
	/* }}} */
}


/** Takes two paths and returns an bool that represents whether
 * the two files pointed to by the two paths are the same or different. If the
 * first member in the tuple is 0, the 2 paths point to identical files (in the
 * broad sense of the word).
 *
 * \param '&first_path' a file path that points to the first file we wish to
 *     compare.
 * \param '&second_path' a file path that points to the second file we wish to
 *     compare.
 * \return a file path that points to the second file we wish to
 *     compare.
 */
PartialFileComparison compare_path(String *first_path, String *second_path) {
	/* {{{ */
	/* Get the file types for both files */
	PartialFileComparison ret;
	ret.cmp = false;
	get_file_mode(first_path->data, &ret.first_fm);
	get_file_mode(second_path->data, &ret.second_fm);

	/* If the two paths point to files that are of different types (e.g. a
	 * directory vs. a symlink, a fifo vs a regular file) then return early,
	 * with the match member set to false */
	if (ret.first_fm != ret.second_fm) {
		return ret;
	}

	pid_t child_pid = fork();

	/* If child process... */
	if (child_pid == 0) {
		/* Execute cmp to compare the two files */
		execlp("cmp", "cmp", first_path->data, second_path->data, \
			(char *) NULL);

		/* If this point is reached, the execlp() command failed */
		fprintf(stderr, "ERROR: cmp call failed\n");
		exit(-1);
	/* If parent process... */
	} else {
		int r;
		waitpid(child_pid, &r, 0);

		/* If 'cmp' succeeded we know that this means the two files are
		 * byte-for-byte identical. Return true for the match member */
		if (r == 0) {
			ret.cmp = true;
		}
	}

	/* If 'cmp' failed (and because we checked whether the paths both pointed
	 * to the same kind of file earlier) we know that this means the two files
	 * are NOT byte-for-byte identical. Return false for the match member */
	return ret;
	/* }}} */
}


/** Returns an unsorted vector list of tuples where the first member represents
 * the result of a comparison between the file represented by the second member
 * and the file represented by the third member. If the first member is 0,
 * the files pointed to by the second and third members are byte-for-byte
 * identical. If the first member is negative, there was an error, and if the
 * first member is positive, the files were different in some way.
 *
 * \param 'file_path' the file path to the (possible) directory to be checked.
 * \return negative int if there was an error, 0 if the file path leads to a
 *     directory, 1 if it does not.
 */
/* DynamicArray<FullFileComparison> */
DynamicArray *compare_directory_trees(String *first_root, \
	String * second_root) {

	/* DynamicArray<FullFileComparison> */
	DynamicArray *ret = malloc(sizeof(DynamicArray));
	dynamic_array_init(ret, 2, &copy_function_FullFileComparison, \
		&compare_function_FullFileComparison, \
		&destroy_function_FullFileComparison);
	/* Get the first directory file list and the second directory file list:
	 * the list of files in each directory */
	/* DynamicArray<String> */
	DynamicArray first_ft = files_in_tree(first_root);
	DynamicArray second_ft = files_in_tree(second_root);

	/* Create a vector that contains both the files from the first directory
	 * tree and the files from the second directory tree */
	/* DynamicArray<String> */
	DynamicArray combined_ft;
	dynamic_array_init(&combined_ft, 2, &copy_function_String, \
		&compare_function_String, &destroy_function_String);
	dynamic_array_concat(&combined_ft, &first_ft);
	dynamic_array_concat(&combined_ft, &second_ft);

	/* Sort the combined file tree and remove duplicate items */
	dynamic_array_sort(&combined_ft);
	/* Remove adjacent duplicate items in the dynamic array */
	dynamic_array_unique(&combined_ft);

	/* Go through all the files in the combined  file list, create two full
	 * paths to the file, one rooted at '&first_root', one rooted at
	 * '&second_root', and compare them */
	for (int i = 0; i < combined_ft.length; i++) {
		String *first_file = \
			path_extend(first_root, (String *) combined_ft.array[i]);
		String *second_file = \
			path_extend(second_root, (String *) combined_ft.array[i]);
		FullFileComparison res;
		res.partial_cmp = compare_path(first_file, second_file);
		duplicate_string(&res.first_path, first_file);
		duplicate_string(&res.second_path, second_file);
		dynamic_array_push(ret, &res);
	}

	return ret;
}


int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stdout, "Expected 2 arguments, received %d\n", argc - 1);
	}

	String *first_path = create_string(argv[1]);
	String *second_path = create_string(argv[2]);
	/* Create an array of arguments that specify directories so that
	 * we can check their validity */
	String *directory_args[2] = { first_path, second_path };

	/* Loop through all the arguments that specify directories and check that
	 * they are valid */
	size_t num_dirs = sizeof(directory_args)/sizeof(directory_args[0]);
	for (int i = 0; i < num_dirs; i++) {
		/* Check if the given argument is a file path that points to something
		* that exists... */
		if (0 != is_dir(directory_args[i]->data)) {
			fprintf(stderr, "Provided directory (%s) does not exist or does " \
				"exist but is not a directory. Exiting...\n", \
				directory_args[i]->data);
			return -1;
		}
	}

	/* Compare the directory trees! */
	DynamicArray *comparisons = \
		compare_directory_trees(first_path, second_path);

	long max_num_file_matches = 0;
	long max_num_dir_matches = 0;
	long num_file_matches = 0;
	long num_dir_matches = 0;

	for (int i = 0; i < comparisons->length; i++) {
		FullFileComparison *ffc = (FullFileComparison *) comparisons->array[i];

		if (ffc->partial_cmp.first_fm == S_IFDIR \
			|| ffc->partial_cmp.second_fm == S_IFDIR) {

			max_num_dir_matches++;
		}
		if (ffc->partial_cmp.first_fm == S_IFREG \
			|| ffc->partial_cmp.second_fm == S_IFREG) {

			max_num_file_matches++;
		}

		if (ffc->partial_cmp.first_fm == S_IFREG \
			&& ffc->partial_cmp.second_fm == S_IFREG) {

			if (ffc->partial_cmp.cmp == true) {
				printf("%s%s\"%s\" == \"%s\"%s\n",
					BOLD, GREEN, ffc->first_path.data, ffc->second_path.data, \
					NORMAL);
				num_file_matches++;
			} else {
				printf("%s%s\"%s\" differs from \"%s\"%s\n",
					BOLD, RED, ffc->first_path.data, ffc->second_path.data, \
					NORMAL);
			}
		} else if (ffc->partial_cmp.first_fm == S_IFDIR \
			&& ffc->partial_cmp.second_fm == S_IFDIR) {

			printf("%s%s\"%s\" == \"%s\"%s\n",
				BOLD, GREEN, ffc->first_path.data, ffc->second_path.data, \
				NORMAL);
			num_dir_matches++;
		} else if (ffc->partial_cmp.first_fm != ffc->partial_cmp.second_fm) {

			// TODO: currently this program calls this printf if one
			// file exists and the other doesn't. You need to have some sort
			// of check for that. Maybe make an enum that represents the
			// outcome of a comparison rather than using stdbool. You can
			// have an enum for "mismatching types", "first file exists but
			// the second file does not", "first file does not exist but
			// second file does", "files are byte for byte identical",
			// "both files were dirs", etc. By using an enum, you can move
			// all these strange file mode/type checks into your
			// comparison functions and avoid calling cmp on mismatching types
			// or dirs. This might end up simplifying your structs since
			// the file mode/types only need to be known prior to the setting
			// of the enum value.
			printf("%s%s\"%s\" is not of the same type as \"%s\"%s\n",
				BOLD, RED, ffc->first_path.data, ffc->second_path.data, \
				NORMAL);
		} else {
			printf("%s%s\"%s\" and \"%s\" are both neither a " \
				"file nor a directory. This is currently not supported." \
				"%s\n", BOLD, RED, ffc->first_path.data, \
				ffc->second_path.data, NORMAL);
		}

		/* fprintf(stdout, "%d, \"%s\", \"%s\"\n", ffc->partial_cmp.cmp, \ */
		/* 	ffc->first_path.data, ffc->second_path.data); */
	}

	fprintf(stdout, "All done!\n");
	fprintf(stdout, "File byte-for-byte matches: %ld/%ld\n", \
		num_file_matches, max_num_file_matches);
	fprintf(stdout, "Directory matches: %ld/%ld\n", num_dir_matches, \
		max_num_dir_matches);
}
