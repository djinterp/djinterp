/*******************************************************************************
* djinterp [test]                               test_options_tests_flag_macros.c
*
* test_options.h's suite, block "config_flag_macro_all": D_TEST_SETTINGS_SHIFT,
* D_TEST_SETTINGS_TO_FLAGS, D_TEST_FLAGS_TO_SETTINGS, and the round trip through
* the last two.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_flag_macros.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 1.  FLAG MANIPULATION MACRO TESTS
//==============================================================================

/*
d_tests_options_settings_shift
  Tests the D_TEST_SETTINGS_SHIFT constant.
  Tests the following:
  - Value is 16
  - Shifting 1 by SETTINGS_SHIFT produces 0x00010000
*/
bool
d_tests_options_settings_shift(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_settings_shift");

    bool result = true;

    // test 1: value is 16
    result = D_TEST_CHECK_MSG(_suite,
                              "settings_shift_value",
                              D_TEST_SETTINGS_SHIFT == 16,
                              "D_TEST_SETTINGS_SHIFT should be 16") && result;

    // test 2: shifting 1 by SETTINGS_SHIFT gives bit 16
    result = D_TEST_CHECK_MSG(_suite,
                              "settings_shift_bit_position",
                              (1u << D_TEST_SETTINGS_SHIFT) == 0x00010000u,
                              "1 << D_TEST_SETTINGS_SHIFT should be 0x00010000") && result;

    return result;
}

/*
d_tests_options_settings_to_flags
  Tests the D_TEST_SETTINGS_TO_FLAGS macro.
  Tests the following:
  - Zero settings produce zero flags
  - Single setting flag is shifted to upper 16 bits
  - Multiple settings flags combine correctly
  - All four settings flags produce expected combined value
*/
bool
d_tests_options_settings_to_flags(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_settings_to_flags");

    bool result = true;

    // test 1: zero settings -> zero flags
    uint32_t flags = D_TEST_SETTINGS_TO_FLAGS(0);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_zero",
                              flags == 0,
                              "D_TEST_SETTINGS_TO_FLAGS(0) should be 0") && result;

    // test 2: STACK_PUSH_FAIL (0x01) shifts to bit 16
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_push_fail",
                              flags == 0x00010000u,
                              "STACK_PUSH_FAIL should shift to 0x00010000") && result;

    // test 3: STACK_PUSH_PASS (0x02) shifts to bit 17
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_push_pass",
                              flags == 0x00020000u,
                              "STACK_PUSH_PASS should shift to 0x00020000") && result;

    // test 4: STACK_PUSH_WARNING (0x04) shifts to bit 18
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_push_warning",
                              flags == 0x00040000u,
                              "STACK_PUSH_WARNING should shift to 0x00040000") && result;

    // test 5: STACK_PUSH_INFO (0x08) shifts to bit 19
    flags = D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_push_info",
                              flags == 0x00080000u,
                              "STACK_PUSH_INFO should shift to 0x00080000") && result;

    // test 6: all four settings combined
    flags = D_TEST_SETTINGS_TO_FLAGS(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |
        D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO);

    result = D_TEST_CHECK_MSG(_suite,
                              "to_flags_all_combined",
                              flags == 0x000F0000u,
                              "All settings combined should be 0x000F0000") && result;

    return result;
}

/*
d_tests_options_flags_to_settings
  Tests the D_TEST_FLAGS_TO_SETTINGS macro.
  Tests the following:
  - Zero flags produce zero settings
  - Packed flag in upper bits extracts to original setting value
  - Message-only flags (lower 16 bits) produce zero settings
  - Mixed flags extract only settings portion
*/
bool
d_tests_options_flags_to_settings(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_flags_to_settings");

    bool result = true;

    // test 1: zero flags -> zero settings
    uint32_t settings = D_TEST_FLAGS_TO_SETTINGS(0);

    result = D_TEST_CHECK_MSG(_suite,
                              "from_flags_zero",
                              settings == 0,
                              "D_TEST_FLAGS_TO_SETTINGS(0) should be 0") && result;

    // test 2: 0x00010000 extracts to STACK_PUSH_FAIL (0x01)
    settings = D_TEST_FLAGS_TO_SETTINGS(0x00010000u);

    result = D_TEST_CHECK_MSG(_suite,
                              "from_flags_push_fail",
                              settings == D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL,
                              "0x00010000 should extract to STACK_PUSH_FAIL") && result;

    // test 3: message-only flags (lower 16 bits) produce zero
    settings = D_TEST_FLAGS_TO_SETTINGS(0x0000FFFFu);

    result = D_TEST_CHECK_MSG(_suite,
                              "from_flags_message_only",
                              settings == 0,
                              "Lower 16 bits should produce zero settings") && result;

    // test 4: mixed flags extract only settings portion
    settings = D_TEST_FLAGS_TO_SETTINGS(0x000F00FFu);

    result = D_TEST_CHECK_MSG(_suite,
                              "from_flags_mixed",
                              settings == 0x0Fu,
                              "Mixed flags should extract only upper portion") && result;

    return result;
}

/*
d_tests_options_flag_roundtrip
  Tests round-trip conversion: TO_FLAGS -> FLAGS_TO_SETTINGS.
  Tests the following:
  - Each individual settings flag survives the round-trip
  - Combined settings flags survive the round-trip
*/
bool
d_tests_options_flag_roundtrip(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_flag_roundtrip");

    bool result = true;

    // test 1: STACK_PUSH_FAIL round-trip
    uint32_t original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL;
    uint32_t recovered = D_TEST_FLAGS_TO_SETTINGS(
                             D_TEST_SETTINGS_TO_FLAGS(original));

    result = D_TEST_CHECK_MSG(_suite,
                              "roundtrip_push_fail",
                              recovered == original,
                              "STACK_PUSH_FAIL should survive round-trip") && result;

    // test 2: STACK_PUSH_PASS round-trip
    original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS;
    recovered = D_TEST_FLAGS_TO_SETTINGS(
                    D_TEST_SETTINGS_TO_FLAGS(original));

    result = D_TEST_CHECK_MSG(_suite,
                              "roundtrip_push_pass",
                              recovered == original,
                              "STACK_PUSH_PASS should survive round-trip") && result;

    // test 3: all settings combined round-trip
    original  = D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |
                D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO;
    recovered = D_TEST_FLAGS_TO_SETTINGS(
                    D_TEST_SETTINGS_TO_FLAGS(original));

    result = D_TEST_CHECK_MSG(_suite,
                              "roundtrip_all_settings",
                              recovered == original,
                              "All settings combined should survive round-trip") && result;

    return result;
}

/*
d_tests_options_flag_macro_all
  Aggregation function that runs all flag manipulation macro tests.
*/
bool
d_tests_options_flag_macro_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "config_flag_macro_all");

    bool result = true;

    result = d_tests_options_settings_shift(_suite) && result;
    result = d_tests_options_settings_to_flags(_suite) && result;
    result = d_tests_options_flags_to_settings(_suite) && result;
    result = d_tests_options_flag_roundtrip(_suite) && result;

    return result;
}
