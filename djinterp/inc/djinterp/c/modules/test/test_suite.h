/*******************************************************************************
* djinterp [c][test]                                                test_suite.h
*
* DTest's C face: the surface a C suite is written in.
*   A suite is a tree of modules, blocks and tests, and a test is a run of
* checks. C suites build that tree in one of two ways, and both end in the
* same report:
*
*   - imperatively, by opening a module, a block and a test in turn and making
*     checks against the test open last (D_TEST_CHECK); a section is then a
*     `void d_tests_<module>_<section>(struct dj_test_suite*)`; or
*   - declaratively, as C++'s spec mode does, by listing each test as a row
*     {name, descriptor, function} in a module spec, where the function is a
*     `bool (void)` that checks with D_TEST_REQUIRE and returns true at its end.
*
*   The runner's main chooses options and nothing else: d_test_run_modules()
* takes the specs, parses the command line, runs every test and returns the
* process's exit code. The report is a line per test (or per failure), with
* the failing checks and any notes beneath it, a closing summary, and, per
* module, the `|...|` results table C++'s test_defaults.hpp prints, in the
* same columns, so tooling reads both.
*
*   This is the C ABI the filesys suite was first written against as
* `dtest_*`; the names here follow the C guide. It is plain C99 with no
* dependency on the C++ framework, so a C-only build runs its suites.
*
* NOT THREAD-SAFE
* ---------------
*   A suite is built and run by one thread, and D_TEST_REQUIRE reaches the suite
* being run through a file-scope pointer, so one suite runs at a time in a
* process. Tests that start threads join them before they return.
*
* HOW TO USE
* ----------
*   A suite is four files (docs/djinterp_testing.md, section 12): a header that
* lists its tests as spec rows, the test sources, a runner and a CMake leaf. The
* runner hands the specs to d_test_run_modules:
*
*     static const struct dj_test_module_spec* const modules[] =
*         { &d_tests_x_module };
*
*     int main(int argc, char** argv)
*     {
*         return d_test_run_modules(argc, argv, "x", modules, 1u);
*     }
*
*   A test is a bool (void) function that checks with D_TEST_CHECK,
* D_TEST_REQUIRE, D_TEST_SKIP and D_TEST_NOTE.
*
* OPTIONS
* -------
*   struct dj_test_suite_options: how much the report shows
* (D_TEST_SUITE_SHOW_*), whether each module prints its results table, the
* stream, and a filter on test paths. d_test_run_modules reads them from the
* command line: --failures, --tests (the default), --checks, --no-table,
* --filter=TEXT and --help.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/modules/test/test_suite.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Report detail
         1.  D_TEST_SUITE_SHOW_FAILURES
         2.  D_TEST_SUITE_SHOW_TESTS
         3.  D_TEST_SUITE_SHOW_CHECKS
    2.  Exit codes
         1.  D_TEST_SUITE_EXIT_PASSED
         2.  D_TEST_SUITE_EXIT_FAILED
         3.  D_TEST_SUITE_EXIT_ERROR
2.  TYPES
    -----
    1.  The suite and its summary
         1.  dj_test_suite
         2.  dj_test_summary
    2.  Options
         1.  dj_test_suite_options
    3.  Specs
         1.  fn_test_case
         2.  dj_test_case_spec
         3.  dj_test_block_spec
         4.  dj_test_module_spec
3.  FUNCTIONS
    ---------
    1.  Lifetime and options
    2.  Building a suite
    3.  Running a suite
    4.  Support for the check macros
    5.  Check helpers
4.  CHECK MACROS
    ------------
    1.  Imperative checks
         1.  D_TEST_CHECK
         2.  D_TEST_CHECK_MSG
    2.  Spec checks
         1.  D_TEST_REQUIRE
         2.  D_TEST_SKIP
         3.  D_TEST_NOTE
*/

#ifndef DJINTERP_C_TEST_SUITE_H
#define DJINTERP_C_TEST_SUITE_H 1

