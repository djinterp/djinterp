/*******************************************************************************
* djinterp [test]                                  byte_array_tests_extraction.c
*
*   Unit tests for byte_array.h byte extraction macros.
*   Tests D_INTERNAL_BYTE_n_OF_m macros for extracting individual bytes from
*   multi-byte values in native (little-endian) byte order.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_extraction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./byte_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_byte_array_byte_0_of_1
    Tests the D_INTERNAL_BYTE_0_OF_1 macro for 1-byte extraction.
    Tests the following:
    - Extracts byte correctly from 1-byte value
    - Masks to 8 bits properly
    - Handles zero value
    - Handles max value (0xFF)
*/
bool d_tests_byte_array_byte_0_of_1(
    struct dj_test_suite* _suite
)
{
    bool    result;
    uint8_t byte0;

    d_test_suite_test(_suite, "byte_array_byte_0_of_1");

    result = true;

    // test 1: extract byte from simple value
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x42);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_1_simple",
                              byte0 == 0x42,
                              "D_INTERNAL_BYTE_0_OF_1(0x42) should return 0x42") && result;

    // test 2: extract byte from zero
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x00);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_1_zero",
                              byte0 == 0x00,
                              "D_INTERNAL_BYTE_0_OF_1(0x00) should return 0x00") && result;

    // test 3: extract byte from max value
    byte0  = D_INTERNAL_BYTE_0_OF_1(0xFF);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_1_max",
                              byte0 == 0xFF,
                              "D_INTERNAL_BYTE_0_OF_1(0xFF) should return 0xFF") && result;

    // test 4: mask truncates larger values
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x1234);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_1_truncate",
                              byte0 == 0x34,
                              "D_INTERNAL_BYTE_0_OF_1(0x1234) should mask to 0x34") && result;

    return result;
}


/*
d_tests_byte_array_bytes_of_2
    Tests the D_INTERNAL_BYTE_n_OF_2 macros for 2-byte extraction.
    Tests the following:
    - BYTE_0_OF_2 extracts low byte (little-endian)
    - BYTE_1_OF_2 extracts high byte (little-endian)
    - Correct extraction from various 16-bit values
*/
bool d_tests_byte_array_bytes_of_2(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t val;
    uint8_t  byte0;
    uint8_t  byte1;

    d_test_suite_test(_suite, "byte_array_bytes_of_2");

    result = true;

    // test 1: extract bytes from 0x1234 (little-endian: byte0=0x34, byte1=0x12)
    val    = 0x1234;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_2_1234",
                              byte0 == 0x34,
                              "D_INTERNAL_BYTE_0_OF_2(0x1234) should return 0x34 (low byte)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_1_of_2_1234",
                              byte1 == 0x12,
                              "D_INTERNAL_BYTE_1_OF_2(0x1234) should return 0x12 (high byte)") && result;

    // test 2: extract bytes from 0xABCD
    val    = 0xABCD;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_2_ABCD",
                              byte0 == 0xCD && byte1 == 0xAB,
                              "D_INTERNAL_BYTE_n_OF_2(0xABCD) should extract 0xCD, 0xAB") && result;

    // test 3: extract bytes from zero
    val    = 0x0000;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_2_zero",
                              byte0 == 0x00 && byte1 == 0x00,
                              "D_INTERNAL_BYTE_n_OF_2(0x0000) should extract 0x00, 0x00") && result;

    // test 4: extract bytes from max value
    val    = 0xFFFF;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_2_max",
                              byte0 == 0xFF && byte1 == 0xFF,
                              "D_INTERNAL_BYTE_n_OF_2(0xFFFF) should extract 0xFF, 0xFF") && result;

    // test 5: extract bytes from 0x00FF (only low byte set)
    val    = 0x00FF;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_2_00FF",
                              byte0 == 0xFF && byte1 == 0x00,
                              "D_INTERNAL_BYTE_n_OF_2(0x00FF) should extract 0xFF, 0x00") && result;

    return result;
}


/*
d_tests_byte_array_bytes_of_4
    Tests the D_INTERNAL_BYTE_n_OF_4 macros for 4-byte extraction.
    Tests the following:
    - BYTE_0_OF_4 through BYTE_3_OF_4 extract correct bytes
    - Little-endian ordering (byte 0 is lowest address)
    - Correct extraction from various 32-bit values
*/
bool d_tests_byte_array_bytes_of_4(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint32_t val;
    uint8_t  byte0;
    uint8_t  byte1;
    uint8_t  byte2;
    uint8_t  byte3;

    d_test_suite_test(_suite, "byte_array_bytes_of_4");

    result = true;

    // test 1: extract bytes from 0x12345678
    // little-endian: byte0=0x78, byte1=0x56, byte2=0x34, byte3=0x12
    val    = 0x12345678u;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_4_12345678",
                              byte0 == 0x78,
                              "D_INTERNAL_BYTE_0_OF_4(0x12345678) should return 0x78") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_1_of_4_12345678",
                              byte1 == 0x56,
                              "D_INTERNAL_BYTE_1_OF_4(0x12345678) should return 0x56") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_2_of_4_12345678",
                              byte2 == 0x34,
                              "D_INTERNAL_BYTE_2_OF_4(0x12345678) should return 0x34") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_3_of_4_12345678",
                              byte3 == 0x12,
                              "D_INTERNAL_BYTE_3_OF_4(0x12345678) should return 0x12") && result;

    // test 2: extract bytes from 0xDEADBEEF
    val    = 0xDEADBEEFu;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_4_DEADBEEF",
                              byte0 == 0xEF && byte1 == 0xBE && byte2 == 0xAD && byte3 == 0xDE,
                              "D_INTERNAL_BYTE_n_OF_4(0xDEADBEEF) extracts correctly") && result;

    // test 3: extract bytes from zero
    val    = 0x00000000u;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_4_zero",
                              byte0 == 0x00 && byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00,
                              "D_INTERNAL_BYTE_n_OF_4(0) should all return 0x00") && result;

    // test 4: extract bytes from max value
    val    = 0xFFFFFFFFu;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = D_TEST_CHECK_MSG(_suite,
                              "bytes_of_4_max",
                              byte0 == 0xFF && byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF,
                              "D_INTERNAL_BYTE_n_OF_4(0xFFFFFFFF) should all return 0xFF") && result;

    return result;
}


