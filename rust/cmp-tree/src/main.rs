use clap::{command, Arg};
use std::io::prelude::*;
use std::fs::File;
use std::path::{Path, PathBuf};


#[derive(Debug)]
enum RegFileCmp {
    Identical,
    DiffLength,
    DiffContents,
}


#[derive(Debug)]
enum FileCmp {
    /* For when the two files (understood in the broad sense) match. For regular files, this
     * indicates that the two files are byte-for-byte identical. For directories, this mean they
     * both exist. */
    Match,
    /* For when the two files (understood in the broad sense) mismatch in their type (e.g. one is a
     * directory, one is a regular file). */
    TypeMismatch,
    /* For when the two files (understood in the broad sense) match in their type, but mismatch in
     * their content (e.g. both are regular files, but they are not byte-for-byte identical). */
    ContentMismatch,
    /* For when neither of the two files (understood in the broad sense) exist. */
    NeitherFileExists,
    /* For when only the first of the two files (understood in the broad sense) exists. */
    OnlyFirstFileExists,
    /* For when only the second of the two files (understood in the broad sense) exists. */
    OnlySecondFileExists,
}


#[derive(Debug)]
struct PartialFileComparison {
    file_cmp: FileCmp,
    first_ft: Option<std::fs::FileType>,
    second_ft: Option<std::fs::FileType>,
}


#[derive(Debug)]
struct FullFileComparison {
    partial_cmp: PartialFileComparison,
    first_path: PathBuf,
    second_path: PathBuf,
}


/* For printing coloured output */
const NOTHING: &str = "";
const BOLD: &str = "\x1B[1m";
const NORMAL: &str = "\x1B[0m";
const RED: &str = "\x1B[31m";
const GREEN: &str = "\x1B[32m";
const YELLOW: &str = "\x1B[33m";
const BLUE: &str = "\x1B[34m";
const MAGENTA: &str = "\x1B[35m";
const CYAN: &str = "\x1B[36m";
const WHITE: &str = "\x1B[37m";


