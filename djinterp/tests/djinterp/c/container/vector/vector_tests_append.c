/*******************************************************************************
* djinterp [container]                                     vector_tests_append.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_append.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_append_element
  Tests the d_vector_append_element function.
  Tests the following:
  - NULL vector rejection
  - successful append of single element
*/
bool d_tests_vector_append_element(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_append_element");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_null_vector",
                              d_vector_append_element(NULL, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful append to empty vector
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        value  = 100;
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_success",
                                  d_vector_append_element(vec, &value) == D_SUCCESS,
                                  "Append element should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_result",
                                  vec->count == 1 && ((int*)vec->elements)[0] == 100,
                                  "Count=1, element[0]=100") && result;

        // test 3: append to non-empty vector
        value  = 200;
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_nonempty_success",
                                  d_vector_append_element(vec, &value) == D_SUCCESS,
                                  "Append to non-empty should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_element_nonempty_result",
                                  vec->count == 2 && ((int*)vec->elements)[1] == 200,
                                  "Count=2, element[1]=200") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_append_elements
  Tests the d_vector_append_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - NULL source with non-zero count failure
  - successful append
*/
bool d_tests_vector_append_elements(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              source[3] = {40, 50, 60};

    d_test_suite_test(_suite, "vector_append_elements");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_null_vector",
                              d_vector_append_elements(NULL, source, 3) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_zero_count",
                                  d_vector_append_elements(vec, source, 0) == D_SUCCESS,
                                  "Zero count should return D_SUCCESS") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_zero_count_unchanged",
                                  vec->count == 3,
                                  "Count should remain unchanged") && result;

        // test 3: successful append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_success",
                                  d_vector_append_elements(vec, source, 3) == D_SUCCESS,
                                  "Append should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_count",
                                  vec->count == 6,
                                  "Count should be 6") && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30) &&
                           (arr[3] == 40) && (arr[4] == 50) && (arr[5] == 60);

            result = D_TEST_CHECK_MSG(_suite,
                                      "append_elements_values",
                                      correct,
                                      "Elements should be [10, 20, 30, 40, 50, 60]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_append_vector
  Tests the d_vector_append_vector function.
  Tests the following:
  - NULL destination rejection
  - NULL source rejection
  - element size mismatch rejection
  - successful vector append
*/
bool d_tests_vector_append_vector(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* dest;
    struct dj_vector* src;
    struct dj_vector* wrong_size;

    d_test_suite_test(_suite, "vector_append_vector");

    result = true;

    // test 1: NULL destination should fail
    src    = d_vector_new_from_args(sizeof(int), 2, 40, 50);
    result = D_TEST_CHECK_MSG(_suite,
                              "append_vector_null_dest",
                              d_vector_append_vector(NULL, src) == D_FAILURE,
                              "NULL destination should return D_FAILURE") && result;

    if (src)
    {
        d_vector_free(src);
    }

    // test 2: NULL source should fail
    dest   = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    result = D_TEST_CHECK_MSG(_suite,
                              "append_vector_null_src",
                              d_vector_append_vector(dest, NULL) == D_FAILURE,
                              "NULL source should return D_FAILURE") && result;

    if (dest)
    {
        d_vector_free(dest);
    }

    // test 3: element size mismatch should fail
    dest       = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    wrong_size = d_vector_new(sizeof(double), 5);

    if ( (dest) &&
         (wrong_size) )
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_size_mismatch",
                                  d_vector_append_vector(dest, wrong_size) == D_FAILURE,
                                  "Element size mismatch should return D_FAILURE") && result;

        d_vector_free(dest);
        d_vector_free(wrong_size);
    }
    else
    {
        if (dest)
        {
            d_vector_free(dest);
        }

        if (wrong_size)
        {
            d_vector_free(wrong_size);
        }
    }

    // test 4: successful vector append
    dest = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    src  = d_vector_new_from_args(sizeof(int), 3, 30, 40, 50);

    if ( (dest) &&
         (src) )
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_success",
                                  d_vector_append_vector(dest, src) == D_SUCCESS,
                                  "Append vector should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_vector_count",
                                  dest->count == 5,
                                  "Destination count should be 5") && result;

        {
            int* arr = (int*)dest->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30) &&
                           (arr[3] == 40) && (arr[4] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "append_vector_values",
                                      correct,
                                      "Elements should be [10, 20, 30, 40, 50]") && result;
        }

        d_vector_free(dest);
        d_vector_free(src);
    }

    return result;
}


