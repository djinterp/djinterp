/******************************************************************************
* djinterp [test]                                       test_handler_tests_sa.c
*
*   Global state, helper functions, event callbacks, factory helpers, and
* the comprehensive aggregation function for all test_handler standalone
* unit tests.
*
*
* file:      \test\test\test_handler_tests_sa.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#include "./test_handler_tests_sa.h"


/******************************************************************************
 * GLOBAL EVENT CALLBACK TRACKERS
 *****************************************************************************/

int g_event_setup_count    = 0;
int g_event_start_count    = 0;
int g_event_success_count  = 0;
int g_event_failure_count  = 0;
int g_event_end_count      = 0;
int g_event_teardown_count = 0;


/******************************************************************************
 * GLOBAL STAGE HOOK TRACKERS
 *****************************************************************************/

int g_stage_setup_count    = 0;
int g_stage_teardown_count = 0;


/*
reset_event_counters
  Resets all global event callback counters to zero.

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


/*
reset_stage_counters
  Resets all global stage hook counters to zero.

Return:
  none.
*/
void
reset_stage_counters
(
    void
)
{
    g_stage_setup_count    = 0;
    g_stage_teardown_count = 0;

    return;
}


/******************************************************************************
 * HELPER TEST FUNCTIONS (fn_test-compatible)
 *****************************************************************************/

/*
handler_test_passing
  A test function that always passes.

Return:
  D_TEST_PASS.
*/
bool
handler_test_passing
(
    void
)
{
    return D_TEST_PASS;
}


/*
handler_test_failing
  A test function that always fails.

Return:
  D_TEST_FAIL.
*/
bool
handler_test_failing
(
    void
)
{
    return D_TEST_FAIL;
}


/*
handler_test_with_assertion
  A test function that creates and evaluates a passing assertion.

Return:
  D_TEST_PASS if the assertion passed, D_TEST_FAIL otherwise.
*/
bool
handler_test_with_assertion
(
    void
)
{
    struct d_assert* assertion;
    bool             result;

    assertion = d_assert_new(true,
                             "Test assertion passed",
                             "Test assertion failed");

    if (!assertion)
    {
        return D_TEST_FAIL;
    }

    result = assertion->result;
    d_assert_free(assertion);

    return result ? D_TEST_PASS
                  : D_TEST_FAIL;
}


/******************************************************************************
 * HELPER FACTORY FUNCTIONS
 *****************************************************************************/

/*
helper_make_passing_test
  Creates a d_test containing one passing test_fn child.

Return:
  Pointer to the new d_test, or NULL on failure.
*/
struct d_test*
helper_make_passing_test
(
    void
)
{
    struct d_test_fn*   fn;
    struct d_test_type* fn_type;
    struct d_test_type* children[1];

    fn = d_test_fn_new((fn_test)handler_test_passing);

    if (!fn)
    {
        return NULL;
    }

    fn_type = d_test_type_new(D_TEST_TYPE_TEST_FN, fn);

    if (!fn_type)
    {
        d_test_fn_free(fn);

        return NULL;
    }

    children[0] = fn_type;

    return d_test_new(children, 1);
}


/*
helper_make_failing_test
  Creates a d_test containing one failing test_fn child.

Return:
  Pointer to the new d_test, or NULL on failure.
*/
struct d_test*
helper_make_failing_test
(
    void
)
{
    struct d_test_fn*   fn;
    struct d_test_type* fn_type;
    struct d_test_type* children[1];

    fn = d_test_fn_new((fn_test)handler_test_failing);

    if (!fn)
    {
        return NULL;
    }

    fn_type = d_test_type_new(D_TEST_TYPE_TEST_FN, fn);

    if (!fn_type)
    {
        d_test_fn_free(fn);

        return NULL;
    }

    children[0] = fn_type;

    return d_test_new(children, 1);
}


/*
helper_add_passing_test_to_block
  Creates a passing test, wraps in d_test_type, adds as child.
Block takes ownership on success.

Parameter(s):
  _block: the block to add the test to.
Return:
  true if the test was added successfully, false otherwise.
*/
bool
helper_add_passing_test_to_block
(
    struct d_test_block* _block
)
{
    struct d_test*      test;
    struct d_test_type* child;

    if (!_block)
    {
        return false;
    }

    test = helper_make_passing_test();

    if (!test)
    {
        return false;
    }

    child = d_test_type_new(D_TEST_TYPE_TEST, test);

    if (!child)
    {
        d_test_free(test);

        return false;
    }

    return d_test_block_add_child(_block, child);
}