/*
d_tests_byte_array_bytes_of_8
    Tests the D_INTERNAL_BYTE_n_OF_8 macros for 8-byte extraction.
    Tests the following:
    - BYTE_0_OF_8 through BYTE_7_OF_8 extract correct bytes
    - Little-endian ordering
    - Correct extraction from various 64-bit values
*/
bool d_tests_byte_array_bytes_of_8(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint64_t val;
    uint8_t  bytes[8];

    d_test_suite_test(_suite, "byte_array_bytes_of_8");

    result = true;

    // test 1: extract bytes from 0x0102030405060708
    // little-endian: byte0=0x08, byte1=0x07, ..., byte7=0x01
    val      = 0x0102030405060708ull;
    bytes[0] = D_INTERNAL_BYTE_0_OF_8(val);
    bytes[1] = D_INTERNAL_BYTE_1_OF_8(val);
    bytes[2] = D_INTERNAL_BYTE_2_OF_8(val);
    bytes[3] = D_INTERNAL_BYTE_3_OF_8(val);
    bytes[4] = D_INTERNAL_BYTE_4_OF_8(val);
    bytes[5] = D_INTERNAL_BYTE_5_OF_8(val);
    bytes[6] = D_INTERNAL_BYTE_6_OF_8(val);
    bytes[7] = D_INTERNAL_BYTE_7_OF_8(val);

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_0_of_8",
                              bytes[0] == 0x08,
                              "D_INTERNAL_BYTE_0_OF_8(0x0102030405060708) should return 0x08") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_1_of_8",
                              bytes[1] == 0x07,
                              "D_INTERNAL_BYTE_1_OF_8(0x0102030405060708) should return 0x07") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_2_of_8",
                              bytes[2] == 0x06,
                              "D_INTERNAL_BYTE_2_OF_8(0x0102030405060708) should return 0x06") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_3_of_8",
                              bytes[3] == 0x05,
                              "D_INTERNAL_BYTE_3_OF_8(0x0102030405060708) should return 0x05") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_4_of_8",
                              bytes[4] == 0x04,
                              "D_INTERNAL_BYTE_4_OF_8(0x0102030405060708) should return 0x04") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_5_of_8",
                              bytes[5] == 0x03,
                              "D_INTERNAL_BYTE_5_OF_8(0x0102030405060708) should return 0x03") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_6_of_8",
                              bytes[6] == 0x02,
                              "D_INTERNAL_BYTE_6_OF_8(0x0102030405060708) should return 0x02") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "byte_7_of_8",
                              bytes[7] == 0x01,
                              "D_INTERNAL_BYTE_7_OF_8(0x0102030405060708) should return 0x01") && result;

    // test 2: extract bytes from zero
    val      = 0x0000000000000000ull;
    bytes[0] = D_INTERNAL_BYTE_0_OF_8(val);
    bytes[7] = D_INTERNAL_BYTE_7_OF_8(val);
    result   = D_TEST_CHECK_MSG(_suite,
                                "bytes_of_8_zero",
                                bytes[0] == 0x00 && bytes[7] == 0x00,
                                "D_INTERNAL_BYTE_n_OF_8(0) should return 0x00") && result;

    // test 3: extract bytes from max value
    val      = 0xFFFFFFFFFFFFFFFFull;
    bytes[0] = D_INTERNAL_BYTE_0_OF_8(val);
    bytes[7] = D_INTERNAL_BYTE_7_OF_8(val);
    result   = D_TEST_CHECK_MSG(_suite,
                                "bytes_of_8_max",
                                bytes[0] == 0xFF && bytes[7] == 0xFF,
                                "D_INTERNAL_BYTE_n_OF_8(max) should return 0xFF") && result;

    return result;
}


/*
d_tests_byte_array_extraction_all
    Aggregation function that runs all byte extraction tests.
*/
bool d_tests_byte_array_extraction_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "byte_array_extraction_all");

    result = true;

    result = d_tests_byte_array_byte_0_of_1(_suite) && result;
    result = d_tests_byte_array_bytes_of_2(_suite) && result;
    result = d_tests_byte_array_bytes_of_4(_suite) && result;
    result = d_tests_byte_array_bytes_of_8(_suite) && result;

    return result;
}
