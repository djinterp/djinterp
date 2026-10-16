/*******************************************************************************
* djinterp [container]                           text_buffer_tests_sa_capacity.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_capacity.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_ensure_capacity
  Tests the d_text_buffer_ensure_capacity function.
  Tests the following:
  - NULL buffer returns false
  - requesting less than current capacity succeeds without reallocation
  - requesting more than current capacity grows the buffer
  - data integrity preserved after growth
  - capacity is at least the requested amount
*/
bool
d_tests_sa_text_buffer_ensure_capacity
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    size_t                old_cap;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_ensure_capacity(NULL, 100) == false,
        "ensure_cap_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        old_cap = d_text_buffer_capacity(buffer);

        // test 2: request less than current
        result = d_assert_standalone(
            d_text_buffer_ensure_capacity(buffer, 1) == true,
            "ensure_cap_less",
            "Requesting less should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) == old_cap,
            "ensure_cap_less_unchanged",
            "Capacity should not change for smaller request",
            _counter) && result;

        // test 3: request much more than current
        result = d_assert_standalone(
            d_text_buffer_ensure_capacity(buffer, 1024) == true,
            "ensure_cap_grow",
            "Growing capacity should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= 1024,
            "ensure_cap_grow_size",
            "Capacity should be at least 1024",
            _counter) && result;

        // test 4: data integrity
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5 &&
            d_strcasecmp(d_text_buffer_get_string(buffer),
                         "Hello") == 0,
            "ensure_cap_data_intact",
            "Data should be intact after growth",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_resize_to_fit
  Tests the d_text_buffer_resize_to_fit function.
  Tests the following:
  - NULL buffer returns false
  - buffer with excess capacity is shrunk
  - capacity after shrink is approximately count + 1
  - data integrity preserved after shrink
*/
bool
d_tests_sa_text_buffer_resize_to_fit
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_resize_to_fit(NULL) == false,
        "resize_fit_null",
        "NULL buffer should return false",
        _counter) && result;

    // create buffer with large initial capacity
    buffer = d_text_buffer_new(1024);

    if (buffer)
    {
        // add small amount of data
        d_text_buffer_append_string(buffer, "Hi");

        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= 1024,
            "resize_fit_precondition",
            "Capacity should be >= 1024 before shrink",
            _counter) && result;

        // test 2: shrink to fit
        result = d_assert_standalone(
            d_text_buffer_resize_to_fit(buffer) == true,
            "resize_fit_success",
            "Resize to fit should succeed",
            _counter) && result;

        // test 3: capacity should be reduced
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) < 1024,
            "resize_fit_smaller",
            "Capacity should be smaller after shrink",
            _counter) && result;

        // test 4: data intact
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 2 &&
            d_strcasecmp(d_text_buffer_get_string(buffer),
                         "Hi") == 0,
            "resize_fit_data",
            "Data should be intact after shrink",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_reserve
  Tests the d_text_buffer_reserve function.
  Tests the following:
  - NULL buffer returns false
  - zero additional capacity succeeds
  - reserving additional space grows capacity
  - data integrity preserved
*/
bool
d_tests_sa_text_buffer_reserve
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    size_t                old_cap;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_reserve(NULL, 100) == false,
        "reserve_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Test");

    if (buffer)
    {
        old_cap = d_text_buffer_capacity(buffer);

        // test 2: zero additional
        result = d_assert_standalone(
            d_text_buffer_reserve(buffer, 0) == true,
            "reserve_zero",
            "Zero additional should succeed",
            _counter) && result;

        // test 3: reserve additional space
        result = d_assert_standalone(
            d_text_buffer_reserve(buffer, 500) == true,
            "reserve_grow",
            "Reserving additional space should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >=
            (old_cap + 500),
            "reserve_grow_size",
            "Capacity should grow by at least 500",
            _counter) && result;

        // test 4: data intact
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 4 &&
            d_strcasecmp(d_text_buffer_get_string(buffer),
                         "Test") == 0,
            "reserve_data_intact",
            "Data should be intact after reserve",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_capacity_all
  Aggregation function that runs all capacity management tests.
*/
bool
d_tests_sa_text_buffer_capacity_all
(
    struct d_test_counter* _counter
)
{
    printf("\n  [SECTION] Capacity Management Functions\n");
    printf("  ----------------------------------------\n");

    return d_tests_sa_text_buffer_ensure_capacity(_counter) &&
           d_tests_sa_text_buffer_resize_to_fit(_counter)   &&
           d_tests_sa_text_buffer_reserve(_counter);
}
