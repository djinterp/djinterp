/******************************************************************************
* djinterp [test]                                ptr_vector_tests_sa_constructor.c
*
*   Unit tests for ptr_vector constructor functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_constructor.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"


/******************************************************************************
 * HELPER DATA FOR TESTS
 *****************************************************************************/

static int g_test_values[] = {100, 200, 300, 400, 500};


/*
d_tests_sa_ptr_vector_new
  Tests the d_ptr_vector_new function for creating vectors with specified
  initial capacity.
*/
bool
d_tests_sa_ptr_vector_new
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: zero capacity should succeed and create empty vector */
    vec = d_ptr_vector_new(0);
    result = d_assert_standalone(
        vec != NULL,
        "new_zero_capacity_not_null",
        "Zero capacity should return non-NULL vector",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0 && vec->capacity == 0 && vec->elements == NULL,
            "new_zero_capacity_state",
            "Zero-capacity vector: count=0, capacity=0, elements=NULL",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 2: valid capacity should allocate correctly */
    vec = d_ptr_vector_new(10);
    result = d_assert_standalone(
        vec != NULL,
        "new_valid_not_null",
        "Valid capacity should return non-NULL vector",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0 && vec->capacity == 10 && vec->elements != NULL,
            "new_valid_state",
            "Valid vector: count=0, capacity=10, elements allocated",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: large capacity should allocate correctly */
    vec = d_ptr_vector_new(1000);
    result = d_assert_standalone(
        vec != NULL && vec->capacity == 1000,
        "new_large_capacity",
        "Large capacity (1000) should be set correctly",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_default
  Tests the d_ptr_vector_new_default function.
*/
bool
d_tests_sa_ptr_vector_new_default
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    vec = d_ptr_vector_new_default();
    result = d_assert_standalone(
        vec != NULL,
        "new_default_not_null",
        "Default constructor should return non-NULL vector",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0,
            "new_default_count_zero",
            "Count should be 0 for newly created vector",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity > 0,
            "new_default_has_capacity",
            "Default vector should have non-zero capacity",
            _counter) && result;

        result = d_assert_standalone(
            vec->elements != NULL,
            "new_default_elements_allocated",
            "Elements array should be allocated",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_from_array
  Tests the d_ptr_vector_new_from_array function.
*/
bool
d_tests_sa_ptr_vector_new_from_array
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                source[3];

    result = true;

    source[0] = &g_test_values[0];
    source[1] = &g_test_values[1];
    source[2] = &g_test_values[2];

    /* test 1: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new_from_array(NULL, 5);
    result = d_assert_standalone(
        vec == NULL,
        "new_from_array_null_source_nonzero",
        "NULL source with non-zero count should return NULL",
        _counter) && result;

    /* test 2: NULL source with zero count should succeed */
    vec = d_ptr_vector_new_from_array(NULL, 0);
    result = d_assert_standalone(
        vec != NULL && vec->count == 0,
        "new_from_array_null_source_zero",
        "NULL source with zero count should succeed with count=0",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 3: valid source with zero count */
    vec = d_ptr_vector_new_from_array((const void**)source, 0);
    result = d_assert_standalone(
        vec != NULL && vec->count == 0,
        "new_from_array_zero_count",
        "Zero count should return valid empty vector",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 4: successful creation from valid array */
    vec = d_ptr_vector_new_from_array((const void**)source, 3);
    result = d_assert_standalone(
        vec != NULL,
        "new_from_array_valid_not_null",
        "Valid array should create non-NULL vector",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 3 && vec->capacity >= 3,
            "new_from_array_valid_count_capacity",
            "Count should be 3, capacity >= 3",
            _counter) && result;

        result = d_assert_standalone(
            vec->elements[0] == source[0] &&
            vec->elements[1] == source[1] &&
            vec->elements[2] == source[2],
            "new_from_array_elements_copied",
            "Elements should match source array",
            _counter) && result;

        result = d_assert_standalone(
            *(int*)vec->elements[0] == 100,
            "new_from_array_pointed_value",
            "Pointed-to value should be 100",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_from_args
  Tests the d_ptr_vector_new_from_args function.
*/
bool
d_tests_sa_ptr_vector_new_from_args
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: zero arg count should create empty vector */
    vec = d_ptr_vector_new_from_args(0);
    result = d_assert_standalone(
        vec != NULL && vec->count == 0,
        "new_from_args_zero",
        "Zero args should return valid empty vector",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 2: successful creation with variadic arguments */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_test_values[0],
                                     &g_test_values[1],
                                     &g_test_values[2]);
    result = d_assert_standalone(
        vec != NULL && vec->count == 3,
        "new_from_args_valid",
        "Variadic args should create vector with count=3",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->elements[0] == &g_test_values[0] &&
            vec->elements[1] == &g_test_values[1] &&
            vec->elements[2] == &g_test_values[2],
            "new_from_args_elements",
            "Elements should match variadic args",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: single argument */
    vec = d_ptr_vector_new_from_args(1, &g_test_values[4]);
    result = d_assert_standalone(
        vec != NULL && vec->count == 1 && *(int*)vec->elements[0] == 500,
        "new_from_args_single",
        "Single arg should create vector with one element (500)",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_copy
  Tests the d_ptr_vector_new_copy function.
*/
bool
d_tests_sa_ptr_vector_new_copy
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* original;
    struct d_ptr_vector* copy;

    result = true;

    /* test 1: NULL source should fail */
    copy = d_ptr_vector_new_copy(NULL);
    result = d_assert_standalone(
        copy == NULL,
        "new_copy_null_source",
        "NULL source should return NULL",
        _counter) && result;

    /* test 2: copy empty vector */
    original = d_ptr_vector_new(5);
    if (original)
    {
        copy = d_ptr_vector_new_copy(original);
        result = d_assert_standalone(
            copy != NULL && copy->count == 0,
            "new_copy_empty",
            "Copy of empty vector should succeed with count=0",
            _counter) && result;

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
        result = d_assert_standalone(
            copy != NULL && copy->count == original->count,
            "new_copy_valid",
            "Copy should have same count as original",
            _counter) && result;

        if (copy)
        {
            /* Verify shallow copy - pointers should be identical */
            result = d_assert_standalone(
                copy->elements[0] == original->elements[0] &&
                copy->elements[1] == original->elements[1] &&
                copy->elements[2] == original->elements[2],
                "new_copy_shallow",
                "Shallow copy: element pointers should be same",
                _counter) && result;

            /* But the elements arrays should be different */
            result = d_assert_standalone(
                copy->elements != original->elements,
                "new_copy_different_arrays",
                "Copy should have different elements array",
                _counter) && result;

            d_ptr_vector_free(copy);
        }

        d_ptr_vector_free(original);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_fill
  Tests the d_ptr_vector_new_fill function.
*/
bool
d_tests_sa_ptr_vector_new_fill
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                fill_ptr;
    size_t               i;
    bool                 all_match;

    result   = true;
    fill_ptr = &g_test_values[3];

    /* test 1: zero count should create empty vector */
    vec = d_ptr_vector_new_fill(0, fill_ptr);
    result = d_assert_standalone(
        vec != NULL && vec->count == 0,
        "new_fill_zero_count",
        "Zero count fill should return empty vector",
        _counter) && result;

    if (vec)
    {
        d_ptr_vector_free(vec);
    }

    /* test 2: NULL fill value should work (fill with NULL) */
    vec = d_ptr_vector_new_fill(5, NULL);
    result = d_assert_standalone(
        vec != NULL && vec->count == 5,
        "new_fill_null_value",
        "NULL fill value should succeed with count=5",
        _counter) && result;

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

        result = d_assert_standalone(
            all_match,
            "new_fill_null_all_null",
            "All elements should be NULL",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: successful fill with valid pointer */
    vec = d_ptr_vector_new_fill(5, fill_ptr);
    result = d_assert_standalone(
        vec != NULL && vec->count == 5 && vec->capacity >= 5,
        "new_fill_valid",
        "Valid fill should succeed with count=5",
        _counter) && result;

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

        result = d_assert_standalone(
            all_match,
            "new_fill_all_match",
            "All elements should match fill pointer",
            _counter) && result;

        result = d_assert_standalone(
            *(int*)vec->elements[0] == 400,
            "new_fill_pointed_value",
            "Pointed-to value should be 400",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_ptr_vector_new_merge
  Tests the d_ptr_vector_new_merge function.
*/
bool
d_tests_sa_ptr_vector_new_merge
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec1;
    struct d_ptr_vector* vec2;
    struct d_ptr_vector* vec3;
    struct d_ptr_vector* merged;

    result = true;

    /* test 1: zero count should return default vector */
    merged = d_ptr_vector_new_merge(0);
    result = d_assert_standalone(
        merged != NULL && merged->count == 0,
        "new_merge_zero_count",
        "Zero count merge should return empty vector",
        _counter) && result;

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
        result = d_assert_standalone(
            merged != NULL && merged->count == 4,
            "new_merge_with_null",
            "Merge with NULL should skip it (count=4)",
            _counter) && result;

        if (merged)
        {
            d_ptr_vector_free(merged);
        }

        /* test 3: successful merge of all three vectors */
        merged = d_ptr_vector_new_merge(3, vec1, vec2, vec3);
        result = d_assert_standalone(
            merged != NULL && merged->count == 5,
            "new_merge_all",
            "Merge of all vectors should have count=5",
            _counter) && result;

        if (merged)
        {
            result = d_assert_standalone(
                merged->elements[0] == &g_test_values[0] &&
                merged->elements[1] == &g_test_values[1] &&
                merged->elements[2] == &g_test_values[2] &&
                merged->elements[3] == &g_test_values[3] &&
                merged->elements[4] == &g_test_values[4],
                "new_merge_order",
                "Merged elements should be in order",
                _counter) && result;

            d_ptr_vector_free(merged);
        }
    }

    if (vec1) d_ptr_vector_free(vec1);
    if (vec2) d_ptr_vector_free(vec2);
    if (vec3) d_ptr_vector_free(vec3);

    return result;
}


/*
d_tests_sa_ptr_vector_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_ptr_vector_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_ptr_vector_new(_counter) && result;
    result = d_tests_sa_ptr_vector_new_default(_counter) && result;
    result = d_tests_sa_ptr_vector_new_from_array(_counter) && result;
    result = d_tests_sa_ptr_vector_new_from_args(_counter) && result;
    result = d_tests_sa_ptr_vector_new_copy(_counter) && result;
    result = d_tests_sa_ptr_vector_new_fill(_counter) && result;
    result = d_tests_sa_ptr_vector_new_merge(_counter) && result;

    return result;
}
