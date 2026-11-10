/*******************************************************************************
* djinterp [container]                                vector_tests_constructor.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_constructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_new
  Tests the d_vector_new constructor function.
  Tests the following:
  - zero element_size rejection
  - successful creation with various element sizes
  - zero initial capacity handling
  - struct initialization verification
*/
bool d_tests_vector_new(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_new");

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new(0, 10);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_element_size",
                              vec == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: successful creation with int elements
    vec    = d_vector_new(sizeof(int), 10);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_int_success",
                              vec != NULL,
                              "d_vector_new with sizeof(int) should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_int_element_size",
                                  vec->element_size == sizeof(int),
                                  "element_size should be sizeof(int)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_int_capacity",
                                  vec->capacity == 10,
                                  "capacity should be 10") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_int_count",
                                  vec->count == 0,
                                  "count should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_int_elements_allocated",
                                  vec->elements != NULL,
                                  "elements should be allocated") && result;

        d_vector_free(vec);
    }

    // test 3: zero initial capacity should succeed
    vec    = d_vector_new(sizeof(int), 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_capacity_success",
                              vec != NULL,
                              "Zero initial capacity should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_zero_capacity_values",
                                  vec->capacity == 0 && vec->count == 0,
                                  "Zero capacity should have count=0, capacity=0") && result;

        d_vector_free(vec);
    }

    // test 4: large struct element size
    {
        struct large_struct
        {
            int    data[100];
            double values[50];
        };

        vec    = d_vector_new(sizeof(struct large_struct), 5);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_large_struct_success",
                                  vec != NULL,
                                  "d_vector_new with large struct should succeed") && result;

        if (vec)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_large_struct_element_size",
                                      vec->element_size == sizeof(struct large_struct),
                                      "element_size should match large struct size") && result;

            d_vector_free(vec);
        }
    }

    return result;
}


/*
d_tests_vector_new_default
  Tests the d_vector_new_default constructor function.
  Tests the following:
  - zero element_size rejection
  - successful creation with default capacity
  - default capacity value verification
*/
bool d_tests_vector_new_default(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_new_default");

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_default(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_zero_element_size",
                              vec == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: successful creation with default capacity
    vec    = d_vector_new_default(sizeof(int));
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_success",
                              vec != NULL,
                              "d_vector_new_default should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_capacity",
                                  vec->capacity == D_VECTOR_DEFAULT_CAPACITY,
                                  "capacity should be D_VECTOR_DEFAULT_CAPACITY") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_count",
                                  vec->count == 0,
                                  "count should be 0") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_new_from_array
  Tests the d_vector_new_from_array constructor function.
  Tests the following:
  - zero element_size rejection
  - NULL source with non-zero count rejection
  - NULL source with zero count handling
  - successful creation from valid array
  - elements correctly copied
*/
bool d_tests_vector_new_from_array(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              source[5] = {10, 20, 30, 40, 50};

    d_test_suite_test(_suite, "vector_new_from_array");

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_from_array(0, source, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_zero_element_size",
                              vec == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: NULL source with non-zero count should return NULL
    vec    = d_vector_new_from_array(sizeof(int), NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_null_source",
                              vec == NULL,
                              "NULL source with non-zero count should return NULL") && result;

    // test 3: NULL source with zero count should succeed
    vec    = d_vector_new_from_array(sizeof(int), NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_null_zero_count",
                              vec != NULL,
                              "NULL source with zero count should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_null_zero_count_empty",
                                  vec->count == 0,
                                  "Should create empty vector") && result;

        d_vector_free(vec);
    }

    // test 4: successful creation from valid array
    vec    = d_vector_new_from_array(sizeof(int), source, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_valid_success",
                              vec != NULL,
                              "d_vector_new_from_array with valid array should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_valid_count",
                                  vec->count == 5,
                                  "count should be 5") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_valid_capacity",
                                  vec->capacity >= 5,
                                  "capacity should be at least 5") && result;

        // test 5: verify elements copied correctly
        {
            int*   arr       = (int*)vec->elements;
            bool   all_match = true;
            size_t i;

            for (i = 0; i < 5; i++)
            {
                if (arr[i] != source[i])
                {
                    all_match = false;

                    break;
                }
            }

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_from_array_elements_copied",
                                      all_match,
                                      "All elements should be correctly copied") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_new_from_args
  Tests the d_vector_new_from_args constructor function.
  Tests the following:
  - zero element_size rejection
  - zero arg_count handling
  - successful creation with variadic arguments
