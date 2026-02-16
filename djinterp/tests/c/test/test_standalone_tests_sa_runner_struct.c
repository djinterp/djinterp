#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS FOR MODULE TESTS
 *****************************************************************************/

/*
helper_module_fn_tree
  Helper function that returns a test object tree for testing fn_test_module.

Return:
  Pointer to a simple test object, or NULL.
*/
static struct d_test_object*
helper_module_fn_tree
(
    void
)
{
    return d_test_object_new_leaf("helper_test", "helper message", true);
}


/*
helper_module_fn_counter
  Helper function that updates a counter for testing fn_test_module_counter.

Parameter(s):
  _counter: counter to update
Return:
  true
*/
static bool
helper_module_fn_counter
(
    struct d_test_counter* _counter
)
{
    if (_counter)
    {
        _counter->assertions_total++;
        _counter->assertions_passed++;
    }

    return true;
}


/******************************************************************************
 * VII. MODULE ENTRY AND RUNNER STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_fn_test_module
  Tests the fn_test_module function pointer type.
  Tests the following:
  - Type can be assigned NULL
  - Type can store function address
  - Function can be called and returns d_test_object*
*/
bool
d_tests_sa_standalone_fn_test_module
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    fn_test_module        test_fn;
    struct d_test_object* obj;

    result = true;

    // test 1: type has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_test_module) == sizeof(void*),
        "fn_test_module_size",
        "fn_test_module should be pointer-sized",
        _counter) && result;

    // test 2: can be assigned NULL
    test_fn = NULL;

    result = d_assert_standalone(
        test_fn == NULL,
        "fn_test_module_null",
        "fn_test_module should be assignable to NULL",
        _counter) && result;

    // test 3: can store function address
    test_fn = helper_module_fn_tree;

    result = d_assert_standalone(
        test_fn != NULL,
        "fn_test_module_assign",
        "fn_test_module should store function address",
        _counter) && result;

    // test 4: function can be called
    obj = test_fn();

    result = d_assert_standalone(
        obj != NULL,
        "fn_test_module_call",
        "fn_test_module function should return non-NULL",
        _counter) && result;

    if (obj)
    {
        d_test_object_free(obj);
    }

    return result;
}