// std
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
#include <stdio.h>   // FILE
// djinterp
#include "../../djinterp.h"                        // framework root
#include "../../../env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Report detail
//------------------------------------------------------------------------------
// 1.1.1
// D_TEST_SUITE_SHOW_FAILURES
//   constant: the report lists failing and skipped tests only, then the
// summary.
#define D_TEST_SUITE_SHOW_FAILURES 0

// 1.1.2
// D_TEST_SUITE_SHOW_TESTS
//   constant: the report lists every test, with each failing check beneath its
// test. The default.
#define D_TEST_SUITE_SHOW_TESTS    1

// 1.1.3
// D_TEST_SUITE_SHOW_CHECKS
//   constant: the report lists every test and every check, passing or not.
#define D_TEST_SUITE_SHOW_CHECKS   2

// 1.2    Exit codes
//------------------------------------------------------------------------------
// 1.2.1
// D_TEST_SUITE_EXIT_PASSED
//   constant: every test passed or was skipped.
#define D_TEST_SUITE_EXIT_PASSED   0

// 1.2.2
// D_TEST_SUITE_EXIT_FAILED
//   constant: a test failed, or made no check and was not skipped.
#define D_TEST_SUITE_EXIT_FAILED   1

// 1.2.3
// D_TEST_SUITE_EXIT_ERROR
//   constant: the suite could not record a node or a check (out of memory),
// or the command line was refused, so the run proves nothing.
#define D_TEST_SUITE_EXIT_ERROR    2


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    The suite and its summary
//------------------------------------------------------------------------------
// 2.1.1
// dj_test_suite
//   struct: an opaque suite: the tree a run has built so far, its checks and
// failures, and how it reports them. Made by d_test_suite_new and released by
// d_test_suite_free.
struct dj_test_suite;

// 2.1.2
// dj_test_summary
//   struct: the counts a run ends with.
struct dj_test_summary
{
    size_t modules;        // modules opened
    size_t tests;          // tests opened
    size_t tests_failed;   // tests with a failing check, or with none at all
    size_t tests_skipped;  // tests skipped
    size_t checks;         // checks made
    size_t checks_failed;  // checks that failed
};

// 2.2    Options
//------------------------------------------------------------------------------
// 2.2.1
// dj_test_suite_options
//   struct: what a run reports and where. d_test_suite_options_init sets the
// defaults: every test listed, the results table on, stdout, no filter.
struct dj_test_suite_options
{
    int         show;    // D_TEST_SUITE_SHOW_*
    bool        table;   // print each module's |...| results table
    bool        color;   // reserved: the report has no color yet
    FILE*       stream;  // where the report goes; NULL is stdout
    const char* filter;  // run only spec tests whose path holds this text
};

// 2.3    Specs
//------------------------------------------------------------------------------
// 2.3.1
// fn_test_case
//   function pointer: a spec test. It checks with D_TEST_REQUIRE and returns
// true at its end; returning false fails the test too.
typedef bool (*fn_test_case)(void);

// 2.3.2
// dj_test_case_spec
//   struct: one test of a spec, a row of C++'s test_spec: its name (the
// function's), a one-clause descriptor of what it verifies, and the function.
struct dj_test_case_spec
{
    const char*  name;
    const char*  descriptor;
    fn_test_case fn;
};

// 2.3.3
// dj_test_block_spec
//   struct: a block of a spec: one section translation unit's tests.
struct dj_test_block_spec
{
    const char*                     name;
    const char*                     descriptor;
    const struct dj_test_case_spec* tests;
    size_t                          count;
};

// 2.3.4
// dj_test_module_spec
//   struct: a module of a spec: one header's suite. A suite header's
// provider, d_tests_<module>_spec(), returns a pointer to a static one.
struct dj_test_module_spec
{
    const char*                      name;
    const char*                      descriptor;
    const struct dj_test_block_spec* blocks;
    size_t                           count;
};


//==============================================================================
// 3.  FUNCTIONS
//==============================================================================


