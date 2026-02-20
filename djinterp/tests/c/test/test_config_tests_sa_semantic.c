#include ".\test_config_tests_sa.h"


/******************************************************************************
 * VII. SEMANTIC CHECK MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_config_semantic_count
  Tests the SHOULD_COUNT_* semantic check macros.
  Tests the following:
  - SHOULD_COUNT_FAILURES detects any fail counter flag
  - SHOULD_COUNT_PASSES detects any pass counter flag
  - Individual category checks (asserts, tests, blocks, modules)
  - Silent mode returns false for all
*/
bool
d_tests_sa_config_semantic_count
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* verbose;
    struct d_test_config* silent;
    struct d_test_config* fail_only;

    result    = true;
    verbose   = d_test_config_new(D_TEST_MODE_VERBOSE);
    silent    = d_test_config_new(D_TEST_MODE_SILENT);
    fail_only = d_test_config_new(D_TEST_MSG_COUNT_FAIL_ALL);

    if ( (!verbose) || (!silent) || (!fail_only) )
    {
        if (verbose)   { d_test_config_free(verbose); }
        if (silent)    { d_test_config_free(silent); }
        if (fail_only) { d_test_config_free(fail_only); }

        return d_assert_standalone(false,
                                   "semantic_count_alloc",
                                   "Failed to allocate configs",
                                   _counter);
    }

    // test 1: verbose has all count flags
    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_FAILURES(verbose) == true,
        "count_failures_verbose",
        "Verbose should count failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_PASSES(verbose) == true,
        "count_passes_verbose",
        "Verbose should count passes",
        _counter) && result;

    // test 2: silent has no count flags
    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_FAILURES(silent) == false,
        "count_failures_silent",
        "Silent should not count failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_PASSES(silent) == false,
        "count_passes_silent",
        "Silent should not count passes",
        _counter) && result;

    // test 3: fail_only counts failures but not passes
    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_FAILURES(fail_only) == true,
        "count_failures_fail_only",
        "Fail-only should count failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_PASSES(fail_only) == false,
        "count_passes_fail_only",
        "Fail-only should not count passes",
        _counter) && result;

    // test 4: individual category checks on verbose
    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_ASSERTS_FAIL(verbose) == true,
        "count_asserts_fail_verbose",
        "Verbose should count assert failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_ASSERTS_PASS(verbose) == true,
        "count_asserts_pass_verbose",
        "Verbose should count assert passes",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_TESTS_FAIL(verbose) == true,
        "count_tests_fail_verbose",
        "Verbose should count test failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_BLOCKS_PASS(verbose) == true,
        "count_blocks_pass_verbose",
        "Verbose should count block passes",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_COUNT_MODULES_FAIL(verbose) == true,
        "count_modules_fail_verbose",
        "Verbose should count module failures",
        _counter) && result;

    d_test_config_free(verbose);
    d_test_config_free(silent);
    d_test_config_free(fail_only);

    return result;
}


/*
d_tests_sa_config_semantic_print
  Tests the SHOULD_PRINT_* semantic check macros.
  Tests the following:
  - SHOULD_PRINT_FAILURES detects any print fail flag
  - SHOULD_PRINT_PASSES detects any print pass flag
  - Individual category checks
  - Normal mode prints failures but not passes
*/
bool
d_tests_sa_config_semantic_print
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* normal;
    struct d_test_config* verbose;

    result  = true;
    normal  = d_test_config_new(D_TEST_MODE_NORMAL);
    verbose = d_test_config_new(D_TEST_MODE_VERBOSE);

    if ( (!normal) || (!verbose) )
    {
        if (normal)  { d_test_config_free(normal); }
        if (verbose) { d_test_config_free(verbose); }

        return d_assert_standalone(false,
                                   "semantic_print_alloc",
                                   "Failed to allocate configs",
                                   _counter);
    }

    // test 1: normal prints failures but not passes
    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_FAILURES(normal) == true,
        "print_failures_normal",
        "Normal should print failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_PASSES(normal) == false,
        "print_passes_normal",
        "Normal should not print passes",
        _counter) && result;

    // test 2: verbose prints everything
    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_FAILURES(verbose) == true,
        "print_failures_verbose",
        "Verbose should print failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_PASSES(verbose) == true,
        "print_passes_verbose",
        "Verbose should print passes",
        _counter) && result;

    // test 3: individual category checks on normal
    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_ASSERTS_FAIL(normal) == true,
        "print_asserts_fail_normal",
        "Normal should print assert failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_ASSERTS_PASS(normal) == false,
        "print_asserts_pass_normal",
        "Normal should not print assert passes",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_TESTS_FAIL(normal) == true,
        "print_tests_fail_normal",
        "Normal should print test failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_BLOCKS_FAIL(normal) == true,
        "print_blocks_fail_normal",
        "Normal should print block failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_MODULES_FAIL(normal) == true,
        "print_modules_fail_normal",
        "Normal should print module failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PRINT_MODULES_PASS(normal) == false,
        "print_modules_pass_normal",
        "Normal should not print module passes",
        _counter) && result;

    d_test_config_free(normal);
    d_test_config_free(verbose);

    return result;
}


