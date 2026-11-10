/*******************************************************************************
* djinterp [test]                                   byte_array_tests_expansion.c
*
*   Unit tests for byte_array.h byte expansion macros.
*   Tests D_BYTES_1, D_BYTES_2, D_BYTES_4, D_BYTES_8, and D_BYTES_N macros
*   for expanding values into byte sequences.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_expansion.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./byte_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_byte_array_bytes_1
    Tests the D_BYTES_1 macro for 1-byte expansion.
    Tests the following:
    - Expands single byte correctly
    - Can be used in array initializers
    - Handles various values
*/
bool d_tests_byte_array_bytes_1(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t arr1[] = { D_BYTES_1(0x42) };
    uint8_t arr2[] = { D_BYTES_1(0x00) };
    uint8_t arr3[] = { D_BYTES_1(0xFF) };

    d_test_suite_test(_suite, "byte_array_bytes_1");

    result = true;

    // test 1: D_BYTES_1 expands simple value
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_1_simple",
                              sizeof(arr1) == 1 && arr1[0] == 0x42,
                              "D_BYTES_1(0x42) should expand to single byte 0x42") && result;

    // test 2: D_BYTES_1 expands zero
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_1_zero",
                              sizeof(arr2) == 1 && arr2[0] == 0x00,
                              "D_BYTES_1(0x00) should expand to single byte 0x00") && result;

    // test 3: D_BYTES_1 expands max value
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_1_max",
                              sizeof(arr3) == 1 && arr3[0] == 0xFF,
                              "D_BYTES_1(0xFF) should expand to single byte 0xFF") && result;

    return result;
}


/*
d_tests_byte_array_bytes_2
    Tests the D_BYTES_2 macro for 2-byte expansion.
    Tests the following:
    - Expands to 2 bytes in correct order (little-endian)
    - Can be used in array initializers
    - Handles various 16-bit values
*/
bool d_tests_byte_array_bytes_2(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t arr1[] = { D_BYTES_2(0x1234) };
    uint8_t arr2[] = { D_BYTES_2(0xABCD) };
    uint8_t arr3[] = { D_BYTES_2(0x0000) };
    uint8_t arr4[] = { D_BYTES_2(0xFFFF) };

    d_test_suite_test(_suite, "byte_array_bytes_2");

    result = true;

    // test 1: D_BYTES_2 expands 0x1234 (little-endian: 0x34, 0x12)
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_2_size",
                              sizeof(arr1) == 2,
                              "D_BYTES_2 should expand to 2 bytes") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_2_1234",
                              arr1[0] == 0x34 && arr1[1] == 0x12,
                              "D_BYTES_2(0x1234) should expand to {0x34, 0x12}") && result;

    // test 2: D_BYTES_2 expands 0xABCD
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_2_ABCD",
                              arr2[0] == 0xCD && arr2[1] == 0xAB,
                              "D_BYTES_2(0xABCD) should expand to {0xCD, 0xAB}") && result;

    // test 3: D_BYTES_2 expands zero
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_2_zero",
                              arr3[0] == 0x00 && arr3[1] == 0x00,
                              "D_BYTES_2(0x0000) should expand to {0x00, 0x00}") && result;

    // test 4: D_BYTES_2 expands max value
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_2_max",
                              arr4[0] == 0xFF && arr4[1] == 0xFF,
                              "D_BYTES_2(0xFFFF) should expand to {0xFF, 0xFF}") && result;

    return result;
}


/*
d_tests_byte_array_bytes_4
    Tests the D_BYTES_4 macro for 4-byte expansion.
    Tests the following:
    - Expands to 4 bytes in correct order (little-endian)
    - Can be used in array initializers
    - Handles various 32-bit values
*/
bool d_tests_byte_array_bytes_4(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t arr1[] = { D_BYTES_4(0x12345678u) };
    uint8_t arr2[] = { D_BYTES_4(0xDEADBEEFu) };
    uint8_t arr3[] = { D_BYTES_4(0x00000000u) };
    uint8_t arr4[] = { D_BYTES_4(0xFFFFFFFFu) };

    d_test_suite_test(_suite, "byte_array_bytes_4");

    result = true;

    // test 1: D_BYTES_4 expands to 4 bytes
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_4_size",
                              sizeof(arr1) == 4,
                              "D_BYTES_4 should expand to 4 bytes") && result;

    // test 2: D_BYTES_4 expands 0x12345678 (little-endian)
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_4_12345678",
                              arr1[0] == 0x78 && arr1[1] == 0x56 && arr1[2] == 0x34 && arr1[3] == 0x12,
                              "D_BYTES_4(0x12345678) should expand to {0x78,0x56,0x34,0x12}") && result;

    // test 3: D_BYTES_4 expands 0xDEADBEEF
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_4_DEADBEEF",
                              arr2[0] == 0xEF && arr2[1] == 0xBE && arr2[2] == 0xAD && arr2[3] == 0xDE,
                              "D_BYTES_4(0xDEADBEEF) should expand to {0xEF,0xBE,0xAD,0xDE}") && result;

    // test 4: D_BYTES_4 expands zero
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_4_zero",
                              arr3[0] == 0x00 && arr3[1] == 0x00 && arr3[2] == 0x00 && arr3[3] == 0x00,
                              "D_BYTES_4(0) should expand to {0x00,0x00,0x00,0x00}") && result;

    // test 5: D_BYTES_4 expands max value
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_4_max",
                              arr4[0] == 0xFF && arr4[1] == 0xFF && arr4[2] == 0xFF && arr4[3] == 0xFF,
                              "D_BYTES_4(max) should expand to {0xFF,0xFF,0xFF,0xFF}") && result;

    return result;
}


