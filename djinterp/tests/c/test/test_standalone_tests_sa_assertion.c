#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * I. ASSERTION MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_assert_true
  Tests the D_ASSERT_TRUE macro.
  Tests the following:
  - Macro is defined
  - Creates leaf object with true condition
  - Creates leaf object with false condition
  - Result field reflects condition value
*/
bool
d_tests_sa_standalone_assert_true
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_true;
    struct d_test_object* obj_false;

    result = true;

    // test 1: D_ASSERT_TRUE is defined (compiles successfully)
    #ifdef D_ASSERT_TRUE
        result = d_assert_standalone(
            true,
            "assert_true_defined",
            "D_ASSERT_TRUE should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_true_defined",
            "D_ASSERT_TRUE should be defined",
            _counter) && result;
    #endif

    // test 2: creates leaf object with true condition
    obj_true = D_ASSERT_TRUE("test_name", true, "test message");

    result = d_assert_standalone(
        obj_true != NULL,
        "assert_true_creates_object",
        "D_ASSERT_TRUE should create non-NULL object",
        _counter) && result;

    if (obj_true)
    {
        // test 3: result field is true for true condition
        result = d_assert_standalone(
            obj_true->result == true,
            "assert_true_result_true",
            "D_ASSERT_TRUE with true condition should have result=true",
            _counter) && result;

        // test 4: is_leaf is true
        result = d_assert_standalone(
            obj_true->is_leaf == true,
            "assert_true_is_leaf",
            "D_ASSERT_TRUE should create leaf object",
            _counter) && result;

        d_test_object_free(obj_true);
    }

    // test 5: creates leaf object with false condition
    obj_false = D_ASSERT_TRUE("test_name", false, "test message");

    result = d_assert_standalone(
        obj_false != NULL,
        "assert_true_false_creates_object",
        "D_ASSERT_TRUE with false condition should create object",
        _counter) && result;

    if (obj_false)
    {
        // test 6: result field is false for false condition
        result = d_assert_standalone(
            obj_false->result == false,
            "assert_true_result_false",
            "D_ASSERT_TRUE with false condition should have result=false",
            _counter) && result;

        d_test_object_free(obj_false);
    }

    return result;
}


/*
d_tests_sa_standalone_assert_false
  Tests the D_ASSERT_FALSE macro.
  Tests the following:
  - Macro is defined
  - Creates leaf object with false condition (should pass)
  - Creates leaf object with true condition (should fail)
  - Result field reflects inverted condition
*/
bool
d_tests_sa_standalone_assert_false
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_false;
    struct d_test_object* obj_true;

    result = true;

    // test 1: D_ASSERT_FALSE is defined
    #ifdef D_ASSERT_FALSE
        result = d_assert_standalone(
            true,
            "assert_false_defined",
            "D_ASSERT_FALSE should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_false_defined",
            "D_ASSERT_FALSE should be defined",
            _counter) && result;
    #endif

    // test 2: false condition results in true (inverted)
    obj_false = D_ASSERT_FALSE("test_name", false, "test message");

    result = d_assert_standalone(
        obj_false != NULL,
        "assert_false_creates_object",
        "D_ASSERT_FALSE should create object",
        _counter) && result;

    if (obj_false)
    {
        // test 3: result is true for false condition (inverted)
        result = d_assert_standalone(
            obj_false->result == true,
            "assert_false_result_inverted",
            "D_ASSERT_FALSE with false condition should have result=true",
            _counter) && result;

        d_test_object_free(obj_false);
    }

    // test 4: true condition results in false (inverted)
    obj_true = D_ASSERT_FALSE("test_name", true, "test message");

    if (obj_true)
    {
        result = d_assert_standalone(
            obj_true->result == false,
            "assert_false_true_gives_false",
            "D_ASSERT_FALSE with true condition should have result=false",
            _counter) && result;

        d_test_object_free(obj_true);
    }

    return result;
}


