/*******************************************************************************
* djinterp [test]                                                file_tests_sa.h
*
*   Unit tests for the c/filesys subframework. One test source per module, so the
* suite mirrors the thing it tests: if file_path grows a function, exactly one
* file changes.
*
*   TWO RULES THIS SUITE FOLLOWS THAT THE OLD dfile SUITE DID NOT.
*
*   1. A test that cannot run is OMITTED, never passed.
*      The old suite did this, in dfile_tests_sa_symlink.c:
*          // symlinks not available (Windows without admin privileges)
*          // pass these tests as they cannot be performed
*          test_create = true;
*          test_link_exists = true;
*      Five assertions, reported green, for code that never executed. A suite
*      that says PASS for something it did not test is worse than one that
*      says nothing: it is a claim, and the claim is false. The framework has
*      no skip state -- d_test_object carries a bool -- so the honest move is
*      to leave the assertion out of the tree entirely and say so in the group
*      name. Nothing green, nothing red, nothing claimed.
*
*   2. Capability is asked at COMPILE time, not probed at runtime.
*      The old suite ran d_tests_dfile_symlinks_available(), which created a
*      symlink to find out whether it could create a symlink. The fs config
*      already knows: D_FILE_LINK_IS_AVAILABLE, D_FILE_STAT_HAS_BIRTHTIME,
*      D_FILE_OPS_COPY_IS_NATIVE, D_FILE_LOCK_IS_PER_DESCRIPTION and the rest
*      are #if-safe and free. A runtime probe for a compile-time fact is a
*      test of the probe.
*      The one thing that genuinely IS a runtime question -- whether THIS
*      PROCESS may create a symlink, which on Windows needs a privilege the
*      macro cannot see -- stays a runtime check, and is reported as an
*      omission rather than a pass.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_FILE_TESTS_SA_
#define DJINTERP_FILE_TESTS_SA_ 1

// djinterp
#include "../../../../inc/djinterp/c/filesys/dfile.h"  // the library under test
#include "../../../../inc/djinterp/test/c/test_standalone.h"  // D_ASSERT_*,
                                                              // d_test_object


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_FILE_TEMP_DIR
//   constant: base directory for the suite's scratch files.
#define D_TEST_FILE_TEMP_DIR       "file_test_tmp"

// D_TEST_FILE_TEST_FILENAME
//   constant: the standard fixture file.
#define D_TEST_FILE_TEST_FILENAME  "test_file.txt"

// D_TEST_FILE_TEST_CONTENT
//   constant: the fixture's contents. Written in binary mode, so its length
// is its byte count on every platform -- a text-mode write would make this
// 37 bytes on Windows and 35 everywhere else, and every size assertion in the
// suite would need a platform branch.
#define D_TEST_FILE_TEST_CONTENT   "Hello, World!\nThis is a test file.\n"

// D_TEST_FILE_PATH_BUF
//   constant: buffer size for path construction in tests.
#define D_TEST_FILE_PATH_BUF       512

// D_TEST_FILE_MAX_ASSERTS
//   constant: the largest number of assertions any one group may hold.
//   Groups build into a local array of this size and report the count they
// actually filled, rather than declaring a count up front and hoping it
// matches. See d_tests_file_group.
#define D_TEST_FILE_MAX_ASSERTS    24

// D_TEST_FILE_HOST_SEP
//   constant: the separator the RUNNING system accepts, which is not the same
// question as D_CFG_FILE_PATH_SYNTAX. That knob says which grammar the lexical
// module reads and writes; this says what the kernel under the test will open.
// They agree on a native build and disagree on a cross-grammar one, and the
// harness needs the second.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
#   define D_TEST_FILE_HOST_SEP    '\\'
#else
#   define D_TEST_FILE_HOST_SEP    '/'
#endif

// D_TEST_FILE_EMIT_SEP
//   constant: the separator file_path EMITS, as a string, for building the
// expected results of lexical assertions.
//   Three separators are in play and they are not the same question:
//     - what file_path ACCEPTS as input   -- both, under the Windows grammar
//     - what file_path EMITS              -- this
//     - what the running kernel opens     -- D_TEST_FILE_HOST_SEP
//   D_FILE_PATH_UNDERSTANDS_WINDOWS is the module's own published answer, and
// its comment already says the suite uses it to decide which syntax to assert.
// It did not: every expected string was written with a literal '/', so the
// whole lexical group failed the first time the cross grammar was built.
#if D_FILE_PATH_UNDERSTANDS_WINDOWS
#   define D_TEST_FILE_EMIT_SEP    "\\"
#else
#   define D_TEST_FILE_EMIT_SEP    "/"
#endif


/******************************************************************************
 * TEST UTILITIES
 *****************************************************************************/

