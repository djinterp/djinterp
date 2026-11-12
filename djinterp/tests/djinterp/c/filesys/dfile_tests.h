/*******************************************************************************
* djinterp [c][filesys]                                            dfile_tests.h
*
*   Unit tests for the dfile module (cross-platform file I/O).
*   Tests cover secure file opening, large file support, file descriptors,
* synchronization, locking, temporary files, metadata, directories, path
* utilities, symbolic links, pipes, and binary I/O helpers.
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_FILESYS_DFILE_TESTS_H
#define DJINTERP_TESTS_C_FILESYS_DFILE_TESTS_H 1

// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../inc/djinterp/c/filesys/dfile.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face
#include "../../../../inc/djinterp/c/string_fn.h"


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
char* d_tests_dfile_get_test_path(
    char*       _buf,
    size_t      _bufsize,
    const char* _filename
);


/******************************************************************************
 * III. SECURE FILE OPENING TESTS
 *****************************************************************************/

// III. secure file opening tests
bool d_tests_dfile_fopen(struct dj_test_suite* _suite);

bool d_tests_dfile_fopen_s(struct dj_test_suite* _suite);

bool d_tests_dfile_freopen(struct dj_test_suite* _suite);

bool d_tests_dfile_freopen_s(struct dj_test_suite* _suite);

bool d_tests_dfile_fdopen(struct dj_test_suite* _suite);

bool d_tests_dfile_secure_file_opening_all(struct dj_test_suite* _suite);

// IV. large file support tests
bool d_tests_dfile_fseeko(struct dj_test_suite* _suite);

bool d_tests_dfile_ftello(struct dj_test_suite* _suite);

bool d_tests_dfile_ftruncate(struct dj_test_suite* _suite);

bool d_tests_dfile_ftruncate_stream(struct dj_test_suite* _suite);

bool d_tests_dfile_large_file_support_all(struct dj_test_suite* _suite);

// V. file descriptor operations tests
bool d_tests_dfile_fileno(struct dj_test_suite* _suite);

bool d_tests_dfile_dup(struct dj_test_suite* _suite);

bool d_tests_dfile_dup2(struct dj_test_suite* _suite);

bool d_tests_dfile_close(struct dj_test_suite* _suite);

bool d_tests_dfile_read_write(struct dj_test_suite* _suite);

bool d_tests_dfile_open(struct dj_test_suite* _suite);

bool d_tests_dfile_descriptor_operations_all(struct dj_test_suite* _suite);

// VI. file synchronization tests
bool d_tests_dfile_fsync(struct dj_test_suite* _suite);

bool d_tests_dfile_fsync_stream(struct dj_test_suite* _suite);

bool d_tests_dfile_fflush(struct dj_test_suite* _suite);

bool d_tests_dfile_synchronization_all(struct dj_test_suite* _suite);

// VII. file locking tests
bool d_tests_dfile_flock(struct dj_test_suite* _suite);

bool d_tests_dfile_flock_stream(struct dj_test_suite* _suite);

bool d_tests_dfile_locking_all(struct dj_test_suite* _suite);

// VIII. temporary file tests
bool d_tests_dfile_tmpfile(struct dj_test_suite* _suite);

bool d_tests_dfile_tmpfile_s(struct dj_test_suite* _suite);

bool d_tests_dfile_mkstemp(struct dj_test_suite* _suite);

bool d_tests_dfile_tmpnam_s(struct dj_test_suite* _suite);

bool d_tests_dfile_tempdir(struct dj_test_suite* _suite);

bool d_tests_dfile_temporary_files_all(struct dj_test_suite* _suite);

// IX. file metadata tests
bool d_tests_dfile_stat(struct dj_test_suite* _suite);

bool d_tests_dfile_fstat(struct dj_test_suite* _suite);

bool d_tests_dfile_lstat(struct dj_test_suite* _suite);

bool d_tests_dfile_access(struct dj_test_suite* _suite);

bool d_tests_dfile_chmod(struct dj_test_suite* _suite);

bool d_tests_dfile_file_size(struct dj_test_suite* _suite);

bool d_tests_dfile_file_size_stream(struct dj_test_suite* _suite);

bool d_tests_dfile_file_exists(struct dj_test_suite* _suite);

bool d_tests_dfile_is_file(struct dj_test_suite* _suite);

bool d_tests_dfile_is_dir(struct dj_test_suite* _suite);

bool d_tests_dfile_metadata_all(struct dj_test_suite* _suite);

// X. directory operations tests
bool d_tests_dfile_mkdir(struct dj_test_suite* _suite);

bool d_tests_dfile_mkdir_p(struct dj_test_suite* _suite);

bool d_tests_dfile_rmdir(struct dj_test_suite* _suite);

bool d_tests_dfile_opendir_readdir_closedir(struct dj_test_suite* _suite);

bool d_tests_dfile_rewinddir(struct dj_test_suite* _suite);

bool d_tests_dfile_directory_operations_all(struct dj_test_suite* _suite);

// XI. file operations tests
bool d_tests_dfile_remove(struct dj_test_suite* _suite);

bool d_tests_dfile_unlink(struct dj_test_suite* _suite);

bool d_tests_dfile_rename(struct dj_test_suite* _suite);

bool d_tests_dfile_copy_file(struct dj_test_suite* _suite);

bool d_tests_dfile_file_operations_all(struct dj_test_suite* _suite);

// XII. path utilities tests
bool d_tests_dfile_getcwd(struct dj_test_suite* _suite);

bool d_tests_dfile_chdir(struct dj_test_suite* _suite);

bool d_tests_dfile_realpath(struct dj_test_suite* _suite);

bool d_tests_dfile_dirname(struct dj_test_suite* _suite);

bool d_tests_dfile_basename(struct dj_test_suite* _suite);

bool d_tests_dfile_path_join(struct dj_test_suite* _suite);

bool d_tests_dfile_path_normalize(struct dj_test_suite* _suite);

bool d_tests_dfile_path_is_absolute(struct dj_test_suite* _suite);

bool d_tests_dfile_get_extension(struct dj_test_suite* _suite);

bool d_tests_dfile_path_utilities_all(struct dj_test_suite* _suite);

#if D_FILE_LINK_IS_AVAILABLE
// XIII. symbolic link tests
bool d_tests_dfile_symlink(struct dj_test_suite* _suite);

bool d_tests_dfile_readlink(struct dj_test_suite* _suite);

bool d_tests_dfile_is_symlink(struct dj_test_suite* _suite);

bool d_tests_dfile_symbolic_links_all(struct dj_test_suite* _suite);

#endif

// XIV. pipe operations tests
bool d_tests_dfile_popen_pclose(struct dj_test_suite* _suite);

bool d_tests_dfile_pipe_operations_all(struct dj_test_suite* _suite);

// XV. binary I/O helpers tests
bool d_tests_dfile_fread_all(struct dj_test_suite* _suite);

bool d_tests_dfile_fwrite_all(struct dj_test_suite* _suite);

bool d_tests_dfile_fappend_all(struct dj_test_suite* _suite);

bool d_tests_dfile_binary_io_all(struct dj_test_suite* _suite);

// null parameter tests
bool d_tests_dfile_null_params_all(struct dj_test_suite* _suite);

// master test runner
bool d_tests_dfile_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_C_FILESYS_DFILE_TESTS_H