// 3.1    Lifetime and options
//------------------------------------------------------------------------------
/**
 * @brief Creates an empty suite, with the default options.
 *
 * @return the suite, or `NULL` if allocation failed.
 * @post The suite belongs to the caller, who releases it with
 *       d_test_suite_free.
 */
struct dj_test_suite*
     d_test_suite_new(void);

/**
 * @brief Releases a suite and everything it recorded.
 *
 * @param[in] _suite  the suite; may be `NULL`.
 * @post `_suite` is invalid.
 */
void d_test_suite_free(struct dj_test_suite* _suite);

/**
 * @brief Sets options to the defaults: every test listed, the results table
 *        on, stdout, no filter.
 *
 * @param[out] _options  the options to set; `NULL` is ignored.
 */
void d_test_suite_options_init(struct dj_test_suite_options* _options);

/**
 * @brief Reads a runner's command line into options.
 *
 * @note The arguments are `--failures`, `--tests` and `--checks` (the report's
 *       detail), `--no-table`, `--filter=TEXT` and `--help`. Anything else is
 *       refused.
 *
 * @param[in,out] _options  the options, already initialized.
 * @param[in]     _argc     the argument count, as main received it.
 * @param[in]     _argv     the arguments, as main received them.
 * @return 0 when the arguments were read, 1 when `--help` printed the usage
 *         (the runner should exit with 0), or -1 when an argument was refused
 *         (the usage is printed to stderr).
 */
int  d_test_suite_options_parse(struct dj_test_suite_options* _options,
                                int                           _argc,
                                char**                        _argv);

/**
 * @brief Sets what a suite reports and where.
 *
 * @param[in,out] _suite    the suite.
 * @param[in]     _options  the options, copied; the filter text is not, so it
 *                          must outlive the run.
 */
void d_test_suite_set_options(struct dj_test_suite*               _suite,
                              const struct dj_test_suite_options* _options);

// 3.2    Building a suite
//------------------------------------------------------------------------------
// Each open call starts a node beneath the one before it: a block belongs to
// the module open last, a test to the block open last (or to the module, when
// it has none). The names are copied. A call returns false when the suite is
// NULL or could not grow, and the suite then reports
// D_TEST_SUITE_EXIT_ERROR.
bool d_test_suite_module(struct dj_test_suite* _suite,
                         const char*           _name);

bool d_test_suite_block(struct dj_test_suite* _suite,
                        const char*           _name);

bool d_test_suite_test(struct dj_test_suite* _suite,
                       const char*           _name);

/**
 * @brief Gives the node opened last its descriptor: one clause saying what it
 *        verifies, as a spec row's descriptor does.
 *
 * @param[in,out] _suite       the suite.
 * @param[in]     _descriptor  the text, copied; `NULL` clears it.
 * @return `true` if recorded, `false` when nothing is open or out of memory.
 */
bool d_test_suite_describe(struct dj_test_suite* _suite,
                           const char*           _descriptor);

/**
 * @brief Attaches a note to the node opened last: what a check cannot carry,
 *        such as the value a failed check got, or a module's build profile.
 *
 * @note The report prints a note beneath its node, in the order it was made
 *       among that node's checks. At D_TEST_SUITE_SHOW_CHECKS every note is
 *       printed; otherwise a test's notes are printed when the test fails, and
 *       a module's or block's when a test beneath it fails. A note's text may
 *       run to several lines; a final newline is dropped.
 *
 * @note Every note is kept until the suite is freed, whatever the options, so
 *       a note belongs beside a check, not in a loop of thousands.
 *
 * @param[in,out] _suite   the suite.
 * @param[in]     _format  a printf format.
 * @param[in]     ...      its arguments.
 * @return `true` if recorded; `false` when nothing is open, `_format` is
 *         `NULL` or cannot be formatted, or out of memory (which, as for a
 *         check, makes the run D_TEST_SUITE_EXIT_ERROR).
 */
