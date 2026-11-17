/*******************************************************************************
* djinterp [c][test]                                    test_suite_tests_specs.c
*
* test_suite.h's suite, section 2: running declarative module specs.
*   The specs under test are built here from small inner test functions, and
* run in inner suites; the inner functions' requires reach the inner suite,
* and the outer test's reach the outer one again afterwards.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_specs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.15
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdlib.h>  // free
#include <string.h>  // strcmp


// d_tests_test_suite_calls
//   variable: how many times the counting inner test has run.
static int d_tests_test_suite_calls = 0;

/*
d_tests_test_suite_inner_counting
  An inner test that counts its runs and passes.
*/
static bool
d_tests_test_suite_inner_counting(void)
{
    d_tests_test_suite_calls++;
    D_TEST_REQUIRE(d_tests_test_suite_calls > 0);

    return true;
}

/*
d_tests_test_suite_inner_second_fails
  An inner test whose second of three requires fails.
*/
static bool
d_tests_test_suite_inner_second_fails(void)
{
    D_TEST_REQUIRE(1 == 1);
    D_TEST_REQUIRE(1 == 2);
    D_TEST_REQUIRE(2 == 2);

    return true;
}

/*
d_tests_test_suite_inner_returns_false
  An inner test whose requires hold and which returns false.
*/
static bool
d_tests_test_suite_inner_returns_false(void)
{
    D_TEST_REQUIRE(true);

    return false;
}

/*
d_tests_test_suite_inner_skips
  An inner test that skips itself.
*/
static bool
d_tests_test_suite_inner_skips(void)
{
    D_TEST_SKIP("needs a feature this build lacks");
}

/*
d_tests_test_suite_inner_bare
  An inner test with no require, which returns true: one check, of its return.
*/
static bool
d_tests_test_suite_inner_bare(void)
{
    return true;
}

/*
d_tests_test_suite_spec_runs_once
  Adding a spec runs each test function once.
  Tests the following:
  - the function ran exactly once
  - the run opened one module and one test, and passed
*/
bool
d_tests_test_suite_spec_runs_once(void)
{
    struct dj_test_case_spec row = { "counting", "counts its runs",
                                     d_tests_test_suite_inner_counting };

    d_tests_test_suite_calls = 0;

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_calls == 1);
    D_TEST_REQUIRE(summary.modules == 1);
    D_TEST_REQUIRE(summary.tests == 1);
    D_TEST_REQUIRE(summary.checks == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    free(report);

    return true;
}

/*
d_tests_test_suite_spec_require
  D_TEST_REQUIRE counts each require and stops at the first failure.
  Tests the following:
  - two of three requires ran, one failed
  - the test failed, naming the failed expression
*/
bool
d_tests_test_suite_spec_require(void)
{
    struct dj_test_case_spec row = { "second_fails", "fails its second",
                                     d_tests_test_suite_inner_second_fails };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "1 == 2"));
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "2 == 2"));
    free(report);

    return true;
}

/*
d_tests_test_suite_spec_false_return
  A test returning false fails though every require held, and a test with no
require counts its return as one check.
  Tests the following:
  - the false return adds a failed check
  - a bare true return adds one passing check
*/
bool
d_tests_test_suite_spec_false_return(void)
{
    struct dj_test_case_spec row  = { "returns_false", "returns false",
                                      d_tests_test_suite_inner_returns_false };
    struct dj_test_case_spec bare = { "bare", "returns true, checks nothing",
                                      d_tests_test_suite_inner_bare };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "the test returned false"));
    free(report);

    report = d_tests_test_suite_run_row(&bare,
                                        NULL,
                                        &summary,
                                        &code);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    free(report);

    return true;
}

/*
d_tests_test_suite_spec_null_fn
  A row without a function is recorded as failed.
  Tests the following:
  - the test fails with "no test function"
*/
bool
d_tests_test_suite_spec_null_fn(void)
{
    struct dj_test_case_spec row     = { "missing", "has no function", NULL };
    struct dj_test_summary   summary = {0};
    int                      code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "no test function"));
    free(report);

    return true;
}

/*
d_tests_test_suite_spec_skip
  D_TEST_SKIP skips a spec test.
  Tests the following:
  - the test counts as skipped, with its reason
  - the run passes
*/
bool
d_tests_test_suite_spec_skip(void)
{
    struct dj_test_case_spec row = { "skips", "skips itself",
                                     d_tests_test_suite_inner_skips };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests_skipped == 1);
    D_TEST_REQUIRE(summary.tests_failed == 0);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "needs a feature this build lacks"));
    free(report);

    return true;
}

/*
d_tests_test_suite_spec_filter
  The filter runs only matching tests.
  Tests the following:
  - a filter matching the test's path runs it
  - a filter matching nothing runs nothing, opens no heading, and fails
*/
bool
d_tests_test_suite_spec_filter(void)
{
    struct dj_test_case_spec row = { "counting", "counts its runs",
                                     d_tests_test_suite_inner_counting };

    d_tests_test_suite_calls = 0;

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              "inner_block/count",
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_calls == 1);
    D_TEST_REQUIRE(summary.tests == 1);
    free(report);

    report = d_tests_test_suite_run_row(&row,
                                        "no such test",
                                        &summary,
                                        &code);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_calls == 1);
    D_TEST_REQUIRE(summary.modules == 0);
    D_TEST_REQUIRE(summary.tests == 0);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "module inner"));
    free(report);

    return true;
}

// d_tests_test_suite_middle_checks
//   variable: the checks the middle suite counted, read by the nesting test.
static size_t d_tests_test_suite_middle_checks = 0;

