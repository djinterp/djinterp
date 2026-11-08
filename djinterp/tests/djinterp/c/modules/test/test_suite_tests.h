/*******************************************************************************
* djinterp [test]                                             test_suite_tests.h
*
* DTest's suite for its own C face, test_suite.h.
*   The suite is written in the form it tests, the C spec form, and is the
* worked example of it: a header with two faces, section translation units,
* and a runner that chooses options.
*
*   - Normal face (the section translation units): the header under test, the
*     shared fixtures, and the test declarations.
*   - Spec face (the runner, which defines DTEST_SPEC_MODE first): the same
*     declarations and the provider, d_tests_test_suite_spec(), whose tables
*     name every test by address.
*
*   Each test builds an inner suite of its own, reporting to a temporary file
* so its report stays out of the console, and checks what the inner suite
* recorded with D_TEST_REQUIRE against the outer one. Inner runs nest: the
* outer test's requires still reach the outer suite after an inner spec run.
*
*
* TABLE OF CONTENTS
* =================
* 1.  FIXTURES
*     --------
*     1.  Inner suites
* 2.  TESTS
*     -----
*     1.  Building
*     2.  Specs
*     3.  Report and options
* 3.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEST_TEST_SUITE_TESTS_H
#define DJINTERP_TESTS_C_TEST_TEST_SUITE_TESTS_H 1

// std
#include <stddef.h>  // size_t
#include <stdio.h>   // FILE
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // under test


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURES
//==============================================================================


// 1.1    Inner suites
//------------------------------------------------------------------------------
// d_tests_test_suite_quiet makes an inner suite that reports to a temporary
// file, handed back through _stream; d_tests_test_suite_report runs the
// suite, frees it, and reads that file back into a buffer the caller frees;
// d_tests_test_suite_contains says whether a report holds a piece of text.
struct dj_test_suite* d_tests_test_suite_quiet(int    _show,
                                               FILE** _stream);
char*                 d_tests_test_suite_report(
                          struct dj_test_suite*   _suite,
                          FILE*                   _stream,
                          int*                    _code,
                          struct dj_test_summary* _summary);
bool                  d_tests_test_suite_contains(const char* _report,
                                                  const char* _text);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Building
//------------------------------------------------------------------------------
bool d_tests_test_suite_new_and_free(void);
bool d_tests_test_suite_counts_checks(void);
bool d_tests_test_suite_open_closes(void);
bool d_tests_test_suite_check_opens_test(void);
bool d_tests_test_suite_null_suite(void);
bool d_tests_test_suite_describe(void);
bool d_tests_test_suite_skip(void);
bool d_tests_test_suite_check_msg(void);

// 2.2    Specs
//------------------------------------------------------------------------------
bool d_tests_test_suite_spec_runs_once(void);
bool d_tests_test_suite_spec_require(void);
bool d_tests_test_suite_spec_false_return(void);
bool d_tests_test_suite_spec_null_fn(void);
bool d_tests_test_suite_spec_skip(void);
bool d_tests_test_suite_spec_filter(void);
bool d_tests_test_suite_spec_nesting(void);

// 2.3    Report and options
//------------------------------------------------------------------------------
bool d_tests_test_suite_exit_codes(void);
bool d_tests_test_suite_empty_is_failure(void);
bool d_tests_test_suite_table_format(void);
bool d_tests_test_suite_table_escape(void);
bool d_tests_test_suite_show_levels(void);
bool d_tests_test_suite_options_defaults(void);
bool d_tests_test_suite_options_parse(void);
bool d_tests_test_suite_many_checks(void);
bool d_tests_test_suite_str_equal(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_test_suite_build_rows
//   constant: the building block's rows.
static const struct dj_test_case_spec d_tests_test_suite_build_rows[] =
{
    { "d_tests_test_suite_new_and_free",
      "a new suite is empty and frees cleanly, NULL included",
      d_tests_test_suite_new_and_free },
    { "d_tests_test_suite_counts_checks",
      "checks count against the open test, failures apart",
      d_tests_test_suite_counts_checks },
    { "d_tests_test_suite_open_closes",
      "opening a block or module closes the open test",
      d_tests_test_suite_open_closes },
    { "d_tests_test_suite_check_opens_test",
      "a check with no test open opens one named after it",
      d_tests_test_suite_check_opens_test },
    { "d_tests_test_suite_null_suite",
      "every call refuses a NULL suite; a check still returns its result",
      d_tests_test_suite_null_suite },
    { "d_tests_test_suite_describe",
      "a descriptor goes to the node opened last and into the table",
      d_tests_test_suite_describe },
    { "d_tests_test_suite_skip",
      "a skipped test counts as skipped, not passed, failed or empty",
      d_tests_test_suite_skip },
    { "d_tests_test_suite_check_msg",
      "a named check reports its name and message",
      d_tests_test_suite_check_msg }
};

// d_tests_test_suite_spec_rows
//   constant: the spec block's rows.
static const struct dj_test_case_spec d_tests_test_suite_spec_rows[] =
{
    { "d_tests_test_suite_spec_runs_once",
      "adding a spec runs each test function exactly once",
      d_tests_test_suite_spec_runs_once },
    { "d_tests_test_suite_spec_require",
      "D_TEST_REQUIRE counts each require and stops at the first failure",
      d_tests_test_suite_spec_require },
    { "d_tests_test_suite_spec_false_return",
      "a test returning false fails though every require held",
      d_tests_test_suite_spec_false_return },
    { "d_tests_test_suite_spec_null_fn",
      "a row without a function is recorded as failed",
      d_tests_test_suite_spec_null_fn },
    { "d_tests_test_suite_spec_skip",
      "D_TEST_SKIP skips a spec test",
      d_tests_test_suite_spec_skip },
    { "d_tests_test_suite_spec_filter",
      "the filter runs only matching tests and opens no empty headings",
      d_tests_test_suite_spec_filter },
    { "d_tests_test_suite_spec_nesting",
      "an inner spec run gives the outer suite back to D_TEST_REQUIRE",
      d_tests_test_suite_spec_nesting }
};

// d_tests_test_suite_report_rows
//   constant: the report block's rows.
static const struct dj_test_case_spec d_tests_test_suite_report_rows[] =
{
    { "d_tests_test_suite_exit_codes",
      "the run returns 0, 1 or 2 for passed, failed and error",
      d_tests_test_suite_exit_codes },
    { "d_tests_test_suite_empty_is_failure",
      "a test with no check fails, and so does a run with no test",
      d_tests_test_suite_empty_is_failure },
    { "d_tests_test_suite_table_format",
      "the results table has C++'s caption, columns and rule",
      d_tests_test_suite_table_format },
    { "d_tests_test_suite_table_escape",
      "a table cell turns | into / and a newline into a space",
      d_tests_test_suite_table_escape },
    { "d_tests_test_suite_show_levels",
      "failures, tests and checks each list what they say",
      d_tests_test_suite_show_levels },
    { "d_tests_test_suite_options_defaults",
      "the defaults are every test, the table on, stdout, no filter",
      d_tests_test_suite_options_defaults },
    { "d_tests_test_suite_options_parse",
      "each argument sets its option; --help and a bad argument say so",
      d_tests_test_suite_options_parse },
    { "d_tests_test_suite_many_checks",
      "a hundred thousand passing checks are counted, not kept",
      d_tests_test_suite_many_checks },
    { "d_tests_test_suite_str_equal",
      "the NULL-safe string equality: two NULLs equal, one NULL not",
      d_tests_test_suite_str_equal }
};

// d_tests_test_suite_blocks
//   constant: the module's blocks, one per section translation unit.
static const struct dj_test_block_spec d_tests_test_suite_blocks[] =
{
    { "building",
      "opening nodes, counting checks, describing and skipping",
      d_tests_test_suite_build_rows,
      sizeof(d_tests_test_suite_build_rows) /
          sizeof(d_tests_test_suite_build_rows[0]) },
    { "specs",
      "running declarative module specs",
      d_tests_test_suite_spec_rows,
      sizeof(d_tests_test_suite_spec_rows) /
          sizeof(d_tests_test_suite_spec_rows[0]) },
    { "report",
      "exit codes, the results table, report detail and options",
      d_tests_test_suite_report_rows,
      sizeof(d_tests_test_suite_report_rows) /
          sizeof(d_tests_test_suite_report_rows[0]) }
};

// d_tests_test_suite_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_test_suite_module =
{
    "test_suite",
    "DTest's C face: building, running and reporting a suite",
    d_tests_test_suite_blocks,
    sizeof(d_tests_test_suite_blocks) / sizeof(d_tests_test_suite_blocks[0])
};

// d_tests_test_suite_spec
//   function: the suite's provider, for its runner.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_test_suite_spec(void)
{
    return &d_tests_test_suite_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEST_TEST_SUITE_TESTS_H