bool d_test_suite_note(struct dj_test_suite* _suite,
                       const char*           _format,
                       ...) D_FORMAT_PRINTF(2, 3);

/**
 * @brief d_test_suite_note with its arguments as a `va_list`, for a helper
 *        that takes its own format.
 *
 * @param[in,out] _suite   the suite.
 * @param[in]     _format  a printf format.
 * @param[in]     _args    its arguments; read as vsnprintf reads them.
 * @return as d_test_suite_note.
 */
bool d_test_suite_vnote(struct dj_test_suite* _suite,
                        const char*           _format,
                        va_list               _args) D_FORMAT_PRINTF(2, 0);

/**
 * @brief Records a check against the test opened last.
 *
 * @note A check made before any test is open opens one, named after the
 *       expression, so it is counted rather than lost.
 *
 * @param[in,out] _suite   the suite.
 * @param[in]     _result  whether the check held.
 * @param[in]     _expr    what was checked, as source text; copied.
 * @param[in]     _file    the source file, as __FILE__ gives it; copied.
 * @param[in]     _line    the source line.
 * @return `_result`, so a caller can stop on a failed check.
 */
bool d_test_suite_check(struct dj_test_suite* _suite,
                        bool                  _result,
                        const char*           _expr,
                        const char*           _file,
                        int                   _line);

/**
 * @brief Records a named check with a message, the form a standalone
 *        D_ASSERT_* leaf converts to.
 *
 * @param[in,out] _suite    the suite.
 * @param[in]     _result   whether the check held.
 * @param[in]     _name     the check's name; copied.
 * @param[in]     _message  what a failure means; copied; may be `NULL`.
 * @param[in]     _file     the source file; copied.
 * @param[in]     _line     the source line.
 * @return `_result`.
 */
bool d_test_suite_check_msg(struct dj_test_suite* _suite,
                            bool                  _result,
                            const char*           _name,
                            const char*           _message,
                            const char*           _file,
                            int                   _line);

/**
 * @brief Marks the test opened last as skipped: neither passed nor failed,
 *        and not empty.
 *
 * @param[in,out] _suite   the suite.
 * @param[in]     _reason  why, for the report; copied; may be `NULL`.
 * @return `true` if recorded, `false` when no test is open or out of memory.
 */
bool d_test_suite_skip(struct dj_test_suite* _suite,
                       const char*           _reason);

/**
 * @brief Adds a module spec: opens its module, blocks and tests in order and
 *        runs each test's function once, recording its checks.
 *
 * @note A test the options' filter excludes is neither opened nor run. A row
 *       with a `NULL` function is recorded as failed.
 *
 * @param[in,out] _suite   the suite.
 * @param[in]     _module  the spec; it and its strings must outlive the call.
 * @return `true` if every node was recorded, `false` otherwise.
 */
bool d_test_suite_add_module(struct dj_test_suite*             _suite,
                             const struct dj_test_module_spec* _module);

// 3.3    Running a suite
//------------------------------------------------------------------------------
/**
 * @brief Reports a suite: the tests, as its options ask, the summary, and each
 *        module's results table.
 *
 * @note Checks are evaluated when they are made, so this only reports; it
 *       runs nothing, and may be called more than once.
 *
 * @param[in]  _suite    the suite.
 * @param[out] _summary  receives the counts; may be `NULL`.
 * @return D_TEST_SUITE_EXIT_PASSED, _FAILED or _ERROR.
 */
int  d_test_suite_run(const struct dj_test_suite* _suite,
                      struct dj_test_summary*     _summary);

/**
 * @brief The whole of a runner's main: reads the command line, adds each
 *        module spec in order, reports, and frees the suite.
 *
 * @param[in] _argc     the argument count, as main received it.
 * @param[in] _argv     the arguments, as main received them.
 * @param[in] _title    the report's heading.
 * @param[in] _modules  the module specs, in order.
 * @param[in] _count    how many there are.
 * @return the process's exit code: D_TEST_SUITE_EXIT_*, or 0 after `--help`.
 */
