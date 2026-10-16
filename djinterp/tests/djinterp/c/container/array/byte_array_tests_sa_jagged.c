/*******************************************************************************
* djinterp [test]                                   byte_array_tests_sa_jagged.c
*
*   Unit tests for byte_array.h jagged array builder macros.
*   Tests D_BYTE_ARRAY_JAGGED and D_BYTE_ARRAY_JAGGED_EX macros for building
*   jagged (variable-length rows) byte arrays.
*
*   NOTE: These tests are skipped on MSVC due to preprocessor incompatibility.
*   The JAGGED macros use a GCC/Clang-specific trick for row iteration that
*   MSVC does not support.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_sa_jagged.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.09.21
*******************************************************************************/

// entire file is skipped on MSVC - check BEFORE includes
#if !defined(_MSC_VER)

#include "./byte_array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_byte_array_jagged_basic
    Tests basic D_BYTE_ARRAY_JAGGED macro functionality.
    Tests the following:
    - Creates jagged array with row count prefix
    - Each row has its own length prefix
    - Layout: [nrows:1][row0_len:1][row0_data...][row1_len:1]...
*/
bool
d_tests_sa_byte_array_jagged_basic
(
    struct d_test_counter* _counter
)
{
    bool    result;
    // 2 rows: row0 has 3 elements, row1 has 2 elements
    uint8_t arr[] = D_BYTE_ARRAY_JAGGED(1, (1, 2, 3), (4, 5));

    result = true;

    // test 1: total size = 1 (nrows) + 1 (row0_len) + 3 (row0_data) +
    //                       1 (row1_len) + 2 (row1_data) = 8
    result = d_assert_standalone(
        sizeof(arr) == 8,
        "byte_array_jagged_basic_size",
        "D_BYTE_ARRAY_JAGGED(2 rows) should have correct size",
        _counter) && result;

    // test 2: row count stored first
    result = d_assert_standalone(
        arr[0] == 2,
        "byte_array_jagged_basic_nrows",
        "D_BYTE_ARRAY_JAGGED row count should be 2",
        _counter) && result;

    // test 3: first row length
    result = d_assert_standalone(
        arr[1] == 3,
        "byte_array_jagged_basic_row0_len",
        "D_BYTE_ARRAY_JAGGED first row length should be 3",
        _counter) && result;

    // test 4: first row data
    result = d_assert_standalone(
        arr[2] == 1 && arr[3] == 2 && arr[4] == 3,
        "byte_array_jagged_basic_row0_data",
        "D_BYTE_ARRAY_JAGGED first row data correct",
        _counter) && result;

    // test 5: second row length
    result = d_assert_standalone(
        arr[5] == 2,
        "byte_array_jagged_basic_row1_len",
        "D_BYTE_ARRAY_JAGGED second row length should be 2",
        _counter) && result;

    // test 6: second row data
    result = d_assert_standalone(
        arr[6] == 4 && arr[7] == 5,
        "byte_array_jagged_basic_row1_data",
        "D_BYTE_ARRAY_JAGGED second row data correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_jagged_layout
    Tests D_BYTE_ARRAY_JAGGED layout with various configurations.
    Tests the following:
    - Multiple rows with different lengths
    - 1-byte elements
*/
bool
d_tests_sa_byte_array_jagged_layout
(
    struct d_test_counter* _counter
)
{
    bool    result;
    // 2 rows with 2 elements each
    uint8_t arr1[] = D_BYTE_ARRAY_JAGGED(1, (0xAA, 0xBB), (0xCC, 0xDD));
    // 3 rows with varying lengths (at least 2 elements each)
    uint8_t arr2[] = D_BYTE_ARRAY_JAGGED(1, (1, 2), (3, 4, 5), (6, 7, 8, 9));
    // 2 rows with 1-byte elements
    uint8_t arr3[] = D_BYTE_ARRAY_JAGGED(1, (0x12, 0x34, 0x56), (0xAB, 0xCD));

    result = true;

    // test 1: two row jagged
    // size = 1 (nrows) + (1 + 2) + (1 + 2) = 7
    result = d_assert_standalone(
        sizeof(arr1) == 7,
        "byte_array_jagged_two_row_size",
        "D_BYTE_ARRAY_JAGGED two row size correct",
        _counter) && result;

    result = d_assert_standalone(
        arr1[0] == 2,
        "byte_array_jagged_two_row_nrows",
        "D_BYTE_ARRAY_JAGGED two rows: nrows=2",
        _counter) && result;

    result = d_assert_standalone(
        arr1[1] == 2 && arr1[2] == 0xAA && arr1[3] == 0xBB,
        "byte_array_jagged_two_row_row0",
        "D_BYTE_ARRAY_JAGGED row0: len=2, data={0xAA,0xBB}",
        _counter) && result;

    result = d_assert_standalone(
        arr1[4] == 2 && arr1[5] == 0xCC && arr1[6] == 0xDD,
        "byte_array_jagged_two_row_row1",
        "D_BYTE_ARRAY_JAGGED row1: len=2, data={0xCC,0xDD}",
        _counter) && result;

    // test 2: three rows with varying lengths
    // size = 1 + (1+2) + (1+3) + (1+4) = 13
    result = d_assert_standalone(
        sizeof(arr2) == 13,
        "byte_array_jagged_three_size",
        "D_BYTE_ARRAY_JAGGED 3 rows size correct",
        _counter) && result;

    result = d_assert_standalone(
        arr2[0] == 3,
        "byte_array_jagged_three_nrows",
        "D_BYTE_ARRAY_JAGGED 3 rows: nrows=3",
        _counter) && result;

    // row 0: len=2, data={1, 2}
    result = d_assert_standalone(
        arr2[1] == 2 && arr2[2] == 1 && arr2[3] == 2,
        "byte_array_jagged_three_row0",
        "D_BYTE_ARRAY_JAGGED row0: len=2, data={1,2}",
        _counter) && result;

    // row 1: len=3, data={3, 4, 5}
    result = d_assert_standalone(
        arr2[4] == 3 && arr2[5] == 3 && arr2[6] == 4 && arr2[7] == 5,
        "byte_array_jagged_three_row1",
        "D_BYTE_ARRAY_JAGGED row1: len=3, data={3,4,5}",
        _counter) && result;

    // row 2: len=4, data={6, 7, 8, 9}
    result = d_assert_standalone(
        arr2[8] == 4 && arr2[9] == 6 && arr2[10] == 7 &&
        arr2[11] == 8 && arr2[12] == 9,
        "byte_array_jagged_three_row2",
        "D_BYTE_ARRAY_JAGGED row2: len=4, data={6,7,8,9}",
        _counter) && result;

    // test 3: 1-byte elements
    // size = 1 + (1 + 3) + (1 + 2) = 8
    result = d_assert_standalone(
        sizeof(arr3) == 8,
        "byte_array_jagged_1byte_size",
        "D_BYTE_ARRAY_JAGGED 1-byte elements size correct",
        _counter) && result;

    result = d_assert_standalone(
        arr3[0] == 2,
        "byte_array_jagged_1byte_nrows",
        "D_BYTE_ARRAY_JAGGED 1-byte: nrows=2",
        _counter) && result;

    // row 0: len=3
    result = d_assert_standalone(
        arr3[1] == 3,
        "byte_array_jagged_1byte_row0_len",
        "D_BYTE_ARRAY_JAGGED 1-byte row0: len=3",
        _counter) && result;

    result = d_assert_standalone(
        arr3[2] == 0x12 && arr3[3] == 0x34 && arr3[4] == 0x56,
        "byte_array_jagged_1byte_row0_data",
        "D_BYTE_ARRAY_JAGGED 1-byte row0 data correct",
        _counter) && result;

    // row 1: len=2
    result = d_assert_standalone(
        arr3[5] == 2 && arr3[6] == 0xAB && arr3[7] == 0xCD,
        "byte_array_jagged_1byte_row1",
        "D_BYTE_ARRAY_JAGGED 1-byte row1 correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_jagged_ex
    Tests the D_BYTE_ARRAY_JAGGED_EX macro with explicit sizes.
    Tests the following:
    - Custom row count size
    - Custom row length size
    - Custom element size
*/
bool
d_tests_sa_byte_array_jagged_ex
(
    struct d_test_counter* _counter
)
{
    bool    result;
    // row_count_size=2, len_count_size=2, element_size=1
    uint8_t arr1[] = D_BYTE_ARRAY_JAGGED_EX(2, 2, 1, (1, 2, 3), (4, 5));
    // row_count_size=1, len_count_size=2, element_size=1
    uint8_t arr2[] = D_BYTE_ARRAY_JAGGED_EX(1, 2, 1, (0x12, 0x34), (0xAB, 0xCD, 0xEF));
    // row_count_size=4, len_count_size=1, element_size=1
    uint8_t arr3[] = D_BYTE_ARRAY_JAGGED_EX(4, 1, 1, (0xAA, 0xBB), (0xCC, 0xDD));

    result = true;

    // test 1: 2-byte row count, 2-byte row lengths
    // size = 2 (nrows) + (2 + 3*1) + (2 + 2*1) = 2 + 5 + 4 = 11
    result = d_assert_standalone(
        sizeof(arr1) == 11,
        "byte_array_jagged_ex_2_2_1_size",
        "D_BYTE_ARRAY_JAGGED_EX(2,2,1) size correct",
        _counter) && result;

    // row count = 2 as 2-byte: {0x02, 0x00}
    result = d_assert_standalone(
        arr1[0] == 0x02 && arr1[1] == 0x00,
        "byte_array_jagged_ex_2byte_nrows",
        "D_BYTE_ARRAY_JAGGED_EX row count as 2-byte",
        _counter) && result;

    // row 0 length = 3 as 2-byte: {0x03, 0x00}
    result = d_assert_standalone(
        arr1[2] == 0x03 && arr1[3] == 0x00,
        "byte_array_jagged_ex_2byte_row0_len",
        "D_BYTE_ARRAY_JAGGED_EX row0 length as 2-byte",
        _counter) && result;

    // row 0 data at offset 4
    result = d_assert_standalone(
        arr1[4] == 1 && arr1[5] == 2 && arr1[6] == 3,
        "byte_array_jagged_ex_row0_data",
        "D_BYTE_ARRAY_JAGGED_EX row0 data correct",
        _counter) && result;

    // row 1 length = 2 as 2-byte: {0x02, 0x00} at offset 7
    result = d_assert_standalone(
        arr1[7] == 0x02 && arr1[8] == 0x00,
        "byte_array_jagged_ex_2byte_row1_len",
        "D_BYTE_ARRAY_JAGGED_EX row1 length as 2-byte",
        _counter) && result;

    // row 1 data at offset 9
    result = d_assert_standalone(
        arr1[9] == 4 && arr1[10] == 5,
        "byte_array_jagged_ex_row1_data",
        "D_BYTE_ARRAY_JAGGED_EX row1 data correct",
        _counter) && result;

    // test 2: 1-byte row count, 2-byte row lengths, 1-byte elements
    // size = 1 + (2 + 2*1) + (2 + 3*1) = 1 + 4 + 5 = 10
    result = d_assert_standalone(
        sizeof(arr2) == 10,
        "byte_array_jagged_ex_1_2_1_size",
        "D_BYTE_ARRAY_JAGGED_EX(1,2,1) size correct",
        _counter) && result;

    // row count = 2 as 1-byte
    result = d_assert_standalone(
        arr2[0] == 0x02,
        "byte_array_jagged_ex_1byte_nrows",
        "D_BYTE_ARRAY_JAGGED_EX row count as 1-byte",
        _counter) && result;

    // row 0 length = 2 as 2-byte: {0x02, 0x00} at offset 1
    result = d_assert_standalone(
        arr2[1] == 0x02 && arr2[2] == 0x00,
        "byte_array_jagged_ex_row0_len_2byte",
        "D_BYTE_ARRAY_JAGGED_EX row0 length as 2-byte",
        _counter) && result;

    // row 0 elements: {0x12, 0x34} at offset 3
    result = d_assert_standalone(
        arr2[3] == 0x12 && arr2[4] == 0x34,
        "byte_array_jagged_ex_row0_1byte_elem",
        "D_BYTE_ARRAY_JAGGED_EX row0 1-byte elements",
        _counter) && result;

    // row 1 length = 3 as 2-byte: {0x03, 0x00} at offset 5
    result = d_assert_standalone(
        arr2[5] == 0x03 && arr2[6] == 0x00,
        "byte_array_jagged_ex_row1_len_2byte",
        "D_BYTE_ARRAY_JAGGED_EX row1 length as 2-byte",
        _counter) && result;

    // test 3: 4-byte row count
    // size = 4 + (1 + 2) + (1 + 2) = 10
    result = d_assert_standalone(
        sizeof(arr3) == 10,
        "byte_array_jagged_ex_4_1_1_size",
        "D_BYTE_ARRAY_JAGGED_EX(4,1,1) size correct",
        _counter) && result;

    // row count = 2 as 4-byte: {0x02, 0x00, 0x00, 0x00}
    result = d_assert_standalone(
        arr3[0] == 0x02 && arr3[1] == 0x00 &&
        arr3[2] == 0x00 && arr3[3] == 0x00,
        "byte_array_jagged_ex_4byte_nrows",
        "D_BYTE_ARRAY_JAGGED_EX row count as 4-byte",
        _counter) && result;

    // row 0 length = 2 as 1-byte at offset 4
    result = d_assert_standalone(
        arr3[4] == 0x02,
        "byte_array_jagged_ex_1byte_len_offset",
        "D_BYTE_ARRAY_JAGGED_EX row length at correct offset",
        _counter) && result;

    // row 0 data at offset 5
    result = d_assert_standalone(
        arr3[5] == 0xAA && arr3[6] == 0xBB,
        "byte_array_jagged_ex_row0_data",
        "D_BYTE_ARRAY_JAGGED_EX row0 data correct",
        _counter) && result;

    // row 1 length = 2 as 1-byte at offset 7
    result = d_assert_standalone(
        arr3[7] == 0x02,
        "byte_array_jagged_ex_row1_len",
        "D_BYTE_ARRAY_JAGGED_EX row1 length correct",
        _counter) && result;

    // row 1 data at offset 8
    result = d_assert_standalone(
        arr3[8] == 0xCC && arr3[9] == 0xDD,
        "byte_array_jagged_ex_row1_data",
        "D_BYTE_ARRAY_JAGGED_EX row1 data correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_jagged_all
    Aggregation function that runs all jagged array builder tests.
*/
bool
d_tests_sa_byte_array_jagged_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Jagged Array Builders\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_byte_array_jagged_basic(_counter) && result;
    result = d_tests_sa_byte_array_jagged_layout(_counter) && result;
    result = d_tests_sa_byte_array_jagged_ex(_counter) && result;

    return result;
}

#endif  // !_MSC_VER
