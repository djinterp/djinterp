/******************************************************************************
* djinterp [test]                                              dfile_tests_sa.h
*
*   Unit tests for the dfile module (cross-platform file I/O).
*   Tests cover secure file opening, large file support, file descriptors,
* synchronization, locking, temporary files, metadata, directories, path
* utilities, symbolic links, pipes, and binary I/O helpers.
*
*
* path:      \inc\c\test\dfile_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/

#ifndef DJINTERP_DFILE_TESTS_SA_
#define DJINTERP_DFILE_TESTS_SA_ 1

#include "..\..\inc\c\test\test_standalone.h"
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\dmemory.h"
#include "..\..\inc\c\dfile.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_DFILE_TEMP_DIR
//   constant: base directory for test temporary files.
#define D_TEST_DFILE_TEMP_DIR       "dfile_test_tmp"

// D_TEST_DFILE_TEST_FILENAME
//   constant: standard test filename.
#define D_TEST_DFILE_TEST_FILENAME  "test_file.txt"

// D_TEST_DFILE_TEST_CONTENT
//   constant: standard test file content.
#define D_TEST_DFILE_TEST_CONTENT   "Hello, World!\nThis is a test file.\n"

// D_TEST_DFILE_LARGE_SIZE
//   constant: size for large file tests (4KB).
#define D_TEST_DFILE_LARGE_SIZE     4096

// D_INTERNAL_TEST_PATH_BUF_SIZE
//   constant: buffer size for test path construction.
#define D_INTERNAL_TEST_PATH_BUF_SIZE 512


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

// d_tests_dfile_setup
//   function: creates test environment (temporary directory and files).
// returns true on success, false on failure.
bool d_tests_dfile_setup(void);

// d_tests_dfile_teardown
//   function: cleans up test environment.
// returns true on success, false on failure.
bool d_tests_dfile_teardown(void);

// d_tests_dfile_get_test_path
//   function: constructs a path within the test directory.
// writes result to buffer and returns buffer, or NULL on failure.
char* d_tests_dfile_get_test_path(char*       _buf,
                                 size_t      _bufsize,
                                 const char* _filename);


/******************************************************************************
 * III. SECURE FILE OPENING TESTS
 *****************************************************************************/

// III. secure file opening tests
struct d_test_object* d_tests_dfile_fopen(void);
struct d_test_object* d_tests_dfile_fopen_s(void);
struct d_test_object* d_tests_dfile_freopen(void);
struct d_test_object* d_tests_dfile_freopen_s(void);
struct d_test_object* d_tests_dfile_fdopen(void);
struct d_test_object* d_tests_dfile_secure_file_opening_all(void);

// IV. large file support tests
struct d_test_object* d_tests_dfile_fseeko(void);
struct d_test_object* d_tests_dfile_ftello(void);
struct d_test_object* d_tests_dfile_ftruncate(void);
struct d_test_object* d_tests_dfile_ftruncate_stream(void);
struct d_test_object* d_tests_dfile_large_file_support_all(void);

// V. file descriptor operations tests
struct d_test_object* d_tests_dfile_fileno(void);
struct d_test_object* d_tests_dfile_dup(void);
struct d_test_object* d_tests_dfile_dup2(void);
struct d_test_object* d_tests_dfile_close(void);
struct d_test_object* d_tests_dfile_read_write(void);
struct d_test_object* d_tests_dfile_open(void);
struct d_test_object* d_tests_dfile_descriptor_operations_all(void);

// VI. file synchronization tests
struct d_test_object* d_tests_dfile_fsync(void);
struct d_test_object* d_tests_dfile_fsync_stream(void);
struct d_test_object* d_tests_dfile_fflush(void);
struct d_test_object* d_tests_dfile_synchronization_all(void);

// VII. file locking tests
struct d_test_object* d_tests_dfile_flock(void);
struct d_test_object* d_tests_dfile_flock_stream(void);
struct d_test_object* d_tests_dfile_locking_all(void);

// VIII. temporary file tests
struct d_test_object* d_tests_dfile_tmpfile(void);
struct d_test_object* d_tests_dfile_tmpfile_s(void);
struct d_test_object* d_tests_dfile_mkstemp(void);
struct d_test_object* d_tests_dfile_tmpnam_s(void);
struct d_test_object* d_tests_dfile_tempdir(void);
struct d_test_object* d_tests_dfile_temporary_files_all(void);

// IX. file metadata tests
struct d_test_object* d_tests_dfile_stat(void);
struct d_test_object* d_tests_dfile_fstat(void);
struct d_test_object* d_tests_dfile_lstat(void);
struct d_test_object* d_tests_dfile_access(void);
struct d_test_object* d_tests_dfile_chmod(void);
struct d_test_object* d_tests_dfile_file_size(void);
struct d_test_object* d_tests_dfile_file_size_stream(void);
struct d_test_object* d_tests_dfile_file_exists(void);
struct d_test_object* d_tests_dfile_is_file(void);
struct d_test_object* d_tests_dfile_is_dir(void);
struct d_test_object* d_tests_dfile_metadata_all(void);

// X. directory operations tests
struct d_test_object* d_tests_dfile_mkdir(void);
struct d_test_object* d_tests_dfile_mkdir_p(void);
struct d_test_object* d_tests_dfile_rmdir(void);
struct d_test_object* d_tests_dfile_opendir_readdir_closedir(void);
struct d_test_object* d_tests_dfile_rewinddir(void);
struct d_test_object* d_tests_dfile_directory_operations_all(void);

// XI. file operations tests
struct d_test_object* d_tests_dfile_remove(void);
struct d_test_object* d_tests_dfile_unlink(void);
struct d_test_object* d_tests_dfile_rename(void);
struct d_test_object* d_tests_dfile_copy_file(void);
struct d_test_object* d_tests_dfile_file_operations_all(void);

// XII. path utilities tests
struct d_test_object* d_tests_dfile_getcwd(void);
struct d_test_object* d_tests_dfile_chdir(void);
struct d_test_object* d_tests_dfile_realpath(void);
struct d_test_object* d_tests_dfile_dirname(void);
struct d_test_object* d_tests_dfile_basename(void);
struct d_test_object* d_tests_dfile_path_join(void);
struct d_test_object* d_tests_dfile_path_normalize(void);
struct d_test_object* d_tests_dfile_path_is_absolute(void);
struct d_test_object* d_tests_dfile_get_extension(void);
struct d_test_object* d_tests_dfile_path_utilities_all(void);

#if D_FILE_HAS_SYMLINKS
// XIII. symbolic link tests
struct d_test_object* d_tests_dfile_symlink(void);
struct d_test_object* d_tests_dfile_readlink(void);
struct d_test_object* d_tests_dfile_is_symlink(void);
struct d_test_object* d_tests_dfile_symbolic_links_all(void);
#endif

// XIV. pipe operations tests
struct d_test_object* d_tests_dfile_popen_pclose(void);
struct d_test_object* d_tests_dfile_pipe_operations_all(void);

// XV. binary I/O helpers tests
struct d_test_object* d_tests_dfile_fread_all(void);
struct d_test_object* d_tests_dfile_fwrite_all(void);
struct d_test_object* d_tests_dfile_fappend_all(void);
struct d_test_object* d_tests_dfile_binary_io_all(void);

// null parameter tests
struct d_test_object* d_tests_dfile_null_params_all(void);

// master test runner
struct d_test_object* d_tests_dfile_run_all(void);


#endif  // DJINTERP_DFILE_TESTS_SA_




