/*******************************************************************************
* djinterp [test]                                    test_suite_tests_building.c
*
* test_suite.h's suite, section 1: building a suite imperatively.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_building.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdlib.h>  // free


/*
d_tests_test_suite_new_and_free
  A new suite is empty.
  Tests the following:
  - d_test_suite_new returns a suite
  - its summary counts nothing
  - d_test_suite_free accepts NULL
*/
bool
d_tests_test_suite_new_and_free(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.modules == 0);
    D_TEST_REQUIRE(summary.tests == 0);
    D_TEST_REQUIRE(summary.checks == 0);
    free(report);

    d_test_suite_free(NULL);

    return true;
}

/*
d_tests_test_suite_counts_checks
  Checks count against the open test.
  Tests the following:
  - passing and failing checks both count
  - only failing ones count as failed
  - a test with a failing check fails, one without passes
*/
bool
d_tests_test_suite_counts_checks(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "m"));
    D_TEST_REQUIRE(d_test_suite_block(suite,
                                      "b"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "good"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                1 + 1 == 2));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                2 > 1));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "bad"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                1 == 1));
    D_TEST_REQUIRE(!D_TEST_CHECK(suite,
                                 1 == 2));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.modules == 1);
    D_TEST_REQUIRE(summary.tests == 2);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(summary.checks == 4);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[PASS] good"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] bad"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "1 == 2"));
    free(report);

    return true;
}

/*
d_tests_test_suite_open_closes
  Opening a block or a module closes the open test.
  Tests the following:
  - a check after a new block opens a test of its own
  - so the earlier test keeps only its own checks
*/
bool
d_tests_test_suite_open_closes(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "m"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "first"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                true));
    D_TEST_REQUIRE(d_test_suite_block(suite,
                                      "b"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                true));
    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "n"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                true));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.modules == 2);
    D_TEST_REQUIRE(summary.tests == 3);
    D_TEST_REQUIRE(summary.checks == 3);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    free(report);

    return true;
}

/*
d_tests_test_suite_check_opens_test
  A check with no test open is counted, not lost.
  Tests the following:
  - it opens a test named after the expression
  - the failure is reported beneath that test
*/
bool
d_tests_test_suite_check_opens_test(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(!D_TEST_CHECK(suite,
                                 3 < 2));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests == 1);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] 3 < 2"));
    free(report);

    return true;
}

/*
d_tests_test_suite_null_suite
  Every call refuses a NULL suite.
  Tests the following:
  - the open calls, describe and skip return false
  - a check returns its own result
  - the run reports an error
*/
bool
d_tests_test_suite_null_suite(void)
{
    D_TEST_REQUIRE(!d_test_suite_module(NULL,
                                        "m"));
    D_TEST_REQUIRE(!d_test_suite_block(NULL,
                                       "b"));
    D_TEST_REQUIRE(!d_test_suite_test(NULL,
                                      "t"));
    D_TEST_REQUIRE(!d_test_suite_describe(NULL,
                                          "d"));
    D_TEST_REQUIRE(!d_test_suite_skip(NULL,
                                      "r"));
    D_TEST_REQUIRE(!d_test_suite_add_module(NULL,
                                            NULL));
    D_TEST_REQUIRE(d_test_suite_check(NULL,
                                      true,
                                      "x",
                                      __FILE__,
                                      __LINE__));

    struct dj_test_summary summary = {0};

    D_TEST_REQUIRE(d_test_suite_run(NULL,
                                    &summary) ==
                   D_TEST_SUITE_EXIT_ERROR);

    return true;
}

/*
d_tests_test_suite_describe
  A descriptor goes to the node opened last.
  Tests the following:
  - describe with nothing open fails
  - a module's descriptor follows its name in the report
  - a test's descriptor fills its table row
*/
bool
d_tests_test_suite_describe(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(!d_test_suite_describe(suite,
                                          "nothing open"));
    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "m"));
    D_TEST_REQUIRE(d_test_suite_describe(suite,
                                         "the module"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "t"));
    D_TEST_REQUIRE(d_test_suite_describe(suite,
                                         "what t verifies"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                true));

    int code = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             NULL);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "module m -- the module"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "| t    | what t verifies |"));
    free(report);

    return true;
}

/*
d_tests_test_suite_skip
  A skipped test is neither passed, failed nor empty.
  Tests the following:
  - skipping with no test open fails
  - the reason is reported
  - a run of passed and skipped tests passes
*/
bool
d_tests_test_suite_skip(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(!d_test_suite_skip(suite,
                                      "no test"));
    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "m"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "skipped"));
    D_TEST_REQUIRE(d_test_suite_skip(suite,
                                     "not on this platform"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "ran"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                true));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests == 2);
    D_TEST_REQUIRE(summary.tests_skipped == 1);
    D_TEST_REQUIRE(summary.tests_failed == 0);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[SKIP] skipped: not on this platform"));
    free(report);

    return true;
}

/*
d_tests_test_suite_check_msg
  A named check reports its name and message.
  Tests the following:
  - the name stands where the expression would
  - the message follows it after " -- "
*/
bool
d_tests_test_suite_check_msg(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);

    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "t"));
    D_TEST_REQUIRE(!D_TEST_CHECK_MSG(suite,
                                     "size_matches",
                                     1 == 0,
                                     "size should be 1"));

    int code = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             NULL);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "size_matches -- size should be 1"));
    free(report);

    return true;
}