/*
d_tests_vector_prepend_element
  Tests the d_vector_prepend_element function.
  Tests the following:
  - NULL vector rejection
  - successful prepend of single element
*/
bool d_tests_vector_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_prepend_element");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_element_null_vector",
                              d_vector_prepend_element(NULL, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful prepend to empty vector
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        value  = 100;
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_success",
                                  d_vector_prepend_element(vec, &value) == D_SUCCESS,
                                  "Prepend element should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_result",
                                  vec->count == 1 && ((int*)vec->elements)[0] == 100,
                                  "Count=1, element[0]=100") && result;

        // test 3: prepend to non-empty vector (shifts existing)
        value  = 50;
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_nonempty_success",
                                  d_vector_prepend_element(vec, &value) == D_SUCCESS,
                                  "Prepend to non-empty should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 50) && (arr[1] == 100);

            result = D_TEST_CHECK_MSG(_suite,
                                      "prepend_element_shifted",
                                      correct && vec->count == 2,
                                      "Elements should be [50, 100]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_prepend_elements
  Tests the d_vector_prepend_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - successful prepend
*/
bool d_tests_vector_prepend_elements(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              source[2] = {1, 2};

    d_test_suite_test(_suite, "vector_prepend_elements");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_null_vector",
                              d_vector_prepend_elements(NULL, source, 2) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_zero_count",
                                  d_vector_prepend_elements(vec, source, 0) == D_SUCCESS,
                                  "Zero count should return D_SUCCESS") && result;

        // test 3: successful prepend
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_success",
                                  d_vector_prepend_elements(vec, source, 2) == D_SUCCESS,
                                  "Prepend should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_count",
                                  vec->count == 5,
                                  "Count should be 5") && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 10) &&
                           (arr[3] == 20) && (arr[4] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "prepend_elements_values",
                                      correct,
                                      "Elements should be [1, 2, 10, 20, 30]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_prepend_vector
  Tests the d_vector_prepend_vector function.
  Tests the following:
  - NULL destination rejection
  - NULL source rejection
  - successful vector prepend
*/
bool d_tests_vector_prepend_vector(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* dest;
    struct dj_vector* src;

    d_test_suite_test(_suite, "vector_prepend_vector");

    result = true;

    // test 1: NULL destination should fail
    src    = d_vector_new_from_args(sizeof(int), 2, 1, 2);
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_vector_null_dest",
                              d_vector_prepend_vector(NULL, src) == D_FAILURE,
                              "NULL destination should return D_FAILURE") && result;

    if (src)
    {
        d_vector_free(src);
    }

    // test 2: NULL source should fail
    dest   = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_vector_null_src",
                              d_vector_prepend_vector(dest, NULL) == D_FAILURE,
                              "NULL source should return D_FAILURE") && result;

    if (dest)
    {
        d_vector_free(dest);
    }

    // test 3: successful vector prepend
    dest = d_vector_new_from_args(sizeof(int), 2, 30, 40);
    src  = d_vector_new_from_args(sizeof(int), 2, 10, 20);

    if ( (dest) &&
         (src) )
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_vector_success",
                                  d_vector_prepend_vector(dest, src) == D_SUCCESS,
                                  "Prepend vector should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_vector_count",
                                  dest->count == 4,
                                  "Destination count should be 4") && result;

        {
            int* arr = (int*)dest->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40);

            result = D_TEST_CHECK_MSG(_suite,
                                      "prepend_vector_values",
                                      correct,
                                      "Elements should be [10, 20, 30, 40]") && result;
        }

        d_vector_free(dest);
        d_vector_free(src);
    }

    return result;
}


/*
d_tests_vector_append_all
  Aggregation function that runs all append/prepend tests.
*/
bool d_tests_vector_append_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_append_all");

    result = true;

    result = d_tests_vector_append_element(_suite) && result;
    result = d_tests_vector_append_elements(_suite) && result;
    result = d_tests_vector_append_vector(_suite) && result;
    result = d_tests_vector_prepend_element(_suite) && result;
    result = d_tests_vector_prepend_elements(_suite) && result;
    result = d_tests_vector_prepend_vector(_suite) && result;

    return result;
}
