/*******************************************************************************
* djinterp [container]                                buffer_common_tests_copy.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_copy.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_copy_to
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
bool d_tests_buffer_common_copy_to(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    source[5] = {10, 20, 30, 40, 50};
    int    dest[10];
    size_t copied;

    d_test_suite_test(_suite, "buffer_common_copy_to");

    result = true;

    // test 1: NULL source should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_source",
                              d_buffer_common_copy_to(NULL, 5, sizeof(int), dest, 10, &copied) == false,
                              "NULL source should return false") && result;

    // test 2: NULL destination should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_dest",
                              d_buffer_common_copy_to(source, 5, sizeof(int), NULL, 10, &copied) == false,
                              "NULL destination should return false") && result;

    // test 3: zero element_size should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_zero_size",
                              d_buffer_common_copy_to(source, 5, 0, dest, 10, &copied) == false,
                              "Zero element_size should return false") && result;

    // test 4: successful full copy
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_success",
                              d_buffer_common_copy_to(source, 5, sizeof(int), dest, 10, &copied) == true,
                              "Full copy should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_copied_count",
                              copied == 5,
                              "Copied count should be 5") && result;

    {
        bool correct = (dest[0] == 10) && (dest[1] == 20) &&
                       (dest[2] == 30) && (dest[3] == 40) &&
                       (dest[4] == 50);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_values",
                                  correct,
                                  "Copied values should match source") && result;
    }

    // test 5: destination smaller truncates
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_truncate_success",
                              d_buffer_common_copy_to(source, 5, sizeof(int), dest, 3, &copied) == true,
                              "Truncated copy should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_truncate_count",
                              copied == 3,
                              "Truncated copied count should be 3") && result;

    {
        bool correct = (dest[0] == 10) && (dest[1] == 20) &&
                       (dest[2] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_truncate_values",
                                  correct,
                                  "Truncated values should be first 3 source elements") && result;
    }

    // test 6: NULL copied_count tolerated
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_copied_count",
                              d_buffer_common_copy_to(source, 5, sizeof(int), dest, 10, NULL) == true,
                              "NULL copied_count should be tolerated") && result;

    return result;
}


/*
d_tests_buffer_common_copy_range_to
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
bool d_tests_buffer_common_copy_range_to(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    source[5] = {10, 20, 30, 40, 50};
    int    dest[10];
    size_t copied;

    d_test_suite_test(_suite, "buffer_common_copy_range_to");

    result = true;

    // test 1: NULL source should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_null_source",
                              d_buffer_common_copy_range_to(NULL, 5, sizeof(int), 1, 3, dest, 10, &copied) == false,
                              "NULL source should return false") && result;

    // test 2: NULL destination should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_null_dest",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 1, 3, NULL, 10, &copied) == false,
                              "NULL destination should return false") && result;

    // test 3: invalid range (start > end)
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_invalid",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 3, 1, dest, 10, &copied) == false,
                              "start > end should return false") && result;

    // test 4: range exceeding source
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_exceeds",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 0, 100, dest, 10, &copied) == false,
                              "Range exceeding source should return false") && result;

    // test 5: successful range copy [1, 4)
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_success",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 1, 4, dest, 10, &copied) == true,
                              "Range copy [1,4) should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_copied_count",
                              copied == 3,
                              "Copied count should be 3") && result;

    {
        bool correct = (dest[0] == 20) && (dest[1] == 30) &&
                       (dest[2] == 40);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_range_values",
                                  correct,
                                  "Copied values should be [20, 30, 40]") && result;
    }

    // test 6: destination truncation
    d_memset(dest, 0, sizeof(dest));
    copied = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_truncate_success",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 0, 5, dest, 2, &copied) == true,
                              "Truncated range copy should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_truncate_count",
                              copied == 2,
                              "Truncated count should be 2") && result;

    // test 7: NULL copied_count tolerated
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_null_copied_count",
                              d_buffer_common_copy_range_to(source, 5, sizeof(int), 1, 3, dest, 10, NULL) == true,
                              "NULL copied_count should be tolerated") && result;

    return result;
}


/*
d_tests_buffer_common_copy_all
  Aggregation function that runs all copy tests.
*/
bool d_tests_buffer_common_copy_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_copy_all");

    result = true;

    result = d_tests_buffer_common_copy_to(_suite) && result;
    result = d_tests_buffer_common_copy_range_to(_suite) && result;

    return result;
}
