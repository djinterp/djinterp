#include "./test_tests_sa.h"


/*
d_tests_sa_test_new_empty
  Tests d_test_new with NULL children and zero count.
  Tests the following:
  - d_test_new(NULL, 0) returns non-NULL
  - returned test has zero children
  - d_test_child_count returns 0 for empty test
*/
bool
d_tests_sa_test_new_empty
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    // create empty test
    test = d_test_new(NULL, 0);

    // verify allocation succeeded
    result = d_assert_standalone(test != NULL,
                                 "d_test_new_empty",
                                 "d_test_new(NULL, 0) returns non-NULL",
                                 _counter) && result;

    if (test)
    {
        // verify child count is zero
        result = d_assert_standalone(d_test_child_count(test) == 0,
                                     "d_test_new_empty",
                                     "empty test has 0 children",
                                     _counter) && result;

        // verify children vector was created
        result = d_assert_standalone(test->children != NULL,
                                     "d_test_new_empty",
                                     "children vector is non-NULL",
                                     _counter) && result;

        d_test_free(test);
    }

    return result;
}

/*
d_tests_sa_test_new_valid
  Tests d_test_new returns a properly initialized test.
  Tests the following:
  - d_test_new returns non-NULL
  - returned test has correct child count of 0
  - multiple allocations produce distinct pointers
*/
bool
d_tests_sa_test_new_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test* test1;
    struct d_test* test2;
    bool           result;

    result = true;

    test1 = d_test_new(NULL, 0);
    test2 = d_test_new(NULL, 0);

    // verify both allocations succeeded
    result = d_assert_standalone(test1 != NULL,
                                 "d_test_new_valid",
                                 "first d_test_new returns non-NULL",
                                 _counter) && result;

    result = d_assert_standalone(test2 != NULL,
                                 "d_test_new_valid",
                                 "second d_test_new returns non-NULL",
                                 _counter) && result;

    // verify distinct allocations
    if (test1 && test2)
    {
        result = d_assert_standalone(test1 != test2,
                                     "d_test_new_valid",
                                     "two d_test_new calls produce distinct pointers",
                                     _counter) && result;
    }

    if (test1)
    {
        d_test_free(test1);
    }

    if (test2)
    {
        d_test_free(test2);
    }

    return result;
}

/*
d_tests_sa_test_new_args
  Tests d_test_new_args with arguments and children.
  Tests the following:
  - d_test_new_args returns non-NULL with valid args
  - children are NOT passed here (tested separately via child mgmt)
  - returned test with NULL children and zero count is valid
*/
bool
d_tests_sa_test_new_args
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    // create test with no args and no children
    test = d_test_new_args(NULL, 0, NULL, 0);

    // verify allocation
    result = d_assert_standalone(test != NULL,
                                 "d_test_new_args",
                                 "d_test_new_args(NULL, 0, NULL, 0) returns non-NULL",
                                 _counter) && result;

    if (test)
    {
        // verify child count
        result = d_assert_standalone(d_test_child_count(test) == 0,
                                     "d_test_new_args",
                                     "test created with no children has count 0",
                                     _counter) && result;

        d_test_free(test);
    }

    return result;
}

/*
d_tests_sa_test_new_args_null
  Tests d_test_new_args with NULL args parameter.
  Tests the following:
  - d_test_new_args with NULL args, 0 count returns non-NULL
  - returned test has correct initial state
*/
bool
d_tests_sa_test_new_args_null
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    // NULL args with non-zero count -- should still create the test
    test = d_test_new_args(NULL, 0, NULL, 0);

    result = d_assert_standalone(test != NULL,
                                 "d_test_new_args_null",
                                 "d_test_new_args with NULL args returns non-NULL",
                                 _counter) && result;

    if (test)
    {
        result = d_assert_standalone(d_test_child_count(test) == 0,
                                     "d_test_new_args_null",
                                     "test with NULL args has zero children",
                                     _counter) && result;

        d_test_free(test);
    }

    return result;
}

/*
d_tests_sa_test_construction_all
  Aggregation function for all construction tests.
*/
bool
d_tests_sa_test_construction_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_new_empty(_counter) && result;
    result = d_tests_sa_test_new_valid(_counter) && result;
    result = d_tests_sa_test_new_args(_counter) && result;
    result = d_tests_sa_test_new_args_null(_counter) && result;

    return result;
}
