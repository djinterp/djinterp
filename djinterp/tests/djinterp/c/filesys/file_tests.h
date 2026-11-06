/*******************************************************************************
* djinterp [test]                                                   file_tests.h
*
*   Unit tests for the djinterp fs subframework (c/filesys/file_*), written in
* DTest's C face (c/modules/test/test_suite.h). The C modules are exercised
* here; the C++ facade (dfile.hpp and friends) is out of scope.
*
*   AUTHORING MODEL.  These tests use DTest's imperative surface: each section
* translation unit defines one `void d_fs_tests_<section>(struct
* dj_test_suite*)` that opens a module, then a block per public function, then
* a test per behaviour, making expression-capturing checks (D_FS_CHECK)
* against the open test.  The runner (config tree) creates one suite, splices
* every section into it, and reports it once.  The runner enumerates the
* section builders by name, as the authoring guide's runner does.
*
*   ENVIRONMENT.  d_fs_test_setup builds an isolated, ABSOLUTE-pathed scratch
* directory and one standard fixture file before any section runs; the absolute
* path keeps the suite correct even across a d_dir_set_current test.  d_fs_test_teardown
* removes the tree afterwards.  Fixtures a test needs to exist are created with
* d_fs_test_write, which uses raw stdio rather than the code under test, so a
* broken d_file_write_all cannot mask a failure elsewhere.
*
*
* path:      /tests/djinterp/c/filesys/file_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_FILE_TESTS_H_
#define DJINTERP_FILE_TESTS_H_ 1

// std
#include <stdbool.h>
#include <stddef.h>
// djinterp
// the fs umbrella: a test harness is exactly the case the umbrella exists for
//   Spelled from the include root, so a single -Iinc suffices. Written as
// bare "file.h" this also needed a -I pointing into inc/djinterp/c/fs, which
// no other TU in the project wants.
#include "../../../../inc/djinterp/c/filesys/dfile.h"
// DTest's C face: the suite, its builders and D_TEST_CHECK. This is the C ABI
// the suite was first written against as dtest_*, under the C guide's names.
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"

// T_PRINT, T_SINK, T_COPY_TO, T_FALSE, T_MISSING_STATUS_OK
//   the shell commands the pipe tests run, in the shell d_pipe_open uses:
// /bin/sh on POSIX, cmd.exe on Windows, whose more stands in for cat (and
// may end the text with a line break) and which reports a missing command as
// 1
#if defined(_WIN32)
    #define T_PRINT(_text)               "<nul set /p =" _text
    #define T_SINK                       "more > nul"
    #define T_COPY_TO(_file)             "more > " _file
    #define T_FALSE                      "exit 1"
    #define T_MISSING_STATUS_OK(_status) ((_status) != 0)
#else
    #define T_PRINT(_text)               "printf " _text
    #define T_SINK                       "cat > /dev/null"
    #define T_COPY_TO(_file)             "cat > " _file
    #define T_FALSE                      "false"
    #define T_MISSING_STATUS_OK(_status) ((_status) == 127)
#endif



#if defined(__cplusplus)
extern "C"
{
#endif


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_FS_TEST_DIR
//   constant: base name of the scratch directory. Resolved to an absolute
// path at setup so the suite survives a d_dir_set_current test that leaves the working
// directory elsewhere.
#define D_FS_TEST_DIR       "djinterp_fs_test_tmp"

// D_FS_TEST_FILE
//   constant: name of the standard fixture file created by setup.
#define D_FS_TEST_FILE      "sample.txt"

// D_FS_TEST_CONTENT
//   constant: exact bytes of the standard fixture file. No embedded NUL, so a
// NUL-terminating read can be compared with strcmp as well as memcmp.
#define D_FS_TEST_CONTENT   "Hello, djinterp fs!\nSecond line.\n"

// D_FS_TEST_PATH_BUF
//   constant: buffer size for constructed test paths.
#define D_FS_TEST_PATH_BUF  512


/******************************************************************************
 * ASSERTION MACRO
 *****************************************************************************/

// D_FS_CHECK
//   macro: record a boolean condition against the open test, carrying the
// source text of the condition plus its file and line so the report can name
// exactly what failed. A test passes iff every check made against it held.
// Every case opens its test first with d_test_suite_test.
#define D_FS_CHECK(_suite, _expr) D_TEST_CHECK(_suite, _expr)


