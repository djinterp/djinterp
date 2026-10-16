/*******************************************************************************
* djinterp [container]                         buffer_common_tests_sa_capacity.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_capacity.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_alloc
  Tests the d_buffer_common_alloc function.
  Tests the following:
  - zero element_size returns NULL
  - zero capacity returns NULL
  - overflow detection (capacity * element_size > SIZE_MAX)
  - successful allocation returns non-NULL
  - allocated memory is zero-initialized
*/
bool
d_tests_sa_buffer_common_alloc
(
    struct d_test_counter* _counter
)
{
    bool  result;
    void* mem;

    result = true;

    // test 1: zero element_size should return NULL
    result = d_assert_standalone(
        d_buffer_common_alloc(0, 10) == NULL,
        "alloc_zero_elem_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: zero capacity should return NULL
    result = d_assert_standalone(
        d_buffer_common_alloc(sizeof(int), 0) == NULL,
        "alloc_zero_capacity",
        "Zero capacity should return NULL",
        _counter) && result;

    // test 3: overflow should return NULL
    result = d_assert_standalone(
        d_buffer_common_alloc(SIZE_MAX, 2) == NULL,
        "alloc_overflow",
        "Overflow should return NULL",
        _counter) && result;

    // test 4: successful allocation
    mem = d_buffer_common_alloc(sizeof(int), 16);
    result = d_assert_standalone(
        mem != NULL,
        "alloc_success",
        "Valid allocation should return non-NULL",
        _counter) && result;

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

        result = d_assert_standalone(
            all_zero,
            "alloc_zeroed",
            "Allocated memory should be zero-initialized",
            _counter) && result;

        d_buffer_common_free_data(mem);
    }

    return result;
}