*/
bool d_tests_vector_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_new_from_args");

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_from_args(0, 3, 1, 2, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_zero_element_size",
                              vec == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: zero arg_count should succeed with empty vector
    vec    = d_vector_new_from_args(sizeof(int), 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_zero_count_success",
                              vec != NULL,
                              "Zero arg_count should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_zero_count_empty",
                                  vec->count == 0,
                                  "Should create empty vector") && result;

        d_vector_free(vec);
    }

    // test 3: successful creation with int arguments
    vec    = d_vector_new_from_args(sizeof(int), 3, 100, 200, 300);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_int_success",
                              vec != NULL,
                              "d_vector_new_from_args with ints should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_int_count",
                                  vec->count == 3,
                                  "count should be 3") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_from_args_int_values",
                                      correct,
                                      "Values should be [100, 200, 300]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_new_copy
  Tests the d_vector_new_copy constructor function.
  Tests the following:
  - NULL source rejection
  - successful deep copy creation
  - all properties copied correctly
  - elements independently copied (not shared)
*/
bool d_tests_vector_new_copy(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* original;
    struct dj_vector* copy;

    d_test_suite_test(_suite, "vector_new_copy");

    result = true;

    // test 1: NULL source should return NULL
    copy   = d_vector_new_copy(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_null_source",
                              copy == NULL,
                              "NULL source should return NULL") && result;

    // test 2: successful copy of non-empty vector
    original = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (original)
    {
        copy   = d_vector_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_success",
                                  copy != NULL,
                                  "d_vector_new_copy should succeed") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_count_match",
                                      copy->count == original->count,
                                      "copy count should match original") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_element_size_match",
                                      copy->element_size == original->element_size,
                                      "copy element_size should match original") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_elements_independent",
                                      copy->elements != original->elements,
                                      "copy elements should be independent allocation") && result;

            // verify elements match
            {
                int* orig_arr = (int*)original->elements;
                int* copy_arr = (int*)copy->elements;
                bool match    = (copy_arr[0] == orig_arr[0]) &&
                                (copy_arr[1] == orig_arr[1]) &&
                                (copy_arr[2] == orig_arr[2]);

                result = D_TEST_CHECK_MSG(_suite,
                                          "new_copy_elements_match",
                                          match,
                                          "copy elements should match original values") && result;
            }

            d_vector_free(copy);
        }

        d_vector_free(original);
    }

    // test 3: copy of empty vector
    original = d_vector_new(sizeof(int), 10);

    if (original)
    {
        copy   = d_vector_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_empty_success",
                                  copy != NULL,
                                  "Copy of empty vector should succeed") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_empty_count",
                                      copy->count == 0,
                                      "Copy of empty vector should have count 0") && result;

            d_vector_free(copy);
        }

        d_vector_free(original);
    }

    return result;
}


/*
d_tests_vector_new_fill
  Tests the d_vector_new_fill constructor function.
  Tests the following:
  - zero element_size rejection
  - zero count handling
  - NULL fill value handling
  - successful fill creation
  - all elements match fill value
*/
bool d_tests_vector_new_fill(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              fill_value;

    d_test_suite_test(_suite, "vector_new_fill");

    result = true;

    // test 1: zero element_size should return NULL
    fill_value = 42;
    vec        = d_vector_new_fill(0, 5, &fill_value);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_zero_element_size",
                                  vec == NULL,
                                  "Zero element_size should return NULL") && result;

    // test 2: zero count should succeed with empty vector
    vec    = d_vector_new_fill(sizeof(int), 0, &fill_value);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_zero_count_success",
                              vec != NULL,
                              "Zero count should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_zero_count_empty",
                                  vec->count == 0,
                                  "Should create empty vector") && result;

        d_vector_free(vec);
    }

    // test 3: NULL fill value with non-zero count should return NULL
    vec    = d_vector_new_fill(sizeof(int), 5, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_null_value",
                              vec == NULL,
                              "NULL fill value with non-zero count should return NULL") && result;

    // test 4: successful fill creation
    fill_value = 777;
    vec        = d_vector_new_fill(sizeof(int), 5, &fill_value);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_valid_success",
                                  vec != NULL,
                                  "d_vector_new_fill with valid params should succeed") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_valid_count",
                                  vec->count == 5,
                                  "count should be 5") && result;

        // verify all elements match fill value
        {
            int*   arr       = (int*)vec->elements;
            bool   all_match = true;
            size_t i;

            for (i = 0; i < 5; i++)
            {
                if (arr[i] != fill_value)
                {
                    all_match = false;

                    break;
                }
            }

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_fill_all_elements_match",
                                      all_match,
                                      "All elements should match fill value (777)") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_vector_constructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_constructor_all");

    result = true;

    result = d_tests_vector_new(_suite) && result;
    result = d_tests_vector_new_default(_suite) && result;
    result = d_tests_vector_new_from_array(_suite) && result;
    result = d_tests_vector_new_from_args(_suite) && result;
    result = d_tests_vector_new_copy(_suite) && result;
    result = d_tests_vector_new_fill(_suite) && result;

    return result;
}