/*
d_tests_sa_config_semantic_push
  Tests the SHOULD_PUSH_* semantic check macros.
  Tests the following:
  - Returns true when corresponding settings flag is set
  - Returns false when corresponding settings flag is not set
*/
bool
d_tests_sa_config_semantic_push
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* push_all;
    struct d_test_config* no_push;
    struct d_test_config* push_fail_only;

    result = true;

    push_all       = d_test_config_new(D_TEST_SETTINGS_STACK_PUSH_ALL);
    no_push        = d_test_config_new(D_TEST_MODE_NORMAL);
    push_fail_only = d_test_config_new(
        D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL));

    if ( (!push_all) || (!no_push) || (!push_fail_only) )
    {
        if (push_all)       { d_test_config_free(push_all); }
        if (no_push)        { d_test_config_free(no_push); }
        if (push_fail_only) { d_test_config_free(push_fail_only); }

        return d_assert_standalone(false,
                                   "semantic_push_alloc",
                                   "Failed to allocate configs",
                                   _counter);
    }

    // test 1: push_all has all push flags
    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_FAILURES(push_all) == true,
        "push_failures_all",
        "PUSH_ALL should push failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_PASSES(push_all) == true,
        "push_passes_all",
        "PUSH_ALL should push passes",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_WARNINGS(push_all) == true,
        "push_warnings_all",
        "PUSH_ALL should push warnings",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_INFO(push_all) == true,
        "push_info_all",
        "PUSH_ALL should push info",
        _counter) && result;

    // test 2: no_push has no push flags
    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_FAILURES(no_push) == false,
        "push_failures_none",
        "No-push config should not push failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_PASSES(no_push) == false,
        "push_passes_none",
        "No-push config should not push passes",
        _counter) && result;

    // test 3: push_fail_only has only PUSH_FAIL
    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_FAILURES(push_fail_only) == true,
        "push_fail_only_failures",
        "Push-fail config should push failures",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_PUSH_PASSES(push_fail_only) == false,
        "push_fail_only_passes",
        "Push-fail config should not push passes",
        _counter) && result;

    d_test_config_free(push_all);
    d_test_config_free(no_push);
    d_test_config_free(push_fail_only);

    return result;
}


/*
d_tests_sa_config_semantic_legacy_aliases
  Tests the legacy alias macros for settings semantic checks.
  Tests the following:
  - SHOULD_STACK_PUSH_FAIL matches SHOULD_PUSH_FAILURES
  - SHOULD_STACK_PUSH_PASS matches SHOULD_PUSH_PASSES
  - SHOULD_STACK_PUSH_WARNING matches SHOULD_PUSH_WARNINGS
  - SHOULD_STACK_PUSH_INFO matches SHOULD_PUSH_INFO
*/
bool
d_tests_sa_config_semantic_legacy_aliases
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "legacy_alias_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    result = d_assert_standalone(
        D_TEST_SHOULD_STACK_PUSH_FAIL(config) ==
            D_TEST_SHOULD_PUSH_FAILURES(config),
        "legacy_push_fail",
        "Legacy STACK_PUSH_FAIL should match PUSH_FAILURES",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_STACK_PUSH_PASS(config) ==
            D_TEST_SHOULD_PUSH_PASSES(config),
        "legacy_push_pass",
        "Legacy STACK_PUSH_PASS should match PUSH_PASSES",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_STACK_PUSH_WARNING(config) ==
            D_TEST_SHOULD_PUSH_WARNINGS(config),
        "legacy_push_warning",
        "Legacy STACK_PUSH_WARNING should match PUSH_WARNINGS",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SHOULD_STACK_PUSH_INFO(config) ==
            D_TEST_SHOULD_PUSH_INFO(config),
        "legacy_push_info",
        "Legacy STACK_PUSH_INFO should match PUSH_INFO",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_semantic_all
  Aggregation function that runs all semantic check macro tests.
*/
bool
d_tests_sa_config_semantic_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Semantic Check Macros\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_config_semantic_count(_counter) && result;
    result = d_tests_sa_config_semantic_print(_counter) && result;
    result = d_tests_sa_config_semantic_push(_counter) && result;
    result = d_tests_sa_config_semantic_legacy_aliases(_counter) && result;

    return result;
}
