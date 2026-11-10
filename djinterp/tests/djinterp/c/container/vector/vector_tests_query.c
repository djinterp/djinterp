/*******************************************************************************
* djinterp [container]                                      vector_tests_query.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_is_empty
  Tests the d_vector_is_empty function.
  Tests the following:
  - NULL vector returns true
  - empty vector returns true
  - non-empty vector returns false
*/
bool d_tests_vector_is_empty(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_is_empty");

    result = true;

    // test 1: NULL vector returns true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_null",
                              d_vector_is_empty(NULL) == true,
                              "NULL vector should return true") && result;

    // test 2: empty vector returns true
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_empty",
                                  d_vector_is_empty(vec) == true,
                                  "Empty vector should return true") && result;

        d_vector_free(vec);
    }

    // test 3: non-empty vector returns false
    vec = d_vector_new_from_args(sizeof(int), 3, 1, 2, 3);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_nonempty",
                                  d_vector_is_empty(vec) == false,
                                  "Non-empty vector should return false") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_is_full
  Tests the d_vector_is_full function.
  Tests the following:
  - NULL vector returns false
  - full vector returns true
  - non-full vector returns false
*/
bool d_tests_vector_is_full(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              fill = 42;

    d_test_suite_test(_suite, "vector_is_full");

    result = true;

    // test 1: NULL vector returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "is_full_null",
                              d_vector_is_full(NULL) == false,
                              "NULL vector should return false") && result;

    // test 2: full vector returns true
    vec = d_vector_new_fill(sizeof(int), 5, &fill);

    if (vec)
    {
        d_vector_shrink_to_fit(vec);

        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_full",
                                  d_vector_is_full(vec) == true,
                                  "Full vector should return true") && result;

        d_vector_free(vec);
    }

    // test 3: non-full vector returns false
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        d_vector_push_back(vec, &fill);

        result = D_TEST_CHECK_MSG(_suite,
                                  "is_full_partial",
                                  d_vector_is_full(vec) == false,
                                  "Partially filled vector should return false") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_size
  Tests the d_vector_size function.
  Tests the following:
  - NULL vector returns 0
  - empty vector returns 0
  - non-empty vector returns correct count
*/
bool d_tests_vector_size(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_size");

    result = true;

    // test 1: NULL vector returns 0
    result = D_TEST_CHECK_MSG(_suite,
                              "size_null",
                              d_vector_size(NULL) == 0,
                              "NULL vector should return 0") && result;

    // test 2: empty vector returns 0
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "size_empty",
                                  d_vector_size(vec) == 0,
                                  "Empty vector should return 0") && result;

        d_vector_free(vec);
    }

    // test 3: non-empty vector returns correct count
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "size_nonempty",
                                  d_vector_size(vec) == 5,
                                  "Size should be 5") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_capacity_query
  Tests the d_vector_capacity function.
  Tests the following:
  - NULL vector returns 0
  - valid vector returns correct capacity
*/
bool d_tests_vector_capacity_query(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_capacity_query");

    result = true;

    // test 1: NULL vector returns 0
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_null",
                              d_vector_capacity(NULL) == 0,
                              "NULL vector should return 0") && result;

    // test 2: valid vector returns correct capacity
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_valid",
                                  d_vector_capacity(vec) == 50,
                                  "Capacity should be 50") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_element_size
  Tests the d_vector_element_size function.
  Tests the following:
  - NULL vector returns 0
  - valid vector returns correct element size
*/
bool d_tests_vector_element_size(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_element_size");

    result = true;

    // test 1: NULL vector returns 0
    result = D_TEST_CHECK_MSG(_suite,
                              "element_size_null",
                              d_vector_element_size(NULL) == 0,
                              "NULL vector should return 0") && result;

    // test 2: valid vector returns correct element size
    vec = d_vector_new(sizeof(double), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "element_size_valid",
                                  d_vector_element_size(vec) == sizeof(double),
                                  "Element size should be sizeof(double)") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_query_all
  Aggregation function that runs all query tests.
*/
bool d_tests_vector_query_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_query_all");

    result = true;

    result = d_tests_vector_is_empty(_suite) && result;
    result = d_tests_vector_is_full(_suite) && result;
    result = d_tests_vector_size(_suite) && result;
    result = d_tests_vector_capacity_query(_suite) && result;
    result = d_tests_vector_element_size(_suite) && result;

    return result;
}