/*
helper_add_failing_test_to_block
  Creates a failing test, wraps in d_test_type, adds as child.
Block takes ownership on success.

Parameter(s):
  _block: the block to add the test to.
Return:
  true if the test was added successfully, false otherwise.
*/
bool
helper_add_failing_test_to_block
(
    struct d_test_block* _block
)
{
    struct d_test*      test;
    struct d_test_type* child;

    if (!_block)
    {
        return false;
    }

    test = helper_make_failing_test();

    if (!test)
    {
        return false;
    }

    child = d_test_type_new(D_TEST_TYPE_TEST, test);

    if (!child)
    {
        d_test_free(test);

        return false;
    }

    return d_test_block_add_child(_block, child);
}


/*
helper_add_block_child_to_block
  Wraps child block in d_test_type and adds to parent.
Parent takes ownership of child on success.

Parameter(s):
  _parent: the parent block.
  _child:  the child block to add.
Return:
  true if the child was added successfully, false otherwise.
*/
bool
helper_add_block_child_to_block
(
    struct d_test_block* _parent,
    struct d_test_block* _child
)
{
    struct d_test_type* child_type;

    if ( (!_parent) ||
         (!_child) )
    {
        return false;
    }

    child_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, _child);

    if (!child_type)
    {
        return false;
    }

    return d_test_block_add_child(_parent, child_type);
}


/******************************************************************************
 * EVENT CALLBACK FUNCTIONS
 *****************************************************************************/

/*
callback_setup
  Event callback that increments the setup counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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
  Event callback that increments the start counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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
  Event callback that increments the success counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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
  Event callback that increments the failure counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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
  Event callback that increments the end counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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
  Event callback that increments the teardown counter.

Parameter(s):
  _size:     number of elements.
  _elements: array of element pointers.
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


/******************************************************************************
 * STAGE HOOK FUNCTIONS
 *****************************************************************************/

/*
stage_hook_setup
  Stage hook that increments the setup counter and succeeds.

Parameter(s):
  _test: the test being set up (unused).
Return:
  true always.
*/
bool
stage_hook_setup
(
    struct d_test* _test
)
{
    (void)_test;
    g_stage_setup_count++;

    return true;
}


/*
stage_hook_teardown
  Stage hook that increments the teardown counter and succeeds.

Parameter(s):
  _test: the test being torn down (unused).
Return:
  true always.
*/
bool
stage_hook_teardown
(
    struct d_test* _test
)
{
    (void)_test;
    g_stage_teardown_count++;

    return true;
}


/*
stage_hook_failing
  Stage hook that always fails.

Parameter(s):
  _test: the test (unused).
Return:
  false always.
*/
bool
stage_hook_failing
(
    struct d_test* _test
)
{
    (void)_test;

    return false;
}


/******************************************************************************
 * COMPREHENSIVE TEST SUITE
 *****************************************************************************/

/*
d_tests_sa_test_handler_all
  Runs all test_handler standalone tests and prints a summary.
  Tests the following:
  - creation, destruction, flags, events, execution
  - assertions, stacks, results, context, DSL helpers
  - statistics, depth, abort, edge cases, integration
  - test block API (child management, stage hooks, direct run)
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
           "====================================\n\n");

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

    printf("--- ABORT ON FAILURE TESTS ---\n");
    if (!d_tests_sa_handler_abort_on_failure(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

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

    printf("--- TEST BLOCK API TESTS ---\n");
    if (!d_tests_sa_handler_block_child_management(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_block_stage_hooks(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_block_direct_run(_test_info))
    {
        all_categories_passed = false;
    }
    if (!d_tests_sa_handler_block_new_args(_test_info))
    {
        all_categories_passed = false;
    }
    printf("\n");

    printf("============================================"
           "====================================\n");
    printf("TEST HANDLER STANDALONE TESTS SUMMARY\n");
    printf("============================================"
           "====================================\n");
    printf("Handler Tests Passed: %zu\n",
           _test_info->tests_passed - initial_tests_passed);

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
           "====================================\n\n");

    return all_categories_passed;
}
