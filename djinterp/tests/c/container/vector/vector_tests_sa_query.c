#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_is_empty
  Tests the d_vector_is_empty function.
  Tests the following:
  - NULL vector returns true
  - empty vector returns true
  - non-empty vector returns false
*/
bool
d_tests_sa_vector_is_empty
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector returns true
    result = d_assert_standalone(
        d_vector_is_empty(NULL) == true,
        "is_empty_null",
        "NULL vector should return true",
        _counter) && result;

    // test 2: empty vector returns true
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_is_empty(vec) == true,
            "is_empty_empty",
            "Empty vector should return true",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: non-empty vector returns false
    vec = d_vector_new_from_args(sizeof(int), 3, 1, 2, 3);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_is_empty(vec) == false,
            "is_empty_nonempty",
            "Non-empty vector should return false",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_is_full
  Tests the d_vector_is_full function.
  Tests the following:
  - NULL vector returns false
  - full vector returns true
  - non-full vector returns false
*/
bool
d_tests_sa_vector_is_full
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              fill = 42;

    result = true;

    // test 1: NULL vector returns false
    result = d_assert_standalone(
        d_vector_is_full(NULL) == false,
        "is_full_null",
        "NULL vector should return false",
        _counter) && result;

    // test 2: full vector returns true
    vec = d_vector_new_fill(sizeof(int), 5, &fill);

    if (vec)
    {
        d_vector_shrink_to_fit(vec);

        result = d_assert_standalone(
            d_vector_is_full(vec) == true,
            "is_full_full",
            "Full vector should return true",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: non-full vector returns false
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        d_vector_push_back(vec, &fill);

        result = d_assert_standalone(
            d_vector_is_full(vec) == false,
            "is_full_partial",
            "Partially filled vector should return false",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_size
  Tests the d_vector_size function.
  Tests the following:
  - NULL vector returns 0
  - empty vector returns 0
  - non-empty vector returns correct count
*/
bool
d_tests_sa_vector_size
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector returns 0
    result = d_assert_standalone(
        d_vector_size(NULL) == 0,
        "size_null",
        "NULL vector should return 0",
        _counter) && result;

    // test 2: empty vector returns 0
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_size(vec) == 0,
            "size_empty",
            "Empty vector should return 0",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: non-empty vector returns correct count
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_size(vec) == 5,
            "size_nonempty",
            "Size should be 5",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_capacity_query
  Tests the d_vector_capacity function.
  Tests the following:
  - NULL vector returns 0
  - valid vector returns correct capacity
*/
bool
d_tests_sa_vector_capacity_query
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector returns 0
    result = d_assert_standalone(
        d_vector_capacity(NULL) == 0,
        "capacity_null",
        "NULL vector should return 0",
        _counter) && result;

    // test 2: valid vector returns correct capacity
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_capacity(vec) == 50,
            "capacity_valid",
            "Capacity should be 50",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_element_size
  Tests the d_vector_element_size function.
  Tests the following:
  - NULL vector returns 0
  - valid vector returns correct element size
*/
bool
d_tests_sa_vector_element_size
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector returns 0
    result = d_assert_standalone(
        d_vector_element_size(NULL) == 0,
        "element_size_null",
        "NULL vector should return 0",
        _counter) && result;

    // test 2: valid vector returns correct element size
    vec = d_vector_new(sizeof(double), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_element_size(vec) == sizeof(double),
            "element_size_valid",
            "Element size should be sizeof(double)",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_query_all
  Aggregation function that runs all query tests.
*/
bool
d_tests_sa_vector_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_vector_is_empty(_counter) && result;
    result = d_tests_sa_vector_is_full(_counter) && result;
    result = d_tests_sa_vector_size(_counter) && result;
    result = d_tests_sa_vector_capacity_query(_counter) && result;
    result = d_tests_sa_vector_element_size(_counter) && result;

    return result;
}
