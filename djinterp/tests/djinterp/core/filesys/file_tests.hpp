/*******************************************************************************
* djinterp [test]                                                 file_tests.hpp
*
*   Unit tests for the djinterp C++ filesystem layer (core/filesys/file_*.hpp),
* authored against the DTest framework's native C++ surface -- test_builder --
* rather than the C ABI bridge the c/filesys suite uses. Same framework underneath;
* this side simply speaks to it directly, which is what makes deferred
* predicates and RAII-aware cases possible.
*
*   WHAT IS UNDER TEST. The C++ layer only: djinterp::path, file_status, file,
* directory, process, space_info, error, and the free functions over them. The
* C modules beneath are exercised by the c/filesys suite and are assumed working
* here -- when a case fails in both suites, the C one names the real fault.
*
*   AUTHORING MODEL. Each section defines one
* `void tests_fs_<module>(suite_type&)` that opens a module, then a block per
* type or function, then a leaf per behaviour, conjoining clauses onto the leaf
* with assert_. Clauses are DEFERRED: assert_ stores the predicate and the
* forest is evaluated later, in one pass, by run(). That is the whole reason to
* prefer the native surface -- construction and evaluation are separate phases,
* so a case is a self-contained scenario rather than a sequence of side effects
* on shared state.
*
*   >>> THE ONE AUTHORING RULE. A deferred clause runs AFTER the function that
*   registered it has returned. A lambda that captures a local by reference is
*   therefore a dangling reference by the time it runs. Every clause here is
*   capture-free and reaches the environment through the free helpers below,
*   which own their storage. Capture by value if you must capture at all; never
*   by reference. <<<
*
*   RAII IS THE POINT. The C suite could only ask what a function returned.
* Here the types own things, so the cases ask what the C++ layer adds: that a
* handle closes itself, that a move transfers ownership and leaves the source
* closed, that copying is not merely discouraged but ill-formed, and that a
* moved-from path is the invalid path every free function must reject. Those
* checks have no counterpart in the C suite and are most of this one's value.
*
*
* path:      /tests/djinterp/core/filesys/file_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.21
*******************************************************************************/

#ifndef DJINTERP_FS_FILE_TESTS_HPP_
#define DJINTERP_FS_FILE_TESTS_HPP_ 1

// std
#include <cstddef>
// djinterp
// djinterp -- the layer under test (bare: the leaf adds inc/djinterp/core/fs)
#include "../../../../inc/djinterp/core/filesys/dfile.hpp"
// djinterp -- the test framework's native C++ builder (full-from-inc)
#include "djinterp/test/test_builder.hpp"

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



namespace djinterp
{
namespace testing
{

// suite_type
//   type: the builder every section populates. test_builder<> defaults to a
// test_tree<basic_test>, which is the forest run() walks.
typedef ::djinterp::test::test_builder<> suite_type;


/******************************************************************************
 * TEST ENVIRONMENT
 *****************************************************************************/

// D_CF_TEST_DIR
//   constant: base name of the scratch directory. Resolved to an ABSOLUTE
// path at setup, so the suite stays correct no matter where the process is
// working from when a clause finally runs.
#define D_CF_TEST_DIR       "djinterp_core_fs_test_tmp"

// D_CF_TEST_FILE
//   constant: name of the standard fixture file created by setup.
#define D_CF_TEST_FILE      "sample.txt"

// D_CF_TEST_CONTENT
//   constant: exact bytes of the standard fixture. No embedded NUL, so it
// compares as a C string as well as by length.
#define D_CF_TEST_CONTENT   "Hello, djinterp fs!\nSecond line.\n"


// fs_test_setup
//   function: create the scratch directory (absolute) and the standard
// fixture. Call once, before run().
// returns true on success.
bool fs_test_setup(void);

// fs_test_teardown
//   function: remove the scratch tree. Call once, after run().
// returns true on success.
bool fs_test_teardown(void);

// fs_test_root
//   function: the absolute scratch directory. Returns a reference to storage
// the helper owns, so a deferred clause may call this instead of capturing a
// path it would otherwise have to keep alive itself.
const path& fs_test_root(void);

// fs_test_path
//   function: _name resolved under the scratch directory.
// returns a fresh path by value.
path fs_test_path(const char* _name);

// fs_test_write
//   function: create or replace the file at _p with exactly _size bytes,
// using raw stdio rather than djinterp::file. Staging a fixture with the code
// under test would let one bug hide another.
// returns true on success.
bool fs_test_write(const path&        _p,
                   const void*        _data,
                   ::std::size_t      _size);

// fs_test_contents_are
//   function: true iff the file at _p holds exactly the NUL-terminated bytes
// of _expect. Reads with raw stdio, for the same reason fs_test_write writes
// with it.
bool fs_test_contents_are(const path& _p,
                          const char* _expect);

// fs_test_exists
//   function: true iff _p names something on disk, answered with a raw stat
// rather than djinterp::exists -- so a case that VERIFIES a removal does not
// depend on the query it is trying to corroborate.
bool fs_test_exists(const path& _p);

// fs_test_invalid_path
//   function: a path in the invalid state -- valid() false -- produced by
// moving one away. This is the portable way to reach the EINVAL branch every
// free function guards with, which allocation failure alone could not reach in
// a test.
path fs_test_invalid_path(void);

// fs_test_symlinks_creatable
//   function: cached probe -- can this process actually CREATE a symlink here?
// The API being compiled in does not imply the privilege to use it (notably on
// Windows). Always false where links are not available at all.
bool fs_test_symlinks_creatable(void);


/******************************************************************************
 * SECTION BUILDERS  (one per module; each populates one test module)
 *****************************************************************************/

// I.     error          -- the errno carrier the whole layer reports through
void tests_fs_error(suite_type& _suite);

// II.    path           -- the lexical path value type
void tests_fs_path(suite_type& _suite);

// III.   file_status    -- one stat, many questions (+ the free queries)
void tests_fs_status(suite_type& _suite);

// IV.    file           -- the RAII FILE* handle
void tests_fs_stream(suite_type& _suite);

// V.     directory      -- the RAII directory handle and its range-for
void tests_fs_dir(suite_type& _suite);

// VI.    operations     -- remove / remove_file / rename / copy_file
void tests_fs_ops(suite_type& _suite);

// VII.   links          -- create_symlink / read_symlink / is_symlink
void tests_fs_link(suite_type& _suite);

// VIII.  temporaries    -- temp_directory_path
void tests_fs_temp(suite_type& _suite);

// IX.    capacity       -- space_info / space
void tests_fs_space(suite_type& _suite);

// X.     recursion      -- walk / remove_all
void tests_fs_recursive(suite_type& _suite);

// XI.    process        -- the RAII command pipe.
//   Always declared; the definition is compiled out where the platform has no
// pipes, and the runner splices it in only when it is there.
void tests_fs_pipe(suite_type& _suite);

// XII.   expected       -- the re_std::expected return surface (C++11).
//   As above, but gated on the library feature rather than the platform.
void tests_fs_expected(suite_type& _suite);

} // namespace testing
} // namespace djinterp

#endif // DJINTERP_FS_FILE_TESTS_HPP_
