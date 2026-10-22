/*******************************************************************************
* djinterp [test]                                             file_tests_dtest.h
*
* The DTest C vocabulary the filesys C suite is written in, implemented for
* the suite alone.
*   file_tests.h was written against a C bridge into the C++ DTest
* (djinterp/test/dtest.h) that no tree holds, so the suite could not compile.
* This gives it the eleven names it calls -- a suite, modules, blocks, tests,
* checks conjoined onto the open test, a console report and a summary -- and
* nothing more. When DTest provides the bridge, delete these two files and
* include its header in file_tests.h instead.
*
* path:      /tests/djinterp/c/filesys/file_tests_dtest.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_FILESYS_FILE_TESTS_DTEST_H
#define DJINTERP_TESTS_C_FILESYS_FILE_TESTS_DTEST_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

#if defined(__cplusplus)
extern "C"
{
#endif

// dtest_suite
//   type: the tests a run has built, and how it reports them.
typedef struct dtest_suite dtest_suite;

// dtest_summary
//   struct: what dtest_run found.
typedef struct dtest_summary
{
    size_t tests;         // tests opened
    size_t tests_failed;  // tests with a failing check, or none at all
    size_t checks;        // checks made
    size_t checks_failed; // checks that failed
} dtest_summary;

// DTEST_SHOW_FAILURES_ONLY, DTEST_SHOW_ALL
//   constant: what the console report prints besides its closing summary.
#define DTEST_SHOW_FAILURES_ONLY 0
#define DTEST_SHOW_ALL           1

/**
 * @brief Creates an empty suite.
 *
 * @return the suite, or `NULL` if allocation failed.
 * @post The suite belongs to the caller, who releases it with dtest_destroy.
 */
dtest_suite* dtest_create(void);
void         dtest_destroy(dtest_suite* _suite);
void         dtest_use_console(dtest_suite* _suite,
                               int          _show);
// the open module, block and test: each opens beneath the one before it, and
// a check counts towards the test open when it is made
void         dtest_module(dtest_suite* _suite,
                          const char*  _name);
void         dtest_block(dtest_suite* _suite,
                         const char*  _name);
void         dtest_test(dtest_suite* _suite,
                        const char*  _name);
void         dtest_assert_expr(dtest_suite* _suite,
                               bool         _result,
                               const char*  _expr,
                               const char*  _file,
                               int          _line);
/**
 * @brief Reports the suite's tests on the console, as dtest_use_console set.
 *
 * @param[in]  _suite    the suite.
 * @param[out] _summary  receives the counts; may be `NULL`.
 * @return 0 when every test passed, 1 when one failed or made no check, 2
 *         when the suite could not record a test or check.
 */
int          dtest_run(dtest_suite*   _suite,
                       dtest_summary* _summary);

#if defined(__cplusplus)
}
#endif


#endif  // DJINTERP_TESTS_C_FILESYS_FILE_TESTS_DTEST_H
