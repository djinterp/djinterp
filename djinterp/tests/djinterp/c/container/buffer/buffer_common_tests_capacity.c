/*******************************************************************************
* djinterp [container]                            buffer_common_tests_capacity.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_alloc
  Tests the d_buffer_common_alloc function.
  Tests the following:
  - zero element_size returns NULL
  - zero capacity returns NULL
  - overflow detection (capacity * element_size > SIZE_MAX)
  - successful allocation returns non-NULL
  - allocated memory is zero-initialized
*/
bool d_tests_buffer_common_alloc(
    struct dj_test_suite* _suite
)
{
    bool  result;
    void* mem;

    d_test_suite_test(_suite, "buffer_common_alloc");

    result = true;

    // test 1: zero element_size should return NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alloc_zero_elem_size",
                              d_buffer_common_alloc(0, 10) == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: zero capacity should return NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alloc_zero_capacity",
                              d_buffer_common_alloc(sizeof(int), 0) == NULL,
                              "Zero capacity should return NULL") && result;

    // test 3: overflow should return NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alloc_overflow",
                              d_buffer_common_alloc(SIZE_MAX, 2) == NULL,
                              "Overflow should return NULL") && result;

    // test 4: successful allocation
    mem = d_buffer_common_alloc(sizeof(int), 16);
    result = D_TEST_CHECK_MSG(_suite,
                              "alloc_success",
                              mem != NULL,
                              "Valid allocation should return non-NULL") && result;

    if (mem)
    {
        // test 5: memory should be zero-initialized
        int* arr = (int*)mem;
        bool all_zero = true;
        size_t i;

        for (i = 0; i < 16; ++i)
        {
            if (arr[i] != 0)
            {
                all_zero = false;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "alloc_zeroed",
                                  all_zero,
                                  "Allocated memory should be zero-initialized") && result;

        d_buffer_common_free_data(mem);
    }

    return result;
}


/*
d_tests_buffer_common_calc_growth
  Tests the d_buffer_common_calc_growth function.
  Tests the following:
  - zero current capacity uses default
  - growth by factor produces larger capacity
  - result is at least the required capacity
  - small required returns factor-based growth
  - large required returns required directly
*/
bool d_tests_buffer_common_calc_growth(
    struct dj_test_suite* _suite
)
{
    bool   result;
    size_t grown;

    d_test_suite_test(_suite, "buffer_common_calc_growth");

    result = true;

    // test 1: zero current capacity should use default
    grown  = d_buffer_common_calc_growth(0, 1);
    result = D_TEST_CHECK_MSG(_suite,
                              "calc_growth_zero_current",
                              grown >= D_BUFFER_DEFAULT_CAPACITY,
                              "Zero current should use default capacity") && result;

    // test 2: growth factor applied
    grown  = d_buffer_common_calc_growth(100, 101);
    result = D_TEST_CHECK_MSG(_suite,
                              "calc_growth_factor_applied",
                              grown >= 101,
                              "Grown capacity should be >= required") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "calc_growth_exceeds_current",
                              grown > 100,
                              "Grown capacity should exceed current") && result;

    // test 3: large required returns at least required
    grown  = d_buffer_common_calc_growth(100, 10000);
    result = D_TEST_CHECK_MSG(_suite,
                              "calc_growth_large_required",
                              grown >= 10000,
                              "Should return at least the large required capacity") && result;

    // test 4: small required returns factor-based growth
    grown  = d_buffer_common_calc_growth(100, 50);
    result = D_TEST_CHECK_MSG(_suite,
                              "calc_growth_small_required",
                              grown >= 100,
                              "Factor-based growth should exceed small required") && result;

    return result;
}


