#include "./test_tests_sa.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

// dummy test function for wrapping
static bool
d_internal_test_dummy_fn(void)
{
    return true;
}


/*
d_tests_sa_test_fn_new_valid
  Tests d_test_fn_new with a valid function pointer.
  Tests the following:
  - d_test_fn_new returns non-NULL with valid function pointer
  - returned wrapper stores the correct function pointer
*/
bool
d_tests_sa_test_fn_new_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test_fn* fn;
    bool              result;

    result = true;

    fn = d_test_fn_new((fn_test)d_internal_test_dummy_fn);

    // verify allocation
    result = d_assert_standalone(fn != NULL,
                                 "d_test_fn_new_valid",
                                 "d_test_fn_new with valid fn returns non-NULL",
                                 _counter) && result;

    if (fn)
    {
        // verify function pointer stored correctly
        result = d_assert_standalone(fn->test_fn == (fn_test)d_internal_test_dummy_fn,
                                     "d_test_fn_new_valid",
                                     "wrapper stores correct function pointer",
                                     _counter) && result;

        d_test_fn_free(fn);
    }

    return result;
}


/*
d_tests_sa_test_fn_new_fields
  Tests d_test_fn_new initializes all fields correctly.
  Tests the following:
  - count field is initialized to 0
  - args field is initialized to NULL
  - test_fn field points to provided function
*/
bool
d_tests_sa_test_fn_new_fields
(
    struct d_test_counter* _counter
)
{
    struct d_test_fn* fn;
    bool              result;

    result = true;

    fn = d_test_fn_new((fn_test)d_internal_test_dummy_fn);

    if (!fn)
    {
        result = d_assert_standalone(false,
                                     "d_test_fn_new_fields",
                                     "d_test_fn_new allocation failed",
                                     _counter);

        return result;
    }

    // verify all fields
    result = d_assert_standalone(fn->test_fn == (fn_test)d_internal_test_dummy_fn,
                                 "d_test_fn_new_fields",
                                 "test_fn field is set correctly",
                                 _counter) && result;

    result = d_assert_standalone(fn->count == 0,
                                 "d_test_fn_new_fields",
                                 "count field initialized to 0",
                                 _counter) && result;

    result = d_assert_standalone(fn->args == NULL,
                                 "d_test_fn_new_fields",
                                 "args field initialized to NULL",
                                 _counter) && result;

    d_test_fn_free(fn);

    return result;
}


/*
d_tests_sa_test_fn_new_null
  Tests d_test_fn_new with NULL function pointer.
  Tests the following:
  - d_test_fn_new with NULL still allocates the wrapper
  - test_fn field is NULL
  - wrapper is still valid (can be freed)
*/
bool
d_tests_sa_test_fn_new_null
(
    struct d_test_counter* _counter
)
{
    struct d_test_fn* fn;
    bool              result;

    result = true;

    // d_test_fn_new uses calloc so NULL fn creates wrapper with NULL fn
    fn = d_test_fn_new(NULL);

    // the implementation callocs so it should succeed
    result = d_assert_standalone(fn != NULL,
                                 "d_test_fn_new_null",
                                 "d_test_fn_new(NULL) returns non-NULL wrapper",
                                 _counter) && result;

    if (fn)
    {
        // verify the fn pointer is NULL
        result = d_assert_standalone(fn->test_fn == NULL,
                                     "d_test_fn_new_null",
                                     "test_fn field is NULL when passed NULL",
                                     _counter) && result;

        d_test_fn_free(fn);
    }

    return result;
}


/*
d_tests_sa_test_fn_wrapper_all
  Aggregation function for all test function wrapper tests.
*/
bool
d_tests_sa_test_fn_wrapper_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_fn_new_valid(_counter) && result;
    result = d_tests_sa_test_fn_new_fields(_counter) && result;
    result = d_tests_sa_test_fn_new_null(_counter) && result;

    return result;
}
