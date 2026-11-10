/*******************************************************************************
* djinterp [container]                            vector_common_tests_capacity.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_reserve
  Tests the d_vector_common_reserve function for reserving storage capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when new capacity <= current capacity
  - successful reallocation when growing
  - capacity correctly updated after reservation
  - existing elements preserved after reallocation
*/
bool d_tests_vector_common_reserve(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_reserve");

    result = true;

    // test 1: NULL elements pointer should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "reserve_null_elements",
                                d_vector_common_reserve(NULL, 5, &capacity, sizeof(int), 20) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "reserve_null_capacity",
                                d_vector_common_reserve(&elements, 5, NULL, sizeof(int), 20) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "reserve_zero_element_size",
                                d_vector_common_reserve(&elements, 5, &capacity, 0, 20) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 4: new capacity <= current capacity should be no-op success
    elements = malloc(20 * sizeof(int));
    capacity = 20;
    result   = D_TEST_CHECK_MSG(_suite,
                                "reserve_no_op_success",
                                d_vector_common_reserve(&elements, 5, &capacity, sizeof(int), 10) == D_SUCCESS,
                                "Smaller capacity should return D_SUCCESS (no-op)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_no_op_capacity_unchanged",
                              capacity == 20,
                              "Capacity should remain unchanged for smaller request") && result;

    free(elements);

    // test 5: successful reservation with larger capacity
    elements = NULL;
    count    = 0;
    capacity = 0;

    // initialize first
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 5);

    // add some elements to verify preservation
    {
        int* int_elements = (int*)elements;
        int_elements[0] = 100;
        int_elements[1] = 200;
        int_elements[2] = 300;
        count = 3;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_grow_success",
                              d_vector_common_reserve(&elements, count, &capacity, sizeof(int), 50) == D_SUCCESS,
                              "Growing capacity should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_grow_capacity_updated",
                              capacity == 50,
                              "Capacity should be updated to new value") && result;

    // test 6: verify existing elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 100) &&
                            (int_elements[1] == 200) &&
                            (int_elements[2] == 300);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_elements_preserved",
                                  preserved,
                                  "Existing elements should be preserved after reserve") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_shrink_to_fit
  Tests the d_vector_common_shrink_to_fit function for reducing capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when capacity equals count
  - empty vector handling (count == 0)
  - successful shrink for non-empty vector
  - elements preserved after shrink
*/
bool d_tests_vector_common_shrink_to_fit(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_shrink_to_fit");

    result = true;

    // test 1: NULL elements pointer should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "shrink_null_elements",
                                d_vector_common_shrink_to_fit(NULL, 5, &capacity, sizeof(int)) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "shrink_null_capacity",
                                d_vector_common_shrink_to_fit(&elements, 5, NULL, sizeof(int)) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "shrink_zero_element_size",
                                d_vector_common_shrink_to_fit(&elements, 5, &capacity, 0) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 4: no-op when capacity equals count
    elements = malloc(5 * sizeof(int));
    capacity = 5;
    result   = D_TEST_CHECK_MSG(_suite,
                                "shrink_equal_no_op",
                                d_vector_common_shrink_to_fit(&elements, 5, &capacity, sizeof(int)) == D_SUCCESS,
                                "Equal capacity and count should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_equal_capacity_unchanged",
                              capacity == 5,
                              "Capacity should remain unchanged when equal to count") && result;

    free(elements);

    // test 5: empty vector (count == 0) should free and nullify
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "shrink_empty_success",
                                d_vector_common_shrink_to_fit(&elements, 0, &capacity, sizeof(int)) == D_SUCCESS,
                                "Shrinking empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_empty_elements_null",
                              elements == NULL,
                              "Elements should be NULL after shrinking empty vector") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_empty_capacity_zero",
                              capacity == 0,
                              "Capacity should be 0 after shrinking empty vector") && result;

    // test 6: successful shrink for non-empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 50);

    // add some elements
    {
        int* int_elements = (int*)elements;
        int_elements[0] = 111;
        int_elements[1] = 222;
        int_elements[2] = 333;
        count = 3;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_valid_success",
                              d_vector_common_shrink_to_fit(&elements, count, &capacity, sizeof(int)) == D_SUCCESS,
                              "Shrinking valid vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_valid_capacity_reduced",
                              capacity == 3,
                              "Capacity should be reduced to count") && result;

    // test 7: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 111) &&
                            (int_elements[1] == 222) &&
                            (int_elements[2] == 333);

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_elements_preserved",
                                  preserved,
                                  "Elements should be preserved after shrink") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_ensure_capacity
  Tests the d_vector_common_ensure_capacity function for ensuring minimum
  capacity exists.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when sufficient capacity exists
  - growth with proper growth factor
  - capacity never less than required
