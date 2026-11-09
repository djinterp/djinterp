/*******************************************************************************
* djinterp [test]                                     test_options_tests_modes.c
*
* test_options.h's suite, block "config_mode_all": the D_TEST_MODE_* modes, the
* D_TEST_OPTIONS_PRESET_* aliases of them, and D_TEST_SETTINGS_STACK_PUSH_ALL.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_modes.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 5.  MODE DEFINITION AND PRESET TESTS
//==============================================================================

/*
d_tests_options_mode_silent
  Tests the D_TEST_MODE_SILENT definition.
  Tests the following:
  - Value is zero
  - No message flags are set
  - No settings flags are set
*/
bool
d_tests_options_mode_silent(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_mode_silent");

    bool result = true;

    result = D_TEST_CHECK_MSG(_suite,
                              "mode_silent_value",
                              D_TEST_MODE_SILENT == 0x00000000u,
                              "D_TEST_MODE_SILENT should be 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "mode_silent_no_messages",
                              (D_TEST_MODE_SILENT & D_TEST_MASK_MESSAGE_FLAGS) == 0,
                              "Silent mode should have no message flags") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "mode_silent_no_settings",
                              (D_TEST_MODE_SILENT & D_TEST_MASK_SETTINGS_FLAGS) == 0,
                              "Silent mode should have no settings flags") && result;

    return result;
}

/*
d_tests_options_mode_minimal
  Tests the D_TEST_MODE_MINIMAL definition.
  Tests the following:
  - All counter fail flags are set
  - Only PRINT_TESTS_FAIL is set for printing
  - No pass counter flags are set
  - No settings flags are set
*/
bool
d_tests_options_mode_minimal(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_mode_minimal");

    bool           result = true;
    const uint32_t mode   = D_TEST_MODE_MINIMAL;

    // test 1: all 4 counter fail flags are set
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_minimal_count_fail",
                              (mode & D_TEST_MSG_COUNT_FAIL_ALL) == D_TEST_MSG_COUNT_FAIL_ALL,
                              "Minimal should have all counter fail flags") && result;

    // test 2: no counter pass flags
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_minimal_no_count_pass",
                              (mode & D_TEST_MSG_COUNT_PASS_ALL) == 0,
                              "Minimal should have no counter pass flags") && result;

    // test 3: only PRINT_TESTS_FAIL is set
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_minimal_print_tests_fail",
                              (mode & D_TEST_MSG_FLAG_PRINT_TESTS_FAIL) != 0,
                              "Minimal should print test failures") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "mode_minimal_no_print_asserts",
                              (mode & D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL) == 0,
                              "Minimal should not print assert failures") && result;

    // test 4: no settings flags
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_minimal_no_settings",
                              (mode & D_TEST_MASK_SETTINGS_FLAGS) == 0,
                              "Minimal should have no settings flags") && result;

    return result;
}

/*
d_tests_options_mode_normal
  Tests the D_TEST_MODE_NORMAL definition.
  Tests the following:
  - All counter flags (fail + pass) are set
  - All print fail flags are set
  - No print pass flags are set
*/
bool
d_tests_options_mode_normal(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_mode_normal");

    bool           result = true;
    const uint32_t mode   = D_TEST_MODE_NORMAL;

    // test 1: all counter flags set
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_normal_count_all",
                              (mode & D_TEST_MSG_COUNT_ALL) == D_TEST_MSG_COUNT_ALL,
                              "Normal should have all counter flags") && result;

    // test 2: all print fail flags set
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_normal_print_fail_all",
                              (mode & D_TEST_MSG_PRINT_FAIL_ALL) == D_TEST_MSG_PRINT_FAIL_ALL,
                              "Normal should have all print fail flags") && result;

    // test 3: no print pass flags
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_normal_no_print_pass",
                              (mode & D_TEST_MSG_PRINT_PASS_ALL) == 0,
                              "Normal should have no print pass flags") && result;

    // test 4: equals COUNT_ALL | PRINT_FAIL_ALL
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_normal_composition",
                              mode == (D_TEST_MSG_COUNT_ALL | D_TEST_MSG_PRINT_FAIL_ALL),
                              "Normal should be COUNT_ALL | PRINT_FAIL_ALL") && result;

    return result;
}