int  d_test_run_modules(int                                      _argc,
                        char**                                   _argv,
                        const char*                              _title,
                        const struct dj_test_module_spec* const* _modules,
                        size_t                                   _count);

// 3.4    Support for the check macros
//------------------------------------------------------------------------------
// The spec macros below reach the suite being run, which
// d_test_suite_add_module sets for the length of each test function, through
// these three. Outside a spec run there is no such suite: a failure is then
// written to stderr, and the macro still returns false from the test; a note
// is dropped.
bool d_test_suite_require(bool        _result,
                          const char* _expr,
                          const char* _file,
                          int         _line);

void d_test_suite_skip_current(const char* _reason);

bool d_test_suite_note_current(const char* _format,
                               ...) D_FORMAT_PRINTF(1, 2);

// 3.5    Check helpers
//------------------------------------------------------------------------------
/**
 * @brief NULL-safe string equality, for a check's condition.
 *
 * @note It is a function, not part of a macro, so that an array argument
 *       decays to a pointer and its NULL test is a real one.
 *
 * @param[in] _a  the first string; may be `NULL`.
 * @param[in] _b  the second string; may be `NULL`.
 * @return `true` if both are `NULL`, or both are non-`NULL` and equal.
 */
bool d_test_suite_str_equal(const char* _a,
                            const char* _b);


//==============================================================================
// 4.  CHECK MACROS
//==============================================================================


// 4.1    Imperative checks
//------------------------------------------------------------------------------
// 4.1.1
// D_TEST_CHECK
//   macro: records whether _expr holds against the open test, with its source
// text, file and line. Evaluates _expr once; the expression's value is the
// result, so `if (!D_TEST_CHECK(s, p != NULL)) { return; }` stops a section.
#define D_TEST_CHECK(_suite, _expr)                                           \
    d_test_suite_check((_suite),                                              \
                       ((_expr) ? true : false),                              \
                       #_expr,                                                \
                       __FILE__,                                              \
                       __LINE__)

// 4.1.2
// D_TEST_CHECK_MSG
//   macro: records a named check with a message, as a standalone D_ASSERT_*
// leaf did.
#define D_TEST_CHECK_MSG(_suite, _name, _expr, _message)                      \
    d_test_suite_check_msg((_suite),                                          \
                           ((_expr) ? true : false),                          \
                           (_name),                                           \
                           (_message),                                        \
                           __FILE__,                                          \
                           __LINE__)

// 4.2    Spec checks
//------------------------------------------------------------------------------
// 4.2.1
// D_TEST_REQUIRE
//   macro: inside a spec test, records whether _expr holds, and returns false
// from the test when it does not. Evaluates _expr once.
#define D_TEST_REQUIRE(_expr)                                                 \
    do                                                                        \
    {                                                                         \
        if (!d_test_suite_require(((_expr) ? true : false),                   \
                                  #_expr,                                     \
                                  __FILE__,                                   \
                                  __LINE__))                                  \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)

// 4.2.2
// D_TEST_SKIP
//   macro: inside a spec test, marks it skipped with a reason and returns
// true, so the test counts as neither passed nor failed.
#define D_TEST_SKIP(_reason)                                                  \
    do                                                                        \
    {                                                                         \
        d_test_suite_skip_current(_reason);                                   \
                                                                              \
        return true;                                                          \
    } while (0)

// 4.2.3
// D_TEST_NOTE
//   macro: inside a spec test, attaches a note to the test, as
// d_test_suite_note does: a printf format, then its arguments. Returns
// whether the note was recorded; the test goes on either way. Defined where
// the preprocessor has variadic macros (not ISO C++98); without them, a
// test calls d_test_suite_note_current, which the macro names.
#if D_ENV_PP_HAS_VARIADIC_MACROS
    #define D_TEST_NOTE(...)                                                  \
        d_test_suite_note_current(__VA_ARGS__)
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_SUITE_H
