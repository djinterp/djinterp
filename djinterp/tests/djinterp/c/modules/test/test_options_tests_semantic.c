/*******************************************************************************
* djinterp [test]                                  test_options_tests_semantic.c
*
* test_options.h's suite, block "config_semantic_all": the
* D_TEST_SHOULD_COUNT_*, D_TEST_SHOULD_PRINT_* and D_TEST_SHOULD_PUSH_* checks.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_semantic.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 7.  SEMANTIC CHECK MACRO TESTS
//==============================================================================

/*
d_tests_options_semantic_count
  Tests the SHOULD_COUNT_* semantic check macros.
  Tests the following:
  - SHOULD_COUNT_FAILURES detects any fail counter flag
  - SHOULD_COUNT_PASSES detects any pass counter flag
  - Individual category checks (asserts, tests, blocks, modules)
  - Silent mode returns false for all
*/
bool
d_tests_options_semantic_count(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_semantic_count");

    struct dj_test_options* verbose = d_test_options_new(D_TEST_MODE_VERBOSE);
    struct dj_test_options* silent  = d_test_options_new(D_TEST_MODE_SILENT);
    struct dj_test_options* fail_only =
        d_test_options_new(D_TEST_MSG_COUNT_FAIL_ALL);

    // an allocation failed: release the rest, and the test fails
    if ( (!verbose) ||
         (!silent)  ||
         (!fail_only) )
    {
        // release whichever were made; d_test_options_free takes NULL
        d_test_options_free(verbose);
        d_test_options_free(silent);
        d_test_options_free(fail_only);

        return D_TEST_CHECK_MSG(_suite,
                                "semantic_count_alloc",
                                false,
                                "Failed to allocate configs");
    }

    // test 1: verbose has all count flags
    result = D_TEST_CHECK_MSG(_suite,
                              "count_failures_verbose",
                              D_TEST_SHOULD_COUNT_FAILURES(verbose) == true,
                              "Verbose should count failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_passes_verbose",
                              D_TEST_SHOULD_COUNT_PASSES(verbose) == true,
                              "Verbose should count passes") && result;

    // test 2: silent has no count flags
    result = D_TEST_CHECK_MSG(_suite,
                              "count_failures_silent",
                              D_TEST_SHOULD_COUNT_FAILURES(silent) == false,
                              "Silent should not count failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_passes_silent",
                              D_TEST_SHOULD_COUNT_PASSES(silent) == false,
                              "Silent should not count passes") && result;

    // test 3: fail_only counts failures but not passes
    result = D_TEST_CHECK_MSG(_suite,
                              "count_failures_fail_only",
                              D_TEST_SHOULD_COUNT_FAILURES(fail_only) == true,
                              "Fail-only should count failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_passes_fail_only",
                              D_TEST_SHOULD_COUNT_PASSES(fail_only) == false,
                              "Fail-only should not count passes") && result;

    // test 4: individual category checks on verbose
    result = D_TEST_CHECK_MSG(_suite,
                              "count_asserts_fail_verbose",
                              D_TEST_SHOULD_COUNT_ASSERTS_FAIL(verbose) == true,
                              "Verbose should count assert failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_asserts_pass_verbose",
                              D_TEST_SHOULD_COUNT_ASSERTS_PASS(verbose) == true,
                              "Verbose should count assert passes") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_tests_fail_verbose",
                              D_TEST_SHOULD_COUNT_TESTS_FAIL(verbose) == true,
                              "Verbose should count test failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_blocks_pass_verbose",
                              D_TEST_SHOULD_COUNT_BLOCKS_PASS(verbose) == true,
                              "Verbose should count block passes") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_modules_fail_verbose",
                              D_TEST_SHOULD_COUNT_MODULES_FAIL(verbose) == true,
                              "Verbose should count module failures") && result;

    d_test_options_free(verbose);
    d_test_options_free(silent);
    d_test_options_free(fail_only);

    return result;
}