/*
d_tests_sa_buffer_common_calc_growth
  Tests the d_buffer_common_calc_growth function.
  Tests the following:
  - zero current capacity uses default
  - growth by factor produces larger capacity
  - result is at least the required capacity
  - small required returns factor-based growth
  - large required returns required directly
*/
bool
d_tests_sa_buffer_common_calc_growth
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t grown;

    result = true;

    // test 1: zero current capacity should use default
    grown  = d_buffer_common_calc_growth(0, 1);
    result = d_assert_standalone(
        grown >= D_BUFFER_DEFAULT_CAPACITY,
        "calc_growth_zero_current",
        "Zero current should use default capacity",
        _counter) && result;

    // test 2: growth factor applied
    grown  = d_buffer_common_calc_growth(100, 101);
    result = d_assert_standalone(
        grown >= 101,
        "calc_growth_factor_applied",
        "Grown capacity should be >= required",
        _counter) && result;

    result = d_assert_standalone(
        grown > 100,
        "calc_growth_exceeds_current",
        "Grown capacity should exceed current",
        _counter) && result;

    // test 3: large required returns at least required
    grown  = d_buffer_common_calc_growth(100, 10000);
    result = d_assert_standalone(
        grown >= 10000,
        "calc_growth_large_required",
        "Should return at least the large required capacity",
        _counter) && result;

    // test 4: small required returns factor-based growth
    grown  = d_buffer_common_calc_growth(100, 50);
    result = d_assert_standalone(
        grown >= 100,
        "calc_growth_small_required",
        "Factor-based growth should exceed small required",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_ensure_capacity
  Tests the d_buffer_common_ensure_capacity function.
  Tests the following:
  - NULL elements pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - already sufficient capacity is a no-op
  - successful growth when required exceeds current
  - capacity updated after growth
*/
bool
d_tests_sa_buffer_common_ensure_capacity
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t capacity;

    result = true;

    // test 1: NULL elements should fail
    capacity = 10;
    result   = d_assert_standalone(
        d_buffer_common_ensure_capacity(NULL, &capacity,
                                        sizeof(int), 20) == false,
        "ensure_cap_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL capacity should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_buffer_common_ensure_capacity(&elements, NULL,
                                        sizeof(int), 20) == false,
        "ensure_cap_null_capacity",
        "NULL capacity should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    elements = NULL;
    capacity = 10;
    result   = d_assert_standalone(
        d_buffer_common_ensure_capacity(&elements, &capacity,
                                        0, 20) == false,
        "ensure_cap_zero_elem_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 4: sufficient capacity is a no-op
    elements = d_buffer_common_alloc(sizeof(int), 32);

    if (elements)
    {
        capacity = 32;

        result = d_assert_standalone(
            d_buffer_common_ensure_capacity(&elements, &capacity,
                                            sizeof(int), 16) == true,
            "ensure_cap_sufficient",
            "Sufficient capacity should succeed",
            _counter) && result;

        result = d_assert_standalone(
            capacity == 32,
            "ensure_cap_sufficient_unchanged",
            "Capacity should remain 32",
            _counter) && result;

        // test 5: growth when required exceeds current
        result = d_assert_standalone(
            d_buffer_common_ensure_capacity(&elements, &capacity,
                                            sizeof(int), 100) == true,
            "ensure_cap_grow_success",
            "Growth should succeed",
            _counter) && result;

        result = d_assert_standalone(
            capacity >= 100,
            "ensure_cap_grow_value",
            "Capacity should be >= 100 after growth",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_resize_to_fit
  Tests the d_buffer_common_resize_to_fit function.
  Tests the following:
  - NULL elements pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - zero count frees and nullifies elements
  - already exact capacity is a no-op
  - successful shrink to actual count
*/
bool
d_tests_sa_buffer_common_resize_to_fit
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t capacity;
    size_t count;

    result = true;

    // test 1: NULL elements should fail
    capacity = 10;
    result   = d_assert_standalone(
        d_buffer_common_resize_to_fit(NULL, &capacity,
                                      sizeof(int), 5) == false,
        "resize_to_fit_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL capacity should fail
    elements = NULL;
    result   = d_assert_standalone(
        d_buffer_common_resize_to_fit(&elements, NULL,
                                      sizeof(int), 5) == false,
        "resize_to_fit_null_capacity",
        "NULL capacity should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    elements = NULL;
    capacity = 10;
    result   = d_assert_standalone(
        d_buffer_common_resize_to_fit(&elements, &capacity,
                                      0, 5) == false,
        "resize_to_fit_zero_elem_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 4: zero count frees allocation
    count    = 0;
    capacity = 0;
    elements = NULL;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 64);

    if (elements)
    {
        result = d_assert_standalone(
            d_buffer_common_resize_to_fit(&elements, &capacity,
                                          sizeof(int), 0) == true,
            "resize_to_fit_zero_count",
            "Zero count should succeed",
            _counter) && result;

        result = d_assert_standalone(
            elements == NULL && capacity == 0,
            "resize_to_fit_zero_freed",
            "Elements should be NULL, capacity 0",
            _counter) && result;
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
        result = d_assert_standalone(
            capacity > 3,
            "resize_to_fit_precondition",
            "Capacity should be > 3 before shrink",
            _counter) && result;

        result = d_assert_standalone(
            d_buffer_common_resize_to_fit(&elements, &capacity,
                                          sizeof(int), 3) == true,
            "resize_to_fit_shrink_success",
            "Shrink to fit should succeed",
            _counter) && result;

        result = d_assert_standalone(
            capacity == 3,
            "resize_to_fit_shrink_value",
            "Capacity should equal count (3)",
            _counter) && result;

        // verify data integrity
        if (elements)
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 3);

            result = d_assert_standalone(
                correct,
                "resize_to_fit_data_intact",
                "Data should be preserved after shrink",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool
d_tests_sa_buffer_common_capacity_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Capacity Management Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_buffer_common_alloc(_counter) && result;
    result = d_tests_sa_buffer_common_calc_growth(_counter) && result;
    result = d_tests_sa_buffer_common_ensure_capacity(_counter) && result;
    result = d_tests_sa_buffer_common_resize_to_fit(_counter) && result;

    return result;
}
