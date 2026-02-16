#include ".\test_common_tests_sa.h"


/******************************************************************************
 * III. ARGUMENT STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_test_arg
  Tests the d_test_arg structure.
  Tests the following:
  - struct d_test_arg has expected size
  - struct d_test_arg members are accessible
  - key member is const char*
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

    // test 1: struct size is at least two pointers (key and value)
    result = d_assert_standalone(
        sizeof(struct d_test_arg) >= (2 * sizeof(void*)),
        "test_arg_size",
        "d_test_arg should be at least two pointer sizes",
        _counter) && result;

    // test 2: struct contains key member (const char*)
    arg.key = "test_key";
    result  = d_assert_standalone(
        arg.key != NULL,
        "test_arg_key_accessible",
        "d_test_arg.key should be accessible",
        _counter) && result;

    // test 3: struct contains value member (void*)
    int_val   = 42;
    arg.value = &int_val;
    result    = d_assert_standalone(
        arg.value != NULL,
        "test_arg_value_accessible",
        "d_test_arg.value should be accessible",
        _counter) && result;

    // test 4: key member stores string correctly
    arg.key = "my_test_key";
    result  = d_assert_standalone(
        d_strcasecmp(arg.key, "my_test_key") == 0,
        "test_arg_key_value",
        "d_test_arg.key should store string value",
        _counter) && result;

    // test 5: value member can hold int pointer
    int_val   = 12345;
    arg.value = &int_val;
    result    = d_assert_standalone(
        *(int*)arg.value == 12345,
        "test_arg_value_int",
        "d_test_arg.value should hold int pointer",
        _counter) && result;

    // test 6: value member can hold double pointer
    double_val = 3.14159;
    arg.value  = &double_val;
    result     = d_assert_standalone(
        *(double*)arg.value == 3.14159,
        "test_arg_value_double",
        "d_test_arg.value should hold double pointer",
        _counter) && result;

    // test 7: value member can hold string pointer
    d_strcpy_s(str_val, sizeof(str_val), "hello world");
    arg.value = str_val;
    result    = d_assert_standalone(
        d_strcasecmp((char*)arg.value, "hello world") == 0,
        "test_arg_value_string",
        "d_test_arg.value should hold string pointer",
        _counter) && result;

    // test 8: value member can be NULL
    arg.value = NULL;
    result    = d_assert_standalone(
        arg.value == NULL,
        "test_arg_value_null",
        "d_test_arg.value should be assignable to NULL",
        _counter) && result;

    // test 9: struct can be initialized with compound literal
    {
        int                init_val = 100;
        struct d_test_arg  init_arg = {"init_key", &init_val};

        result = d_assert_standalone(
            (d_strcasecmp(init_arg.key, "init_key") == 0) &&
            (*(int*)init_arg.value == 100),
            "test_arg_compound_init",
            "d_test_arg should support compound literal initialization",
            _counter) && result;
    }

    // test 10: struct can be copied
    int_val   = 999;
    arg.key   = "original_key";
    arg.value = &int_val;

    arg2 = arg;  // struct copy

    result = d_assert_standalone(
        (arg2.key == arg.key) && (arg2.value == arg.value),
        "test_arg_copy",
        "d_test_arg should be copyable",
        _counter) && result;

    // test 11: copied struct shares same pointers (shallow copy)
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
  - count member is size_t
  - args member is pointer to d_test_arg array
  - struct can represent empty list
  - struct can represent list with multiple args
  - struct can be initialized with compound literal
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

    // test 1: struct size is at least size_t + pointer
    result = d_assert_standalone(
        sizeof(struct d_test_arg_list) >= (sizeof(size_t) + sizeof(void*)),
        "test_arg_list_size",
        "d_test_arg_list should be at least size_t + pointer size",
        _counter) && result;

    // test 2: count member is accessible and assignable
    arg_list.count = 5;
    result         = d_assert_standalone(
        arg_list.count == 5,
        "test_arg_list_count_accessible",
        "d_test_arg_list.count should be accessible",
        _counter) && result;

    // test 3: args member is accessible and assignable
    arg_list.args = NULL;
    result        = d_assert_standalone(
        arg_list.args == NULL,
        "test_arg_list_args_accessible",
        "d_test_arg_list.args should be accessible",
        _counter) && result;

    // test 4: empty list representation
    arg_list.count = 0;
    arg_list.args  = NULL;
    result         = d_assert_standalone(
        (arg_list.count == 0) && (arg_list.args == NULL),
        "test_arg_list_empty",
        "d_test_arg_list should represent empty list",
        _counter) && result;

    // test 5: single item list
    values[0]         = 100;
    args_array[0].key   = "single_key";
    args_array[0].value = &values[0];

    arg_list.count = 1;
    arg_list.args  = args_array;

    result = d_assert_standalone(
        (arg_list.count == 1) &&
        (arg_list.args != NULL) &&
        (d_strcasecmp(arg_list.args[0].key, "single_key") == 0),
        "test_arg_list_single",
        "d_test_arg_list should hold single argument",
        _counter) && result;

    // test 6: multiple item list
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    args_array[0].key   = "first";
    args_array[0].value = &values[0];
    args_array[1].key   = "second";
    args_array[1].value = &values[1];
    args_array[2].key   = "third";
    args_array[2].value = &values[2];

    arg_list.count = 3;
    arg_list.args  = args_array;

    result = d_assert_standalone(
        arg_list.count == 3,
        "test_arg_list_multiple_count",
        "d_test_arg_list should hold multiple count",
        _counter) && result;

    result = d_assert_standalone(
        (d_strcasecmp(arg_list.args[0].key, "first") == 0) &&
        (*(int*)arg_list.args[0].value == 10),
        "test_arg_list_multiple_first",
        "d_test_arg_list should access first element",
        _counter) && result;

    result = d_assert_standalone(
        (d_strcasecmp(arg_list.args[1].key, "second") == 0) &&
        (*(int*)arg_list.args[1].value == 20),
        "test_arg_list_multiple_second",
        "d_test_arg_list should access second element",
        _counter) && result;

    result = d_assert_standalone(
        (d_strcasecmp(arg_list.args[2].key, "third") == 0) &&
        (*(int*)arg_list.args[2].value == 30),
        "test_arg_list_multiple_third",
        "d_test_arg_list should access third element",
        _counter) && result;

    // test 7: struct can be copied
    {
        struct d_test_arg_list arg_list_copy;

        arg_list_copy = arg_list;

        result = d_assert_standalone(
            (arg_list_copy.count == arg_list.count) &&
            (arg_list_copy.args == arg_list.args),
            "test_arg_list_copy",
            "d_test_arg_list should be copyable (shallow)",
            _counter) && result;
    }

    // test 8: iteration through args
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

    // test 9: count type is size_t
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