/*
d_tests_sa_standalone_fn_test_module_counter
  Tests the fn_test_module_counter function pointer type.
  Tests the following:
  - Type can be assigned NULL
  - Type can store function address
  - Function can be called with counter parameter
*/
bool
d_tests_sa_standalone_fn_test_module_counter
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    fn_test_module_counter test_fn;
    struct d_test_counter  local_counter;
    bool                   call_result;

    result = true;

    // test 1: type has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_test_module_counter) == sizeof(void*),
        "fn_test_module_counter_size",
        "fn_test_module_counter should be pointer-sized",
        _counter) && result;

    // test 2: can be assigned NULL
    test_fn = NULL;

    result = d_assert_standalone(
        test_fn == NULL,
        "fn_test_module_counter_null",
        "fn_test_module_counter should be assignable to NULL",
        _counter) && result;

    // test 3: can store function address
    test_fn = helper_module_fn_counter;

    result = d_assert_standalone(
        test_fn != NULL,
        "fn_test_module_counter_assign",
        "fn_test_module_counter should store function address",
        _counter) && result;

    // test 4: function can be called
    d_test_counter_reset(&local_counter);
    call_result = test_fn(&local_counter);

    result = d_assert_standalone(
        call_result == true,
        "fn_test_module_counter_call",
        "fn_test_module_counter function should return true",
        _counter) && result;

    // test 5: counter was updated
    result = d_assert_standalone(
        local_counter.assertions_total == 1,
        "fn_test_module_counter_updates",
        "fn_test_module_counter should update counter",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_module_entry_struct
  Tests the d_test_sa_module_entry structure.
  Tests the following:
  - Structure has expected members
  - All members are accessible
*/
bool
d_tests_sa_standalone_module_entry_struct
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    struct d_test_sa_module_entry entry;

    result = true;

    // test 1: name member is accessible
    entry.name = "test_module";

    result = d_assert_standalone(
        entry.name != NULL,
        "module_entry_name_accessible",
        "name member should be accessible",
        _counter) && result;

    // test 2: description member is accessible
    entry.description = "Test module description";

    result = d_assert_standalone(
        entry.description != NULL,
        "module_entry_description_accessible",
        "description member should be accessible",
        _counter) && result;

    // test 3: run_fn member is accessible
    entry.run_fn = helper_module_fn_tree;

    result = d_assert_standalone(
        entry.run_fn != NULL,
        "module_entry_run_fn_accessible",
        "run_fn member should be accessible",
        _counter) && result;

    // test 4: run_counter member is accessible
    entry.run_counter = helper_module_fn_counter;

    result = d_assert_standalone(
        entry.run_counter != NULL,
        "module_entry_run_counter_accessible",
        "run_counter member should be accessible",
        _counter) && result;

    // test 5: note_count member is accessible
    entry.note_count = 5;

    result = d_assert_standalone(
        entry.note_count == 5,
        "module_entry_note_count_accessible",
        "note_count member should be accessible",
        _counter) && result;

    // test 6: notes member is accessible
    entry.notes = NULL;

    result = d_assert_standalone(
        entry.notes == NULL,
        "module_entry_notes_accessible",
        "notes member should be accessible",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_struct
  Tests the d_test_sa_runner structure.
  Tests the following:
  - Structure has expected members
  - All members are accessible
  - modules array has correct size
*/
bool
d_tests_sa_standalone_runner_struct
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_test_sa_runner  runner;

    result = true;

    // test 1: suite_name member is accessible
    runner.suite_name = "Test Suite";

    result = d_assert_standalone(
        runner.suite_name != NULL,
        "runner_suite_name_accessible",
        "suite_name member should be accessible",
        _counter) && result;

    // test 2: suite_description member is accessible
    runner.suite_description = "Test suite description";

    result = d_assert_standalone(
        runner.suite_description != NULL,
        "runner_suite_description_accessible",
        "suite_description member should be accessible",
        _counter) && result;

    // test 3: module_count member is accessible
    runner.module_count = 0;

    result = d_assert_standalone(
        runner.module_count == 0,
        "runner_module_count_accessible",
        "module_count member should be accessible",
        _counter) && result;

    // test 4: modules array is accessible
    runner.modules[0].name = "first_module";

    result = d_assert_standalone(
        runner.modules[0].name != NULL,
        "runner_modules_array_accessible",
        "modules array should be accessible",
        _counter) && result;

    // test 5: wait_for_input member is accessible
    runner.wait_for_input = true;

    result = d_assert_standalone(
        runner.wait_for_input == true,
        "runner_wait_for_input_accessible",
        "wait_for_input member should be accessible",
        _counter) && result;

    // test 6: show_notes member is accessible
    runner.show_notes = false;

    result = d_assert_standalone(
        runner.show_notes == false,
        "runner_show_notes_accessible",
        "show_notes member should be accessible",
        _counter) && result;

    // test 7: results member is accessible
    runner.results.modules_total = 10;

    result = d_assert_standalone(
        runner.results.modules_total == 10,
        "runner_results_accessible",
        "results member should be accessible",
        _counter) && result;

    // test 8: modules array has D_TEST_SA_MAX_MODULES elements
    // verify by setting the last element
    runner.modules[D_TEST_SA_MAX_MODULES - 1].name = "last_module";

    result = d_assert_standalone(
        runner.modules[D_TEST_SA_MAX_MODULES - 1].name != NULL,
        "runner_modules_max_size",
        "modules array should have D_TEST_SA_MAX_MODULES elements",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_struct_all
  Aggregation function that runs all module entry and runner structure tests.
*/
bool
d_tests_sa_standalone_runner_struct_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Module Entry and Runner Structures\n");
    printf("  ----------------------------------------------\n");

    result = d_tests_sa_standalone_fn_test_module(_counter) && result;
    result = d_tests_sa_standalone_fn_test_module_counter(_counter) && result;
    result = d_tests_sa_standalone_module_entry_struct(_counter) && result;
    result = d_tests_sa_standalone_runner_struct(_counter) && result;

    return result;
}
