#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
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

	dst->file_cmp = src->file_cmp;
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
	if (root->data[root->length - 1] == '/') {
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


/** Returns an int representing whether the file (understood in the broad
 * sense) pointed to by the given path points to a file that exists.
 *
 * \param '*file_path' the file path to the (possible) file (understood in the
 *     broad sense) whose existence we wish to check.
 * \return negative int if there was an error or if the file path leads to a
 *     file which does not exist, and 0 if the file path leads to a file which
 *     exists.
 */
int exists(char *file_path) {
	/* {{{ */
	if (access(file_path, F_OK) == 0) {
		return 0;
	} else {
		return -1;
	}
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


// TODO: update doc from copy paste from cpp
/** Takes two paths and returns 0 if the files are byte-for-byte identical,
 * and -1 if they are not. Both file paths must point to regular files and
 * both regular files must exist.
 *
 * \param '&first_path' a file path that points to the first file we wish to
 *     compare.
 * \param '&second_path' a file path that points to the second file we wish to
 *     compare.
 * \return 0 if they files are byte-for-byte identical, -1 otherwise.
 */
int compare_files(char * first_path, char * second_path) {
	/* {{{ */
	/* Check if the files differ in size. If they do, they cannot be
	 * byte-for-byte identical */
	struct stat first_file_info;
	struct stat second_file_info;

	if(stat(first_path, &first_file_info) != 0) {
		/* stat() failed, return -1 */
		return -1;
	}

	if(stat(second_path, &second_file_info) != 0) {
		/* stat() failed, return -1 */
		return -1;
	}

	if (first_file_info.st_size != second_file_info.st_size) {
		return -1;
	}

	/* Read through both files simultaneously, comparing their bytes. If at any
	 * point two bytes at the same location in the files differ, return -1 */
	int first_file = open(first_path, O_RDONLY);
	if (first_file == -1) return -1;
	int second_file = open(second_path, O_RDONLY);
	if (second_file == -1) return -1;
	/* Advise the kernel that we will be reading these two files sequentially.
	 * This seem to have little to no effect on runtime, but that might be
	 * because my tests involved only small files. */
	posix_fadvise(first_file, 0, 0, 1);
	posix_fadvise(second_file, 0, 0, 1);
	/* Create a buffer of 8192 chars, all initialized to 0(?) */
	size_t num_bytes = 8192;
	/* char *first_buf = calloc(num_bytes, sizeof(char)); */
	char *first_buf = malloc(num_bytes);
	if (first_buf == NULL) return -1;
	/* char *second_buf = calloc(num_bytes, sizeof(char)); */
	char *second_buf = malloc(num_bytes);
	if (second_buf == NULL) return -1;

	size_t first_bytes_read = 0;
	size_t second_bytes_read = 0;
	while(0 < (first_bytes_read = read(first_file, first_buf, num_bytes))
		&& 0 < (second_bytes_read = read(second_file, second_buf, num_bytes)) ) {

		/* One file ended before the other */
		if (first_bytes_read != second_bytes_read) {
			return -1;
		}

		if (0 != memcmp(first_buf, second_buf, first_bytes_read)) {
			return -1;
		}
	}

	return 0;
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
	PartialFileComparison ret;

	/* Check file existences first. If neither path points to files that exist,
	 * return that neither exists. If one file exists, but the other does not,
	 * get the file mode/type of the existing file and return, setting the
	 * comparison member so that the caller knows which file does not exist */
	if (exists(first_path->data) != 0 && exists(second_path->data) != 0) {
		ret.file_cmp = MISMATCH_NEITHER_EXISTS;
		return ret;
	} else if (exists(first_path->data) == 0 && exists(second_path->data) != 0) {
		get_file_mode(first_path->data, &ret.first_fm);
		ret.file_cmp = MISMATCH_ONLY_FIRST_EXISTS;
		return ret;
	} else if (exists(first_path->data) != 0 && exists(second_path->data) == 0) {
		get_file_mode(second_path->data, &ret.second_fm);
		ret.file_cmp = MISMATCH_ONLY_SECOND_EXISTS;
		return ret;
	}

	/* Check file modes/types. At this point we know both files exist, but if
	 * they are of different types (e.g. a fifo vs a regular file) then
	 * return with the two file modes/types and setting the comparison member
	 * so the caller knows the types of the two files */
	get_file_mode(first_path->data, &ret.first_fm);
	get_file_mode(second_path->data, &ret.second_fm);

	if (ret.first_fm != ret.second_fm) {
		ret.file_cmp = MISMATCH_TYPE;
		return ret;
	}

	/* Check that the two files are equivalent. At this point we know both
	 * files exist and that they are of the same type. The various types the
	 * files could both be need individual methods for checking equivalence.
	 * Regular files will use 'cmp', directories will simply return a match
	 * since both files are directories */
	if (ret.first_fm == S_IFDIR) {
		ret.file_cmp = MATCH;
		return ret;
	} else if (ret.first_fm == S_IFREG) {
		/* If the file comparison succeeded we know that this means the two
		 * files are byte-for-byte identical. Return with the comparison
		 * member set to match */
		printf("about to compare %s and %s\n", first_path->data, second_path->data);
		if (compare_files(first_path->data, second_path->data) == 0) {
			ret.file_cmp = MATCH;
			return ret;
		} else {
			ret.file_cmp = MISMATCH_CONTENT;
			return ret;
		}
	/* TODO: Other file types do not yet have support. At the moment, they are
	 * treated the same way directories are: if they both exist, and are of
	 * the same type, return that they match. */
	} else {
		ret.file_cmp = MATCH;
		return ret;
	}
	/* }}} */
}


/** Helper function for compressing a file through multiple threads */
void *compare_directory_trees_thread(void *arg) {
	CDTThreadArgs *t = (CDTThreadArgs *) arg;

	/* Go through the assigned files in the combined  file list, create two
	 * full paths to the file, one rooted at '&first_root', one rooted at
	 * '&second_root', and compare them */
	for (size_t i = t->start; i <= t->end; i++) {
		String *first_file = \
			path_extend(t->first_root, (String *) (t->rel_paths[i]));
		String *second_file = \
			path_extend(t->second_root, (String *) (t->rel_paths[i]));
		FullFileComparison res;
		res.partial_cmp = compare_path(first_file, second_file);
		duplicate_string(&res.first_path, first_file);
		duplicate_string(&res.second_path, second_file);
		/* Store full file comparison result in the shared return array */
		t->ret_ffcs[i] = copy_function_FullFileComparison(&res);
	}

	return NULL;
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

	/* DynamicArray<FullFileComparison> */
	DynamicArray *ret = malloc(sizeof(DynamicArray));
	dynamic_array_init(ret, combined_ft.length, &copy_function_FullFileComparison, \
		&compare_function_FullFileComparison, \
		&destroy_function_FullFileComparison);

	// TODO: You can use this opportunity to write a references page for how to
	// write multithreaded code in C.
	
	char num_threads;
	size_t paths_per_thread;
	long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	if (number_of_processors < 1) number_of_processors = 1;

	// TODO: maybe what is slowing things down is that even when I limit the
	// multithreading to 1 thread, this program still does a lot of weird setup
	// work for that 1 thread that it doesn't have to do... if it's only 1
	// thread. I should test it back in original single threaded mode to see
	// what's up. For now though, max-core multithreading gives me a 185->165
	// millisecond improvement

	// TODO: remove
	number_of_processors = 1;

	/* If we divide all the paths among all the cores of the machine and
	 * that exceeds the minimum number of comparisons each thread must make,
	 * then use all the cores of the machine and have each do an equal amount
	 * of work. */
	if ((combined_ft.length / number_of_processors) \
		>= MIN_COMPARISONS_PER_THREAD) {

		num_threads = number_of_processors;
		if (num_threads < 1) num_threads = 1;
		// + 1 for ceil
		paths_per_thread = (combined_ft.length / num_threads) + 1;
	} else {
		num_threads = combined_ft.length / MIN_COMPARISONS_PER_THREAD;
		if (num_threads < 1) num_threads = 1;
		// + 1 for ceil
		paths_per_thread = (combined_ft.length / num_threads) + 1;
	}
	printf("%d thread(s) each comparing at most %ld paths\n", num_threads, paths_per_thread);
	size_t paths_to_assign = combined_ft.length;
	size_t paths_assigned = 0;
	CDTThreadArgs args[num_threads];

	/* Set thread arguments */
	for (int i = 0; i < num_threads; i++) {
		args[i].first_root = first_root;
		args[i].second_root = second_root;

		args[i].start = paths_assigned;
		paths_assigned += paths_per_thread;
		if (paths_assigned >= paths_to_assign) {
			paths_assigned = paths_to_assign - 1;
		}
		args[i].end = paths_assigned;
		paths_assigned++;

		args[i].rel_paths = combined_ft.array;
		args[i].ret_ffcs = ret->array;
	}

	/* Run Threads */
	pthread_t thread_id[num_threads];
	/* Create threads with their given tasks/arguments */
	for (int t = 0; t < num_threads - 1; t++) {
		if (0 != pthread_create(&thread_id[t], NULL, \
			compare_directory_trees_thread, &args[t])) {

			fprintf(stderr, "ERROR: Could not create threads\n");
		}
	}
	/* Have this "thread" do its work as well since otherwise it would be
	 * waiting idly */
	compare_directory_trees_thread(&args[num_threads - 1]);

	/* Wait for all the threads to finish their work */
	for (int t = 0; t < num_threads - 1; t++) {
		pthread_join(thread_id[t], NULL);
	}

	/* Hack to fix return array */
	ret->length = combined_ft.length;

	return ret;
}


int main(int argc, char **argv) {
	bool flag_print_totals = false;
	bool flag_print_matches = false;
	bool flag_pretty_output = false;

	int opt;
	struct option opt_table[] = {
		{ "matches",  no_argument,  NULL,  'm' },
		{ "pretty",   no_argument,  NULL,  'p' },
		{ "totals",   no_argument,  NULL,  't' },
		{ 0, 0, 0, 0 }
	};
	char opt_string[] = { "mpt" };

	while ((opt = getopt_long(argc, argv, opt_string, opt_table, NULL)) != -1) {
		switch (opt) {
			case 'm': flag_print_matches = true; break;
			case 'p': flag_pretty_output = true; break;
			case 't': flag_print_totals = true; break;
		}
	}

	/* If after parsing all the flags there aren't 2 arguments left */
	if (optind + 2 != argc) {
		fprintf(stdout, "Expected 2 arguments, received %d\n", argc - 1);
	}

	String *first_path = create_string(argv[optind]);
	optind++;
	String *second_path = create_string(argv[optind]);
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

		if (flag_print_totals) {
			if (ffc->partial_cmp.first_fm == S_IFDIR \
				|| ffc->partial_cmp.second_fm == S_IFDIR) {

				max_num_dir_matches++;
			}
			if (ffc->partial_cmp.first_fm == S_IFREG \
				|| ffc->partial_cmp.second_fm == S_IFREG) {

				max_num_file_matches++;
			}
		}

		switch (ffc->partial_cmp.file_cmp) {
			case MATCH:
				if (flag_print_matches) {
					if (flag_pretty_output) printf("%s%s", BOLD, GREEN);
					printf("\"%s\" == \"%s\"\n",
						ffc->first_path.data, ffc->second_path.data);
					if (flag_pretty_output) printf("%s", NORMAL);
				}
				if (ffc->partial_cmp.first_fm == S_IFREG) {
					num_file_matches++;
				} else if (ffc->partial_cmp.first_fm == S_IFDIR) {
					num_dir_matches++;
				}
				break;
			case MISMATCH_TYPE:
				if (flag_pretty_output) printf("%s%s", BOLD, RED);
				printf("\"%s\" is not of the same type as \"%s\"\n",
					ffc->first_path.data, ffc->second_path.data);
				if (flag_pretty_output) printf("%s", NORMAL);
				break;
			case MISMATCH_CONTENT:
				if (flag_pretty_output) printf("%s%s", BOLD, RED);
				printf("\"%s\" differs from \"%s\"\n",
					ffc->first_path.data, ffc->second_path.data);
				if (flag_pretty_output) printf("%s", NORMAL);
				break;
			case MISMATCH_NEITHER_EXISTS:
				if (flag_pretty_output) printf("%s%s", BOLD, RED);
				printf("Neither \"%s\" nor \"%s\" exist\n",
					ffc->first_path.data, ffc->second_path.data);
				if (flag_pretty_output) printf("%s", NORMAL);
				break;
			case MISMATCH_ONLY_FIRST_EXISTS:
				if (flag_pretty_output) printf("%s%s", BOLD, RED);
				printf("\"%s\" exists, but \"%s\" does NOT exist\n",
					ffc->first_path.data, ffc->second_path.data);
				if (flag_pretty_output) printf("%s", NORMAL);
				break;
			case MISMATCH_ONLY_SECOND_EXISTS:
				if (flag_pretty_output) printf("%s%s", BOLD, RED);
				printf("\"%s\" does NOT exist, but \"%s\" does exist\n",
					ffc->first_path.data, ffc->second_path.data);
				if (flag_pretty_output) printf("%s", NORMAL);
				break;
		}
	}

	if (flag_print_totals) {
		fprintf(stdout, "All done!\n");
		fprintf(stdout, "File byte-for-byte matches: %ld/%ld\n", \
			num_file_matches, max_num_file_matches);
		fprintf(stdout, "Directory matches: %ld/%ld\n", num_dir_matches, \
			max_num_dir_matches);
	}
}
