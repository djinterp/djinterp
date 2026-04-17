#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_reserve
  Tests the d_vector_reserve function.
  Tests the following:
  - NULL vector rejection
  - no-op when new capacity <= current capacity
  - successful reservation with larger capacity
  - elements preserved after reservation
*/
bool
d_tests_sa_vector_reserve
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_reserve(NULL, 100) == D_FAILURE,
        "reserve_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: no-op when new capacity <= current capacity
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_reserve(vec, 30) == D_SUCCESS,
            "reserve_smaller_success",
            "Smaller capacity should return D_SUCCESS (no-op)",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 50,
            "reserve_smaller_unchanged",
            "Capacity should remain unchanged",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: successful reservation with larger capacity
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        // add some elements
        value = 100;
        d_vector_push_back(vec, &value);
        value = 200;
        d_vector_push_back(vec, &value);
        value = 300;
        d_vector_push_back(vec, &value);

        result = d_assert_standalone(
            d_vector_reserve(vec, 100) == D_SUCCESS,
            "reserve_larger_success",
            "Larger capacity reservation should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 100,
            "reserve_larger_capacity",
            "Capacity should be updated to 100",
            _counter) && result;

        // test 4: elements preserved
        {
            int* arr      = (int*)vec->elements;
            bool preserved = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

            result = d_assert_standalone(
                preserved && vec->count == 3,
                "reserve_elements_preserved",
                "Elements should be preserved after reservation",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_shrink_to_fit
  Tests the d_vector_shrink_to_fit function.
  Tests the following:
  - NULL vector rejection
  - no-op when capacity equals count
  - successful shrink of over-allocated vector
  - elements preserved after shrink
*/
bool
d_tests_sa_vector_shrink_to_fit
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_shrink_to_fit(NULL) == D_FAILURE,
        "shrink_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful shrink
    vec = d_vector_new(sizeof(int), 100);

    if (vec)
    {
        // add a few elements
        value = 10;
        d_vector_push_back(vec, &value);
        value = 20;
        d_vector_push_back(vec, &value);
        value = 30;
        d_vector_push_back(vec, &value);

        result = d_assert_standalone(
            vec->capacity == 100 && vec->count == 3,
            "shrink_before_state",
            "Before shrink: capacity=100, count=3",
            _counter) && result;

        result = d_assert_standalone(
            d_vector_shrink_to_fit(vec) == D_SUCCESS,
            "shrink_success",
            "shrink_to_fit should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 3,
            "shrink_capacity_reduced",
            "Capacity should be reduced to 3",
            _counter) && result;

        // test 3: elements preserved
        {
            int* arr      = (int*)vec->elements;
            bool preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = d_assert_standalone(
                preserved,
                "shrink_elements_preserved",
                "Elements should be preserved after shrink",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    // test 4: shrink empty vector
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_shrink_to_fit(vec) == D_SUCCESS,
            "shrink_empty_success",
            "Shrinking empty vector should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 0,
            "shrink_empty_capacity",
            "Empty vector should shrink to capacity 0",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_ensure_capacity
  Tests the d_vector_ensure_capacity function.
  Tests the following:
  - NULL vector rejection
  - no-op when sufficient capacity exists
  - successful growth when needed
*/
bool
d_tests_sa_vector_ensure_capacity
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_ensure_capacity(NULL, 100) == D_FAILURE,
        "ensure_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: no-op when sufficient capacity exists
    vec = d_vector_new(sizeof(int), 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_ensure_capacity(vec, 30) == D_SUCCESS,
            "ensure_sufficient_success",
            "Sufficient capacity should return D_SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 50,
            "ensure_sufficient_unchanged",
            "Capacity should remain unchanged",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: growth when needed
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_ensure_capacity(vec, 100) == D_SUCCESS,
            "ensure_growth_success",
            "Growth should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity >= 100,
            "ensure_growth_capacity",
            "Capacity should be at least 100",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_available
  Tests the d_vector_available function.
  Tests the following:
  - NULL vector returns 0
  - empty vector returns full capacity
  - partially filled vector returns remaining
  - full vector returns 0
*/
bool
d_tests_sa_vector_available
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;
    size_t           avail;

    result = true;

    // test 1: NULL vector should return 0
    avail  = d_vector_available(NULL);
    result = d_assert_standalone(
        avail == 0,
        "available_null_vector",
        "NULL vector should return 0",
        _counter) && result;

    // test 2: empty vector returns full capacity
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        avail  = d_vector_available(vec);
        result = d_assert_standalone(
            avail == 10,
            "available_empty",
            "Empty vector should return full capacity (10)",
            _counter) && result;

        // test 3: partially filled
        value = 1;
        d_vector_push_back(vec, &value);
        d_vector_push_back(vec, &value);
        d_vector_push_back(vec, &value);

        avail  = d_vector_available(vec);
        result = d_assert_standalone(
            avail == 7,
            "available_partial",
            "Partially filled should return 7",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 4: full vector
    vec = d_vector_new_fill(sizeof(int), 5, &value);

    if (vec)
    {
        d_vector_shrink_to_fit(vec);

        avail  = d_vector_available(vec);
        result = d_assert_standalone(
            avail == 0,
            "available_full",
            "Full vector should return 0",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool
d_tests_sa_vector_capacity_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Capacity Management Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_vector_reserve(_counter) && result;
    result = d_tests_sa_vector_shrink_to_fit(_counter) && result;
    result = d_tests_sa_vector_ensure_capacity(_counter) && result;
    result = d_tests_sa_vector_available(_counter) && result;

    return result;
}
