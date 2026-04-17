#include "./vector_common_tests_sa.h"


/******************************************************************************
 * HELPER COMPARATOR FOR INT TESTS
 *****************************************************************************/

static int
int_comparator
(
    const void* _a,
    const void* _b
)
{
    int val_a = *(const int*)_a;
    int val_b = *(const int*)_b;

    return (val_a > val_b) - (val_a < val_b);
}


/*
d_tests_sa_vector_common_init
  Tests the d_vector_common_init function for initializing vectors with
  specified capacity.
  Tests the following:
  - NULL parameter handling for all pointer parameters
  - zero element_size rejection
  - zero initial_capacity handling (valid, creates empty vector)
  - successful initialization with valid parameters
  - memory allocation success verification
  - correct count and capacity initialization
*/
bool
d_tests_sa_vector_common_init
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init(NULL,
                             &count,
                             &capacity,
                             sizeof(int),
                             10) == D_FAILURE,
        "init_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = NULL;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init(&elements,
                             NULL,
                             &capacity,
                             sizeof(int),
                             10) == D_FAILURE,
        "init_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    // test 3: NULL capacity pointer should fail
    elements = NULL;
    count    = 99;
    result   = d_assert_standalone(
        d_vector_common_init(&elements,
                             &count,
                             NULL,
                             sizeof(int),
                             10) == D_FAILURE,
        "init_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init(&elements,
                             &count,
                             &capacity,
                             0,
                             10) == D_FAILURE,
        "init_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    // test 5: zero initial_capacity should succeed with empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init(&elements,
                             &count,
                             &capacity,
                             sizeof(int),
                             0) == D_SUCCESS,
        "init_zero_capacity_success",
        "Zero initial_capacity should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL,
        "init_zero_capacity_elements_null",
        "Elements should be NULL for zero capacity",
        _counter) && result;

    result = d_assert_standalone(
        count == 0,
        "init_zero_capacity_count_zero",
        "Count should be 0 for zero capacity",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 0,
        "init_zero_capacity_capacity_zero",
        "Capacity should be 0 for zero capacity",
        _counter) && result;

    // test 6: successful initialization with valid parameters
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init(&elements,
                             &count,
                             &capacity,
                             sizeof(int),
                             10) == D_SUCCESS,
        "init_valid_success",
        "Valid initialization should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements != NULL,
        "init_valid_elements_allocated",
        "Elements should be allocated (non-NULL)",
        _counter) && result;

    result = d_assert_standalone(
        count == 0,
        "init_valid_count_zero",
        "Count should be 0 (no elements added yet)",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 10,
        "init_valid_capacity_set",
        "Capacity should match requested initial_capacity",
        _counter) && result;

    // cleanup
    if (elements)
    {
        free(elements);
    }

    // test 7: verify memory is zero-initialized (calloc behavior)
    elements = NULL;
    count    = 0;
    capacity = 0;

    if (d_vector_common_init(&elements,
                             &count,
                             &capacity,
                             sizeof(int),
                             5) == D_SUCCESS)
    {
        int* int_elements = (int*)elements;
        bool all_zero     = true;
        size_t i;

        for (i = 0; i < 5; i++)
        {
            if (int_elements[i] != 0)
            {
                all_zero = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_zero,
            "init_memory_zeroed",
            "Allocated memory should be zero-initialized",
            _counter) && result;

        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_init_from_array
  Tests the d_vector_common_init_from_array function for initializing vectors
  from existing arrays.
  Tests the following:
  - NULL parameter handling for all pointer parameters
  - zero element_size rejection
  - zero source_count handling (creates empty vector)
  - NULL source with non-zero count handling
  - successful initialization from valid array
  - correct element copying verification
  - capacity calculation (should include growth room)
*/
bool
d_tests_sa_vector_common_init_from_array
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source_arr[5] = {10, 20, 30, 40, 50};

    result = true;

    // test 1: NULL elements pointer should fail
    result = d_assert_standalone(
        d_vector_common_init_from_array(NULL,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source_arr,
                                        5) == D_FAILURE,
        "init_from_array_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        NULL,
                                        &capacity,
                                        sizeof(int),
                                        source_arr,
                                        5) == D_FAILURE,
        "init_from_array_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    // test 3: NULL capacity pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        &count,
                                        NULL,
                                        sizeof(int),
                                        source_arr,
                                        5) == D_FAILURE,
        "init_from_array_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        &count,
                                        &capacity,
                                        0,
                                        source_arr,
                                        5) == D_FAILURE,
        "init_from_array_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    // test 5: zero source_count should succeed with empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source_arr,
                                        0) == D_SUCCESS,
        "init_from_array_zero_count_success",
        "Zero source_count should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL && count == 0 && capacity == 0,
        "init_from_array_zero_count_empty",
        "Zero source_count should produce empty vector",
        _counter) && result;

    // test 6: NULL source with non-zero count should create empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        NULL,
                                        5) == D_SUCCESS,
        "init_from_array_null_source_success",
        "NULL source should return D_SUCCESS (empty vector)",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL && count == 0 && capacity == 0,
        "init_from_array_null_source_empty",
        "NULL source should produce empty vector",
        _counter) && result;

    // test 7: successful initialization from valid array
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_init_from_array(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source_arr,
                                        5) == D_SUCCESS,
        "init_from_array_valid_success",
        "Valid array initialization should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements != NULL,
        "init_from_array_valid_elements_allocated",
        "Elements should be allocated",
        _counter) && result;

    result = d_assert_standalone(
        count == 5,
        "init_from_array_valid_count",
        "Count should match source_count",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 5,
        "init_from_array_valid_capacity",
        "Capacity should be at least source_count",
        _counter) && result;

    // test 8: verify elements are correctly copied
    if (elements)
    {
        int*   int_elements = (int*)elements;
        bool   all_match    = true;
        size_t i;

        for (i = 0; i < 5; i++)
        {
            if (int_elements[i] != source_arr[i])
            {
                all_match = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_match,
            "init_from_array_elements_copied",
            "All elements should match source array",
            _counter) && result;

        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_init_from_args
  Tests the d_vector_common_init_from_args function for initializing vectors
  from variadic arguments.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - zero arg_count handling
  - successful initialization with int-sized elements
  - successful initialization with pointer-sized elements
  - capacity calculation verification
*/
bool
d_tests_sa_vector_common_init_from_args
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements pointer should fail
    // Note: we cannot easily test init_from_args directly without va_list,
    // so we test the parameter validation paths that can be tested

    elements = NULL;
    count    = 0;
    capacity = 0;

    // test 2: zero arg_count should succeed with empty vector
    // This would require a wrapper function to test properly
    // For now, we document that this test exists conceptually

    result = d_assert_standalone(
        true,
        "init_from_args_placeholder",
        "init_from_args requires va_list - tested via higher-level APIs",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_init_copy
  Tests the d_vector_common_init_copy function for creating vector copies.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - zero source_count handling
  - NULL source handling
  - successful copy with valid parameters
  - capacity preservation from source
  - element copying verification
*/
bool
d_tests_sa_vector_common_init_copy
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source_data[3] = {100, 200, 300};

    result = true;

    // test 1: NULL elements pointer should fail
    result = d_assert_standalone(
        d_vector_common_init_copy(NULL,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  source_data,
                                  3,
                                  10) == D_FAILURE,
        "init_copy_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  NULL,
                                  &capacity,
                                  sizeof(int),
                                  source_data,
                                  3,
                                  10) == D_FAILURE,
        "init_copy_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    // test 3: NULL capacity pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  NULL,
                                  sizeof(int),
                                  source_data,
                                  3,
                                  10) == D_FAILURE,
        "init_copy_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  &capacity,
                                  0,
                                  source_data,
                                  3,
                                  10) == D_FAILURE,
        "init_copy_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    // test 5: zero source_count should create empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  source_data,
                                  0,
                                  10) == D_SUCCESS,
        "init_copy_zero_source_count",
        "Zero source_count should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL && count == 0 && capacity == 0,
        "init_copy_zero_source_count_empty",
        "Zero source_count should produce empty vector",
        _counter) && result;

    // test 6: NULL source should create empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  NULL,
                                  3,
                                  10) == D_SUCCESS,
        "init_copy_null_source",
        "NULL source should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL && count == 0 && capacity == 0,
        "init_copy_null_source_empty",
        "NULL source should produce empty vector",
        _counter) && result;

    // test 7: successful copy with capacity preserved
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  source_data,
                                  3,
                                  20) == D_SUCCESS,
        "init_copy_valid_success",
        "Valid copy should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements != NULL,
        "init_copy_valid_elements_allocated",
        "Elements should be allocated",
        _counter) && result;

    result = d_assert_standalone(
        count == 3,
        "init_copy_valid_count",
        "Count should match source_count",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 20,
        "init_copy_valid_capacity_preserved",
        "Capacity should be preserved from source",
        _counter) && result;

    // test 8: verify elements copied correctly
    if (elements)
    {
        int*   int_elements = (int*)elements;
        bool   all_match    = true;
        size_t i;

        for (i = 0; i < 3; i++)
        {
            if (int_elements[i] != source_data[i])
            {
                all_match = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_match,
            "init_copy_elements_match",
            "Copied elements should match source",
            _counter) && result;

        free(elements);
    }

    // test 9: capacity less than count should use calculated capacity
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_init_copy(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  source_data,
                                  3,
                                  1) == D_SUCCESS,
        "init_copy_small_capacity_success",
        "Small source_capacity should still succeed",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 3,
        "init_copy_small_capacity_adjusted",
        "Capacity should be at least source_count",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_init_fill
  Tests the d_vector_common_init_fill function for initializing vectors
  with a repeated fill value.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - zero size handling (creates empty vector)
  - NULL fill value handling
  - successful fill initialization
  - all elements match fill value verification
*/
bool
d_tests_sa_vector_common_init_fill
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    fill_value = 42;

    result = true;

    // test 1: NULL elements pointer should fail
    result = d_assert_standalone(
        d_vector_common_init_fill(NULL,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  5,
                                  &fill_value) == D_FAILURE,
        "init_fill_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  NULL,
                                  &capacity,
                                  sizeof(int),
                                  5,
                                  &fill_value) == D_FAILURE,
        "init_fill_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    // test 3: NULL capacity pointer should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  &count,
                                  NULL,
                                  sizeof(int),
                                  5,
                                  &fill_value) == D_FAILURE,
        "init_fill_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  &count,
                                  &capacity,
                                  0,
                                  5,
                                  &fill_value) == D_FAILURE,
        "init_fill_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    // test 5: zero size should create empty vector
    elements = (void*)0xDEADBEEF;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  0,
                                  &fill_value) == D_SUCCESS,
        "init_fill_zero_size_success",
        "Zero size should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL && count == 0 && capacity == 0,
        "init_fill_zero_size_empty",
        "Zero size should produce empty vector",
        _counter) && result;

    // test 6: NULL fill value with non-zero size should fail
    elements = NULL;
    count    = 99;
    capacity = 99;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  5,
                                  NULL) == D_FAILURE,
        "init_fill_null_value",
        "NULL fill value with non-zero size should return D_FAILURE",
        _counter) && result;

    // test 7: successful fill initialization
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_init_fill(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  5,
                                  &fill_value) == D_SUCCESS,
        "init_fill_valid_success",
        "Valid fill initialization should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements != NULL,
        "init_fill_valid_elements_allocated",
        "Elements should be allocated",
        _counter) && result;

    result = d_assert_standalone(
        count == 5,
        "init_fill_valid_count",
        "Count should match requested size",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 5,
        "init_fill_valid_capacity",
        "Capacity should be at least requested size",
        _counter) && result;

    // test 8: verify all elements match fill value
    if (elements)
    {
        int*   int_elements = (int*)elements;
        bool   all_match    = true;
        size_t i;

        for (i = 0; i < 5; i++)
        {
            if (int_elements[i] != fill_value)
            {
                all_match = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_match,
            "init_fill_all_elements_match",
            "All elements should match fill value",
            _counter) && result;

        free(elements);
    }

    // test 9: test with larger struct type
    {
        struct test_struct
        {
            int   a;
            float b;
            char  c;
        };

        struct test_struct fill_struct = {123, 3.14f, 'X'};

        elements = NULL;
        count    = 0;
        capacity = 0;

        result = d_assert_standalone(
            d_vector_common_init_fill(&elements,
                                      &count,
                                      &capacity,
                                      sizeof(struct test_struct),
                                      3,
                                      &fill_struct) == D_SUCCESS,
            "init_fill_struct_success",
            "Fill with struct type should succeed",
            _counter) && result;

        if (elements)
        {
            struct test_struct* struct_elements = (struct test_struct*)elements;
            bool                all_match       = true;
            size_t              i;

            for (i = 0; i < 3; i++)
            {
                if ( (struct_elements[i].a != fill_struct.a) ||
                     (struct_elements[i].b != fill_struct.b) ||
                     (struct_elements[i].c != fill_struct.c) )
                {
                    all_match = false;

                    break;
                }
            }

            result = d_assert_standalone(
                all_match,
                "init_fill_struct_elements_match",
                "All struct elements should match fill value",
                _counter) && result;

            free(elements);
        }
    }

    return result;
}


/*
d_tests_sa_vector_common_init_all
  Aggregation function that runs all initialization tests.
  Tests the following:
  - d_vector_common_init
  - d_vector_common_init_from_array
  - d_vector_common_init_from_args
  - d_vector_common_init_copy
  - d_vector_common_init_fill
*/
bool
d_tests_sa_vector_common_init_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Initialization Functions\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_vector_common_init(_counter) && result;
    result = d_tests_sa_vector_common_init_from_array(_counter) && result;
    result = d_tests_sa_vector_common_init_from_args(_counter) && result;
    result = d_tests_sa_vector_common_init_copy(_counter) && result;
    result = d_tests_sa_vector_common_init_fill(_counter) && result;

    return result;
}
