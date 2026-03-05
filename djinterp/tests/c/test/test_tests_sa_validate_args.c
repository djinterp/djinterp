#include "./test_tests_sa.h"


/*
d_tests_sa_test_validate_args_null
  Tests d_test_validate_args with NULL arguments.
  Tests the following:
  - d_test_validate_args(NULL, 0) returns NULL
  - d_test_validate_args(NULL, 5) returns NULL
*/
bool
d_tests_sa_test_validate_args_null
(
    struct d_test_counter* _counter
)
{
    struct d_test_options* config;
    bool                  result;

    result = true;

    // NULL args, zero count
    config = d_test_validate_args(NULL, 0);

    result = d_assert_standalone(config == NULL,
                                 "d_test_validate_args_null",
                                 "d_test_validate_args(NULL, 0) returns NULL",
                                 _counter) && result;

    // NULL args, non-zero count
    config = d_test_validate_args(NULL, 5);

    result = d_assert_standalone(config == NULL,
                                 "d_test_validate_args_null",
                                 "d_test_validate_args(NULL, 5) returns NULL",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_validate_args_zero
  Tests d_test_validate_args with valid args array but zero count.
  Tests the following:
  - d_test_validate_args with zero count returns NULL
*/
bool
d_tests_sa_test_validate_args_zero
(
    struct d_test_counter* _counter
)
{
    struct d_test_options* config;
    struct d_test_arg     args[1];
    bool                  result;

    result = true;

    args[0].key   = "name";
    args[0].value = (void*)"test_name";

    // valid args, zero count
    config = d_test_validate_args(args, 0);

    result = d_assert_standalone(config == NULL,
                                 "d_test_validate_args_zero",
                                 "d_test_validate_args with 0 count returns NULL",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_fn_validate_args_null
  Tests d_test_fn_validate_args with NULL arguments.
  Tests the following:
  - d_test_fn_validate_args(NULL, 0) returns NULL
  - d_test_fn_validate_args(NULL, 3) returns NULL
*/
bool
d_tests_sa_test_fn_validate_args_null
(
    struct d_test_counter* _counter
)
{
    struct d_test_options* config;
    bool                  result;

    result = true;

    // NULL args, zero count
    config = d_test_fn_validate_args(NULL, 0);

    result = d_assert_standalone(config == NULL,
                                 "d_test_fn_validate_args_null",
                                 "d_test_fn_validate_args(NULL, 0) returns NULL",
                                 _counter) && result;

    // NULL args, non-zero count
    config = d_test_fn_validate_args(NULL, 3);

    result = d_assert_standalone(config == NULL,
                                 "d_test_fn_validate_args_null",
                                 "d_test_fn_validate_args(NULL, 3) returns NULL",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_fn_validate_args_zero
  Tests d_test_fn_validate_args with valid args but zero count.
  Tests the following:
  - d_test_fn_validate_args with zero count returns NULL
*/
bool
d_tests_sa_test_fn_validate_args_zero
(
    struct d_test_counter* _counter
)
{
    struct d_test_options* config;
    struct d_test_arg     args[1];
    bool                  result;

    result = true;

    args[0].key   = "name";
    args[0].value = (void*)"fn_name";

    // valid args, zero count
    config = d_test_fn_validate_args(args, 0);

    result = d_assert_standalone(config == NULL,
                                 "d_test_fn_validate_args_zero",
                                 "d_test_fn_validate_args with 0 count returns NULL",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_validate_args_all
  Aggregation function for all validate_args tests.
*/
bool
d_tests_sa_test_validate_args_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_validate_args_null(_counter) && result;
    result = d_tests_sa_test_validate_args_zero(_counter) && result;
    result = d_tests_sa_test_fn_validate_args_null(_counter) && result;
    result = d_tests_sa_test_fn_validate_args_zero(_counter) && result;

    return result;
}
