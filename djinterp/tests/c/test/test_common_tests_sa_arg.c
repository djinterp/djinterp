#include "./test_common_tests_sa.h"


/******************************************************************************
 * III. ARGUMENT STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_test_arg
  Tests the d_test_arg structure.
  Tests the following:
  - struct d_test_arg has expected size
  - key member is int16_t
  - value member is void*
  - struct can be initialized with compound literal
  - struct can hold various value types via void*
  - struct can be copied
*/
bool
d_tests_sa_test_common_test_arg
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_test_arg  arg;
    struct d_test_arg  arg2;
    int                int_val;
    double             double_val;
    char               str_val[32];

    result = true;

    // test 1: struct size is at least int16_t + void*
    result = d_assert_standalone(
        sizeof(struct d_test_arg) >= (sizeof(int16_t) + sizeof(void*)),
        "test_arg_size",
        "d_test_arg should be at least int16_t + void* size",
        _counter) && result;

    // test 2: key member is accessible and stores int16_t
    arg.key = 42;
    result  = d_assert_standalone(
        arg.key == 42,
        "test_arg_key_accessible",
        "d_test_arg.key should be accessible as int16_t",
        _counter) && result;

    // test 3: key member can hold negative values (signed)
    arg.key = -1;
    result  = d_assert_standalone(
        arg.key == -1,
        "test_arg_key_signed",
        "d_test_arg.key should hold negative values (int16_t)",
        _counter) && result;

    // test 4: key member range covers DTestArgKey values
    arg.key = D_TEST_ARG_USER;
    result  = d_assert_standalone(
        arg.key == 256,
        "test_arg_key_range",
        "d_test_arg.key should hold D_TEST_ARG_USER (256)",
        _counter) && result;

    // test 5: value member is accessible
    int_val   = 42;
    arg.value = &int_val;
    result    = d_assert_standalone(
        arg.value != NULL,
        "test_arg_value_accessible",
        "d_test_arg.value should be accessible",
        _counter) && result;

    // test 6: value member can hold int pointer
    int_val   = 12345;
    arg.value = &int_val;
    result    = d_assert_standalone(
        *(int*)arg.value == 12345,
        "test_arg_value_int",
        "d_test_arg.value should hold int pointer",
        _counter) && result;

    // test 7: value member can hold double pointer
    double_val = 3.14159;
    arg.value  = &double_val;
    result     = d_assert_standalone(
        *(double*)arg.value == 3.14159,
        "test_arg_value_double",
        "d_test_arg.value should hold double pointer",
        _counter) && result;

    // test 8: value member can hold string pointer
    d_strcpy_s(str_val, sizeof(str_val), "hello world");
    arg.value = str_val;
    result    = d_assert_standalone(
        d_strcasecmp((char*)arg.value, "hello world") == 0,
        "test_arg_value_string",
        "d_test_arg.value should hold string pointer",
        _counter) && result;

    // test 9: value member can be NULL
    arg.value = NULL;
    result    = d_assert_standalone(
        arg.value == NULL,
        "test_arg_value_null",
        "d_test_arg.value should be assignable to NULL",
        _counter) && result;

    // test 10: struct can be initialized with compound literal
    {
        int                init_val = 100;
        struct d_test_arg  init_arg = {D_TEST_ARG_CONTEXT, &init_val};

        result = d_assert_standalone(
            (init_arg.key == D_TEST_ARG_CONTEXT) &&
            (*(int*)init_arg.value == 100),
            "test_arg_compound_init",
            "d_test_arg should support compound literal initialization",
            _counter) && result;
    }

    // test 11: struct can be copied
    int_val   = 999;
    arg.key   = D_TEST_ARG_ASSERTION_NUMBER;
    arg.value = &int_val;

    arg2 = arg;  // struct copy

    result = d_assert_standalone(
        (arg2.key == arg.key) && (arg2.value == arg.value),
        "test_arg_copy",
        "d_test_arg should be copyable",
        _counter) && result;

    // test 12: copied struct shares same pointers (shallow copy)
    result = d_assert_standalone(
        *(int*)arg2.value == 999,
        "test_arg_copy_value",
        "Copied d_test_arg should share value pointer",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_test_arg_list
  Tests the d_test_arg_list structure.
  Tests the following:
  - struct d_test_arg_list has expected size
  - count and capacity members are size_t
  - args member is pointer to d_test_arg array
  - struct can represent empty list
  - struct can represent list with multiple args
  - struct can be copied (shallow)
*/
bool
d_tests_sa_test_common_test_arg_list
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_arg_list  arg_list;
    struct d_test_arg       args_array[3];
    int                     values[3];

    result = true;

    // test 1: struct size is at least pointer + 2 * size_t
    result = d_assert_standalone(
        sizeof(struct d_test_arg_list) >=
            (sizeof(void*) + 2 * sizeof(size_t)),
        "test_arg_list_size",
        "d_test_arg_list should be at least pointer + 2*size_t",
        _counter) && result;

    // test 2: count member is accessible and assignable
    arg_list.count = 5;
    result         = d_assert_standalone(
        arg_list.count == 5,
        "test_arg_list_count_accessible",
        "d_test_arg_list.count should be accessible",
        _counter) && result;

    // test 3: capacity member is accessible and assignable
    arg_list.capacity = 10;
    result            = d_assert_standalone(
        arg_list.capacity == 10,
        "test_arg_list_capacity_accessible",
        "d_test_arg_list.capacity should be accessible",
        _counter) && result;

    // test 4: args member is accessible and assignable
    arg_list.args = NULL;
    result        = d_assert_standalone(
        arg_list.args == NULL,
        "test_arg_list_args_accessible",
        "d_test_arg_list.args should be accessible",
        _counter) && result;

    // test 5: empty list representation
    arg_list.count    = 0;
    arg_list.capacity = 0;
    arg_list.args     = NULL;
    result            = d_assert_standalone(
        (arg_list.count == 0) &&
        (arg_list.capacity == 0) &&
        (arg_list.args == NULL),
        "test_arg_list_empty",
        "d_test_arg_list should represent empty list",
        _counter) && result;

    // test 6: single item list
    values[0]           = 100;
    args_array[0].key   = D_TEST_ARG_CONTEXT;
    args_array[0].value = &values[0];

    arg_list.count    = 1;
    arg_list.capacity = 3;
    arg_list.args     = args_array;

    result = d_assert_standalone(
        (arg_list.count == 1) &&
        (arg_list.args != NULL) &&
        (arg_list.args[0].key == D_TEST_ARG_CONTEXT),
        "test_arg_list_single",
        "d_test_arg_list should hold single argument",
        _counter) && result;

    // test 7: multiple item list with int16_t keys
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    args_array[0].key   = D_TEST_ARG_ASSERTION_NUMBER;
    args_array[0].value = &values[0];
    args_array[1].key   = D_TEST_ARG_TEST_NUMBER;
    args_array[1].value = &values[1];
    args_array[2].key   = D_TEST_ARG_CONTEXT;
    args_array[2].value = &values[2];

    arg_list.count    = 3;
    arg_list.capacity = 3;
    arg_list.args     = args_array;

    result = d_assert_standalone(
        arg_list.count == 3,
        "test_arg_list_multiple_count",
        "d_test_arg_list should hold multiple count",
        _counter) && result;

    result = d_assert_standalone(
        (arg_list.args[0].key == D_TEST_ARG_ASSERTION_NUMBER) &&
        (*(int*)arg_list.args[0].value == 10),
        "test_arg_list_multiple_first",
        "d_test_arg_list should access first element",
        _counter) && result;

    result = d_assert_standalone(
        (arg_list.args[1].key == D_TEST_ARG_TEST_NUMBER) &&
        (*(int*)arg_list.args[1].value == 20),
        "test_arg_list_multiple_second",
        "d_test_arg_list should access second element",
        _counter) && result;

    result = d_assert_standalone(
        (arg_list.args[2].key == D_TEST_ARG_CONTEXT) &&
        (*(int*)arg_list.args[2].value == 30),
        "test_arg_list_multiple_third",
        "d_test_arg_list should access third element",
        _counter) && result;

    // test 8: struct can be copied
    {
        struct d_test_arg_list arg_list_copy;

        arg_list_copy = arg_list;

        result = d_assert_standalone(
            (arg_list_copy.count == arg_list.count) &&
            (arg_list_copy.capacity == arg_list.capacity) &&
            (arg_list_copy.args == arg_list.args),
            "test_arg_list_copy",
            "d_test_arg_list should be copyable (shallow)",
            _counter) && result;
    }

    // test 9: iteration through args
    {
        size_t i;
        int    sum;
        bool   iter_ok;

        sum     = 0;
        iter_ok = true;

        for (i = 0; i < arg_list.count; i++)
        {
            if (arg_list.args[i].value != NULL)
            {
                sum += *(int*)arg_list.args[i].value;
            }
            else
            {
                iter_ok = false;
            }
        }

        result = d_assert_standalone(
            iter_ok && (sum == 60),
            "test_arg_list_iteration",
            "d_test_arg_list should support iteration (sum=60)",
            _counter) && result;
    }

    // test 10: count type is size_t
    arg_list.count = SIZE_MAX;
    result         = d_assert_standalone(
        arg_list.count == SIZE_MAX,
        "test_arg_list_count_type",
        "d_test_arg_list.count should be size_t (hold SIZE_MAX)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_arg_all
  Aggregation function that runs all argument structure tests.
*/
bool
d_tests_sa_test_common_arg_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Argument Structures\n");
    printf("  ------------------------------\n");

    result = d_tests_sa_test_common_test_arg(_counter) && result;
    result = d_tests_sa_test_common_test_arg_list(_counter) && result;

    return result;
}
