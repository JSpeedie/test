/* C++ includes */
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <tuple>
#include <vector>

/* C includes */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;


// TODO: update this copy-pasted doc
/** Returns an unsorted vector list of file names for all files (including
 * hidden files) in a directory tree rooted at the directory pointed to by
 * '&dir_path'.

 * \param '&dir_path' the file path to the directory for which we wish
 *     to get a list of all the files in the directory tree.
 * \return an unsorted vector list of the relative file paths for all the files
 *     in the directory tree rooted at '&dir_path'.
 */
std::vector<fs::path> relative_files_in_tree( \
	fs::path &root, fs::path &extension) {

	std::vector<fs::path> ret;
	fs::path dir_path = root / extension;

	DIR *dir;
	/* If we are able to open the directory successfully */
	if ((dir = opendir(dir_path.c_str())) != NULL) {
		struct dirent *dir_entry;
		while ((dir_entry = readdir(dir)) != NULL) {
			fs::path file_name(dir_entry->d_name);
			/* Only includes files that are not the special "." and ".."
			 * entries */
			if (0 != file_name.compare(".") && 0 != file_name.compare("..")) {
				fs::path file_fp = dir_path / file_name;
				fs::path file_rp = extension / file_name;
				ret.push_back(file_rp);

				/* If the current element is a directory... */
				if (fs::is_directory(file_fp)) {
					/* Recurse and append the sub directory relative file
					 * paths */
					std::vector<fs::path> sub_dir_files = \
						relative_files_in_tree(root, file_rp);
					ret.insert(ret.end(), sub_dir_files.begin(), sub_dir_files.end());
				}
			}
		}
		closedir(dir);
	/* If we are NOT able to open the directory successfully */
	} else {
		std::cout << "Was not able to open the directory\n";
	}

	return ret;
}


/** Returns an unsorted vector list file paths for all the files (in the broad
 * sense of the word, including links and directories, as well as hidden regular
 * files) in a directory tree rooted at the directory pointed to by '&root'.

 * \param '&dir_path' the file path to the directory for which we wish to get
 *     a list of all the files in the directory tree.
 * \return an unsorted vector list of the relative file paths for all the files
 *     in the directory tree rooted at '&root'.
 */
std::vector<fs::path> files_in_tree(fs::path &root) {
	fs::path extension = "";
	return relative_files_in_tree(root, extension);
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
std::tuple<bool, fs::file_type, fs::file_type> \
compare_path( \
	fs::path &first_path, \
	fs::path &second_path) {

	/* Get the file types for both files */
	fs::file_type first_file_type = \
		fs::status(first_path).type();
	fs::file_type second_file_type = \
		fs::status(second_path).type();

	/* Initialize the return tuple */
	std::tuple<bool, fs::file_type, fs::file_type> \
		ret = std::make_tuple(false, first_file_type, second_file_type);

	/* If the two paths point to files that are of different types (e.g. a
	 * directory vs. a symlink, a fifo vs a regular file) then return early,
	 * with the match member set to false */
	if (first_file_type != second_file_type) {
		return ret;
	}

	pid_t child_pid = fork();

	/* If child process... */
	if (child_pid == 0) {
		/* Execute cmp to compare the two files */
		execlp("cmp", "cmp", first_path.c_str(), second_path.c_str(), (char *)NULL);

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
			std::get<0>(ret) = true;
		}
	}

	/* If 'cmp' failed (and because we checked whether the paths both pointed
	 * to the same kind of file earlier) we know that this means the two files
	 * are NOT byte-for-byte identical. Return false for the match member */
	return ret;
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
std::vector<std::tuple<std::tuple<bool, fs::file_type, fs::file_type>, fs::path, fs::path>>
compare_directory_trees( \
	fs::path &first_root, \
	fs::path &second_root) {

	std::vector<std::tuple<std::tuple<bool, fs::file_type, fs::file_type>, fs::path, fs::path>> ret;
	/* Get the first directory file list and the second directory file list:
	 * the list of files in each directory */
	std::vector<fs::path> first_ft = files_in_tree(first_root);
	std::vector<fs::path> second_ft = files_in_tree(second_root);

	/* Create a vector that contains both the files from the first directory
	 * tree and the files from the second directory tree */
	std::vector<fs::path> combined_ft(first_ft.begin(), first_ft.end());
	combined_ft.insert(combined_ft.end(), second_ft.begin(), second_ft.end());
	/* Sort the combined file tree and remove duplicate items */
	std::sort(combined_ft.begin(), combined_ft.end());
	auto last = std::unique(combined_ft.begin(), combined_ft.end());
	combined_ft.erase(last, combined_ft.end());

	/* Go through all the files in the combined  file list, create two full
	 * paths to the file, one rooted at '&first_root', one rooted at
	 * '&second_root', and compare them */
	for (auto &e: combined_ft) {
		fs::path first_file = first_root / e;
		fs::path second_file = second_root / e;
		std::tuple<std::tuple<bool, fs::file_type, fs::file_type>, fs::path, fs::path> res;
		std::get<0>(res) = compare_path(first_file, second_file);
		std::get<1>(res) = first_file;
		std::get<2>(res) = second_file;
		ret.push_back(res);
	}

	return ret;
}


int main(int argc, char **argv) {
	if (argc != 3) {
		std::cout << "Expected 2 arguments, received " << argc - 1 << "\n";
	}

	fs::path first_path(argv[1]);
	fs::path second_path(argv[2]);
	/* Create an array of arguments that specify directories so that
	 * we can check their validity */
	std::array<fs::path, 2> directory_args = \
		{ first_path, second_path };

	/* Loop through all the arguments that specify directories and check that
	 * they are valid */
	for (auto &e: directory_args) {
		/* Check if the given argument is a file path that points to something
		* that exists... */
		if (!fs::exists(e)) {
			std::cout << "Provided directory (" << e << \
				") does not exist. Exiting...\n";
			return -1;
		} else {
			/* ... and that it points to a directory */
			if (!fs::is_directory(e)) {
				std::cout << "Provided directory (" << e << \
					") is not a directory. Exiting...\n";
				return -1;
			}
		}
	}

	auto comparisons = compare_directory_trees(first_path, second_path);
	std::cout << "Comparisons:\n";
	for (auto e: comparisons) {
		std::cout \
			<< std::get<0>(std::get<0>(e)) << ", " \
			<< std::get<1>(e) << ", " \
			<< std::get<2>(e) << "\n";
	}
}
