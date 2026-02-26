#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_reserve
  Tests the d_vector_common_reserve function for reserving storage capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when new capacity <= current capacity
  - successful reallocation when growing
  - capacity correctly updated after reservation
  - existing elements preserved after reallocation
*/
bool
d_tests_sa_vector_common_reserve
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
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_reserve(NULL,
                                5,
                                &capacity,
                                sizeof(int),
                                20) == D_FAILURE,
        "reserve_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_reserve(&elements,
                                5,
                                NULL,
                                sizeof(int),
                                20) == D_FAILURE,
        "reserve_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_reserve(&elements,
                                5,
                                &capacity,
                                0,
                                20) == D_FAILURE,
        "reserve_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: new capacity <= current capacity should be no-op success
    elements = malloc(20 * sizeof(int));
    capacity = 20;
    result   = d_assert_standalone(
        d_vector_common_reserve(&elements,
                                5,
                                &capacity,
                                sizeof(int),
                                10) == D_SUCCESS,
        "reserve_no_op_success",
        "Smaller capacity should return D_SUCCESS (no-op)",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 20,
        "reserve_no_op_capacity_unchanged",
        "Capacity should remain unchanged for smaller request",
        _counter) && result;

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

    result = d_assert_standalone(
        d_vector_common_reserve(&elements,
                                count,
                                &capacity,
                                sizeof(int),
                                50) == D_SUCCESS,
        "reserve_grow_success",
        "Growing capacity should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 50,
        "reserve_grow_capacity_updated",
        "Capacity should be updated to new value",
        _counter) && result;

    // test 6: verify existing elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 100) &&
                            (int_elements[1] == 200) &&
                            (int_elements[2] == 300);

        result = d_assert_standalone(
            preserved,
            "reserve_elements_preserved",
            "Existing elements should be preserved after reserve",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_shrink_to_fit
  Tests the d_vector_common_shrink_to_fit function for reducing capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when capacity equals count
  - empty vector handling (count == 0)
  - successful shrink for non-empty vector
  - elements preserved after shrink
*/
bool
d_tests_sa_vector_common_shrink_to_fit
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
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_shrink_to_fit(NULL,
                                      5,
                                      &capacity,
                                      sizeof(int)) == D_FAILURE,
        "shrink_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_shrink_to_fit(&elements,
                                      5,
                                      NULL,
                                      sizeof(int)) == D_FAILURE,
        "shrink_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_shrink_to_fit(&elements,
                                      5,
                                      &capacity,
                                      0) == D_FAILURE,
        "shrink_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: no-op when capacity equals count
    elements = malloc(5 * sizeof(int));
    capacity = 5;
    result   = d_assert_standalone(
        d_vector_common_shrink_to_fit(&elements,
                                      5,
                                      &capacity,
                                      sizeof(int)) == D_SUCCESS,
        "shrink_equal_no_op",
        "Equal capacity and count should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 5,
        "shrink_equal_capacity_unchanged",
        "Capacity should remain unchanged when equal to count",
        _counter) && result;

    free(elements);

    // test 5: empty vector (count == 0) should free and nullify
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_shrink_to_fit(&elements,
                                      0,
                                      &capacity,
                                      sizeof(int)) == D_SUCCESS,
        "shrink_empty_success",
        "Shrinking empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        elements == NULL,
        "shrink_empty_elements_null",
        "Elements should be NULL after shrinking empty vector",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 0,
        "shrink_empty_capacity_zero",
        "Capacity should be 0 after shrinking empty vector",
        _counter) && result;

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

    result = d_assert_standalone(
        d_vector_common_shrink_to_fit(&elements,
                                      count,
                                      &capacity,
                                      sizeof(int)) == D_SUCCESS,
        "shrink_valid_success",
        "Shrinking valid vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 3,
        "shrink_valid_capacity_reduced",
        "Capacity should be reduced to count",
        _counter) && result;

    // test 7: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 111) &&
                            (int_elements[1] == 222) &&
                            (int_elements[2] == 333);

        result = d_assert_standalone(
            preserved,
            "shrink_elements_preserved",
            "Elements should be preserved after shrink",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_ensure_capacity
  Tests the d_vector_common_ensure_capacity function for ensuring minimum
  capacity exists.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no-op when sufficient capacity exists
  - growth with proper growth factor
  - capacity never less than required
*/
bool
d_tests_sa_vector_common_ensure_capacity
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
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_ensure_capacity(NULL,
                                        5,
                                        &capacity,
                                        sizeof(int),
                                        20) == D_FAILURE,
        "ensure_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_ensure_capacity(&elements,
                                        5,
                                        NULL,
                                        sizeof(int),
                                        20) == D_FAILURE,
        "ensure_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_ensure_capacity(&elements,
                                        5,
                                        &capacity,
                                        0,
                                        20) == D_FAILURE,
        "ensure_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: no-op when sufficient capacity exists
    elements = malloc(20 * sizeof(int));
    capacity = 20;
    result   = d_assert_standalone(
        d_vector_common_ensure_capacity(&elements,
                                        5,
                                        &capacity,
                                        sizeof(int),
                                        10) == D_SUCCESS,
        "ensure_sufficient_no_op",
        "Sufficient capacity should return D_SUCCESS (no-op)",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 20,
        "ensure_sufficient_unchanged",
        "Capacity should remain unchanged when sufficient",
        _counter) && result;

    free(elements);

    // test 5: growth from zero capacity
    elements = NULL;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_ensure_capacity(&elements,
                                        0,
                                        &capacity,
                                        sizeof(int),
                                        10) == D_SUCCESS,
        "ensure_from_zero_success",
        "Growing from zero capacity should succeed",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 10,
        "ensure_from_zero_meets_required",
        "Capacity should be at least required amount",
        _counter) && result;

    result = d_assert_standalone(
        elements != NULL,
        "ensure_from_zero_allocated",
        "Elements should be allocated",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    // test 6: growth applies growth factor
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = d_assert_standalone(
        d_vector_common_ensure_capacity(&elements,
                                        count,
                                        &capacity,
                                        sizeof(int),
                                        15) == D_SUCCESS,
        "ensure_growth_factor_success",
        "Growth with factor should succeed",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 15,
        "ensure_growth_factor_meets_required",
        "Capacity should meet required amount",
        _counter) && result;

    // growth factor is 2.0, so 10 * 2 = 20 >= 15
    result = d_assert_standalone(
        capacity >= 20,
        "ensure_growth_factor_applied",
        "Growth factor should be applied (capacity >= 20)",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_grow
  Tests the d_vector_common_grow function for doubling capacity.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - growth from zero capacity (uses default)
  - growth with growth factor applied
  - existing elements preserved
*/
bool
d_tests_sa_vector_common_grow
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
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_grow(NULL,
                             5,
                             &capacity,
                             sizeof(int)) == D_FAILURE,
        "grow_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_grow(&elements,
                             5,
                             NULL,
                             sizeof(int)) == D_FAILURE,
        "grow_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_grow(&elements,
                             5,
                             &capacity,
                             0) == D_FAILURE,
        "grow_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: growth from zero capacity uses default
    elements = NULL;
    capacity = 0;
    result   = d_assert_standalone(
        d_vector_common_grow(&elements,
                             0,
                             &capacity,
                             sizeof(int)) == D_SUCCESS,
        "grow_from_zero_success",
        "Growing from zero capacity should succeed",
        _counter) && result;

    result = d_assert_standalone(
        capacity == D_VECTOR_DEFAULT_CAPACITY,
        "grow_from_zero_default",
        "Zero capacity should grow to default capacity",
        _counter) && result;

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

    result = d_assert_standalone(
        d_vector_common_grow(&elements,
                             count,
                             &capacity,
                             sizeof(int)) == D_SUCCESS,
        "grow_factor_success",
        "Growing with factor should succeed",
        _counter) && result;

    // growth factor is 2.0, so 16 * 2 = 32
    result = d_assert_standalone(
        capacity == 32,
        "grow_factor_doubled",
        "Capacity should be doubled (16 -> 32)",
        _counter) && result;

    // test 6: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 1) &&
                            (int_elements[1] == 2) &&
                            (int_elements[2] == 3);

        result = d_assert_standalone(
            preserved,
            "grow_elements_preserved",
            "Elements should be preserved after growth",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_maybe_shrink
  Tests the d_vector_common_maybe_shrink function for conditional shrinking.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - no shrink when capacity at or below minimum
  - no shrink when usage above threshold
  - successful shrink when usage below threshold
  - elements preserved after shrink
*/
bool
d_tests_sa_vector_common_maybe_shrink
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
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_maybe_shrink(NULL,
                                     1,
                                     &capacity,
                                     sizeof(int)) == D_FAILURE,
        "maybe_shrink_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_maybe_shrink(&elements,
                                     1,
                                     NULL,
                                     sizeof(int)) == D_FAILURE,
        "maybe_shrink_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_maybe_shrink(&elements,
                                     1,
                                     &capacity,
                                     0) == D_FAILURE,
        "maybe_shrink_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: no shrink when capacity at minimum
    elements = malloc(D_VECTOR_MIN_CAPACITY * sizeof(int));
    capacity = D_VECTOR_MIN_CAPACITY;
    count    = 1;
    result   = d_assert_standalone(
        d_vector_common_maybe_shrink(&elements,
                                     count,
                                     &capacity,
                                     sizeof(int)) == D_SUCCESS,
        "maybe_shrink_at_min_success",
        "At minimum capacity should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity == D_VECTOR_MIN_CAPACITY,
        "maybe_shrink_at_min_unchanged",
        "Capacity should remain at minimum",
        _counter) && result;

    free(elements);

    // test 5: no shrink when usage above threshold (25%)
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 100);
    count = 30;  // 30% usage, above 25% threshold

    result = d_assert_standalone(
        d_vector_common_maybe_shrink(&elements,
                                     count,
                                     &capacity,
                                     sizeof(int)) == D_SUCCESS,
        "maybe_shrink_above_threshold_success",
        "Above threshold should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity == 100,
        "maybe_shrink_above_threshold_unchanged",
        "Capacity should remain unchanged above threshold",
        _counter) && result;

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

    result = d_assert_standalone(
        d_vector_common_maybe_shrink(&elements,
                                     count,
                                     &capacity,
                                     sizeof(int)) == D_SUCCESS,
        "maybe_shrink_below_threshold_success",
        "Below threshold should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        capacity < 100,
        "maybe_shrink_below_threshold_reduced",
        "Capacity should be reduced below threshold",
        _counter) && result;

    // test 7: verify elements preserved
    {
        int* int_elements = (int*)elements;
        bool preserved    = (int_elements[0] == 10) &&
                            (int_elements[1] == 20);

        result = d_assert_standalone(
            preserved,
            "maybe_shrink_elements_preserved",
            "Elements should be preserved after shrink",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_available
  Tests the d_vector_common_available function for computing available slots.
  Tests the following:
  - empty vector (full capacity available)
  - partially filled vector
  - full vector (no available slots)
  - count exceeds capacity edge case
*/
bool
d_tests_sa_vector_common_available
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t available;

    result = true;

    // test 1: empty vector has full capacity available
    available = d_vector_common_available(0, 10);
    result    = d_assert_standalone(
        available == 10,
        "available_empty",
        "Empty vector should have full capacity available",
        _counter) && result;

    // test 2: partially filled vector
    available = d_vector_common_available(3, 10);
    result    = d_assert_standalone(
        available == 7,
        "available_partial",
        "Partially filled vector should have (capacity - count) available",
        _counter) && result;

    // test 3: full vector has no available slots
    available = d_vector_common_available(10, 10);
    result    = d_assert_standalone(
        available == 0,
        "available_full",
        "Full vector should have 0 available slots",
        _counter) && result;

    // test 4: count exceeds capacity (edge case, returns 0)
    available = d_vector_common_available(15, 10);
    result    = d_assert_standalone(
        available == 0,
        "available_overflow",
        "Count > capacity should return 0",
        _counter) && result;

    // test 5: both zero
    available = d_vector_common_available(0, 0);
    result    = d_assert_standalone(
        available == 0,
        "available_both_zero",
        "Both zero should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_vector_common_capacity_all
  Aggregation function that runs all capacity management tests.
  Tests the following:
  - d_vector_common_reserve
  - d_vector_common_shrink_to_fit
  - d_vector_common_ensure_capacity
  - d_vector_common_grow
  - d_vector_common_maybe_shrink
  - d_vector_common_available
*/
bool
d_tests_sa_vector_common_capacity_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Capacity Management Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_vector_common_reserve(_counter) && result;
    result = d_tests_sa_vector_common_shrink_to_fit(_counter) && result;
    result = d_tests_sa_vector_common_ensure_capacity(_counter) && result;
    result = d_tests_sa_vector_common_grow(_counter) && result;
    result = d_tests_sa_vector_common_maybe_shrink(_counter) && result;
    result = d_tests_sa_vector_common_available(_counter) && result;

    return result;
}
