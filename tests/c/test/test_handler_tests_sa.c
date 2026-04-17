#include "./test_handler_tests_sa.h"


//
// global event callback trackers
//

int g_event_setup_count    = 0;
int g_event_start_count    = 0;
int g_event_success_count  = 0;
int g_event_failure_count  = 0;
int g_event_end_count      = 0;
int g_event_teardown_count = 0;


/*
reset_event_counters
  Resets all global event counters to zero.
  Used before each test to ensure clean state.

Parameter(s):
  none.
Return:
  none.
*/
void
reset_event_counters
(
    void
)
{
    g_event_setup_count    = 0;
    g_event_start_count    = 0;
    g_event_success_count  = 0;
    g_event_failure_count  = 0;
    g_event_end_count      = 0;
    g_event_teardown_count = 0;

    return;
}


//
// helper test functions
//

/*
handler_test_passing
  Simple test function that always returns success.
  Used for testing passing test execution behavior.

Parameter(s):
  _test: test context (unused but required by signature).
Return:
  Always returns D_TEST_PASS (true).
*/
bool
handler_test_passing
(
    struct d_test* _test
)
{
    (void)_test;

    return D_TEST_PASS;
}

/*
handler_test_failing
  Simple test function that always returns failure.
  Used for testing failing test execution behavior.

Parameter(s):
  _test: test context (unused but required by signature).
Return:
  Always returns D_TEST_FAIL (false).
*/
bool
handler_test_failing
(
    struct d_test* _test
)
{
    (void)_test;

    return D_TEST_FAIL;
}

/*
handler_test_with_assertion
  Test function that creates and checks a passing assertion.
  Used for testing assertion tracking during execution.

Parameter(s):
  _test: test context (unused but required by signature).
Return:
  D_TEST_PASS if the assertion succeeds, D_TEST_FAIL otherwise.
*/
bool
handler_test_with_assertion
(
    struct d_test* _test
)
{
    struct d_assert* assertion;
    bool             result;

    (void)_test;

    assertion = d_assert_new(true,
                             "Test assertion passed",
                             "Test assertion failed");

    // check if assertion allocation succeeded
    if (!assertion)
    {
        return D_TEST_FAIL;
    }

    result = assertion->result;
    d_assert_free(assertion);

    return result
        ? D_TEST_PASS
        : D_TEST_FAIL;
}


// event callback functions

/*
callback_setup
  Event callback for D_TEST_EVENT_SETUP.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_setup
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_setup_count++;

    return;
}

/*
callback_start
  Event callback for D_TEST_EVENT_START.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_start
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_start_count++;

    return;
}

/*
callback_success
  Event callback for D_TEST_EVENT_SUCCESS.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_success
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_success_count++;

    return;
}

/*
callback_failure
  Event callback for D_TEST_EVENT_FAILURE.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_failure
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_failure_count++;

    return;
}

/*
callback_end
  Event callback for D_TEST_EVENT_END.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_end
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_end_count++;

    return;
}

/*
callback_teardown
  Event callback for D_TEST_EVENT_TEAR_DOWN.
  Increments global counter when called.

Parameter(s):
  _size:     number of elements (unused).
  _elements: element array (unused).
Return:
  none.
*/
void
callback_teardown
(
    size_t  _size,
    void**  _elements
)
{
    (void)_size;
    (void)_elements;
    g_event_teardown_count++;

    return;
}


//
// comprehensive test suite
//

/*
d_tests_sa_test_handler_all
  Runs all test_handler unit tests across every section.
  This is the master aggregation function that executes all test categories
in order and produces a summary.

Parameter(s):
  _test_info: test counter for tracking results.
Return:
  true if all tests passed, false otherwise.
*/
bool
d_tests_sa_test_handler_all
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_categories_passed;

    initial_tests_passed  = _test_info->tests_passed;
    all_categories_passed = true;

    printf("\n");
    printf("============================================"
           "====================================\n");
    printf("RUNNING TEST HANDLER STANDALONE TESTS\n");
    printf("============================================"
           "====================================\n");
    printf("\n");

    // section 1: creation and destruction
    printf("--- CREATION AND DESTRUCTION TESTS ---\n");
    if (!d_tests_sa_handler_new(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_new_with_events(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_new_full(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_free(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 2: flag management
    printf("--- FLAG MANAGEMENT TESTS ---\n");
    if (!d_tests_sa_handler_set_flag(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_clear_flag(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_has_flag(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 3: event listener registration
    printf("--- EVENT LISTENER REGISTRATION TESTS ---\n");
    if (!d_tests_sa_handler_register_listener(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_unregister_listener(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_listener_enable_disable(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 4: event emission
    printf("--- EVENT EMISSION TESTS ---\n");
    if (!d_tests_sa_handler_emit_event(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_event_lifecycle(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 5: test execution
    printf("--- TEST EXECUTION TESTS ---\n");
    if (!d_tests_sa_handler_run_test(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_run_block(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_run_test_type(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_nested_execution(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_config_override(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_run_module(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_run_tree_and_session(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 6: assertion tracking
    printf("--- ASSERTION TRACKING TESTS ---\n");
    if (!d_tests_sa_handler_record_assertion(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_assertion_statistics(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 7: stack operations
    printf("--- STACK OPERATIONS TESTS ---\n");
    if (!d_tests_sa_handler_result_stack(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_context_stack(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_stack_no_alloc(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 8: result queries
    printf("--- RESULT QUERY TESTS ---\n");
    if (!d_tests_sa_handler_get_results(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_reset_results(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_print_results(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_get_pass_rate(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_get_assertion_rate(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 9: context helpers
    printf("--- CONTEXT HELPER TESTS ---\n");
    if (!d_tests_sa_handler_context_new(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_context_init(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_context_free(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 10: DSL helpers
    printf("--- DSL HELPER TESTS ---\n");
    if (!d_tests_sa_handler_create_module_metadata(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_set_metadata_str(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_create_block_from_nodes(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_create_module_from_decl(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 11: statistics and depth tracking
    printf("--- STATISTICS AND DEPTH TRACKING TESTS ---\n");
    if (!d_tests_sa_handler_depth_tracking(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_max_depth(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_block_counting(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 12: abort on failure
    printf("--- ABORT ON FAILURE TESTS ---\n");
    if (!d_tests_sa_handler_abort_on_failure(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 13: edge cases and error handling
    printf("--- EDGE CASES AND ERROR HANDLING TESTS ---\n");
    if (!d_tests_sa_handler_null_parameters(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_empty_blocks(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_no_events(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // section 14: integration
    printf("--- INTEGRATION TESTS ---\n");
    if (!d_tests_sa_handler_complex_execution(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_event_integration(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_statistics_accuracy(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    // summary
    {
        size_t tests_passed_in_suite;

        tests_passed_in_suite = _test_info->tests_passed
                              - initial_tests_passed;

        printf("============================================"
               "====================================\n");
        printf("TEST HANDLER STANDALONE TESTS SUMMARY\n");
        printf("============================================"
               "====================================\n");
        printf("Handler Tests Passed: %zu\n",
               tests_passed_in_suite);

        if (all_categories_passed)
        {
            printf("Result: [SUCCESS] All test handler tests "
                   "passed!\n");
        }
        else
        {
            printf("Result: [FAILURE] Some test handler tests "
                   "failed.\n");
        }

        printf("============================================"
               "====================================\n");
        printf("\n");
    }

    return all_categories_passed;
}
