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
	String *ret;

	if (0 == root->length) {
		ret = malloc(extension->capacity);
		memcpy(ret->data, extension->data, extension->capacity);
		return ret;
	}

	if (root->data[root->length] == '/') {
		ret = malloc(root->length + extension->length + 1);
		memcpy(&ret->data[0], root->data, root->length);
		memcpy(&ret->data[root->length], extension->data, extension->capacity);
	} else {
		ret = malloc(root->length + extension->length + 2);
		memcpy(&ret->data[0], root->data, root->length);
		ret->data[root->length] = '/';
		memcpy(&ret->data[root->length], extension->data, extension->capacity);
	}

	return ret;
	/* }}} */
}


/** Returns an int represent whether the given file path points to a directory
 * or not.

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


// TODO:
/** Returns an int represent whether the given file path points to a directory
 * or not.

 * \param '*file_path' the file path to the (possible) directory to be checked.
 * \return negative int if there was an error, 0 if the file path leads to a
 *     directory, 1 if it does not.
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


// TODO: update this copy-pasted doc
/** Returns an unsorted vector list of file names for all files (including
 * hidden files) in a directory tree rooted at the directory pointed to by
 * '&dir_path'.

 * \param '&dir_path' the file path to the directory for which we wish
 *     to get a list of all the files in the directory tree.
 * \return an unsorted vector list of the relative file paths for all the files
 *     in the directory tree rooted at '&dir_path'.
 */
DynamicArray relative_files_in_tree(String *root, String *extension) {
	/* {{{ */
	DynamicArray ret;
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
			if (0 != str_eq(file_name.data, ".") && 0 != str_eq(file_name.data, "..")) {
				String *file_fp = path_extend(dir_path, &file_name);
				String *file_rp = path_extend(extension, &file_name);
				dynamic_array_push(&ret, file_rp->data);

				/* If the current element is a directory... */
				if (is_dir(file_fp->data)) {
					/* Recurse and append the sub directory relative file
					 * paths */
					DynamicArray sub_ret = relative_files_in_tree(root, file_rp);
					dynamic_array_concat(&ret, &sub_ret);
				}
			}
		}
		closedir(dir);
	/* If we are NOT able to open the directory successfully */
	} else {
		fprintf(stderr, "Was not able to open the directory\n");
	}

	return ret;
	/* }}} */
}


/** Returns an unsorted vector list file paths for all the files (in the broad
 * sense of the word, including links and directories, as well as hidden regular
 * files) in a directory tree rooted at the directory pointed to by '&root'.

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
		execlp("cmp", "cmp", first_path->data, second_path->data, (char *)NULL);

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

 * \param 'file_path' the file path to the (possible) directory to be checked.
 * \return negative int if there was an error, 0 if the file path leads to a
 *     directory, 1 if it does not.
 */
//
//std::vector<FullFileComparison> compare_directory_trees(String *first_root, \
//	String * second_root) {
DynamicArray compare_directory_trees(String *first_root, \
	String * second_root) {

	// std::vector<FullFileComparison> ret;
	DynamicArray ret;
	// /* Get the first directory file list and the second directory file list:
	//  * the list of files in each directory */
	// DynamicArray first_ft = files_in_tree(first_root);
	// DynamicArray second_ft = files_in_tree(second_root);

	// /* Create a vector that contains both the files from the first directory
	//  * tree and the files from the second directory tree */
	// DynamicArray combined_ft;
	// dynamic_array_concat(&combined_ft, &first_ft);
	// dynamic_array_concat(&combined_ft, &second_ft);
	// // TODO:
	// /* Sort the combined file tree and remove duplicate items */
	// std::sort(combined_ft.begin(), combined_ft.end());
	// auto last = std::unique(combined_ft.begin(), combined_ft.end());
	// combined_ft.erase(last, combined_ft.end());

	// /* Go through all the files in the combined  file list, create two full
	//  * paths to the file, one rooted at '&first_root', one rooted at
	//  * '&second_root', and compare them */
	// for (int i = 0; i < combined_ft.length; i++) {
	// 	String *first_file = path_extend(first_root, &combined_ft[i]);
	// 	String *second_file = path_extend(second_root, &combined_ft[i]);
	// 	FullFileComparison res;
	// 	res.partial_cmp = compare_path(first_file, second_file);
	// 	res.first_path = first_file;
	// 	res.second_path = second_file;
	// 	dynamic_array_push(&ret, res);
	// }

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
	for (int i = 0; i < sizeof(directory_args); i++) {
		/* Check if the given argument is a file path that points to something
		* that exists... */
		if (0 != is_dir(directory_args[i]->data)) {
			fprintf(stderr, "Provided directory (%s) does not exist or does " \
				"exist but is not a directory. Exiting...\n", \
				directory_args[i]->data);
			return -1;
		}
	}

	// auto comparisons = compare_directory_trees(first_path, second_path);
	// std::cout << "Comparisons:\n";
	// for (auto e: comparisons) {
	// 	std::cout \
	// 		<< std::get<0>(std::get<0>(e)) << ", " \
	// 		<< std::get<1>(e) << ", " \
	// 		<< std::get<2>(e) << "\n";
	// }
}
