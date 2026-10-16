/*******************************************************************************
* djinterp [test]                               byte_array_tests_sa_extraction.c
*
*   Unit tests for byte_array.h byte extraction macros.
*   Tests D_INTERNAL_BYTE_n_OF_m macros for extracting individual bytes from
*   multi-byte values in native (little-endian) byte order.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_sa_extraction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./byte_array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_byte_array_byte_0_of_1
    Tests the D_INTERNAL_BYTE_0_OF_1 macro for 1-byte extraction.
    Tests the following:
    - Extracts byte correctly from 1-byte value
    - Masks to 8 bits properly
    - Handles zero value
    - Handles max value (0xFF)
*/
bool
d_tests_sa_byte_array_byte_0_of_1
(
    struct d_test_counter* _counter
)
{
    bool    result;
    uint8_t byte0;

    result = true;

    // test 1: extract byte from simple value
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x42);
    result = d_assert_standalone(
        byte0 == 0x42,
        "byte_0_of_1_simple",
        "D_INTERNAL_BYTE_0_OF_1(0x42) should return 0x42",
        _counter) && result;

    // test 2: extract byte from zero
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x00);
    result = d_assert_standalone(
        byte0 == 0x00,
        "byte_0_of_1_zero",
        "D_INTERNAL_BYTE_0_OF_1(0x00) should return 0x00",
        _counter) && result;

    // test 3: extract byte from max value
    byte0  = D_INTERNAL_BYTE_0_OF_1(0xFF);
    result = d_assert_standalone(
        byte0 == 0xFF,
        "byte_0_of_1_max",
        "D_INTERNAL_BYTE_0_OF_1(0xFF) should return 0xFF",
        _counter) && result;

    // test 4: mask truncates larger values
    byte0  = D_INTERNAL_BYTE_0_OF_1(0x1234);
    result = d_assert_standalone(
        byte0 == 0x34,
        "byte_0_of_1_truncate",
        "D_INTERNAL_BYTE_0_OF_1(0x1234) should mask to 0x34",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_bytes_of_2
    Tests the D_INTERNAL_BYTE_n_OF_2 macros for 2-byte extraction.
    Tests the following:
    - BYTE_0_OF_2 extracts low byte (little-endian)
    - BYTE_1_OF_2 extracts high byte (little-endian)
    - Correct extraction from various 16-bit values
*/
bool
d_tests_sa_byte_array_bytes_of_2
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t val;
    uint8_t  byte0;
    uint8_t  byte1;

    result = true;

    // test 1: extract bytes from 0x1234 (little-endian: byte0=0x34, byte1=0x12)
    val    = 0x1234;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = d_assert_standalone(
        byte0 == 0x34,
        "byte_0_of_2_1234",
        "D_INTERNAL_BYTE_0_OF_2(0x1234) should return 0x34 (low byte)",
        _counter) && result;

    result = d_assert_standalone(
        byte1 == 0x12,
        "byte_1_of_2_1234",
        "D_INTERNAL_BYTE_1_OF_2(0x1234) should return 0x12 (high byte)",
        _counter) && result;

    // test 2: extract bytes from 0xABCD
    val    = 0xABCD;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = d_assert_standalone(
        byte0 == 0xCD && byte1 == 0xAB,
        "bytes_of_2_ABCD",
        "D_INTERNAL_BYTE_n_OF_2(0xABCD) should extract 0xCD, 0xAB",
        _counter) && result;

    // test 3: extract bytes from zero
    val    = 0x0000;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = d_assert_standalone(
        byte0 == 0x00 && byte1 == 0x00,
        "bytes_of_2_zero",
        "D_INTERNAL_BYTE_n_OF_2(0x0000) should extract 0x00, 0x00",
        _counter) && result;

    // test 4: extract bytes from max value
    val    = 0xFFFF;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = d_assert_standalone(
        byte0 == 0xFF && byte1 == 0xFF,
        "bytes_of_2_max",
        "D_INTERNAL_BYTE_n_OF_2(0xFFFF) should extract 0xFF, 0xFF",
        _counter) && result;

    // test 5: extract bytes from 0x00FF (only low byte set)
    val    = 0x00FF;
    byte0  = D_INTERNAL_BYTE_0_OF_2(val);
    byte1  = D_INTERNAL_BYTE_1_OF_2(val);
    result = d_assert_standalone(
        byte0 == 0xFF && byte1 == 0x00,
        "bytes_of_2_00FF",
        "D_INTERNAL_BYTE_n_OF_2(0x00FF) should extract 0xFF, 0x00",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_bytes_of_4
    Tests the D_INTERNAL_BYTE_n_OF_4 macros for 4-byte extraction.
    Tests the following:
    - BYTE_0_OF_4 through BYTE_3_OF_4 extract correct bytes
    - Little-endian ordering (byte 0 is lowest address)
    - Correct extraction from various 32-bit values
*/
bool
d_tests_sa_byte_array_bytes_of_4
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t val;
    uint8_t  byte0;
    uint8_t  byte1;
    uint8_t  byte2;
    uint8_t  byte3;

    result = true;

    // test 1: extract bytes from 0x12345678
    // little-endian: byte0=0x78, byte1=0x56, byte2=0x34, byte3=0x12
    val    = 0x12345678u;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = d_assert_standalone(
        byte0 == 0x78,
        "byte_0_of_4_12345678",
        "D_INTERNAL_BYTE_0_OF_4(0x12345678) should return 0x78",
        _counter) && result;

    result = d_assert_standalone(
        byte1 == 0x56,
        "byte_1_of_4_12345678",
        "D_INTERNAL_BYTE_1_OF_4(0x12345678) should return 0x56",
        _counter) && result;

    result = d_assert_standalone(
        byte2 == 0x34,
        "byte_2_of_4_12345678",
        "D_INTERNAL_BYTE_2_OF_4(0x12345678) should return 0x34",
        _counter) && result;

    result = d_assert_standalone(
        byte3 == 0x12,
        "byte_3_of_4_12345678",
        "D_INTERNAL_BYTE_3_OF_4(0x12345678) should return 0x12",
        _counter) && result;

    // test 2: extract bytes from 0xDEADBEEF
    val    = 0xDEADBEEFu;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = d_assert_standalone(
        byte0 == 0xEF && byte1 == 0xBE && byte2 == 0xAD && byte3 == 0xDE,
        "bytes_of_4_DEADBEEF",
        "D_INTERNAL_BYTE_n_OF_4(0xDEADBEEF) extracts correctly",
        _counter) && result;

    // test 3: extract bytes from zero
    val    = 0x00000000u;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = d_assert_standalone(
        byte0 == 0x00 && byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00,
        "bytes_of_4_zero",
        "D_INTERNAL_BYTE_n_OF_4(0) should all return 0x00",
        _counter) && result;

    // test 4: extract bytes from max value
    val    = 0xFFFFFFFFu;
    byte0  = D_INTERNAL_BYTE_0_OF_4(val);
    byte1  = D_INTERNAL_BYTE_1_OF_4(val);
    byte2  = D_INTERNAL_BYTE_2_OF_4(val);
    byte3  = D_INTERNAL_BYTE_3_OF_4(val);
    result = d_assert_standalone(
        byte0 == 0xFF && byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF,
        "bytes_of_4_max",
        "D_INTERNAL_BYTE_n_OF_4(0xFFFFFFFF) should all return 0xFF",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_bytes_of_8
    Tests the D_INTERNAL_BYTE_n_OF_8 macros for 8-byte extraction.
    Tests the following:
    - BYTE_0_OF_8 through BYTE_7_OF_8 extract correct bytes
    - Little-endian ordering
    - Correct extraction from various 64-bit values
*/
bool
d_tests_sa_byte_array_bytes_of_8
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint64_t val;
    uint8_t  bytes[8];

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

    result = d_assert_standalone(
        bytes[0] == 0x08,
        "byte_0_of_8",
        "D_INTERNAL_BYTE_0_OF_8(0x0102030405060708) should return 0x08",
        _counter) && result;

    result = d_assert_standalone(
        bytes[1] == 0x07,
        "byte_1_of_8",
        "D_INTERNAL_BYTE_1_OF_8(0x0102030405060708) should return 0x07",
        _counter) && result;

    result = d_assert_standalone(
        bytes[2] == 0x06,
        "byte_2_of_8",
        "D_INTERNAL_BYTE_2_OF_8(0x0102030405060708) should return 0x06",
        _counter) && result;

    result = d_assert_standalone(
        bytes[3] == 0x05,
        "byte_3_of_8",
        "D_INTERNAL_BYTE_3_OF_8(0x0102030405060708) should return 0x05",
        _counter) && result;

    result = d_assert_standalone(
        bytes[4] == 0x04,
        "byte_4_of_8",
        "D_INTERNAL_BYTE_4_OF_8(0x0102030405060708) should return 0x04",
        _counter) && result;

    result = d_assert_standalone(
        bytes[5] == 0x03,
        "byte_5_of_8",
        "D_INTERNAL_BYTE_5_OF_8(0x0102030405060708) should return 0x03",
        _counter) && result;

    result = d_assert_standalone(
        bytes[6] == 0x02,
        "byte_6_of_8",
        "D_INTERNAL_BYTE_6_OF_8(0x0102030405060708) should return 0x02",
        _counter) && result;

    result = d_assert_standalone(
        bytes[7] == 0x01,
        "byte_7_of_8",
        "D_INTERNAL_BYTE_7_OF_8(0x0102030405060708) should return 0x01",
        _counter) && result;

    // test 2: extract bytes from zero
    val      = 0x0000000000000000ull;
    bytes[0] = D_INTERNAL_BYTE_0_OF_8(val);
    bytes[7] = D_INTERNAL_BYTE_7_OF_8(val);
    result   = d_assert_standalone(
        bytes[0] == 0x00 && bytes[7] == 0x00,
        "bytes_of_8_zero",
        "D_INTERNAL_BYTE_n_OF_8(0) should return 0x00",
        _counter) && result;

    // test 3: extract bytes from max value
    val      = 0xFFFFFFFFFFFFFFFFull;
    bytes[0] = D_INTERNAL_BYTE_0_OF_8(val);
    bytes[7] = D_INTERNAL_BYTE_7_OF_8(val);
    result   = d_assert_standalone(
        bytes[0] == 0xFF && bytes[7] == 0xFF,
        "bytes_of_8_max",
        "D_INTERNAL_BYTE_n_OF_8(max) should return 0xFF",
        _counter) && result;

    return result;
}


/*
d_tests_sa_byte_array_extraction_all
    Aggregation function that runs all byte extraction tests.
*/
bool
d_tests_sa_byte_array_extraction_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Byte Extraction Macros\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_byte_array_byte_0_of_1(_counter) && result;
    result = d_tests_sa_byte_array_bytes_of_2(_counter) && result;
    result = d_tests_sa_byte_array_bytes_of_4(_counter) && result;
    result = d_tests_sa_byte_array_bytes_of_8(_counter) && result;

    return result;
}
