/*******************************************************************************
* djinterp [container]                             buffer_common_tests_sa_init.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_init.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_init_sized
  Tests the d_buffer_common_init_sized function.
  Tests the following:
  - NULL destination pointer rejection
  - NULL count pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - zero initial_capacity uses default capacity
  - successful initialization with explicit capacity
  - count is set to zero after init
  - capacity matches requested value
*/
bool
d_tests_sa_buffer_common_init_sized
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(NULL, &count, &capacity,
                                   sizeof(int), 10) == false,
        "init_sized_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 2: NULL count should fail
    elements = NULL;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(&elements, NULL, &capacity,
                                   sizeof(int), 10) == false,
        "init_sized_null_count",
        "NULL count should return false",
        _counter) && result;

    // test 3: NULL capacity should fail
    elements = NULL;
    count    = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(&elements, &count, NULL,
                                   sizeof(int), 10) == false,
        "init_sized_null_capacity",
        "NULL capacity should return false",
        _counter) && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(&elements, &count, &capacity,
                                   0, 10) == false,
        "init_sized_zero_elem_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 5: zero initial_capacity should use default
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(&elements, &count, &capacity,
                                   sizeof(int), 0) == true,
        "init_sized_zero_cap_success",
        "Zero capacity should succeed (uses default)",
        _counter) && result;

    if (elements)
    {
        result = d_assert_standalone(
            capacity == D_BUFFER_DEFAULT_CAPACITY,
            "init_sized_zero_cap_default",
            "Capacity should equal D_BUFFER_DEFAULT_CAPACITY",
            _counter) && result;

        result = d_assert_standalone(
            count == 0,
            "init_sized_zero_cap_count",
            "Count should be 0",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    // test 6: successful init with explicit capacity
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_sized(&elements, &count, &capacity,
                                   sizeof(int), 64) == true,
        "init_sized_explicit_success",
        "Explicit capacity init should succeed",
        _counter) && result;

    if (elements)
    {
        result = d_assert_standalone(
            capacity == 64,
            "init_sized_explicit_cap",
            "Capacity should be 64",
            _counter) && result;

        result = d_assert_standalone(
            count == 0,
            "init_sized_explicit_count",
            "Count should be 0",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_init_from_data
  Tests the d_buffer_common_init_from_data function.
  Tests the following:
  - NULL destination rejection
  - NULL source with non-zero count rejection
  - NULL source with zero count succeeds
  - successful copy of source data
  - extra capacity is honoured
  - count and capacity set correctly
*/
bool
d_tests_sa_buffer_common_init_from_data
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[5] = {10, 20, 30, 40, 50};

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_from_data(NULL, &count, &capacity,
                                       sizeof(int), source, 5, 0) == false,
        "init_from_data_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 2: NULL source with non-zero count should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), NULL, 5, 0) == false,
        "init_from_data_null_src_nonzero",
        "NULL source with count>0 should return false",
        _counter) && result;

    // test 3: NULL source with zero count should succeed
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), NULL, 0, 0) == true,
        "init_from_data_null_src_zero",
        "NULL source with count=0 should succeed",
        _counter) && result;

    if (elements)
    {
        result = d_assert_standalone(
            count == 0,
            "init_from_data_null_src_zero_count",
            "Count should be 0",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    // test 4: successful copy of source data
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0) == true,
        "init_from_data_success",
        "Init from data should succeed",
        _counter) && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = d_assert_standalone(
            count == 5,
            "init_from_data_count",
            "Count should be 5",
            _counter) && result;

        {
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40) &&
                           (arr[4] == 50);

            result = d_assert_standalone(
                correct,
                "init_from_data_values",
                "Elements should match source [10..50]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    // test 5: extra capacity honoured
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5,
                                       500) == true,
        "init_from_data_extra_cap_success",
        "Extra capacity init should succeed",
        _counter) && result;

    if (elements)
    {
        result = d_assert_standalone(
            capacity >= 505,
            "init_from_data_extra_cap_value",
            "Capacity should be >= source_count + extra",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_init_copy
  Tests the d_buffer_common_init_copy function.
  Tests the following:
  - NULL parameters rejection
  - successful copy initialization
  - copied data matches source
*/
bool
d_tests_sa_buffer_common_init_copy
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_copy(NULL, &count, &capacity,
                                  sizeof(int), source, 3) == false,
        "init_copy_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 2: successful copy
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_copy(&elements, &count, &capacity,
                                  sizeof(int), source, 3) == true,
        "init_copy_success",
        "Init copy should succeed",
        _counter) && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = d_assert_standalone(
            count == 3,
            "init_copy_count",
            "Count should be 3",
            _counter) && result;

        {
            bool correct = (arr[0] == 100) && (arr[1] == 200) &&
                           (arr[2] == 300);

            result = d_assert_standalone(
                correct,
                "init_copy_values",
                "Elements should match source [100, 200, 300]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_init_fill
  Tests the d_buffer_common_init_fill function.
  Tests the following:
  - NULL destination rejection
  - NULL value rejection
  - zero element_size rejection
  - successful fill with single value
  - all elements match fill value
*/
bool
d_tests_sa_buffer_common_init_fill
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    fill_val;

    result = true;

    // test 1: NULL destination should fail
    fill_val = 42;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_fill(NULL, &count, &capacity,
                                  sizeof(int), 5, &fill_val) == false,
        "init_fill_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 2: NULL value should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_fill(&elements, &count, &capacity,
                                  sizeof(int), 5, NULL) == false,
        "init_fill_null_value",
        "NULL value should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = d_assert_standalone(
        d_buffer_common_init_fill(&elements, &count, &capacity,
                                  0, 5, &fill_val) == false,
        "init_fill_zero_elem_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 4: successful fill
    elements = NULL;
    count    = 0;
    capacity = 0;
    fill_val = 77;
    result   = d_assert_standalone(
        d_buffer_common_init_fill(&elements, &count, &capacity,
                                  sizeof(int), 5, &fill_val) == true,
        "init_fill_success",
        "Init fill should succeed",
        _counter) && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = d_assert_standalone(
            count == 5,
            "init_fill_count",
            "Count should be 5",
            _counter) && result;

        // test 5: all elements match fill value
        {
            bool all_match = true;
            size_t i;

            for (i = 0; i < 5; ++i)
            {
                if (arr[i] != 77)
                {
                    all_match = false;
                }
            }

            result = d_assert_standalone(
                all_match,
                "init_fill_values",
                "All elements should be 77",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_init_all
  Aggregation function that runs all initialization tests.
*/
bool
d_tests_sa_buffer_common_init_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Initialization Functions\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_buffer_common_init_sized(_counter) && result;
    result = d_tests_sa_buffer_common_init_from_data(_counter) && result;
    result = d_tests_sa_buffer_common_init_copy(_counter) && result;
    result = d_tests_sa_buffer_common_init_fill(_counter) && result;

    return result;
}
