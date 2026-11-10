/*******************************************************************************
* djinterp [test]                                 ptr_vector_tests_constructor.c
*
*   Unit tests for ptr_vector constructor functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_constructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER DATA FOR TESTS
 *****************************************************************************/

static int g_test_values[] = {100, 200, 300, 400, 500};


/*
d_tests_ptr_vector_new
  Tests the d_ptr_vector_new function for creating vectors with specified
  initial capacity.
*/
bool d_tests_ptr_vector_new(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_new");

    result = true;

    /* test 1: zero capacity should succeed and create empty vector */
    vec = d_ptr_vector_new(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_capacity_not_null",
                              vec != NULL,
                              "Zero capacity should return non-NULL vector") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_zero_capacity_state",
                                  vec->count == 0 && vec->capacity == 0 && vec->elements == NULL,
                                  "Zero-capacity vector: count=0, capacity=0, elements=NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 2: valid capacity should allocate correctly */
    vec = d_ptr_vector_new(10);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_valid_not_null",
                              vec != NULL,
                              "Valid capacity should return non-NULL vector") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_valid_state",
                                  vec->count == 0 && vec->capacity == 10 && vec->elements != NULL,
                                  "Valid vector: count=0, capacity=10, elements allocated") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: large capacity should allocate correctly */
    vec = d_ptr_vector_new(1000);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_large_capacity",
                              vec != NULL && vec->capacity == 1000,
                              "Large capacity (1000) should be set correctly") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_ptr_vector_new_default
  Tests the d_ptr_vector_new_default function.
*/
bool d_tests_ptr_vector_new_default(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_new_default");

    result = true;

    vec = d_ptr_vector_new_default();
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_not_null",
                              vec != NULL,
                              "Default constructor should return non-NULL vector") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_count_zero",
                                  vec->count == 0,
                                  "Count should be 0 for newly created vector") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_has_capacity",
                                  vec->capacity > 0,
                                  "Default vector should have non-zero capacity") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_elements_allocated",
                                  vec->elements != NULL,
                                  "Elements array should be allocated") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_ptr_vector_new_from_array
  Tests the d_ptr_vector_new_from_array function.
*/
bool d_tests_ptr_vector_new_from_array(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                source[3];

    d_test_suite_test(_suite, "ptr_vector_new_from_array");

    result = true;

    source[0] = &g_test_values[0];
    source[1] = &g_test_values[1];
    source[2] = &g_test_values[2];

    /* test 1: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new_from_array(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_null_source_nonzero",
                              vec == NULL,
                              "NULL source with non-zero count should return NULL") && result;

    /* test 2: NULL source with zero count should succeed */
    vec = d_ptr_vector_new_from_array(NULL, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_null_source_zero",
                              vec != NULL && vec->count == 0,
                              "NULL source with zero count should succeed with count=0") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 3: valid source with zero count */
    vec = d_ptr_vector_new_from_array((const void**)source, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_zero_count",
                              vec != NULL && vec->count == 0,
                              "Zero count should return valid empty vector") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 4: successful creation from valid array */
    vec = d_ptr_vector_new_from_array((const void**)source, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_array_valid_not_null",
                              vec != NULL,
                              "Valid array should create non-NULL vector") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_valid_count_capacity",
                                  vec->count == 3 && vec->capacity >= 3,
                                  "Count should be 3, capacity >= 3") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_elements_copied",
                                  vec->elements[0] == source[0] && vec->elements[1] == source[1] && vec->elements[2] == source[2],
                                  "Elements should match source array") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_array_pointed_value",
                                  *(int*)vec->elements[0] == 100,
                                  "Pointed-to value should be 100") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_ptr_vector_new_from_args
  Tests the d_ptr_vector_new_from_args function.
*/
bool d_tests_ptr_vector_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_new_from_args");

    result = true;

    /* test 1: zero arg count should create empty vector */
    vec = d_ptr_vector_new_from_args(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_zero",
                              vec != NULL && vec->count == 0,
                              "Zero args should return valid empty vector") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 2: successful creation with variadic arguments */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_test_values[0],
                                     &g_test_values[1],
                                     &g_test_values[2]);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_valid",
                              vec != NULL && vec->count == 3,
                              "Variadic args should create vector with count=3") && result;

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_from_args_elements",
                                  vec->elements[0] == &g_test_values[0] && vec->elements[1] == &g_test_values[1] && vec->elements[2] == &g_test_values[2],
                                  "Elements should match variadic args") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: single argument */
    vec = d_ptr_vector_new_from_args(1, &g_test_values[4]);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_from_args_single",
                              vec != NULL && vec->count == 1 && *(int*)vec->elements[0] == 500,
                              "Single arg should create vector with one element (500)") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_ptr_vector_new_copy
  Tests the d_ptr_vector_new_copy function.