/// Returns an unsorted vector list of relative file paths for all files (in the broad sense of the
/// word, including links and directories, as well as hidden files) in a directory tree rooted at
/// the directory pointed to by the path `root` / `extension`. The file paths included in the
/// list will all begin with `extension`, but not `root`.
///
/// #### Parameters:
/// * `root` the beginning of the file path to the directory for which we wish to get a list of
///      all the files in the directory tree. It will be combined with `extension` to produce the
///      complete path.
/// * `extension` the end of the file path to the directory for which we wish to get a list of all
///     the files in the directory tree. It will be combined with `root` to produce the complete
///     path.
/// #### Return:
/// * an unsorted vector list of the relative file paths for all the files in the directory tree
///     rooted at `root` / `extension`. The file paths included in the list will omit `root` from
///     their path, but include `extension`.
fn relative_files_in_tree(root: &Path, extension: &Path) -> Vec<PathBuf> {
    /* {{{ */
    let full_dir_path = root.join(extension);
    let mut ret: Vec<PathBuf> = Vec::new();

    /* Get all the files in the dir relative to the 'root' directory */
    match std::fs::read_dir(full_dir_path) {
        Ok(dir_entries) => {
            for e in dir_entries {
                match e {
                    Ok(entry) => {
                        if let Ok(file_type) = entry.file_type() {
                            let rel_path: PathBuf = extension.join(entry.file_name());
                            ret.push(rel_path);

                            if file_type.is_dir() {
                                let subdir_rel_paths = relative_files_in_tree(root,
                                    &extension.join(entry.file_name()));
                                /* Append all the relative paths from the sub dir to our return
                                 * list */
                                ret.extend(subdir_rel_paths);
                            }
                        } else {
                            println!("Error getting the file type of the directory
                                entry");
                        }
                    },
                    Err(_) => {
                        println!("Error reading one of the directory entries");
                    }
                }
            }
        },
        Err(_) => {
            println!("Error reading contents of the directory");
        }
    }

    return ret;
    /* }}} */
}


/// Returns an unsorted vector list of file paths for all the files (in the broad sense of the
/// word, including links and directories, as well as hidden files) in a directory tree rooted at
/// the directory pointed to by 'root'.
///
/// #### Parameters:
/// * `root` the file path to the directory for which we wish to get a list of all the files in the
///     directory tree.
/// #### Return:
/// * an unsorted vector list of the relative file paths for all the files in the directory tree
///     rooted at `root`.
fn files_in_tree(root: &Path) -> Vec<PathBuf> {
	/* {{{ */
	let extension = Path::new("");
	return relative_files_in_tree(root, extension);
	/* }}} */
}


/// Takes two paths and a result representing how the files compare. Both file paths must point to
/// regular files and both regular files must exist.
///
/// #### Parameters:
/// * `first_path` a file path that points to the first file we wish to compare.
/// * `second_path` a file path that points to the second file we wish to compare.
/// #### Return:
/// * `Ok(RegFileCmp)` on success and `Err(())` on failure.
fn compare_regular_files(first_path: &Path, second_path: &Path) -> Result<RegFileCmp, ()> {
    /* {{{ */
    const BYTE_COUNT: usize = 8192;

    let first_file_res = File::open(first_path);
    let second_file_res = File::open(second_path);
    let mut first_file: File;
    let mut second_file: File;
    let mut first_buf = [0; BYTE_COUNT];
    let mut second_buf = [0; BYTE_COUNT];

    /* Check if the files differ in size. If they do, they cannot be byte-for-byte identical */
    match first_path.metadata() {
        Ok(first_md) => match second_path.metadata() {
            Ok(second_md) => {
                if first_md.len() != second_md.len() {
                    return Ok(RegFileCmp::DiffLength);
                }
            },
            Err(_) => return Err(()),
        },
        Err(_) => return Err(()),
    }

    if first_file_res.is_ok() && second_file_res.is_ok() {
        first_file = first_file_res.unwrap();
        second_file = second_file_res.unwrap();
    } else {
        return Err(());
    }

    loop {
        match first_file.read(&mut first_buf) {
            Ok(first_bytes_read) => match second_file.read(&mut second_buf) {
                Ok(second_bytes_read) => {
                    /* One file ended before the other */
                    if first_bytes_read != second_bytes_read {
                        return Ok(RegFileCmp::DiffLength);
                    }
                    /* If both reads read 0 bytes, that means we have hit the end of both files and
                     * the two files are identical */
                    if first_bytes_read == 0 && second_bytes_read == 0 {
                        return Ok(RegFileCmp::Identical);
                    }
                    // TODO: is this a (1) functional and a (2) good equivalent for C++ memcmp?
                    if first_buf != second_buf {
                        return Ok(RegFileCmp::DiffContents);
                    }
                },
                Err(_) => {
                    return Err(());
                }
            },
            Err(_) => {
                return Err(());
            }
        }
    }
    /* }}} */
}


/// Takes two paths and returns a `Result` that either contains a `PartialFileComparison` that
/// represents how the two files (understood in the broad sense) pointed to by the two paths
/// compare or an `Err` indicating that an error occurred in the process of comparing the two
/// files.
///
/// #### Parameters:
/// * `first_path` a file path that points to the first file we wish to compare.
/// * `second_path` a file path that points to the second file we wish to compare.
/// #### Return:
/// * a `PartialFileComparison` that represents whether the two files are equivalent, if they
///     differ and how they differ, as well as the two file types of the files.
fn compare_files(first_path: &Path, second_path: &Path) -> Result<PartialFileComparison, ()> {
    /* {{{ */
    /* Check file existences first. If neither path points to files that exist, return that neither
     * exists. If one file exists, but the other does not, get the file mode/type of the existing
     * file and return, setting the comparison member so that the caller knows which file does not
     * exist */
    if !first_path.exists() && !second_path.exists() {
        return Ok(PartialFileComparison {
            first_ft: None,
            second_ft: None,
            file_cmp: FileCmp::NeitherFileExists,
        });
    } else if first_path.exists() && !second_path.exists() {
        match first_path.metadata() {
            Ok(md) => {
                return Ok(PartialFileComparison {
                    first_ft: Some(md.file_type()),
                    second_ft: None,
                    file_cmp: FileCmp::OnlyFirstFileExists,
                });
            },
            Err(_) => return Err(())
        }
    } else if !first_path.exists() && second_path.exists() {
        match second_path.metadata() {
            Ok(md) => {
                return Ok(PartialFileComparison {
                    first_ft: None,
                    second_ft: Some(md.file_type()),
                    file_cmp: FileCmp::OnlySecondFileExists,
                });
            },
            Err(_) => return Err(())
        }
    }

    /* Check file modes/types. At this point we know both files exist, but if they are of different
     * types (e.g. a fifo vs a regular file) then return with the two file modes/types and setting
     * the comparison member so the caller knows the types of the two files */
    let first_filetype;
    let second_filetype;

    match first_path.metadata() {
        Ok(md) => first_filetype = Some(md.file_type()),
        Err(_) => return Err(()),
    }
    match second_path.metadata() {
        Ok(md) => second_filetype = Some(md.file_type()),
        Err(_) => return Err(()),
    }

    /* If the two paths point to files that are of different types (e.g. a directory vs. a symlink,
     * a fifo vs a regular file) then return early */
    if first_filetype != second_filetype {
        return Ok(PartialFileComparison {
            first_ft: first_filetype,
            second_ft: second_filetype,
            file_cmp: FileCmp::TypeMismatch,
        });
    }

    /* Check that the two files are equivalent. At this point we know both files exist and that
     * they are of the same type. The various types the files could both be need individual methods
     * for checking equivalence. Regular files check that every byte of their contents are
     * identical, directories will simply return a match since both files are directories */
    if first_filetype.unwrap().is_dir() {
        return Ok(PartialFileComparison {
            first_ft: first_filetype,
            second_ft: second_filetype,
            file_cmp: FileCmp::Match,
        });
    } else if first_filetype.unwrap().is_file() {
        /* Call the regular file specific comparison function and return accordingly */
        match compare_regular_files(first_path, second_path) {
            Ok(RegFileCmp::Identical) => {
                return Ok(PartialFileComparison {
                    first_ft: first_filetype,
                    second_ft: second_filetype,
                    file_cmp: FileCmp::Match,
                });
            },
            Ok(RegFileCmp::DiffLength | RegFileCmp::DiffContents) => {
                return Ok(PartialFileComparison {
                    first_ft: first_filetype,
                    second_ft: second_filetype,
                    file_cmp: FileCmp::ContentMismatch,
                });
            },
            Err(_) => return Err(())
        }
    /* TODO: Other file types do not yet have support. At the moment, they are treated the same way
     * directories are: if they both exist, and are of the same type, return that they match. */
    } else {
        return Ok(PartialFileComparison {
            first_ft: first_filetype,
            second_ft: second_filetype,
            file_cmp: FileCmp::Match,
        });
    }
    /* }}} */
}


fn compare_directory_trees(first_root: &Path, second_root: &Path) -> 
    Result<Vec<FullFileComparison>, ()> {
    /* {{{ */

    let mut ret = Vec::new();
    /* Get the first directory file list and the second directory file list:
     * the list of files in each directory */
    let first_ft = files_in_tree(first_root);
    let second_ft = files_in_tree(second_root);

    /* Combine all the relative paths from the first and second directory roots into one combined
     * list of relative paths */
    let mut combined_ft = Vec::new();
    combined_ft.extend(first_ft);
    combined_ft.extend(second_ft);
    /* Sort the combined file tree and remove duplicate items */
    combined_ft.sort();
    combined_ft.dedup();

    /* Go through all the file paths in the combined  file list, creating two full paths to the
     * file, one rooted at `first_root`, one rooted at `second_root`, and compare them */
    for e in &combined_ft {
        let first_path = first_root.join(e);
        let second_path = second_root.join(e);

        let cmp_res = compare_files(&first_path, &second_path);

        if cmp_res.is_ok() {
            ret.push(
                FullFileComparison {
                    first_path: first_path,
                    second_path: second_path,
                    partial_cmp: cmp_res.unwrap(),
                }
            );
        }
    }

    return Ok(ret);
    /* }}} */
}


fn main() {
    let mut flag_print_totals: bool = false;
    let mut flag_print_matches: bool = false;
    // TODO: add the -p flag (pretty print) flag implementation
    let mut flag_pretty_output: bool = false;

    let match_result = command!()
        .arg(
            Arg::new("first_root_dir").required(true).index(1)
        )
        .arg(
            Arg::new("second_root_dir").required(true).index(2)
        )
        .arg(
            Arg::new("matches").short('m').long("matches").num_args(0)
        )
        .arg(
            Arg::new("pretty").short('p').long("pretty").num_args(0)
        )
        .arg(
            Arg::new("totals").short('t').long("totals").num_args(0)
        ).get_matches();

    let first_dir_arg = match_result.get_one::<String>("first_root_dir");
    let second_dir_arg = match_result.get_one::<String>("second_root_dir");

    if first_dir_arg.is_none() {
        println!("Expected 2 paths to 2 directories, received 0\n");
    }
    if second_dir_arg.is_none() {
        println!("Expected 2 paths to 2 directories, received 1\n");
    }

    /* These unwraps are guaranteed not to fail because we checked if the result was none
     * already */
    let first_dir = Path::new(first_dir_arg.unwrap());
    let second_dir = Path::new(second_dir_arg.unwrap());

    if match_result.get_flag("matches") {
        flag_print_matches = true;
    }
    if match_result.get_flag("pretty") {
        flag_pretty_output = true;
    }
    if match_result.get_flag("totals") {
        flag_print_totals = true;
    }

    let mut max_num_file_matches: u128 = 0;
    let mut max_num_dir_matches: u128 = 0;
    let mut num_file_matches: u128 = 0;
    let mut num_dir_matches: u128 = 0;

    match compare_directory_trees(first_dir, second_dir) {
        Ok(list) => {
            for e in list {
                if flag_print_totals {
                    match e.partial_cmp.first_ft {
                        Some(f_ft) => {
                            if f_ft.is_dir() {
                                max_num_dir_matches += 1;
                            } else {
                                match e.partial_cmp.second_ft {
                                    Some(s_ft) => {
                                        if s_ft.is_dir() {
                                            max_num_dir_matches += 1;
                                        }
                                    },
                                    None => (),
                                }
                            }
                        },
                        None => {
                            match e.partial_cmp.second_ft {
                                Some(s_ft) => {
                                    if s_ft.is_dir() {
                                        max_num_dir_matches += 1;
                                    }
                                },
                                None => (),
                            }
                        },
                    }
                    match e.partial_cmp.first_ft {
                        Some(f_ft) => {
                            if f_ft.is_file() {
                                max_num_file_matches += 1;
                            } else {
                                match e.partial_cmp.second_ft {
                                    Some(s_ft) => {
                                        if s_ft.is_file() {
                                            max_num_file_matches += 1;
                                        }
                                    },
                                    None => (),
                                }
                            }
                        },
                        None => {
                            match e.partial_cmp.second_ft {
                                Some(s_ft) => {
                                    if s_ft.is_file() {
                                        max_num_file_matches += 1;
                                    }
                                },
                                None => (),
                            }
                        },
                    }
                }

                match e.partial_cmp.file_cmp {
                    FileCmp::Match => {
                        if flag_print_matches {
                            if flag_pretty_output { print!("{BOLD}{GREEN}"); }
                            println!("{:?} == {:?}", e.first_path, e.second_path);
                            if flag_pretty_output { print!("{NORMAL}"); }
                        }
                        if e.partial_cmp.first_ft.unwrap().is_file() {
                            num_file_matches += 1;
                        } else if e.partial_cmp.first_ft.unwrap().is_dir() {
                            num_dir_matches += 1;
                        }
                    },
                    FileCmp::TypeMismatch => {
                        if flag_pretty_output { print!("{BOLD}{RED}"); }
                        println!("{:?} is not of the same type as {:?}", e.first_path,
                            e.second_path);
                        if flag_pretty_output { print!("{NORMAL}"); }
                    },
                    FileCmp::ContentMismatch => {
                        if flag_pretty_output { print!("{BOLD}{RED}"); }
                        println!("{:?} differs from {:?}", e.first_path, e.second_path);
                        if flag_pretty_output { print!("{NORMAL}"); }
                    },
                    FileCmp::NeitherFileExists => {
                        if flag_pretty_output { print!("{BOLD}{RED}"); }
                        println!("Neither {:?} nor {:?} exist", e.first_path, e.second_path);
                        if flag_pretty_output { print!("{NORMAL}"); }
                    },
                    FileCmp::OnlyFirstFileExists => {
                        if flag_pretty_output { print!("{BOLD}{RED}"); }
                        println!("{:?} exists, but {:?} does NOT exist", e.first_path, e.second_path);
                        if flag_pretty_output { print!("{NORMAL}"); }
                    },
                    FileCmp::OnlySecondFileExists => {
                        if flag_pretty_output { print!("{BOLD}{RED}"); }
                        println!("{:?} does NOT exist, but {:?} does exist", e.first_path,
                            e.second_path);
                        if flag_pretty_output { print!("{NORMAL}"); }
                    },
                }
            }
        },
        Err(_) => {
            println!("Error getting the list of comparisons between the two directory trees");
        }
    }

    if flag_print_totals {
        println!("All done!");
        println!("File byte-for-byte matches: {num_file_matches}/{max_num_file_matches}");
        println!("Directory matches: {num_dir_matches}/{max_num_dir_matches}");
    }
}
