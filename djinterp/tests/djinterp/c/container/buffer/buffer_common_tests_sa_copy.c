/*******************************************************************************
* djinterp [container]                             buffer_common_tests_sa_copy.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_copy.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_copy_to
  Tests the d_buffer_common_copy_to function.
  Tests the following:
  - NULL source rejection
  - NULL destination rejection
  - zero element_size rejection
  - successful full copy
  - destination smaller than source truncates
  - copied_count output is correct
  - NULL copied_count is tolerated
*/
bool
d_tests_sa_buffer_common_copy_to
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    source[5] = {10, 20, 30, 40, 50};
    int    dest[10];
    size_t copied;

    result = true;

    // test 1: NULL source should fail
    result = d_assert_standalone(
        d_buffer_common_copy_to(NULL, 5, sizeof(int),
                                dest, 10, &copied) == false,
        "copy_to_null_source",
        "NULL source should return false",
        _counter) && result;

    // test 2: NULL destination should fail
    result = d_assert_standalone(
        d_buffer_common_copy_to(source, 5, sizeof(int),
                                NULL, 10, &copied) == false,
        "copy_to_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    result = d_assert_standalone(
        d_buffer_common_copy_to(source, 5, 0,
                                dest, 10, &copied) == false,
        "copy_to_zero_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 4: successful full copy
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = d_assert_standalone(
        d_buffer_common_copy_to(source, 5, sizeof(int),
                                dest, 10, &copied) == true,
        "copy_to_success",
        "Full copy should succeed",
        _counter) && result;

    result = d_assert_standalone(
        copied == 5,
        "copy_to_copied_count",
        "Copied count should be 5",
        _counter) && result;

    {
        bool correct = (dest[0] == 10) && (dest[1] == 20) &&
                       (dest[2] == 30) && (dest[3] == 40) &&
                       (dest[4] == 50);

        result = d_assert_standalone(
            correct,
            "copy_to_values",
            "Copied values should match source",
            _counter) && result;
    }

    // test 5: destination smaller truncates
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = d_assert_standalone(
        d_buffer_common_copy_to(source, 5, sizeof(int),
                                dest, 3, &copied) == true,
        "copy_to_truncate_success",
        "Truncated copy should succeed",
        _counter) && result;

    result = d_assert_standalone(
        copied == 3,
        "copy_to_truncate_count",
        "Truncated copied count should be 3",
        _counter) && result;

    {
        bool correct = (dest[0] == 10) && (dest[1] == 20) &&
                       (dest[2] == 30);

        result = d_assert_standalone(
            correct,
            "copy_to_truncate_values",
            "Truncated values should be first 3 source elements",
            _counter) && result;
    }

    // test 6: NULL copied_count tolerated
    result = d_assert_standalone(
        d_buffer_common_copy_to(source, 5, sizeof(int),
                                dest, 10, NULL) == true,
        "copy_to_null_copied_count",
        "NULL copied_count should be tolerated",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_copy_range_to
  Tests the d_buffer_common_copy_range_to function.
  Tests the following:
  - NULL source rejection
  - NULL destination rejection
  - zero element_size rejection
  - invalid range (start > end) rejection
  - range exceeding source count rejection
  - successful range copy
  - destination truncation on small capacity
  - NULL copied_count tolerated
*/
bool
d_tests_sa_buffer_common_copy_range_to
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    source[5] = {10, 20, 30, 40, 50};
    int    dest[10];
    size_t copied;

    result = true;

    // test 1: NULL source should fail
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(NULL, 5, sizeof(int),
                                      1, 3, dest, 10,
                                      &copied) == false,
        "copy_range_null_source",
        "NULL source should return false",
        _counter) && result;

    // test 2: NULL destination should fail
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      1, 3, NULL, 10,
                                      &copied) == false,
        "copy_range_null_dest",
        "NULL destination should return false",
        _counter) && result;

    // test 3: invalid range (start > end)
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      3, 1, dest, 10,
                                      &copied) == false,
        "copy_range_invalid",
        "start > end should return false",
        _counter) && result;

    // test 4: range exceeding source
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      0, 100, dest, 10,
                                      &copied) == false,
        "copy_range_exceeds",
        "Range exceeding source should return false",
        _counter) && result;

    // test 5: successful range copy [1, 4)
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      1, 4, dest, 10,
                                      &copied) == true,
        "copy_range_success",
        "Range copy [1,4) should succeed",
        _counter) && result;

    result = d_assert_standalone(
        copied == 3,
        "copy_range_copied_count",
        "Copied count should be 3",
        _counter) && result;

    {
        bool correct = (dest[0] == 20) && (dest[1] == 30) &&
                       (dest[2] == 40);

        result = d_assert_standalone(
            correct,
            "copy_range_values",
            "Copied values should be [20, 30, 40]",
            _counter) && result;
    }

    // test 6: destination truncation
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      0, 5, dest, 2,
                                      &copied) == true,
        "copy_range_truncate_success",
        "Truncated range copy should succeed",
        _counter) && result;

    result = d_assert_standalone(
        copied == 2,
        "copy_range_truncate_count",
        "Truncated count should be 2",
        _counter) && result;

    // test 7: NULL copied_count tolerated
    result = d_assert_standalone(
        d_buffer_common_copy_range_to(source, 5, sizeof(int),
                                      1, 3, dest, 10,
                                      NULL) == true,
        "copy_range_null_copied_count",
        "NULL copied_count should be tolerated",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_copy_all
  Aggregation function that runs all copy tests.
*/
bool
d_tests_sa_buffer_common_copy_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Copy Functions\n");
    printf("  -------------------------\n");

    result = d_tests_sa_buffer_common_copy_to(_counter) && result;
    result = d_tests_sa_buffer_common_copy_range_to(_counter) && result;

    return result;
}