*/
bool d_tests_vector_common_ensure_capacity(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_ensure_capacity");

    result = true;

    // test 1: NULL elements pointer should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_null_elements",
                                d_vector_common_ensure_capacity(NULL, 5, &capacity, sizeof(int), 20) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_null_capacity",
                                d_vector_common_ensure_capacity(&elements, 5, NULL, sizeof(int), 20) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_zero_element_size",
                                d_vector_common_ensure_capacity(&elements, 5, &capacity, 0, 20) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 4: no-op when sufficient capacity exists
    elements = malloc(20 * sizeof(int));
    capacity = 20;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_sufficient_no_op",
                                d_vector_common_ensure_capacity(&elements, 5, &capacity, sizeof(int), 10) == D_SUCCESS,
                                "Sufficient capacity should return D_SUCCESS (no-op)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_sufficient_unchanged",
                              capacity == 20,
                              "Capacity should remain unchanged when sufficient") && result;

    free(elements);

    // test 5: growth from zero capacity
    elements = NULL;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "ensure_from_zero_success",
                                d_vector_common_ensure_capacity(&elements, 0, &capacity, sizeof(int), 10) == D_SUCCESS,
                                "Growing from zero capacity should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_from_zero_meets_required",
                              capacity >= 10,
                              "Capacity should be at least required amount") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_from_zero_allocated",
                              elements != NULL,
                              "Elements should be allocated") && result;

    if (elements)
    {
        free(elements);
    }

    // test 6: growth applies growth factor
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_growth_factor_success",
                              d_vector_common_ensure_capacity(&elements, count, &capacity, sizeof(int), 15) == D_SUCCESS,
                              "Growth with factor should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_growth_factor_meets_required",
                              capacity >= 15,
                              "Capacity should meet required amount") && result;

    // growth factor is 2.0, so 10 * 2 = 20 >= 15
    result = D_TEST_CHECK_MSG(_suite,
                              "ensure_growth_factor_applied",
                              capacity >= 20,
                              "Growth factor should be applied (capacity >= 20)") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_grow
  Tests the d_vector_common_grow function for doubling capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - growth from zero capacity (uses default)
  - growth with growth factor applied
  - existing elements preserved
*/
bool d_tests_vector_common_grow(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_grow");

    result = true;

    // test 1: NULL elements pointer should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "grow_null_elements",
                                d_vector_common_grow(NULL, 5, &capacity, sizeof(int)) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "grow_null_capacity",
                                d_vector_common_grow(&elements, 5, NULL, sizeof(int)) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "grow_zero_element_size",
                                d_vector_common_grow(&elements, 5, &capacity, 0) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 4: growth from zero capacity uses default
    elements = NULL;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "grow_from_zero_success",
                                d_vector_common_grow(&elements, 0, &capacity, sizeof(int)) == D_SUCCESS,
                                "Growing from zero capacity should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "grow_from_zero_default",
                              capacity == D_VECTOR_DEFAULT_CAPACITY,
                              "Zero capacity should grow to default capacity") && result;

    if (elements)
    {
        free(elements);
    }

    // test 5: growth with growth factor applied
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 16);

    // add elements
    {
        int* int_elements = (int*)elements;
        int_elements[0] = 1;
        int_elements[1] = 2;
        int_elements[2] = 3;
        count = 3;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "grow_factor_success",
                              d_vector_common_grow(&elements, count, &capacity, sizeof(int)) == D_SUCCESS,
                              "Growing with factor should succeed") && result;

    // growth factor is 2.0, so 16 * 2 = 32
    result = D_TEST_CHECK_MSG(_suite,
                              "grow_factor_doubled",
                              capacity == 32,
                              "Capacity should be doubled (16 -> 32)") && result;

    // test 6: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 1) &&
                            (int_elements[1] == 2) &&
                            (int_elements[2] == 3);

        result = D_TEST_CHECK_MSG(_suite,
                                  "grow_elements_preserved",
                                  preserved,
                                  "Elements should be preserved after growth") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_maybe_shrink
  Tests the d_vector_common_maybe_shrink function for conditional shrinking.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no shrink when capacity at or below minimum
  - no shrink when usage above threshold
  - successful shrink when usage below threshold
  - elements preserved after shrink