*/
bool d_tests_ptr_vector_new_copy(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* original;
    struct dj_ptr_vector* copy;

    d_test_suite_test(_suite, "ptr_vector_new_copy");

    result = true;

    /* test 1: NULL source should fail */
    copy = d_ptr_vector_new_copy(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_null_source",
                              copy == NULL,
                              "NULL source should return NULL") && result;

    /* test 2: copy empty vector */
    original = d_ptr_vector_new(5);
    if (original)
    {
        copy = d_ptr_vector_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_empty",
                                  copy != NULL && copy->count == 0,
                                  "Copy of empty vector should succeed with count=0") && result;

        if (copy)
        {
            d_ptr_vector_free(copy);
        }

        d_ptr_vector_free(original);
    }

    /* test 3: copy non-empty vector (shallow copy verification) */
    original = d_ptr_vector_new_from_args(3,
                                          &g_test_values[0],
                                          &g_test_values[1],
                                          &g_test_values[2]);
    if (original)
    {
        copy = d_ptr_vector_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_valid",
                                  copy != NULL && copy->count == original->count,
                                  "Copy should have same count as original") && result;

        if (copy)
        {
            /* Verify shallow copy - pointers should be identical */
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_shallow",
                                      copy->elements[0] == original->elements[0] && copy->elements[1] == original->elements[1] && copy->elements[2] == original->elements[2],
                                      "Shallow copy: element pointers should be same") && result;

            /* But the elements arrays should be different */
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_different_arrays",
                                      copy->elements != original->elements,
                                      "Copy should have different elements array") && result;

            d_ptr_vector_free(copy);
        }

        d_ptr_vector_free(original);
    }

    return result;
}


/*
d_tests_ptr_vector_new_fill
  Tests the d_ptr_vector_new_fill function.
*/
bool d_tests_ptr_vector_new_fill(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                fill_ptr;
    size_t               i;
    bool                 all_match;

    d_test_suite_test(_suite, "ptr_vector_new_fill");

    result   = true;
    fill_ptr = &g_test_values[3];

    /* test 1: zero count should create empty vector */
    vec = d_ptr_vector_new_fill(0, fill_ptr);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_zero_count",
                              vec != NULL && vec->count == 0,
                              "Zero count fill should return empty vector") && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 2: NULL fill value should work (fill with NULL) */
    vec = d_ptr_vector_new_fill(5, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_null_value",
                              vec != NULL && vec->count == 5,
                              "NULL fill value should succeed with count=5") && result;

    if (vec)
    {
        all_match = true;
        for (i = 0; i < vec->count; i++)
        {
            if (vec->elements[i] != NULL)
            {
                all_match = false;
                break;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_null_all_null",
                                  all_match,
                                  "All elements should be NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: successful fill with valid pointer */
    vec = d_ptr_vector_new_fill(5, fill_ptr);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_valid",
                              vec != NULL && vec->count == 5 && vec->capacity >= 5,
                              "Valid fill should succeed with count=5") && result;

    if (vec)
    {
        all_match = true;
        for (i = 0; i < vec->count; i++)
        {
            if (vec->elements[i] != fill_ptr)
            {
                all_match = false;
                break;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_all_match",
                                  all_match,
                                  "All elements should match fill pointer") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_pointed_value",
                                  *(int*)vec->elements[0] == 400,
                                  "Pointed-to value should be 400") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_ptr_vector_new_merge
  Tests the d_ptr_vector_new_merge function.
*/
bool d_tests_ptr_vector_new_merge(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec1;
    struct dj_ptr_vector* vec2;
    struct dj_ptr_vector* vec3;
    struct dj_ptr_vector* merged;

    d_test_suite_test(_suite, "ptr_vector_new_merge");

    result = true;

    /* test 1: zero count should return default vector */
    merged = d_ptr_vector_new_merge(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_merge_zero_count",
                              merged != NULL && merged->count == 0,
                              "Zero count merge should return empty vector") && result;

    if (merged)
    {
        d_ptr_vector_free(merged);
    }

    /* Setup vectors for merge tests */
    vec1 = d_ptr_vector_new_from_args(2, &g_test_values[0], &g_test_values[1]);
    vec2 = d_ptr_vector_new_from_args(2, &g_test_values[2], &g_test_values[3]);
    vec3 = d_ptr_vector_new_from_args(1, &g_test_values[4]);

    if (vec1 && vec2 && vec3)
    {
        /* test 2: merge with NULL vector in arguments */
        merged = d_ptr_vector_new_merge(3, vec1, NULL, vec2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_with_null",
                                  merged != NULL && merged->count == 4,
                                  "Merge with NULL should skip it (count=4)") && result;

        if (merged)
        {
            d_ptr_vector_free(merged);
        }

        /* test 3: successful merge of all three vectors */
        merged = d_ptr_vector_new_merge(3, vec1, vec2, vec3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_merge_all",
                                  merged != NULL && merged->count == 5,
                                  "Merge of all vectors should have count=5") && result;

        if (merged)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_merge_order",
                                      merged->elements[0] == &g_test_values[0] && merged->elements[1] == &g_test_values[1] && merged->elements[2] == &g_test_values[2] && merged->elements[3] == &g_test_values[3] && merged->elements[4] == &g_test_values[4],
                                      "Merged elements should be in order") && result;

            d_ptr_vector_free(merged);
        }
    }

    if (vec1) d_ptr_vector_free(vec1);
    if (vec2) d_ptr_vector_free(vec2);
    if (vec3) d_ptr_vector_free(vec3);

    return result;
}


/*
d_tests_ptr_vector_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_ptr_vector_constructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_constructor_all");

    result = true;

    result = d_tests_ptr_vector_new(_suite) && result;
    result = d_tests_ptr_vector_new_default(_suite) && result;
    result = d_tests_ptr_vector_new_from_array(_suite) && result;
    result = d_tests_ptr_vector_new_from_args(_suite) && result;
    result = d_tests_ptr_vector_new_copy(_suite) && result;
    result = d_tests_ptr_vector_new_fill(_suite) && result;
    result = d_tests_ptr_vector_new_merge(_suite) && result;

    return result;
}