/******************************************************************************
 * TEST ENVIRONMENT HELPERS
 *****************************************************************************/

// d_fs_test_setup
//   function: create the scratch directory (absolute) and the standard
// fixture file. Call once, before any section runs.
// returns true on success, false otherwise.
bool d_fs_test_setup(void);

// d_fs_test_teardown
//   function: remove every file in the scratch directory and the directory
// itself. Call once, after the suite has run.
// returns true on success, false otherwise.
bool d_fs_test_teardown(void);

// d_fs_test_root
//   function: the absolute path of the scratch directory, valid between setup
// and teardown.
// returns a pointer to internal storage; never NULL after a successful setup.
const char* d_fs_test_root(void);

// d_fs_test_path
//   function: join _name onto the scratch directory into _buf.
// returns _buf on success, or NULL on failure.
char* d_fs_test_path(
    char*       _buf,
    size_t      _bufsize,
    const char* _name
);

// d_fs_test_write
//   function: create or replace the file at _path with exactly _size bytes of
// _data, using raw stdio rather than the code under test. This is how a test
// stages a fixture without depending on d_file_write_all.
// returns true on success, false otherwise.
bool d_fs_test_write(
    const char* _path,
    const void* _data,
    size_t      _size
);


/******************************************************************************
 * SECTION BUILDERS  (one per like-group module; each populates one module)
 *****************************************************************************/

// I.    Foundation      -- notifications, backend/level naming (file_common)
void d_fs_tests_common(
    struct dj_test_suite* _suite
);

// II.   Streams         -- fopen / fopen_s / freopen / freopen_s / fdopen /
//                          fclose (file_open)
void d_fs_tests_open(
    struct dj_test_suite* _suite
);

// III.  Descriptors     -- open / fileno / dup / dup2 / close (file_desc)
void d_fs_tests_desc(
    struct dj_test_suite* _suite
);

// IV.   Transfer        -- read / read_full / pread / write / write_full /
//                          pwrite / fread_all* / fwrite_all / fappend_all
//                          (file_io)
void d_fs_tests_io(
    struct dj_test_suite* _suite
);

// V.    Position        -- fseeko / ftello / rewind / ftruncate /
//                          ftruncate_stream (file_seek)
void d_fs_tests_seek(
    struct dj_test_suite* _suite
);

// VI.   Durability      -- fsync / fsync_stream / fflush (file_sync)
void d_fs_tests_sync(
    struct dj_test_suite* _suite
);

// VII.  Locking         -- flock / flock_stream (file_lock)
void d_fs_tests_lock(
    struct dj_test_suite* _suite
);

// VIII. Metadata        -- stat / lstat / fstat / access / chmod / file_size /
//                          file_size_stream / exists / is_file / is_dir
//                          (file_stat)
void d_fs_tests_stat(
    struct dj_test_suite* _suite
);

// IX.   Paths           -- dirname / basename / get_extension / path_stem /
//                          path_join / path_is_absolute / path_root_length /
//                          path_normalize (file_path)
void d_fs_tests_path(
    struct dj_test_suite* _suite
);

// X.    Directories     -- mkdir / mkdir_p / rmdir / opendir / readdir /
//                          rewinddir / closedir / getcwd / chdir / realpath
//                          (file_dir)
void d_fs_tests_dir(
    struct dj_test_suite* _suite
);

// XI.   Operations      -- remove / unlink / rename / copy_file (file_ops)
void d_fs_tests_ops(
    struct dj_test_suite* _suite
);

// XII.  Temporaries     -- tmpfile / tmpfile_s / mkstemp / tmpnam_s / tempdir
//                          (file_temp)
void d_fs_tests_temp(
    struct dj_test_suite* _suite
);

// XIII. Links           -- symlink / readlink / is_symlink (file_link).
//   Always declared; the definition is compiled out when the platform has no
// symbolic links, and the runner splices it in only when they are available.
void d_fs_tests_link(
    struct dj_test_suite* _suite
);

// XIV.  Pipes           -- popen / pclose (file_pipe). As links above.
void d_fs_tests_pipe(
    struct dj_test_suite* _suite
);

// XV.   Capacity        -- space (file_space)
void d_fs_tests_space(
    struct dj_test_suite* _suite
);


#if defined(__cplusplus)
}  // extern "C"
#endif


#endif  // DJINTERP_FILE_TESTS_H_
