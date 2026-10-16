/*******************************************************************************
* djinterp [test]                                       byte_array_tests_sa_1d.c
*
*   Unit tests for byte_array.h 1D array builder macros.
*   Tests D_BYTE_ARRAY, D_BYTE_ARRAY_INIT, D_BYTE_ARRAY_n_m variants, and
*   D_BYTE_ARRAY_CUSTOM macros.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_sa_1d.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./byte_array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_byte_array_default
    Tests the D_BYTE_ARRAY macro (default 1-byte count, 1-byte elements).
    Tests the following:
    - Creates array with count prefix
    - Count is stored as 1 byte
    - Elements are stored as 1 byte each
    - Layout: [count:1][elem0:1][elem1:1]...
*/
bool
d_tests_sa_byte_array_default
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr1[] = D_BYTE_ARRAY(0x10, 0x20, 0x30);
    uint8_t arr2[] = D_BYTE_ARRAY(0xEE, 0xFF);
    uint8_t arr3[] = D_BYTE_ARRAY(1, 2, 3, 4, 5);

    result = true;

    // test 1: array size is count + elements (1 + 3 = 4)
    result = d_assert_standalone(
        sizeof(arr1) == 4,
        "byte_array_default_size",
        "D_BYTE_ARRAY(3 elems) should have size 4 (1 count + 3 elements)",
        _counter) && result;

    // test 2: count is stored in first byte
    result = d_assert_standalone(
        arr1[0] == 3,
        "byte_array_default_count",
        "D_BYTE_ARRAY count byte should be 3",
        _counter) && result;

    // test 3: elements follow count
    result = d_assert_standalone(
        arr1[1] == 0x10 && arr1[2] == 0x20 && arr1[3] == 0x30,
        "byte_array_default_elements",
        "D_BYTE_ARRAY elements should follow count byte",
        _counter) && result;

    // test 4: two element array
    result = d_assert_standalone(
        sizeof(arr2) == 3 && arr2[0] == 2 && arr2[1] == 0xEE && arr2[2] == 0xFF,
        "byte_array_default_two",
        "D_BYTE_ARRAY two elements: {count=2, elem=0xEE, elem=0xFF}",
        _counter) && result;

    // test 5: five elements
    result = d_assert_standalone(
        sizeof(arr3) == 6 && arr3[0] == 5,
        "byte_array_default_five",
        "D_BYTE_ARRAY(5 elems) should have count=5",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_init
    Tests the D_BYTE_ARRAY_INIT macro (no outer braces).
    Tests the following:
    - Produces initializer without braces
    - Can be embedded in larger structures
    - Same layout as D_BYTE_ARRAY
*/
bool
d_tests_sa_byte_array_init
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr[] = { D_BYTE_ARRAY_INIT(0xAA, 0xBB, 0xCC) };
    uint8_t combined[] = { 0x00, D_BYTE_ARRAY_INIT(1, 2), 0xFF };

    result = true;

    // test 1: D_BYTE_ARRAY_INIT produces same layout
    result = d_assert_standalone(
        sizeof(arr) == 4 && arr[0] == 3,
        "byte_array_init_layout",
        "D_BYTE_ARRAY_INIT produces [count][elements] layout",
        _counter) && result;

    // test 2: elements correct
    result = d_assert_standalone(
        arr[1] == 0xAA && arr[2] == 0xBB && arr[3] == 0xCC,
        "byte_array_init_elements",
        "D_BYTE_ARRAY_INIT elements are correct",
        _counter) && result;

    // test 3: can be embedded in larger array
    result = d_assert_standalone(
        sizeof(combined) == 5,
        "byte_array_init_embedded_size",
        "D_BYTE_ARRAY_INIT can be embedded in larger initializer",
        _counter) && result;

    // test 4: embedded layout correct (0x00, [count=2, 1, 2], 0xFF)
    result = d_assert_standalone(
        combined[0] == 0x00 && combined[1] == 2 &&
        combined[2] == 1 && combined[3] == 2 && combined[4] == 0xFF,
        "byte_array_init_embedded_values",
        "D_BYTE_ARRAY_INIT embedded values correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_1_x
    Tests D_BYTE_ARRAY_1_1, D_BYTE_ARRAY_1_2, D_BYTE_ARRAY_1_4, D_BYTE_ARRAY_1_8.
    Tests the following:
    - 1-byte count prefix for all variants
    - Element sizes of 1, 2, 4, and 8 bytes
    - Correct total array sizes
*/
bool
d_tests_sa_byte_array_1_x
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr1_1[] = D_BYTE_ARRAY_1_1(0xAA, 0xBB);
    uint8_t arr1_2[] = D_BYTE_ARRAY_1_2(0x1234, 0x5678);
    uint8_t arr1_4[] = D_BYTE_ARRAY_1_4(0x12345678u, 0xDEADBEEFu);
    uint8_t arr1_8[] = D_BYTE_ARRAY_1_8(0x0102030405060708ull, 0xFFFFFFFFFFFFFFFFull);

    result = true;

    // test 1: D_BYTE_ARRAY_1_1 (1-byte count, 1-byte elements)
    result = d_assert_standalone(
        sizeof(arr1_1) == 3 && arr1_1[0] == 2,
        "byte_array_1_1_layout",
        "D_BYTE_ARRAY_1_1: 1-byte count + 1-byte elements",
        _counter) && result;

    result = d_assert_standalone(
        arr1_1[1] == 0xAA && arr1_1[2] == 0xBB,
        "byte_array_1_1_elements",
        "D_BYTE_ARRAY_1_1 elements correct",
        _counter) && result;

    // test 2: D_BYTE_ARRAY_1_2 (1-byte count, 2-byte elements)
    // size = 1 (count) + 2*2 (elements) = 5
    result = d_assert_standalone(
        sizeof(arr1_2) == 5 && arr1_2[0] == 2,
        "byte_array_1_2_layout",
        "D_BYTE_ARRAY_1_2: 1-byte count + 2-byte elements",
        _counter) && result;

    // first 2-byte element: 0x1234 -> {0x34, 0x12} (little-endian)
    result = d_assert_standalone(
        arr1_2[1] == 0x34 && arr1_2[2] == 0x12,
        "byte_array_1_2_elem0",
        "D_BYTE_ARRAY_1_2 first element little-endian",
        _counter) && result;

    // second 2-byte element: 0x5678 -> {0x78, 0x56}
    result = d_assert_standalone(
        arr1_2[3] == 0x78 && arr1_2[4] == 0x56,
        "byte_array_1_2_elem1",
        "D_BYTE_ARRAY_1_2 second element little-endian",
        _counter) && result;

    // test 3: D_BYTE_ARRAY_1_4 (1-byte count, 4-byte elements)
    // size = 1 (count) + 2*4 (elements) = 9
    result = d_assert_standalone(
        sizeof(arr1_4) == 9 && arr1_4[0] == 2,
        "byte_array_1_4_layout",
        "D_BYTE_ARRAY_1_4: 1-byte count + 4-byte elements",
        _counter) && result;

    // first 4-byte element: 0x12345678 -> {0x78, 0x56, 0x34, 0x12}
    result = d_assert_standalone(
        arr1_4[1] == 0x78 && arr1_4[2] == 0x56 &&
        arr1_4[3] == 0x34 && arr1_4[4] == 0x12,
        "byte_array_1_4_element",
        "D_BYTE_ARRAY_1_4 element little-endian",
        _counter) && result;

    // test 4: D_BYTE_ARRAY_1_8 (1-byte count, 8-byte elements)
    // size = 1 (count) + 2*8 (elements) = 17
    result = d_assert_standalone(
        sizeof(arr1_8) == 17 && arr1_8[0] == 2,
        "byte_array_1_8_layout",
        "D_BYTE_ARRAY_1_8: 1-byte count + 8-byte elements",
        _counter) && result;

    // first byte of first 8-byte element
    result = d_assert_standalone(
        arr1_8[1] == 0x08 && arr1_8[8] == 0x01,
        "byte_array_1_8_element",
        "D_BYTE_ARRAY_1_8 element little-endian",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_2_x
    Tests D_BYTE_ARRAY_2_1, D_BYTE_ARRAY_2_2, D_BYTE_ARRAY_2_4, D_BYTE_ARRAY_2_8.
    Tests the following:
    - 2-byte count prefix for all variants
    - Various element sizes
*/
bool
d_tests_sa_byte_array_2_x
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr2_1[] = D_BYTE_ARRAY_2_1(0xAA, 0xBB, 0xCC);
    uint8_t arr2_2[] = D_BYTE_ARRAY_2_2(0x1234, 0x5678);
    uint8_t arr2_4[] = D_BYTE_ARRAY_2_4(0xDEADBEEFu, 0x12345678u);

    result = true;

    // test 1: D_BYTE_ARRAY_2_1 (2-byte count, 1-byte elements)
    // size = 2 (count) + 3*1 (elements) = 5
    result = d_assert_standalone(
        sizeof(arr2_1) == 5,
        "byte_array_2_1_size",
        "D_BYTE_ARRAY_2_1: 2-byte count + 1-byte elements",
        _counter) && result;

    // count = 3, stored as 2 bytes little-endian: {0x03, 0x00}
    result = d_assert_standalone(
        arr2_1[0] == 0x03 && arr2_1[1] == 0x00,
        "byte_array_2_1_count",
        "D_BYTE_ARRAY_2_1 count as 2-byte little-endian",
        _counter) && result;

    // elements follow
    result = d_assert_standalone(
        arr2_1[2] == 0xAA && arr2_1[3] == 0xBB && arr2_1[4] == 0xCC,
        "byte_array_2_1_elements",
        "D_BYTE_ARRAY_2_1 elements correct",
        _counter) && result;

    // test 2: D_BYTE_ARRAY_2_2 (2-byte count, 2-byte elements)
    // size = 2 (count) + 2*2 (elements) = 6
    result = d_assert_standalone(
        sizeof(arr2_2) == 6,
        "byte_array_2_2_size",
        "D_BYTE_ARRAY_2_2: 2-byte count + 2-byte elements",
        _counter) && result;

    // count = 2 as 2-byte: {0x02, 0x00}
    result = d_assert_standalone(
        arr2_2[0] == 0x02 && arr2_2[1] == 0x00,
        "byte_array_2_2_count",
        "D_BYTE_ARRAY_2_2 count correct",
        _counter) && result;

    // test 3: D_BYTE_ARRAY_2_4 (2-byte count, 4-byte elements)
    // size = 2 (count) + 2*4 (elements) = 10
    result = d_assert_standalone(
        sizeof(arr2_4) == 10,
        "byte_array_2_4_size",
        "D_BYTE_ARRAY_2_4: 2-byte count + 4-byte elements",
        _counter) && result;

    // count = 2 as 2-byte: {0x02, 0x00}
    result = d_assert_standalone(
        arr2_4[0] == 0x02 && arr2_4[1] == 0x00,
        "byte_array_2_4_count",
        "D_BYTE_ARRAY_2_4 count correct",
        _counter) && result;

    // first element: 0xDEADBEEF -> {0xEF, 0xBE, 0xAD, 0xDE}
    result = d_assert_standalone(
        arr2_4[2] == 0xEF && arr2_4[3] == 0xBE &&
        arr2_4[4] == 0xAD && arr2_4[5] == 0xDE,
        "byte_array_2_4_element",
        "D_BYTE_ARRAY_2_4 element little-endian",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_4_x
    Tests D_BYTE_ARRAY_4_1, D_BYTE_ARRAY_4_2, D_BYTE_ARRAY_4_4, D_BYTE_ARRAY_4_8.
    Tests the following:
    - 4-byte count prefix for all variants
    - Various element sizes
*/
bool
d_tests_sa_byte_array_4_x
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr4_1[] = D_BYTE_ARRAY_4_1(0x11, 0x22);
    uint8_t arr4_4[] = D_BYTE_ARRAY_4_4(0x12345678u, 0xDEADBEEFu);

    result = true;

    // test 1: D_BYTE_ARRAY_4_1 (4-byte count, 1-byte elements)
    // size = 4 (count) + 2*1 (elements) = 6
    result = d_assert_standalone(
        sizeof(arr4_1) == 6,
        "byte_array_4_1_size",
        "D_BYTE_ARRAY_4_1: 4-byte count + 1-byte elements",
        _counter) && result;

    // count = 2 as 4-byte: {0x02, 0x00, 0x00, 0x00}
    result = d_assert_standalone(
        arr4_1[0] == 0x02 && arr4_1[1] == 0x00 &&
        arr4_1[2] == 0x00 && arr4_1[3] == 0x00,
        "byte_array_4_1_count",
        "D_BYTE_ARRAY_4_1 count as 4-byte little-endian",
        _counter) && result;

    // elements follow
    result = d_assert_standalone(
        arr4_1[4] == 0x11 && arr4_1[5] == 0x22,
        "byte_array_4_1_elements",
        "D_BYTE_ARRAY_4_1 elements correct",
        _counter) && result;

    // test 2: D_BYTE_ARRAY_4_4 (4-byte count, 4-byte elements)
    // size = 4 (count) + 2*4 (elements) = 12
    result = d_assert_standalone(
        sizeof(arr4_4) == 12,
        "byte_array_4_4_size",
        "D_BYTE_ARRAY_4_4: 4-byte count + 4-byte elements",
        _counter) && result;

    // count = 2 as 4-byte
    result = d_assert_standalone(
        arr4_4[0] == 0x02 && arr4_4[1] == 0x00 &&
        arr4_4[2] == 0x00 && arr4_4[3] == 0x00,
        "byte_array_4_4_count",
        "D_BYTE_ARRAY_4_4 count correct",
        _counter) && result;

    // first element: 0x12345678 at offset 4
    result = d_assert_standalone(
        arr4_4[4] == 0x78 && arr4_4[5] == 0x56 &&
        arr4_4[6] == 0x34 && arr4_4[7] == 0x12,
        "byte_array_4_4_elem0",
        "D_BYTE_ARRAY_4_4 first element correct",
        _counter) && result;

    // second element: 0xDEADBEEF at offset 8
    result = d_assert_standalone(
        arr4_4[8] == 0xEF && arr4_4[9] == 0xBE &&
        arr4_4[10] == 0xAD && arr4_4[11] == 0xDE,
        "byte_array_4_4_elem1",
        "D_BYTE_ARRAY_4_4 second element correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_8_x
    Tests D_BYTE_ARRAY_8_1, D_BYTE_ARRAY_8_2, D_BYTE_ARRAY_8_4, D_BYTE_ARRAY_8_8.
    Tests the following:
    - 8-byte count prefix for all variants
    - Various element sizes

    NOTE: On MSVC, the 8-byte count macros cause C4293 warnings because
    shifting small integer literals by 32+ bits is undefined behavior.
    These tests are skipped on MSVC.
*/
#if !defined(_MSC_VER)
bool
d_tests_sa_byte_array_8_x
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr8_1[] = D_BYTE_ARRAY_8_1(0xAA, 0xBB);
    uint8_t arr8_8[] = D_BYTE_ARRAY_8_8(0x0102030405060708ull, 0xFFFFFFFFFFFFFFFFull);

    result = true;

    // test 1: D_BYTE_ARRAY_8_1 (8-byte count, 1-byte elements)
    // size = 8 (count) + 2*1 (elements) = 10
    result = d_assert_standalone(
        sizeof(arr8_1) == 10,
        "byte_array_8_1_size",
        "D_BYTE_ARRAY_8_1: 8-byte count + 1-byte elements",
        _counter) && result;

    // count = 2 as 8-byte: {0x02, 0x00, ...}
    result = d_assert_standalone(
        arr8_1[0] == 0x02 && arr8_1[1] == 0x00 &&
        arr8_1[7] == 0x00,
        "byte_array_8_1_count",
        "D_BYTE_ARRAY_8_1 count as 8-byte little-endian",
        _counter) && result;

    // elements at offset 8
    result = d_assert_standalone(
        arr8_1[8] == 0xAA && arr8_1[9] == 0xBB,
        "byte_array_8_1_element",
        "D_BYTE_ARRAY_8_1 elements correct",
        _counter) && result;

    // test 2: D_BYTE_ARRAY_8_8 (8-byte count, 8-byte elements)
    // size = 8 (count) + 2*8 (elements) = 24
    result = d_assert_standalone(
        sizeof(arr8_8) == 24,
        "byte_array_8_8_size",
        "D_BYTE_ARRAY_8_8: 8-byte count + 8-byte elements",
        _counter) && result;

    // count = 2 as 8-byte
    result = d_assert_standalone(
        arr8_8[0] == 0x02 && arr8_8[7] == 0x00,
        "byte_array_8_8_count",
        "D_BYTE_ARRAY_8_8 count correct",
        _counter) && result;

    // first element at offset 8: 0x0102030405060708 little-endian
    result = d_assert_standalone(
        arr8_8[8] == 0x08 && arr8_8[15] == 0x01,
        "byte_array_8_8_element",
        "D_BYTE_ARRAY_8_8 element little-endian",
        _counter) && result;

    return result;
}
#endif  // !_MSC_VER


/*
d_tests_sa_byte_array_custom
    Tests the D_BYTE_ARRAY_CUSTOM macro.
    Tests the following:
    - Allows arbitrary count_size and element_size
    - Produces correct layout
*/
bool
d_tests_sa_byte_array_custom
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t arr_c1[] = D_BYTE_ARRAY_CUSTOM(1, 2, 0x1234, 0x5678);
    uint8_t arr_c2[] = D_BYTE_ARRAY_CUSTOM(2, 4, 0xDEADBEEFu, 0x12345678u);
    uint8_t arr_c3[] = D_BYTE_ARRAY_CUSTOM(4, 1, 0xAA, 0xBB, 0xCC);

    result = true;

    // test 1: D_BYTE_ARRAY_CUSTOM(1, 2, ...) = 1-byte count, 2-byte elements
    // size = 1 + 2*2 = 5
    result = d_assert_standalone(
        sizeof(arr_c1) == 5,
        "byte_array_custom_1_2_size",
        "D_BYTE_ARRAY_CUSTOM(1, 2, 2 elems) size = 5",
        _counter) && result;

    result = d_assert_standalone(
        arr_c1[0] == 2,
        "byte_array_custom_1_2_count",
        "D_BYTE_ARRAY_CUSTOM(1, 2) count correct",
        _counter) && result;

    // test 2: D_BYTE_ARRAY_CUSTOM(2, 4, ...) = 2-byte count, 4-byte elements
    // size = 2 + 2*4 = 10
    result = d_assert_standalone(
        sizeof(arr_c2) == 10,
        "byte_array_custom_2_4_size",
        "D_BYTE_ARRAY_CUSTOM(2, 4, 2 elems) size = 10",
        _counter) && result;

    result = d_assert_standalone(
        arr_c2[0] == 0x02 && arr_c2[1] == 0x00,
        "byte_array_custom_2_4_count",
        "D_BYTE_ARRAY_CUSTOM(2, 4) count as 2-byte",
        _counter) && result;

    // test 3: D_BYTE_ARRAY_CUSTOM(4, 1, ...) = 4-byte count, 1-byte elements
    // size = 4 + 3*1 = 7
    result = d_assert_standalone(
        sizeof(arr_c3) == 7,
        "byte_array_custom_4_1_size",
        "D_BYTE_ARRAY_CUSTOM(4, 1, 3 elems) size = 7",
        _counter) && result;

    result = d_assert_standalone(
        arr_c3[0] == 0x03 && arr_c3[1] == 0x00 &&
        arr_c3[2] == 0x00 && arr_c3[3] == 0x00,
        "byte_array_custom_4_1_count",
        "D_BYTE_ARRAY_CUSTOM(4, 1) count as 4-byte",
        _counter) && result;

    result = d_assert_standalone(
        arr_c3[4] == 0xAA && arr_c3[5] == 0xBB && arr_c3[6] == 0xCC,
        "byte_array_custom_4_1_elements",
        "D_BYTE_ARRAY_CUSTOM(4, 1) elements correct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_1d_all
    Aggregation function that runs all 1D array builder tests.
*/
bool
d_tests_sa_byte_array_1d_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] 1D Array Builders\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_byte_array_default(_counter) && result;
    result = d_tests_sa_byte_array_init(_counter) && result;
    result = d_tests_sa_byte_array_1_x(_counter) && result;
    result = d_tests_sa_byte_array_2_x(_counter) && result;
    result = d_tests_sa_byte_array_4_x(_counter) && result;
#if !defined(_MSC_VER)
    result = d_tests_sa_byte_array_8_x(_counter) && result;
#endif
    result = d_tests_sa_byte_array_custom(_counter) && result;

    return result;
}
