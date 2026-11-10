/*******************************************************************************
* djinterp [container]                                   vector_tests_capacity.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_reserve
  Tests the d_vector_reserve function.
  Tests the following:
  - NULL vector rejection
  - no-op when new capacity <= current capacity
  - successful reservation with larger capacity
  - elements preserved after reservation
*/
bool d_tests_vector_reserve(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_reserve");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_null_vector",
                              d_vector_reserve(NULL, 100) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: no-op when new capacity <= current capacity
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_smaller_success",
                                  d_vector_reserve(vec, 30) == D_SUCCESS,
                                  "Smaller capacity should return D_SUCCESS (no-op)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_smaller_unchanged",
                                  vec->capacity == 50,
                                  "Capacity should remain unchanged") && result;

        d_vector_free(vec);
    }

    // test 3: successful reservation with larger capacity
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        // add some elements
        value = 100;
        d_vector_push_back(vec, &value);
        value = 200;
        d_vector_push_back(vec, &value);
        value = 300;
        d_vector_push_back(vec, &value);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_larger_success",
                                  d_vector_reserve(vec, 100) == D_SUCCESS,
                                  "Larger capacity reservation should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_larger_capacity",
                                  vec->capacity == 100,
                                  "Capacity should be updated to 100") && result;

        // test 4: elements preserved
        {
            int* arr      = (int*)vec->elements;
            bool preserved = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

            result = D_TEST_CHECK_MSG(_suite,
                                      "reserve_elements_preserved",
                                      preserved && vec->count == 3,
                                      "Elements should be preserved after reservation") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_shrink_to_fit
  Tests the d_vector_shrink_to_fit function.
  Tests the following:
  - NULL vector rejection
  - no-op when capacity equals count
  - successful shrink of over-allocated vector
  - elements preserved after shrink
*/
bool d_tests_vector_shrink_to_fit(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_shrink_to_fit");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_null_vector",
                              d_vector_shrink_to_fit(NULL) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful shrink
    vec = d_vector_new(sizeof(int), 100);

    if (vec)
    {
        // add a few elements
        value = 10;
        d_vector_push_back(vec, &value);
        value = 20;
        d_vector_push_back(vec, &value);
        value = 30;
        d_vector_push_back(vec, &value);

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_before_state",
                                  vec->capacity == 100 && vec->count == 3,
                                  "Before shrink: capacity=100, count=3") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_success",
                                  d_vector_shrink_to_fit(vec) == D_SUCCESS,
                                  "shrink_to_fit should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_capacity_reduced",
                                  vec->capacity == 3,
                                  "Capacity should be reduced to 3") && result;

        // test 3: elements preserved
        {
            int* arr      = (int*)vec->elements;
            bool preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "shrink_elements_preserved",
                                      preserved,
                                      "Elements should be preserved after shrink") && result;
        }

        d_vector_free(vec);
    }

    // test 4: shrink empty vector
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_empty_success",
                                  d_vector_shrink_to_fit(vec) == D_SUCCESS,
                                  "Shrinking empty vector should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_empty_capacity",
                                  vec->capacity == 0,
                                  "Empty vector should shrink to capacity 0") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_ensure_capacity
  Tests the d_vector_ensure_capacity function.
  Tests the following:
  - NULL vector rejection
  - no-op when sufficient capacity exists
  - successful growth when needed
*/
bool d_tests_vector_ensure_capacity(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_ensure_capacity");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_null_vector",
                              d_vector_ensure_capacity(NULL, 100) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: no-op when sufficient capacity exists
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_sufficient_success",
                                  d_vector_ensure_capacity(vec, 30) == D_SUCCESS,
                                  "Sufficient capacity should return D_SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_sufficient_unchanged",
                                  vec->capacity == 50,
                                  "Capacity should remain unchanged") && result;

        d_vector_free(vec);
    }

    // test 3: growth when needed
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_growth_success",
                                  d_vector_ensure_capacity(vec, 100) == D_SUCCESS,
                                  "Growth should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_growth_capacity",
                                  vec->capacity >= 100,
                                  "Capacity should be at least 100") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_available
  Tests the d_vector_available function.
  Tests the following:
  - NULL vector returns 0
  - empty vector returns full capacity
  - partially filled vector returns remaining
  - full vector returns 0
*/
bool d_tests_vector_available(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;
    size_t           avail;

    d_test_suite_test(_suite, "vector_available");

    result = true;

    // test 1: NULL vector should return 0
    avail  = d_vector_available(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "available_null_vector",
                              avail == 0,
                              "NULL vector should return 0") && result;

    // test 2: empty vector returns full capacity
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        avail  = d_vector_available(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_empty",
                                  avail == 10,
                                  "Empty vector should return full capacity (10)") && result;

        // test 3: partially filled
        value = 1;
        d_vector_push_back(vec, &value);
        d_vector_push_back(vec, &value);
        d_vector_push_back(vec, &value);

        avail  = d_vector_available(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_partial",
                                  avail == 7,
                                  "Partially filled should return 7") && result;

        d_vector_free(vec);
    }

    // test 4: full vector
    vec = d_vector_new_fill(sizeof(int), 5, &value);

    if (vec)
    {
        d_vector_shrink_to_fit(vec);

        avail  = d_vector_available(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "available_full",
                                  avail == 0,
                                  "Full vector should return 0") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool d_tests_vector_capacity_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_capacity_all");

    result = true;

    result = d_tests_vector_reserve(_suite) && result;
    result = d_tests_vector_shrink_to_fit(_suite) && result;
    result = d_tests_vector_ensure_capacity(_suite) && result;
    result = d_tests_vector_available(_suite) && result;

    return result;
}
