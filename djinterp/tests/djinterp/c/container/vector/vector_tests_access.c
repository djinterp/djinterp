/*******************************************************************************
* djinterp [container]                                     vector_tests_access.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_at
  Tests the d_vector_at function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid positive index
  - valid negative index
  - out-of-bounds returns NULL
*/
bool d_tests_vector_at(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    void*            ptr;

    d_test_suite_test(_suite, "vector_at");

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_at(NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "at_null_vector",
                              ptr == NULL,
                              "NULL vector should return NULL") && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_at(vec, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_empty_vector",
                                  ptr == NULL,
                                  "Empty vector should return NULL") && result;

        d_vector_free(vec);
    }

    // test 3: valid positive index
    vec = d_vector_new_from_args(sizeof(int), 3, 100, 200, 300);

    if (vec)
    {
        ptr    = d_vector_at(vec, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_positive_index",
                                  ptr != NULL && *(int*)ptr == 200,
                                  "Index 1 should return pointer to 200") && result;

        // test 4: valid negative index
        ptr    = d_vector_at(vec, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_negative_index",
                                  ptr != NULL && *(int*)ptr == 300,
                                  "Index -1 should return pointer to 300") && result;

        // test 5: out-of-bounds returns NULL
        ptr    = d_vector_at(vec, 10);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_out_of_bounds",
                                  ptr == NULL,
                                  "Out-of-bounds should return NULL") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_front
  Tests the d_vector_front function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid front access
*/
bool d_tests_vector_front(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    void*            ptr;

    d_test_suite_test(_suite, "vector_front");

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_front(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "front_null_vector",
                              ptr == NULL,
                              "NULL vector should return NULL") && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_front(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_empty_vector",
                                  ptr == NULL,
                                  "Empty vector should return NULL") && result;

        d_vector_free(vec);
    }

    // test 3: valid front access
    vec = d_vector_new_from_args(sizeof(int), 3, 111, 222, 333);

    if (vec)
    {
        ptr    = d_vector_front(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "front_valid",
                                  ptr != NULL && *(int*)ptr == 111,
                                  "Front should return pointer to 111") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_back
  Tests the d_vector_back function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid back access
*/
bool d_tests_vector_back(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    void*            ptr;

    d_test_suite_test(_suite, "vector_back");

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_back(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "back_null_vector",
                              ptr == NULL,
                              "NULL vector should return NULL") && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_back(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_empty_vector",
                                  ptr == NULL,
                                  "Empty vector should return NULL") && result;

        d_vector_free(vec);
    }

    // test 3: valid back access
    vec = d_vector_new_from_args(sizeof(int), 3, 111, 222, 333);

    if (vec)
    {
        ptr    = d_vector_back(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "back_valid",
                                  ptr != NULL && *(int*)ptr == 333,
                                  "Back should return pointer to 333") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_data
  Tests the d_vector_data function.
  Tests the following:
  - NULL vector returns NULL
  - valid data access
*/
bool d_tests_vector_data(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    void*            ptr;

    d_test_suite_test(_suite, "vector_data");

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_data(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "data_null_vector",
                              ptr == NULL,
                              "NULL vector should return NULL") && result;

    // test 2: valid data access
    vec = d_vector_new_from_args(sizeof(int), 3, 1, 2, 3);

    if (vec)
    {
        ptr    = d_vector_data(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "data_valid",
                                  ptr == vec->elements,
                                  "Data should return elements pointer") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_get
  Tests the d_vector_get function.
  Tests the following:
  - NULL vector rejection
  - NULL out_value rejection
  - empty vector rejection
  - successful get
*/
bool d_tests_vector_get(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              out_value;

    d_test_suite_test(_suite, "vector_get");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_vector",
                              d_vector_get(NULL, 0, &out_value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: NULL out_value should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_null_out",
                                  d_vector_get(vec, 0, NULL) == D_FAILURE,
                                  "NULL out_value should return D_FAILURE") && result;

        // test 3: successful get
        out_value = 0;
        result    = D_TEST_CHECK_MSG(_suite,
                                     "get_success",
                                     d_vector_get(vec, 1, &out_value) == D_SUCCESS,
                                     "Get should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "get_value",
                                  out_value == 20,
                                  "out_value should be 20") && result;

        // test 4: negative index
        out_value = 0;
        result    = D_TEST_CHECK_MSG(_suite,
                                     "get_negative",
                                     d_vector_get(vec, -1, &out_value) == D_SUCCESS && out_value == 30,
                                     "Get with -1 should return 30") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_set
  Tests the d_vector_set function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - empty vector rejection
  - successful set
*/
bool d_tests_vector_set(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_set");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_vector",
                              d_vector_set(NULL, 0, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: NULL value should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_null_value",
                                  d_vector_set(vec, 0, NULL) == D_FAILURE,
                                  "NULL value should return D_FAILURE") && result;

        // test 3: successful set
        value  = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_success",
                                  d_vector_set(vec, 1, &value) == D_SUCCESS,
                                  "Set should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_value",
                                  ((int*)vec->elements)[1] == 999,
                                  "Element at index 1 should be 999") && result;

        // test 4: negative index
        value  = 888;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_negative",
                                  d_vector_set(vec, -1, &value) == D_SUCCESS,
                                  "Set with negative index should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_negative_value",
                                  ((int*)vec->elements)[2] == 888,
                                  "Last element should be 888") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_access_all
  Aggregation function that runs all access tests.
*/
bool d_tests_vector_access_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_access_all");

    result = true;

    result = d_tests_vector_at(_suite) && result;
    result = d_tests_vector_front(_suite) && result;
    result = d_tests_vector_back(_suite) && result;
    result = d_tests_vector_data(_suite) && result;
    result = d_tests_vector_get(_suite) && result;
    result = d_tests_vector_set(_suite) && result;

    return result;
}