/*
d_tests_options_semantic_print
  Tests the SHOULD_PRINT_* semantic check macros.
  Tests the following:
  - SHOULD_PRINT_FAILURES detects any print fail flag
  - SHOULD_PRINT_PASSES detects any print pass flag
  - Individual category checks
  - Normal mode prints failures but not passes
*/
bool
d_tests_options_semantic_print(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_semantic_print");

    struct dj_test_options* normal  = d_test_options_new(D_TEST_MODE_NORMAL);
    struct dj_test_options* verbose = d_test_options_new(D_TEST_MODE_VERBOSE);

    // an allocation failed: release the rest, and the test fails
    if ( (!normal) ||
         (!verbose) )
    {
        // release whichever were made; d_test_options_free takes NULL
        d_test_options_free(normal);
        d_test_options_free(verbose);

        return D_TEST_CHECK_MSG(_suite,
                                "semantic_print_alloc",
                                false,
                                "Failed to allocate configs");
    }

    // test 1: normal prints failures but not passes
    result = D_TEST_CHECK_MSG(_suite,
                              "print_failures_normal",
                              D_TEST_SHOULD_PRINT_FAILURES(normal) == true,
                              "Normal should print failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_passes_normal",
                              D_TEST_SHOULD_PRINT_PASSES(normal) == false,
                              "Normal should not print passes") && result;

    // test 2: verbose prints everything
    result = D_TEST_CHECK_MSG(_suite,
                              "print_failures_verbose",
                              D_TEST_SHOULD_PRINT_FAILURES(verbose) == true,
                              "Verbose should print failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_passes_verbose",
                              D_TEST_SHOULD_PRINT_PASSES(verbose) == true,
                              "Verbose should print passes") && result;

    // test 3: individual category checks on normal
    result = D_TEST_CHECK_MSG(_suite,
                              "print_asserts_fail_normal",
                              D_TEST_SHOULD_PRINT_ASSERTS_FAIL(normal) == true,
                              "Normal should print assert failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_asserts_pass_normal",
                              D_TEST_SHOULD_PRINT_ASSERTS_PASS(normal) == false,
                              "Normal should not print assert passes") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_tests_fail_normal",
                              D_TEST_SHOULD_PRINT_TESTS_FAIL(normal) == true,
                              "Normal should print test failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_blocks_fail_normal",
                              D_TEST_SHOULD_PRINT_BLOCKS_FAIL(normal) == true,
                              "Normal should print block failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_modules_fail_normal",
                              D_TEST_SHOULD_PRINT_MODULES_FAIL(normal) == true,
                              "Normal should print module failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_modules_pass_normal",
                              D_TEST_SHOULD_PRINT_MODULES_PASS(normal) == false,
                              "Normal should not print module passes") && result;

    d_test_options_free(normal);
    d_test_options_free(verbose);

    return result;
}

/*
d_tests_options_semantic_push
  Tests the SHOULD_PUSH_* semantic check macros.
  Tests the following:
  - Returns true when corresponding settings flag is set
  - Returns false when corresponding settings flag is not set
*/
bool
d_tests_options_semantic_push(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_semantic_push");

    struct dj_test_options* push_all =
        d_test_options_new(D_TEST_SETTINGS_STACK_PUSH_ALL);
    struct dj_test_options* no_push = d_test_options_new(D_TEST_MODE_NORMAL);
    struct dj_test_options* push_fail_only =
        d_test_options_new(
            D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL));

    // an allocation failed: release the rest, and the test fails
    if ( (!push_all) ||
         (!no_push)  ||
         (!push_fail_only) )
    {
        // release whichever were made; d_test_options_free takes NULL
        d_test_options_free(push_all);
        d_test_options_free(no_push);
        d_test_options_free(push_fail_only);

        return D_TEST_CHECK_MSG(_suite,
                                "semantic_push_alloc",
                                false,
                                "Failed to allocate configs");
    }

    // test 1: push_all has all push flags
    result = D_TEST_CHECK_MSG(_suite,
                              "push_failures_all",
                              D_TEST_SHOULD_PUSH_FAILURES(push_all) == true,
                              "PUSH_ALL should push failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_passes_all",
                              D_TEST_SHOULD_PUSH_PASSES(push_all) == true,
                              "PUSH_ALL should push passes") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_warnings_all",
                              D_TEST_SHOULD_PUSH_WARNINGS(push_all) == true,
                              "PUSH_ALL should push warnings") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_info_all",
                              D_TEST_SHOULD_PUSH_INFO(push_all) == true,
                              "PUSH_ALL should push info") && result;

    // test 2: no_push has no push flags
    result = D_TEST_CHECK_MSG(_suite,
                              "push_failures_none",
                              D_TEST_SHOULD_PUSH_FAILURES(no_push) == false,
                              "No-push config should not push failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_passes_none",
                              D_TEST_SHOULD_PUSH_PASSES(no_push) == false,
                              "No-push config should not push passes") && result;

    // test 3: push_fail_only has only PUSH_FAIL
    result = D_TEST_CHECK_MSG(_suite,
                              "push_fail_only_failures",
                              D_TEST_SHOULD_PUSH_FAILURES(push_fail_only) == true,
                              "Push-fail config should push failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_fail_only_passes",
                              D_TEST_SHOULD_PUSH_PASSES(push_fail_only) == false,
                              "Push-fail config should not push passes") && result;

    d_test_options_free(push_all);
    d_test_options_free(no_push);
    d_test_options_free(push_fail_only);

    return result;
}

/*
d_tests_options_semantic_all
  Aggregation function that runs all semantic check macro tests.
*/
bool
d_tests_options_semantic_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_block(_suite,
                       "config_semantic_all");

    result = d_tests_options_semantic_count(_suite) && result;
    result = d_tests_options_semantic_print(_suite) && result;
    result = d_tests_options_semantic_push(_suite)  && result;

    return result;
}