/*
d_tests_sa_standalone_assert_null
  Tests the D_ASSERT_NULL macro.
  Tests the following:
  - Macro is defined
  - NULL pointer results in pass
  - Non-NULL pointer results in fail
*/
bool
d_tests_sa_standalone_assert_null
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_null;
    struct d_test_object* obj_not_null;
    int                   dummy_value;

    result      = true;
    dummy_value = 42;

    // test 1: D_ASSERT_NULL is defined
    #ifdef D_ASSERT_NULL
        result = d_assert_standalone(
            true,
            "assert_null_defined",
            "D_ASSERT_NULL should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_null_defined",
            "D_ASSERT_NULL should be defined",
            _counter) && result;
    #endif

    // test 2: NULL pointer results in pass
    obj_null = D_ASSERT_NULL("test_name", NULL, "test message");

    result = d_assert_standalone(
        obj_null != NULL,
        "assert_null_creates_object",
        "D_ASSERT_NULL should create object",
        _counter) && result;

    if (obj_null)
    {
        result = d_assert_standalone(
            obj_null->result == true,
            "assert_null_passes",
            "D_ASSERT_NULL with NULL should have result=true",
            _counter) && result;

        d_test_object_free(obj_null);
    }

    // test 3: non-NULL pointer results in fail
    obj_not_null = D_ASSERT_NULL("test_name", &dummy_value, "test message");

    if (obj_not_null)
    {
        result = d_assert_standalone(
            obj_not_null->result == false,
            "assert_null_fails_non_null",
            "D_ASSERT_NULL with non-NULL should have result=false",
            _counter) && result;

        d_test_object_free(obj_not_null);
    }

    return result;
}


/*
d_tests_sa_standalone_assert_not_null
  Tests the D_ASSERT_NOT_NULL macro.
  Tests the following:
  - Macro is defined
  - Non-NULL pointer results in pass
  - NULL pointer results in fail
*/
bool
d_tests_sa_standalone_assert_not_null
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_not_null;
    struct d_test_object* obj_null;
    int                   dummy_value;

    result      = true;
    dummy_value = 42;

    // test 1: D_ASSERT_NOT_NULL is defined
    #ifdef D_ASSERT_NOT_NULL
        result = d_assert_standalone(
            true,
            "assert_not_null_defined",
            "D_ASSERT_NOT_NULL should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_not_null_defined",
            "D_ASSERT_NOT_NULL should be defined",
            _counter) && result;
    #endif

    // test 2: non-NULL pointer results in pass
    obj_not_null = D_ASSERT_NOT_NULL("test_name", &dummy_value, "test message");

    result = d_assert_standalone(
        obj_not_null != NULL,
        "assert_not_null_creates_object",
        "D_ASSERT_NOT_NULL should create object",
        _counter) && result;

    if (obj_not_null)
    {
        result = d_assert_standalone(
            obj_not_null->result == true,
            "assert_not_null_passes",
            "D_ASSERT_NOT_NULL with non-NULL should have result=true",
            _counter) && result;

        d_test_object_free(obj_not_null);
    }

    // test 3: NULL pointer results in fail
    obj_null = D_ASSERT_NOT_NULL("test_name", NULL, "test message");

    if (obj_null)
    {
        result = d_assert_standalone(
            obj_null->result == false,
            "assert_not_null_fails_null",
            "D_ASSERT_NOT_NULL with NULL should have result=false",
            _counter) && result;

        d_test_object_free(obj_null);
    }

    return result;
}


/*
d_tests_sa_standalone_assert_equal
  Tests the D_ASSERT_EQUAL macro.
  Tests the following:
  - Macro is defined
  - Equal values result in pass
  - Unequal values result in fail
*/
bool
d_tests_sa_standalone_assert_equal
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_equal;
    struct d_test_object* obj_not_equal;

    result = true;

    // test 1: D_ASSERT_EQUAL is defined
    #ifdef D_ASSERT_EQUAL
        result = d_assert_standalone(
            true,
            "assert_equal_defined",
            "D_ASSERT_EQUAL should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_equal_defined",
            "D_ASSERT_EQUAL should be defined",
            _counter) && result;
    #endif

    // test 2: equal values result in pass
    obj_equal = D_ASSERT_EQUAL("test_name", 42, 42, "test message");

    result = d_assert_standalone(
        obj_equal != NULL,
        "assert_equal_creates_object",
        "D_ASSERT_EQUAL should create object",
        _counter) && result;

    if (obj_equal)
    {
        result = d_assert_standalone(
            obj_equal->result == true,
            "assert_equal_passes",
            "D_ASSERT_EQUAL with equal values should have result=true",
            _counter) && result;

        d_test_object_free(obj_equal);
    }

    // test 3: unequal values result in fail
    obj_not_equal = D_ASSERT_EQUAL("test_name", 42, 99, "test message");

    if (obj_not_equal)
    {
        result = d_assert_standalone(
            obj_not_equal->result == false,
            "assert_equal_fails_unequal",
            "D_ASSERT_EQUAL with unequal values should have result=false",
            _counter) && result;

        d_test_object_free(obj_not_equal);
    }

    return result;
}