// d_tests_file_setup
//   function: creates the scratch directory and the fixture file.
// returns true on success.
bool d_tests_file_setup(void);

// d_tests_file_teardown
//   function: removes the scratch directory and everything in it.
// returns true on success.
bool d_tests_file_teardown(void);

// d_tests_file_path
//   function: builds a path inside the scratch directory.
// writes to _buf and returns it, or NULL on failure.
char* d_tests_file_path(char*       _buf,
                        size_t      _bufsize,
                        const char* _filename);

// d_tests_file_group
//   function: builds an interior node from an array of already-created
// assertions, taking the count from what the caller actually filled.
//   This exists to delete a bug class. The old suite wrote
// d_test_object_new_interior("d_file_symlink", 4) and then four elements[idx++]
// lines, with the 4 asserted by hand -- so an assertion added, removed, or
// #if'd out silently desynchronised the count from the contents, leaving the
// tail of the array uninitialised. Here the count is DERIVED from the fill,
// so it cannot disagree, and a conditionally-omitted assertion just makes the
// group smaller.
// returns the group, or NULL on failure.
// d_tests_file_child
//   function: joins _name under _parent with the HOST separator.
//   The filesystem-facing counterpart of d_path_join, for tests that build a
// path in order to CREATE it. Under the cross grammar d_path_join emits a
// backslash -- correctly, that is the grammar it was asked for -- and a
// directory test that used it was naming one file with a backslash in it
// rather than a child of the parent.

char* d_tests_file_child(char*       _buf,
                         size_t      _bufsize,
                         const char* _parent,
                         const char* _name);

struct d_test_object* d_tests_file_group(const char*            _name,
                                         struct d_test_object** _items,
                                         size_t                 _count);


/******************************************************************************
 * PER-MODULE SUITES  (one per c/filesys module, same order as dfile.h)
 *****************************************************************************/

// I.    file_common  -- notifications, backend introspection
struct d_test_object* d_tests_file_common_all(void);

// II.   file_open
struct d_test_object* d_tests_file_open_all(void);

// III.  file_desc
struct d_test_object* d_tests_file_desc_all(void);

// IV.   file_io
struct d_test_object* d_tests_file_io_all(void);

// V.    file_seek
struct d_test_object* d_tests_file_seek_all(void);

// VI.   file_sync
struct d_test_object* d_tests_file_sync_all(void);

// VII.  file_lock
struct d_test_object* d_tests_file_lock_all(void);

// VIII. file_stat
struct d_test_object* d_tests_file_stat_all(void);

// IX.   file_path
struct d_test_object* d_tests_file_path_all(void);

// X.    file_dir
struct d_test_object* d_tests_file_dir_all(void);

// XI.   file_ops
struct d_test_object* d_tests_file_ops_all(void);

// XII.  file_temp
struct d_test_object* d_tests_file_temp_all(void);

// XIII. file_link
//   Compiled only where the platform has symbolic links. The suite omits the
// whole group otherwise rather than reporting an empty pass.
#if D_FILE_LINK_IS_AVAILABLE
struct d_test_object* d_tests_file_link_all(void);
#endif

// XIV.  file_pipe
#if D_FILE_PIPE_IS_AVAILABLE
struct d_test_object* d_tests_file_pipe_all(void);
#endif

// XV.   file_space
#if D_FILE_SPACE_IS_AVAILABLE
struct d_test_object* d_tests_file_space_all(void);
#endif

// master runner
struct d_test_object* d_tests_file_run_all(void);


#endif  // DJINTERP_FILE_TESTS_SA_
