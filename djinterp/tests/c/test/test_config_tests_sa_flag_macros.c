#include ".\test_config_tests_sa.h"


/******************************************************************************
 * I. FLAG MANIPULATION MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_config_settings_shift
  Tests the D_TEST_SETTINGS_SHIFT constant.
  Tests the following:
  - Value is 16
  - Shifting 1 by SETTINGS_SHIFT produces 0x00010000
*/
bool
d_tests_sa_config_settings_shift
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 16
    result = d_assert_standalone(
        D_TEST_SETTINGS_SHIFT == 16,
        "settings_shift_value",
        "D_TEST_SETTINGS_SHIFT should be 16",
        _counter) && result;

    // test 2: shifting 1 by SETTINGS_SHIFT gives bit 16
    result = d_assert_standalone(
        (1u << D_TEST_SETTINGS_SHIFT) == 0x00010000u,
        "settings_shift_bit_position",
        "1 << D_TEST_SETTINGS_SHIFT should be 0x00010000",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_settings_to_flags
  Tests the D_TEST_SETTINGS_TO_FLAGS macro.
  Tests the following:
  - Zero settings produce zero flags
  - Single setting flag is shifted to upper 16 bits
  - Multiple settings flags combine correctly
  - All four settings flags produce expected combined value
*/
bool
d_tests_sa_config_settings_to_flags
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t flags;

    result = true;

    // test 1: zero settings -> zero flags
    flags = D_TEST_SETTINGS_TO_FLAGS(0);

    result = d_assert_standalone(
        flags == 0,
        "to_flags_zero",
        "D_TEST_SETTINGS_TO_FLAGS(0) should be 0",
        _counter) && result;

    // test 2: STACK_PUSH_FAIL (0x01) shifts to bit 16
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL);

    result = d_assert_standalone(
        flags == 0x00010000u,
        "to_flags_push_fail",
        "STACK_PUSH_FAIL should shift to 0x00010000",
        _counter) && result;

    // test 3: STACK_PUSH_PASS (0x02) shifts to bit 17
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS);

    result = d_assert_standalone(
        flags == 0x00020000u,
        "to_flags_push_pass",
        "STACK_PUSH_PASS should shift to 0x00020000",
        _counter) && result;

    // test 4: STACK_PUSH_WARNING (0x04) shifts to bit 18
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING);

    result = d_assert_standalone(
        flags == 0x00040000u,
        "to_flags_push_warning",
        "STACK_PUSH_WARNING should shift to 0x00040000",
        _counter) && result;

    // test 5: STACK_PUSH_INFO (0x08) shifts to bit 19
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO);

    result = d_assert_standalone(
        flags == 0x00080000u,
        "to_flags_push_info",
        "STACK_PUSH_INFO should shift to 0x00080000",
        _counter) && result;

    // test 6: all four settings combined
    flags = D_TEST_SETTINGS_TO_FLAGS(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO);

    result = d_assert_standalone(
        flags == 0x000F0000u,
        "to_flags_all_combined",
        "All settings combined should be 0x000F0000",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_flags_to_settings
  Tests the D_TEST_FLAGS_TO_SETTINGS macro.
  Tests the following:
  - Zero flags produce zero settings
  - Packed flag in upper bits extracts to original setting value
  - Message-only flags (lower 16 bits) produce zero settings
  - Mixed flags extract only settings portion
*/
bool
d_tests_sa_config_flags_to_settings
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t settings;

    result = true;

    // test 1: zero flags -> zero settings
    settings = D_TEST_FLAGS_TO_SETTINGS(0);

    result = d_assert_standalone(
        settings == 0,
        "from_flags_zero",
        "D_TEST_FLAGS_TO_SETTINGS(0) should be 0",
        _counter) && result;

    // test 2: 0x00010000 extracts to STACK_PUSH_FAIL (0x01)
    settings = D_TEST_FLAGS_TO_SETTINGS(0x00010000u);

    result = d_assert_standalone(
        settings == D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL,
        "from_flags_push_fail",
        "0x00010000 should extract to STACK_PUSH_FAIL",
        _counter) && result;

    // test 3: message-only flags (lower 16 bits) produce zero
    settings = D_TEST_FLAGS_TO_SETTINGS(0x0000FFFFu);

    result = d_assert_standalone(
        settings == 0,
        "from_flags_message_only",
        "Lower 16 bits should produce zero settings",
        _counter) && result;

    // test 4: mixed flags extract only settings portion
    settings = D_TEST_FLAGS_TO_SETTINGS(0x000F00FFu);

    result = d_assert_standalone(
        settings == 0x0Fu,
        "from_flags_mixed",
        "Mixed flags should extract only upper portion",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_flag_roundtrip
  Tests round-trip conversion: TO_FLAGS -> FLAGS_TO_SETTINGS.
  Tests the following:
  - Each individual settings flag survives the round-trip
  - Combined settings flags survive the round-trip
*/
bool
d_tests_sa_config_flag_roundtrip
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t original;
    uint32_t recovered;

    result = true;

    // test 1: STACK_PUSH_FAIL round-trip
    original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL;
    recovered = D_TEST_FLAGS_TO_SETTINGS(
                    D_TEST_SETTINGS_TO_FLAGS(original));

    result = d_assert_standalone(
        recovered == original,
        "roundtrip_push_fail",
        "STACK_PUSH_FAIL should survive round-trip",
        _counter) && result;

    // test 2: STACK_PUSH_PASS round-trip
    original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS;
    recovered = D_TEST_FLAGS_TO_SETTINGS(
                    D_TEST_SETTINGS_TO_FLAGS(original));

    result = d_assert_standalone(
        recovered == original,
        "roundtrip_push_pass",
        "STACK_PUSH_PASS should survive round-trip",
        _counter) && result;

    // test 3: all settings combined round-trip
    original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO;
    recovered = D_TEST_FLAGS_TO_SETTINGS(
                    D_TEST_SETTINGS_TO_FLAGS(original));

    result = d_assert_standalone(
        recovered == original,
        "roundtrip_all_settings",
        "All settings combined should survive round-trip",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_flag_macro_all
  Aggregation function that runs all flag manipulation macro tests.
*/
bool
d_tests_sa_config_flag_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Flag Manipulation Macros\n");
    printf("  ------------------------------------\n");

    result = d_tests_sa_config_settings_shift(_counter) && result;
    result = d_tests_sa_config_settings_to_flags(_counter) && result;
    result = d_tests_sa_config_flags_to_settings(_counter) && result;
    result = d_tests_sa_config_flag_roundtrip(_counter) && result;

    return result;
}