*/
bool d_tests_vector_common_maybe_shrink(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_maybe_shrink");

    result = true;

    // test 1: NULL elements pointer should fail
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "maybe_shrink_null_elements",
                                d_vector_common_maybe_shrink(NULL, 1, &capacity, sizeof(int)) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "maybe_shrink_null_capacity",
                                d_vector_common_maybe_shrink(&elements, 1, NULL, sizeof(int)) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "maybe_shrink_zero_element_size",
                                d_vector_common_maybe_shrink(&elements, 1, &capacity, 0) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 4: no shrink when capacity at minimum
    elements = malloc(D_VECTOR_MIN_CAPACITY * sizeof(int));
    capacity = D_VECTOR_MIN_CAPACITY;
    count    = 1;
    result   = D_TEST_CHECK_MSG(_suite,
                                "maybe_shrink_at_min_success",
                                d_vector_common_maybe_shrink(&elements, count, &capacity, sizeof(int)) == D_SUCCESS,
                                "At minimum capacity should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "maybe_shrink_at_min_unchanged",
                              capacity == D_VECTOR_MIN_CAPACITY,
                              "Capacity should remain at minimum") && result;

    free(elements);

    // test 5: no shrink when usage above threshold (25%)
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 100);
    count = 30;  // 30% usage, above 25% threshold

    result = D_TEST_CHECK_MSG(_suite,
                              "maybe_shrink_above_threshold_success",
                              d_vector_common_maybe_shrink(&elements, count, &capacity, sizeof(int)) == D_SUCCESS,
                              "Above threshold should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "maybe_shrink_above_threshold_unchanged",
                              capacity == 100,
                              "Capacity should remain unchanged above threshold") && result;

    if (elements)
    {
        free(elements);
    }

    // test 6: shrink when usage below threshold
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 100);

    // add a few elements
    {
        int* int_elements = (int*)elements;
        int_elements[0] = 10;
        int_elements[1] = 20;
        count = 2;  // 2% usage, well below 25% threshold
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "maybe_shrink_below_threshold_success",
                              d_vector_common_maybe_shrink(&elements, count, &capacity, sizeof(int)) == D_SUCCESS,
                              "Below threshold should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "maybe_shrink_below_threshold_reduced",
                              capacity < 100,
                              "Capacity should be reduced below threshold") && result;

    // test 7: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 10) &&
                            (int_elements[1] == 20);

        result = D_TEST_CHECK_MSG(_suite,
                                  "maybe_shrink_elements_preserved",
                                  preserved,
                                  "Elements should be preserved after shrink") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_available
  Tests the d_vector_common_available function for computing available slots.
  Tests the following:
  - empty vector (full capacity available)
  - partially filled vector
  - full vector (no available slots)
  - count exceeds capacity edge case
*/
bool d_tests_vector_common_available(
    struct dj_test_suite* _suite
)
{
    bool   result;
    size_t available;

    d_test_suite_test(_suite, "vector_common_available");

    result = true;

    // test 1: empty vector has full capacity available
    available = d_vector_common_available(0, 10);
    result    = D_TEST_CHECK_MSG(_suite,
                                 "available_empty",
                                 available == 10,
                                 "Empty vector should have full capacity available") && result;

    // test 2: partially filled vector
    available = d_vector_common_available(3, 10);
    result    = D_TEST_CHECK_MSG(_suite,
                                 "available_partial",
                                 available == 7,
                                 "Partially filled vector should have (capacity - count) available") && result;

    // test 3: full vector has no available slots
    available = d_vector_common_available(10, 10);
    result    = D_TEST_CHECK_MSG(_suite,
                                 "available_full",
                                 available == 0,
                                 "Full vector should have 0 available slots") && result;

    // test 4: count exceeds capacity (edge case, returns 0)
    available = d_vector_common_available(15, 10);
    result    = D_TEST_CHECK_MSG(_suite,
                                 "available_overflow",
                                 available == 0,
                                 "Count > capacity should return 0") && result;

    // test 5: both zero
    available = d_vector_common_available(0, 0);
    result    = D_TEST_CHECK_MSG(_suite,
                                 "available_both_zero",
                                 available == 0,
                                 "Both zero should return 0") && result;

    return result;
}


/*
d_tests_vector_common_capacity_all
  Aggregation function that runs all capacity management tests.
  Tests the following:
  - d_vector_common_reserve
  - d_vector_common_shrink_to_fit
  - d_vector_common_ensure_capacity
  - d_vector_common_grow
  - d_vector_common_maybe_shrink
  - d_vector_common_available
*/
bool d_tests_vector_common_capacity_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_capacity_all");

    result = true;

    result = d_tests_vector_common_reserve(_suite) && result;
    result = d_tests_vector_common_shrink_to_fit(_suite) && result;
    result = d_tests_vector_common_ensure_capacity(_suite) && result;
    result = d_tests_vector_common_grow(_suite) && result;
    result = d_tests_vector_common_maybe_shrink(_suite) && result;
    result = d_tests_vector_common_available(_suite) && result;

    return result;
}
