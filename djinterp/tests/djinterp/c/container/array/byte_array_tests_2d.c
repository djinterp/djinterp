/*******************************************************************************
* djinterp [test]                                          byte_array_tests_2d.c
*
*   Unit tests for byte_array.h 2D array builder macros.
*   Tests D_BYTE_ARRAY_2D and D_BYTE_ARRAY_2D_EX macros for building
*   rectangular 2D byte arrays.
*
*   NOTE: These tests are skipped on MSVC due to preprocessor incompatibility.
*   The 2D macros use a GCC/Clang-specific trick for row iteration that MSVC
*   does not support.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_2d.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.11.09
*******************************************************************************/

// entire file is skipped on MSVC - check BEFORE includes
#if !defined(_MSC_VER)

#include "./byte_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_byte_array_2d_basic
    Tests basic D_BYTE_ARRAY_2D macro functionality.
    Tests the following:
    - Creates 2D array with row and column counts
    - Default uses 1-byte counts and 1-byte elements
    - Layout: [nrows:1][ncols:1][data...]
*/
bool d_tests_byte_array_2d_basic(
    struct dj_test_suite* _suite
)
{
    bool    result;
    // 2 rows x 3 cols, element_size = 1
    uint8_t arr[] = D_BYTE_ARRAY_2D(1, (1, 2, 3), (4, 5, 6));

    d_test_suite_test(_suite, "byte_array_2d_basic");

    result = true;

    // test 1: total size = 2 (counts) + 2*3*1 (data) = 8
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_basic_size",
                              sizeof(arr) == 8,
                              "D_BYTE_ARRAY_2D(2x3, 1-byte elems) should have size 8") && result;

    // test 2: row count stored first
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_basic_rows",
                              arr[0] == 2,
                              "D_BYTE_ARRAY_2D row count should be 2") && result;

    // test 3: column count stored second
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_basic_cols",
                              arr[1] == 3,
                              "D_BYTE_ARRAY_2D column count should be 3") && result;

    // test 4: first row data
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_basic_row0",
                              arr[2] == 1 && arr[3] == 2 && arr[4] == 3,
                              "D_BYTE_ARRAY_2D first row data correct") && result;

    // test 5: second row data
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_basic_row1",
                              arr[5] == 4 && arr[6] == 5 && arr[7] == 6,
                              "D_BYTE_ARRAY_2D second row data correct") && result;

    return result;
}


/*
d_tests_byte_array_2d_layout
    Tests D_BYTE_ARRAY_2D layout with various sizes.
    Tests the following:
    - Multiple rows with 1-byte elements
    - Data stored in row-major order
*/
bool d_tests_byte_array_2d_layout(
    struct dj_test_suite* _suite
)
{
    bool    result;
    // 2 rows x 2 cols
    uint8_t arr1[] = D_BYTE_ARRAY_2D(1, (0xAA, 0xBB), (0xCC, 0xDD));
    // 3 rows x 2 cols
    uint8_t arr2[] = D_BYTE_ARRAY_2D(1, (1, 2), (3, 4), (5, 6));
    // 2 rows x 4 cols with 1-byte elements
    uint8_t arr3[] = D_BYTE_ARRAY_2D(1, (0x12, 0x34, 0x56, 0x78), (0xAB, 0xCD, 0xEF, 0x01));

    d_test_suite_test(_suite, "byte_array_2d_layout");

    result = true;

    // test 1: 2x2 array
    // size = 2 (counts) + 2*2*1 (data) = 6
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_2x2_size",
                              sizeof(arr1) == 6,
                              "D_BYTE_ARRAY_2D 2x2 array size correct") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_2x2_counts",
                              arr1[0] == 2 && arr1[1] == 2,
                              "D_BYTE_ARRAY_2D 2x2 counts (2 rows, 2 cols)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_2x2_data",
                              arr1[2] == 0xAA && arr1[3] == 0xBB && arr1[4] == 0xCC && arr1[5] == 0xDD,
                              "D_BYTE_ARRAY_2D 2x2 data correct") && result;

    // test 2: three rows
    // size = 2 (counts) + 3*2*1 (data) = 8
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_three_rows_size",
                              sizeof(arr2) == 8,
                              "D_BYTE_ARRAY_2D 3x2 array size correct") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_three_rows_counts",
                              arr2[0] == 3 && arr2[1] == 2,
                              "D_BYTE_ARRAY_2D 3 rows, 2 cols") && result;

    // row-major order: row0, row1, row2
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_three_rows_data",
                              arr2[2] == 1 && arr2[3] == 2 && arr2[4] == 3 && arr2[5] == 4 && arr2[6] == 5 && arr2[7] == 6,
                              "D_BYTE_ARRAY_2D row-major data order") && result;

    // test 3: 2x4 array with 1-byte elements
    // size = 2 (counts) + 2*4*1 (data) = 10
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_more_cols_size",
                              sizeof(arr3) == 10,
                              "D_BYTE_ARRAY_2D 2x4 array size correct") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_more_cols_counts",
                              arr3[0] == 2 && arr3[1] == 4,
                              "D_BYTE_ARRAY_2D 2 rows, 4 cols") && result;

    // first row data
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_more_cols_row0",
                              arr3[2] == 0x12 && arr3[3] == 0x34 && arr3[4] == 0x56 && arr3[5] == 0x78,
                              "D_BYTE_ARRAY_2D first row data correct") && result;

    return result;
}