/*
d_tests_test_suite_inner_middle
  A middle test that runs an inner spec of its own, then requires twice: the
two must reach the middle suite, not the inner one and not stderr.
*/
static bool
d_tests_test_suite_inner_middle(void)
{
    struct dj_test_case_spec row = { "bare", "returns true",
                                     d_tests_test_suite_inner_bare };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    free(report);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(summary.checks == 1);

    return true;
}

/*
d_tests_test_suite_spec_nesting
  An inner spec run gives the outer suite back to D_TEST_REQUIRE.
  Tests the following:
  - the middle suite counts the middle test's two requires, made after its
    own inner run
*/
bool
d_tests_test_suite_spec_nesting(void)
{
    struct dj_test_case_spec row = { "middle", "runs an inner spec",
                                     d_tests_test_suite_inner_middle };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    d_tests_test_suite_middle_checks = summary.checks;
    D_TEST_REQUIRE(d_tests_test_suite_middle_checks == 2);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    free(report);

    return true;
}

// d_tests_test_suite_expected
//   variable: how many of the expecting inner test's four checks it reached.
static int d_tests_test_suite_expected = 0;

/*
d_tests_test_suite_inner_expects
  An inner test whose second of four expectations fails; it goes on to the
end and returns true, and records what each D_TEST_EXPECT gave.
*/
static bool
d_tests_test_suite_inner_expects(void)
{
    const bool first  = D_TEST_EXPECT(1 == 1);
    const bool second = D_TEST_EXPECT(1 == 2);

    d_tests_test_suite_expected = 2;

    // the values the two gave: true, then false
    if ( (first) &&
         (!second) )
    {
        d_tests_test_suite_expected++;
    }

    (void)D_TEST_EXPECT(2 == 2);
    d_tests_test_suite_expected++;

    return true;
}

/*
d_tests_test_suite_spec_expect
  D_TEST_EXPECT counts each check and goes on after a failure.
  Tests the following:
  - every expectation is counted, the failed one once
  - the test reached its end, and each D_TEST_EXPECT gave its check's result
  - a failed expectation fails the test though the test returned true
  - the failed one is reported with its file, line and expression
*/
bool
d_tests_test_suite_spec_expect(void)
{
    struct dj_test_case_spec row = { "expects", "four expectations",
                                     d_tests_test_suite_inner_expects };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    d_tests_test_suite_expected = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_expected == 4);
    D_TEST_REQUIRE(summary.checks == 3);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "test_suite_tests_specs.c:"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               ": 1 == 2\n"));
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "the test returned"));
    free(report);

    return true;
}

// dj_tests_test_suite_point
//   struct: a pair, for a compound literal whose braces hold a bare comma.
struct dj_tests_test_suite_point
{
    int x;
    int y;
};

/*
d_tests_test_suite_inner_variadic
  An inner test whose requires hold a comma no parenthesis protects: a
compound literal's, which C's preprocessor splits as it splits a C++
template's arguments.
*/
static bool
d_tests_test_suite_inner_variadic(void)
{
    D_TEST_REQUIRE((struct dj_tests_test_suite_point){ 1, 2 }.y == 2);
    D_TEST_REQUIRE((struct dj_tests_test_suite_point){ 3, 4 }.x == 4);

    return true;
}

/*
d_tests_test_suite_spec_variadic
  D_TEST_REQUIRE takes an expression with a bare comma.
  Tests the following:
  - both requires are made, and the second fails and stops the test
  - the failed one's text is the whole expression, its comma included
*/
bool
d_tests_test_suite_spec_variadic(void)
{
    struct dj_test_case_spec row = { "variadic", "two bare commas",
                                     d_tests_test_suite_inner_variadic };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "(struct dj_tests_test_suite_"
                                               "point){ 3, 4 }.x == 4\n"));
    free(report);

    return true;
}

// d_tests_test_suite_case_rows
//   constant: two rows written by D_TEST_CASE.
static const struct dj_test_case_spec d_tests_test_suite_case_rows[] =
{
    D_TEST_CASE(d_tests_test_suite_inner_bare,
                "returns true"),
    D_TEST_CASE(d_tests_test_suite_inner_returns_false,
                "returns false")
};

/*
d_tests_test_suite_spec_case
  D_TEST_CASE writes a row named after its function.
  Tests the following:
  - each row's name is its function's name, its descriptor as given, and
    its function the one named
  - the rows run as any other: one passes, one fails, under their names
*/
bool
d_tests_test_suite_spec_case(void)
{
    const struct dj_test_case_spec* const rows = d_tests_test_suite_case_rows;

    D_TEST_REQUIRE(strcmp(rows[0].name,
                          "d_tests_test_suite_inner_bare") == 0);
    D_TEST_REQUIRE(strcmp(rows[0].descriptor,
                          "returns true") == 0);
    D_TEST_REQUIRE(rows[0].fn == d_tests_test_suite_inner_bare);
    D_TEST_REQUIRE(strcmp(rows[1].name,
                          "d_tests_test_suite_inner_returns_false") == 0);
    D_TEST_REQUIRE(rows[1].fn == d_tests_test_suite_inner_returns_false);

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&rows[0],
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[PASS] d_tests_test_suite_"
                                               "inner_bare\n"));
    free(report);
    report = d_tests_test_suite_run_row(&rows[1],
                                        NULL,
                                        &summary,
                                        &code);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] d_tests_test_suite_"
                                               "inner_returns_false\n"));
    free(report);

    return true;
}
