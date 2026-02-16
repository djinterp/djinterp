#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS FOR PRINT FUNCTION TESTS
 *****************************************************************************/

/*
helper_print_object
  Helper print function for testing fn_print_object.
*/
static void
helper_print_object
(
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _counter
)
{
    // suppress unused parameter warnings
    (void)_obj;
    (void)_indent_level;

    // just increment counter to verify function was called
    if (_counter)
    {
        _counter->assertions_total++;
    }

    return;
}


/*
helper_print_object_file
  Helper print function for testing fn_print_object_file.
*/
static void
helper_print_object_file
(
    FILE*                       _file,
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _counter
)
{
    // suppress unused parameter warnings
    (void)_file;
    (void)_obj;
    (void)_indent_level;

    // just increment counter to verify function was called
    if (_counter)
    {
        _counter->assertions_total++;
    }

    return;
}


/******************************************************************************
 * VIII. FUNCTION POINTER TYPE TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_fn_print_object
  Tests the fn_print_object function pointer type.
  Tests the following:
  - Type has correct size (pointer size)
  - Type can be assigned NULL
  - Type can store function address
  - Function can be called
*/
bool
d_tests_sa_standalone_fn_print_object
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    fn_print_object       print_fn;
    struct d_test_object* obj;
    struct d_test_counter local_counter;

    result = true;

    // test 1: type has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_print_object) == sizeof(void*),
        "fn_print_object_size",
        "fn_print_object should be pointer-sized",
        _counter) && result;

    // test 2: can be assigned NULL
    print_fn = NULL;

    result = d_assert_standalone(
        print_fn == NULL,
        "fn_print_object_null",
        "fn_print_object should be assignable to NULL",
        _counter) && result;

    // test 3: can store function address
    print_fn = helper_print_object;

    result = d_assert_standalone(
        print_fn != NULL,
        "fn_print_object_assign",
        "fn_print_object should store function address",
        _counter) && result;

    // test 4: function can be called
    obj = d_test_object_new_leaf("test", "message", true);
    d_test_counter_reset(&local_counter);

    print_fn(obj, 0, &local_counter);

    result = d_assert_standalone(
        local_counter.assertions_total == 1,
        "fn_print_object_call",
        "fn_print_object function should be callable",
        _counter) && result;

    // test 5: can store default print function
    print_fn = d_test_default_print_object;

    result = d_assert_standalone(
        print_fn == d_test_default_print_object,
        "fn_print_object_default",
        "fn_print_object should store default print function",
        _counter) && result;

    if (obj)
    {
        d_test_object_free(obj);
    }

    return result;
}


/*
d_tests_sa_standalone_fn_print_object_file
  Tests the fn_print_object_file function pointer type.
  Tests the following:
  - Type has correct size (pointer size)
  - Type can be assigned NULL
  - Type can store function address
  - Function can be called
*/
bool
d_tests_sa_standalone_fn_print_object_file
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    fn_print_object_file  print_fn;
    struct d_test_object* obj;
    struct d_test_counter local_counter;

    result = true;

    // test 1: type has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_print_object_file) == sizeof(void*),
        "fn_print_object_file_size",
        "fn_print_object_file should be pointer-sized",
        _counter) && result;

    // test 2: can be assigned NULL
    print_fn = NULL;

    result = d_assert_standalone(
        print_fn == NULL,
        "fn_print_object_file_null",
        "fn_print_object_file should be assignable to NULL",
        _counter) && result;

    // test 3: can store function address
    print_fn = helper_print_object_file;

    result = d_assert_standalone(
        print_fn != NULL,
        "fn_print_object_file_assign",
        "fn_print_object_file should store function address",
        _counter) && result;

    // test 4: function can be called
    obj = d_test_object_new_leaf("test", "message", true);
    d_test_counter_reset(&local_counter);

    print_fn(stdout, obj, 0, &local_counter);

    result = d_assert_standalone(
        local_counter.assertions_total == 1,
        "fn_print_object_file_call",
        "fn_print_object_file function should be callable",
        _counter) && result;

    // test 5: can store default print function
    print_fn = d_test_default_print_object_to_file;

    result = d_assert_standalone(
        print_fn == d_test_default_print_object_to_file,
        "fn_print_object_file_default",
        "fn_print_object_file should store default print function",
        _counter) && result;

    if (obj)
    {
        d_test_object_free(obj);
    }

    return result;
}


/*
d_tests_sa_standalone_fn_ptr_all
  Aggregation function that runs all function pointer type tests.
*/
bool
d_tests_sa_standalone_fn_ptr_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Function Pointer Types\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_standalone_fn_print_object(_counter) && result;
    result = d_tests_sa_standalone_fn_print_object_file(_counter) && result;

    return result;
}