/*
d_tests_byte_array_2d_ex
    Tests the D_BYTE_ARRAY_2D_EX macro with explicit sizes.
    Tests the following:
    - Custom row count size
    - Custom column count size
    - Custom element size
*/
bool d_tests_byte_array_2d_ex(
    struct dj_test_suite* _suite
)
{
    bool    result;
    // row_count_size=2, col_count_size=2, element_size=1
    // 2 rows x 3 cols
    uint8_t arr1[] = D_BYTE_ARRAY_2D_EX(2, 2, 1, (1, 2, 3), (4, 5, 6));
    // row_count_size=1, col_count_size=2, element_size=1
    uint8_t arr2[] = D_BYTE_ARRAY_2D_EX(1, 2, 1, (0x12, 0x34), (0xAB, 0xCD));
    // row_count_size=4, col_count_size=1, element_size=1
    uint8_t arr3[] = D_BYTE_ARRAY_2D_EX(4, 1, 1, (0xAA, 0xBB), (0xCC, 0xDD));

    d_test_suite_test(_suite, "byte_array_2d_ex");

    result = true;

    // test 1: 2-byte row count, 2-byte col count
    // size = 2 (row count) + 2 (col count) + 2*3*1 (data) = 10
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_2_2_1_size",
                              sizeof(arr1) == 10,
                              "D_BYTE_ARRAY_2D_EX(2,2,1) size correct") && result;

    // row count = 2 as 2-byte: {0x02, 0x00}
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_row_count",
                              arr1[0] == 0x02 && arr1[1] == 0x00,
                              "D_BYTE_ARRAY_2D_EX row count as 2-byte") && result;

    // col count = 3 as 2-byte: {0x03, 0x00}
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_col_count",
                              arr1[2] == 0x03 && arr1[3] == 0x00,
                              "D_BYTE_ARRAY_2D_EX col count as 2-byte") && result;

    // data starts at offset 4
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_data",
                              arr1[4] == 1 && arr1[5] == 2 && arr1[6] == 3 && arr1[7] == 4 && arr1[8] == 5 && arr1[9] == 6,
                              "D_BYTE_ARRAY_2D_EX data correct") && result;

    // test 2: 1-byte row count, 2-byte col count, 1-byte elements
    // size = 1 (row) + 2 (col) + 2*2*1 (data) = 7
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_1_2_1_size",
                              sizeof(arr2) == 7,
                              "D_BYTE_ARRAY_2D_EX(1,2,1) size correct") && result;

    // row count = 2 as 1-byte
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_1byte_row",
                              arr2[0] == 0x02,
                              "D_BYTE_ARRAY_2D_EX row count as 1-byte") && result;

    // col count = 2 as 2-byte: {0x02, 0x00}
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_2byte_col",
                              arr2[1] == 0x02 && arr2[2] == 0x00,
                              "D_BYTE_ARRAY_2D_EX col count as 2-byte") && result;

    // first element at offset 3
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_1byte_elem",
                              arr2[3] == 0x12 && arr2[4] == 0x34,
                              "D_BYTE_ARRAY_2D_EX 1-byte elements correct") && result;

    // test 3: 4-byte row count
    // size = 4 (row) + 1 (col) + 2*2*1 (data) = 9
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_4_1_1_size",
                              sizeof(arr3) == 9,
                              "D_BYTE_ARRAY_2D_EX(4,1,1) size correct") && result;

    // row count = 2 as 4-byte: {0x02, 0x00, 0x00, 0x00}
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_4byte_row",
                              arr3[0] == 0x02 && arr3[1] == 0x00 && arr3[2] == 0x00 && arr3[3] == 0x00,
                              "D_BYTE_ARRAY_2D_EX row count as 4-byte") && result;

    // col count = 2 as 1-byte at offset 4
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_1byte_col_offset",
                              arr3[4] == 0x02,
                              "D_BYTE_ARRAY_2D_EX col count at correct offset") && result;

    // data at offset 5
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_array_2d_ex_data_offset",
                              arr3[5] == 0xAA && arr3[6] == 0xBB && arr3[7] == 0xCC && arr3[8] == 0xDD,
                              "D_BYTE_ARRAY_2D_EX data at correct offset") && result;

    return result;
}


/*
d_tests_byte_array_2d_all
    Aggregation function that runs all 2D array builder tests.
*/
bool d_tests_byte_array_2d_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "byte_array_2d_all");

    result = true;

    result = d_tests_byte_array_2d_basic(_suite) && result;
    result = d_tests_byte_array_2d_layout(_suite) && result;
    result = d_tests_byte_array_2d_ex(_suite) && result;

    return result;
}

#endif  // !_MSC_VER