/*
d_tests_buffer_common_ensure_capacity
  Tests the d_buffer_common_ensure_capacity function.
  Tests the following:
  - NULL elements pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - already sufficient capacity is a no-op
  - successful growth when required exceeds current
  - capacity updated after growth
*/
bool d_tests_buffer_common_ensure_capacity(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_ensure_capacity");

    result = true;

    // test 1: NULL elements should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_cap_null_elements",
                                d_buffer_common_ensure_capacity(NULL, &capacity, sizeof(int), 20) == false,
                                "NULL elements should return false") && result;

    // test 2: NULL capacity should fail
    elements = NULL;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_cap_null_capacity",
                                d_buffer_common_ensure_capacity(&elements, NULL, sizeof(int), 20) == false,
                                "NULL capacity should return false") && result;

    // test 3: zero element_size should fail
    elements = NULL;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_cap_zero_elem_size",
                                d_buffer_common_ensure_capacity(&elements, &capacity, 0, 20) == false,
                                "Zero element_size should return false") && result;

    // test 4: sufficient capacity is a no-op
    elements = d_buffer_common_alloc(sizeof(int), 32);

    if (elements)
    {
        capacity = 32;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_sufficient",
                                  d_buffer_common_ensure_capacity(&elements, &capacity, sizeof(int), 16) == true,
                                  "Sufficient capacity should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_sufficient_unchanged",
                                  capacity == 32,
                                  "Capacity should remain 32") && result;

        // test 5: growth when required exceeds current
        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_grow_success",
                                  d_buffer_common_ensure_capacity(&elements, &capacity, sizeof(int), 100) == true,
                                  "Growth should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "ensure_cap_grow_value",
                                  capacity >= 100,
                                  "Capacity should be >= 100 after growth") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_resize_to_fit
  Tests the d_buffer_common_resize_to_fit function.
  Tests the following:
  - NULL elements pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - zero count frees and nullifies elements
  - already exact capacity is a no-op
  - successful shrink to actual count
*/
bool d_tests_buffer_common_resize_to_fit(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t capacity;
    size_t count;

    d_test_suite_test(_suite, "buffer_common_resize_to_fit");

    result = true;

    // test 1: NULL elements should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_to_fit_null_elements",
                                d_buffer_common_resize_to_fit(NULL, &capacity, sizeof(int), 5) == false,
                                "NULL elements should return false") && result;

    // test 2: NULL capacity should fail
    elements = NULL;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_to_fit_null_capacity",
                                d_buffer_common_resize_to_fit(&elements, NULL, sizeof(int), 5) == false,
                                "NULL capacity should return false") && result;

    // test 3: zero element_size should fail
    elements = NULL;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_to_fit_zero_elem_size",
                                d_buffer_common_resize_to_fit(&elements, &capacity, 0, 5) == false,
                                "Zero element_size should return false") && result;

    // test 4: zero count frees allocation
    count    = 0;
    capacity = 0;
    elements = NULL;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 64);

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_fit_zero_count",
                                  d_buffer_common_resize_to_fit(&elements, &capacity, sizeof(int), 0) == true,
                                  "Zero count should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_fit_zero_freed",
                                  elements == NULL && capacity == 0,
                                  "Elements should be NULL, capacity 0") && result;
    }

    // test 5: shrink to actual count
    count    = 0;
    capacity = 0;
    elements = NULL;

    {
        int source[3] = {1, 2, 3};

        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 3, 500);
    }

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_fit_precondition",
                                  capacity > 3,
                                  "Capacity should be > 3 before shrink") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_fit_shrink_success",
                                  d_buffer_common_resize_to_fit(&elements, &capacity, sizeof(int), 3) == true,
                                  "Shrink to fit should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_to_fit_shrink_value",
                                  capacity == 3,
                                  "Capacity should equal count (3)") && result;

        // verify data integrity
        if (elements)
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 3);

            result = D_TEST_CHECK_MSG(_suite,
                                      "resize_to_fit_data_intact",
                                      correct,
                                      "Data should be preserved after shrink") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool d_tests_buffer_common_capacity_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_capacity_all");

    result = true;

    result = d_tests_buffer_common_alloc(_suite) && result;
    result = d_tests_buffer_common_calc_growth(_suite) && result;
    result = d_tests_buffer_common_ensure_capacity(_suite) && result;
    result = d_tests_buffer_common_resize_to_fit(_suite) && result;

    return result;
}