/*
d_tests_options_mode_verbose
  Tests the D_TEST_MODE_VERBOSE definition.
  Tests the following:
  - Equals D_TEST_MSG_ALL
  - All counter and print flags are set
  - Equals 0xFFFF
*/
bool
d_tests_options_mode_verbose(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_mode_verbose");

    bool result = true;

    // test 1: equals MSG_ALL
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_verbose_equals_msg_all",
                              D_TEST_MODE_VERBOSE == D_TEST_MSG_ALL,
                              "Verbose should equal D_TEST_MSG_ALL") && result;

    // test 2: all message flags set
    result = D_TEST_CHECK_MSG(_suite,
                              "mode_verbose_value",
                              D_TEST_MODE_VERBOSE == 0xFFFFu,
                              "Verbose should be 0xFFFF") && result;

    return result;
}

/*
d_tests_options_presets
  Tests that configuration presets match their corresponding modes.
  Tests the following:
  - PRESET_SILENT == MODE_SILENT
  - PRESET_MINIMAL == MODE_MINIMAL
  - PRESET_NORMAL == MODE_NORMAL
  - PRESET_VERBOSE == MODE_VERBOSE
*/
bool
d_tests_options_presets(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_presets");

    bool result = true;

    result = D_TEST_CHECK_MSG(_suite,
                              "preset_silent",
                              D_TEST_OPTIONS_PRESET_SILENT == D_TEST_MODE_SILENT,
                              "PRESET_SILENT should equal MODE_SILENT") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "preset_minimal",
                              D_TEST_OPTIONS_PRESET_MINIMAL == D_TEST_MODE_MINIMAL,
                              "PRESET_MINIMAL should equal MODE_MINIMAL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "preset_normal",
                              D_TEST_OPTIONS_PRESET_NORMAL == D_TEST_MODE_NORMAL,
                              "PRESET_NORMAL should equal MODE_NORMAL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "preset_verbose",
                              D_TEST_OPTIONS_PRESET_VERBOSE == D_TEST_MODE_VERBOSE,
                              "PRESET_VERBOSE should equal MODE_VERBOSE") && result;

    return result;
}

/*
d_tests_options_settings_stack_push_all
  Tests the D_TEST_SETTINGS_STACK_PUSH_ALL combination.
  Tests the following:
  - Contains all four stack push settings shifted to upper bits
  - Resides entirely within the stack mask
*/
bool
d_tests_options_settings_stack_push_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_settings_stack_push_all");

    bool           result   = true;
    const uint32_t push_all = D_TEST_SETTINGS_STACK_PUSH_ALL;

    // test 1: all four push flags present
    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_has_fail",
                              (push_all & D_TEST_SETTINGS_TO_FLAGS( D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL)) != 0,
                              "STACK_PUSH_ALL should include PUSH_FAIL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_has_pass",
                              (push_all & D_TEST_SETTINGS_TO_FLAGS( D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS)) != 0,
                              "STACK_PUSH_ALL should include PUSH_PASS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_has_warning",
                              (push_all & D_TEST_SETTINGS_TO_FLAGS( D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING)) != 0,
                              "STACK_PUSH_ALL should include PUSH_WARNING") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_has_info",
                              (push_all & D_TEST_SETTINGS_TO_FLAGS( D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO)) != 0,
                              "STACK_PUSH_ALL should include PUSH_INFO") && result;

    // test 2: resides within stack mask
    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_in_mask",
                              (push_all & ~D_TEST_MASK_STACK_FLAGS) == 0,
                              "STACK_PUSH_ALL should reside within stack mask") && result;

    // test 3: equals 0x000F0000
    result = D_TEST_CHECK_MSG(_suite,
                              "stack_push_all_value",
                              push_all == 0x000F0000u,
                              "STACK_PUSH_ALL should be 0x000F0000") && result;

    return result;
}

/*
d_tests_options_mode_all
  Aggregation function that runs all mode definition and preset tests.
*/
bool
d_tests_options_mode_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "config_mode_all");

    bool result = true;

    result = d_tests_options_mode_silent(_suite) && result;
    result = d_tests_options_mode_minimal(_suite) && result;
    result = d_tests_options_mode_normal(_suite) && result;
    result = d_tests_options_mode_verbose(_suite) && result;
    result = d_tests_options_presets(_suite) && result;
    result = d_tests_options_settings_stack_push_all(_suite) && result;

    return result;
}