/*
d_tests_byte_array_bytes_8
    Tests the D_BYTES_8 macro for 8-byte expansion.
    Tests the following:
    - Expands to 8 bytes in correct order (little-endian)
    - Can be used in array initializers
    - Handles various 64-bit values
*/
bool d_tests_byte_array_bytes_8(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t arr1[] = { D_BYTES_8(0x0102030405060708ull) };
    uint8_t arr2[] = { D_BYTES_8(0x0000000000000000ull) };
    uint8_t arr3[] = { D_BYTES_8(0xFFFFFFFFFFFFFFFFull) };

    d_test_suite_test(_suite, "byte_array_bytes_8");

    result = true;

    // test 1: D_BYTES_8 expands to 8 bytes
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_8_size",
                              sizeof(arr1) == 8,
                              "D_BYTES_8 should expand to 8 bytes") && result;

    // test 2: D_BYTES_8 expands 0x0102030405060708 (little-endian)
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_8_sequence",
                              arr1[0] == 0x08 && arr1[1] == 0x07 && arr1[2] == 0x06 && arr1[3] == 0x05 && arr1[4] == 0x04 && arr1[5] == 0x03 && arr1[6] == 0x02 && arr1[7] == 0x01,
                              "D_BYTES_8(0x0102030405060708) little-endian expansion") && result;

    // test 3: D_BYTES_8 expands zero
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_8_zero",
                              arr2[0] == 0x00 && arr2[7] == 0x00,
                              "D_BYTES_8(0) should expand to all zeros") && result;

    // test 4: D_BYTES_8 expands max value
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_8_max",
                              arr3[0] == 0xFF && arr3[7] == 0xFF,
                              "D_BYTES_8(max) should expand to all 0xFF") && result;

    return result;
}


/*
d_tests_byte_array_bytes_n
    Tests the D_BYTES_N dispatcher macro.
    Tests the following:
    - D_BYTES_N(1, x) dispatches to D_BYTES_1
    - D_BYTES_N(2, x) dispatches to D_BYTES_2
    - D_BYTES_N(4, x) dispatches to D_BYTES_4
    - D_BYTES_N(8, x) dispatches to D_BYTES_8
*/
bool d_tests_byte_array_bytes_n(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t arr1[] = { D_BYTES_N(1, 0x42) };
    uint8_t arr2[] = { D_BYTES_N(2, 0x1234) };
    uint8_t arr4[] = { D_BYTES_N(4, 0x12345678u) };
    uint8_t arr8[] = { D_BYTES_N(8, 0x0102030405060708ull) };

    d_test_suite_test(_suite, "byte_array_bytes_n");

    result = true;

    // test 1: D_BYTES_N(1, x) dispatches correctly
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_n_1",
                              sizeof(arr1) == 1 && arr1[0] == 0x42,
                              "D_BYTES_N(1, 0x42) should expand to single byte") && result;

    // test 2: D_BYTES_N(2, x) dispatches correctly
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_n_2",
                              sizeof(arr2) == 2 && arr2[0] == 0x34 && arr2[1] == 0x12,
                              "D_BYTES_N(2, 0x1234) should expand to 2 bytes") && result;

    // test 3: D_BYTES_N(4, x) dispatches correctly
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_n_4",
                              sizeof(arr4) == 4 && arr4[0] == 0x78 && arr4[1] == 0x56 && arr4[2] == 0x34 && arr4[3] == 0x12,
                              "D_BYTES_N(4, x) should expand to 4 bytes") && result;

    // test 4: D_BYTES_N(8, x) dispatches correctly
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_n_8",
                              sizeof(arr8) == 8 && arr8[0] == 0x08 && arr8[7] == 0x01,
                              "D_BYTES_N(8, x) should expand to 8 bytes") && result;

    return result;
}


/*
d_tests_byte_array_expansion_all
    Aggregation function that runs all byte expansion tests.
*/
bool d_tests_byte_array_expansion_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "byte_array_expansion_all");

    result = true;

    result = d_tests_byte_array_bytes_1(_suite) && result;
    result = d_tests_byte_array_bytes_2(_suite) && result;
    result = d_tests_byte_array_bytes_4(_suite) && result;
    result = d_tests_byte_array_bytes_8(_suite) && result;
    result = d_tests_byte_array_bytes_n(_suite) && result;

    return result;
}