/*
d_tests_sa_standalone_assert_str_equal
  Tests the D_ASSERT_STR_EQUAL macro.
  Tests the following:
  - Macro is defined
  - Equal strings result in pass
  - Unequal strings result in fail
  - NULL strings result in fail
*/
bool
d_tests_sa_standalone_assert_str_equal
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_equal;
    struct d_test_object* obj_not_equal;
    struct d_test_object* obj_null1;
    struct d_test_object* obj_null2;

    result = true;

    // test 1: D_ASSERT_STR_EQUAL is defined
    #ifdef D_ASSERT_STR_EQUAL
        result = d_assert_standalone(
            true,
            "assert_str_equal_defined",
            "D_ASSERT_STR_EQUAL should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "assert_str_equal_defined",
            "D_ASSERT_STR_EQUAL should be defined",
            _counter) && result;
    #endif

    // test 2: equal strings result in pass
    obj_equal = D_ASSERT_STR_EQUAL("test_name", "hello", "hello", "test message");

    result = d_assert_standalone(
        obj_equal != NULL,
        "assert_str_equal_creates_object",
        "D_ASSERT_STR_EQUAL should create object",
        _counter) && result;

    if (obj_equal)
    {
        result = d_assert_standalone(
            obj_equal->result == true,
            "assert_str_equal_passes",
            "D_ASSERT_STR_EQUAL with equal strings should have result=true",
            _counter) && result;

        d_test_object_free(obj_equal);
    }

    // test 3: unequal strings result in fail
    obj_not_equal = D_ASSERT_STR_EQUAL("test_name", "hello", "world", "test message");

    if (obj_not_equal)
    {
        result = d_assert_standalone(
            obj_not_equal->result == false,
            "assert_str_equal_fails_unequal",
            "D_ASSERT_STR_EQUAL with unequal strings should have result=false",
            _counter) && result;

        d_test_object_free(obj_not_equal);
    }

    // test 4: NULL first string results in fail
    obj_null1 = D_ASSERT_STR_EQUAL("test_name", NULL, "hello", "test message");

    if (obj_null1)
    {
        result = d_assert_standalone(
            obj_null1->result == false,
            "assert_str_equal_fails_null1",
            "D_ASSERT_STR_EQUAL with NULL first string should have result=false",
            _counter) && result;

        d_test_object_free(obj_null1);
    }

    // test 5: NULL second string results in fail
    obj_null2 = D_ASSERT_STR_EQUAL("test_name", "hello", NULL, "test message");

    if (obj_null2)
    {
        result = d_assert_standalone(
            obj_null2->result == false,
            "assert_str_equal_fails_null2",
            "D_ASSERT_STR_EQUAL with NULL second string should have result=false",
            _counter) && result;

        d_test_object_free(obj_null2);
    }

    return result;
}


/*
d_tests_sa_standalone_assertion_macro_all
  Aggregation function that runs all assertion macro tests.
*/
bool
d_tests_sa_standalone_assertion_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Assertion Macros\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_standalone_assert_true(_counter) && result;
    result = d_tests_sa_standalone_assert_false(_counter) && result;
    result = d_tests_sa_standalone_assert_null(_counter) && result;
    result = d_tests_sa_standalone_assert_not_null(_counter) && result;
    result = d_tests_sa_standalone_assert_equal(_counter) && result;
    result = d_tests_sa_standalone_assert_str_equal(_counter) && result;

    return result;
}
