#include ".\test_config_tests_sa.h"


/******************************************************************************
 * V. MODE DEFINITION AND PRESET TESTS
 *****************************************************************************/

/*
d_tests_sa_config_mode_silent
  Tests the D_TEST_MODE_SILENT definition.
  Tests the following:
  - Value is zero
  - No message flags are set
  - No settings flags are set
*/
bool
d_tests_sa_config_mode_silent
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        D_TEST_MODE_SILENT == 0x00000000u,
        "mode_silent_value",
        "D_TEST_MODE_SILENT should be 0",
        _counter) && result;

    result = d_assert_standalone(
        (D_TEST_MODE_SILENT & D_TEST_MASK_MESSAGE_FLAGS) == 0,
        "mode_silent_no_messages",
        "Silent mode should have no message flags",
        _counter) && result;

    result = d_assert_standalone(
        (D_TEST_MODE_SILENT & D_TEST_MASK_SETTINGS_FLAGS) == 0,
        "mode_silent_no_settings",
        "Silent mode should have no settings flags",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mode_minimal
  Tests the D_TEST_MODE_MINIMAL definition.
  Tests the following:
  - All counter fail flags are set
  - Only PRINT_TESTS_FAIL is set for printing
  - No pass counter flags are set
  - No settings flags are set
*/
bool
d_tests_sa_config_mode_minimal
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t mode;

    result = true;
    mode   = D_TEST_MODE_MINIMAL;

    // test 1: all 4 counter fail flags are set
    result = d_assert_standalone(
        (mode & D_TEST_MSG_COUNT_FAIL_ALL) == D_TEST_MSG_COUNT_FAIL_ALL,
        "mode_minimal_count_fail",
        "Minimal should have all counter fail flags",
        _counter) && result;

    // test 2: no counter pass flags
    result = d_assert_standalone(
        (mode & D_TEST_MSG_COUNT_PASS_ALL) == 0,
        "mode_minimal_no_count_pass",
        "Minimal should have no counter pass flags",
        _counter) && result;

    // test 3: only PRINT_TESTS_FAIL is set
    result = d_assert_standalone(
        (mode & D_TEST_MSG_FLAG_PRINT_TESTS_FAIL) != 0,
        "mode_minimal_print_tests_fail",
        "Minimal should print test failures",
        _counter) && result;

    result = d_assert_standalone(
        (mode & D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL) == 0,
        "mode_minimal_no_print_asserts",
        "Minimal should not print assert failures",
        _counter) && result;

    // test 4: no settings flags
    result = d_assert_standalone(
        (mode & D_TEST_MASK_SETTINGS_FLAGS) == 0,
        "mode_minimal_no_settings",
        "Minimal should have no settings flags",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mode_normal
  Tests the D_TEST_MODE_NORMAL definition.
  Tests the following:
  - All counter flags (fail + pass) are set
  - All print fail flags are set
  - No print pass flags are set
*/
bool
d_tests_sa_config_mode_normal
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t mode;

    result = true;
    mode   = D_TEST_MODE_NORMAL;

    // test 1: all counter flags set
    result = d_assert_standalone(
        (mode & D_TEST_MSG_COUNT_ALL) == D_TEST_MSG_COUNT_ALL,
        "mode_normal_count_all",
        "Normal should have all counter flags",
        _counter) && result;

    // test 2: all print fail flags set
    result = d_assert_standalone(
        (mode & D_TEST_MSG_PRINT_FAIL_ALL) == D_TEST_MSG_PRINT_FAIL_ALL,
        "mode_normal_print_fail_all",
        "Normal should have all print fail flags",
        _counter) && result;

    // test 3: no print pass flags
    result = d_assert_standalone(
        (mode & D_TEST_MSG_PRINT_PASS_ALL) == 0,
        "mode_normal_no_print_pass",
        "Normal should have no print pass flags",
        _counter) && result;

    // test 4: equals COUNT_ALL | PRINT_FAIL_ALL
    result = d_assert_standalone(
        mode == (D_TEST_MSG_COUNT_ALL | D_TEST_MSG_PRINT_FAIL_ALL),
        "mode_normal_composition",
        "Normal should be COUNT_ALL | PRINT_FAIL_ALL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mode_verbose
  Tests the D_TEST_MODE_VERBOSE definition.
  Tests the following:
  - Equals D_TEST_MSG_ALL
  - All counter and print flags are set
  - Equals 0xFFFF
*/
bool
d_tests_sa_config_mode_verbose
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: equals MSG_ALL
    result = d_assert_standalone(
        D_TEST_MODE_VERBOSE == D_TEST_MSG_ALL,
        "mode_verbose_equals_msg_all",
        "Verbose should equal D_TEST_MSG_ALL",
        _counter) && result;

    // test 2: all message flags set
    result = d_assert_standalone(
        D_TEST_MODE_VERBOSE == 0xFFFFu,
        "mode_verbose_value",
        "Verbose should be 0xFFFF",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_presets
  Tests that configuration presets match their corresponding modes.
  Tests the following:
  - PRESET_SILENT == MODE_SILENT
  - PRESET_MINIMAL == MODE_MINIMAL
  - PRESET_NORMAL == MODE_NORMAL
  - PRESET_VERBOSE == MODE_VERBOSE
*/
bool
d_tests_sa_config_presets
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        D_TEST_CONFIG_PRESET_SILENT == D_TEST_MODE_SILENT,
        "preset_silent",
        "PRESET_SILENT should equal MODE_SILENT",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_PRESET_MINIMAL == D_TEST_MODE_MINIMAL,
        "preset_minimal",
        "PRESET_MINIMAL should equal MODE_MINIMAL",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_PRESET_NORMAL == D_TEST_MODE_NORMAL,
        "preset_normal",
        "PRESET_NORMAL should equal MODE_NORMAL",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_PRESET_VERBOSE == D_TEST_MODE_VERBOSE,
        "preset_verbose",
        "PRESET_VERBOSE should equal MODE_VERBOSE",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_settings_stack_push_all
  Tests the D_TEST_SETTINGS_STACK_PUSH_ALL combination.
  Tests the following:
  - Contains all four stack push settings shifted to upper bits
  - Resides entirely within the stack mask
*/
bool
d_tests_sa_config_settings_stack_push_all
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t push_all;

    result   = true;
    push_all = D_TEST_SETTINGS_STACK_PUSH_ALL;

    // test 1: all four push flags present
    result = d_assert_standalone(
        (push_all & D_TEST_SETTINGS_TO_FLAGS(
             D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL)) != 0,
        "stack_push_all_has_fail",
        "STACK_PUSH_ALL should include PUSH_FAIL",
        _counter) && result;

    result = d_assert_standalone(
        (push_all & D_TEST_SETTINGS_TO_FLAGS(
             D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS)) != 0,
        "stack_push_all_has_pass",
        "STACK_PUSH_ALL should include PUSH_PASS",
        _counter) && result;

    result = d_assert_standalone(
        (push_all & D_TEST_SETTINGS_TO_FLAGS(
             D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING)) != 0,
        "stack_push_all_has_warning",
        "STACK_PUSH_ALL should include PUSH_WARNING",
        _counter) && result;

    result = d_assert_standalone(
        (push_all & D_TEST_SETTINGS_TO_FLAGS(
             D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO)) != 0,
        "stack_push_all_has_info",
        "STACK_PUSH_ALL should include PUSH_INFO",
        _counter) && result;

    // test 2: resides within stack mask
    result = d_assert_standalone(
        (push_all & ~D_TEST_MASK_STACK_FLAGS) == 0,
        "stack_push_all_in_mask",
        "STACK_PUSH_ALL should reside within stack mask",
        _counter) && result;

    // test 3: equals 0x000F0000
    result = d_assert_standalone(
        push_all == 0x000F0000u,
        "stack_push_all_value",
        "STACK_PUSH_ALL should be 0x000F0000",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mode_all
  Aggregation function that runs all mode definition and preset tests.
*/
bool
d_tests_sa_config_mode_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Mode Definitions and Presets\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_config_mode_silent(_counter) && result;
    result = d_tests_sa_config_mode_minimal(_counter) && result;
    result = d_tests_sa_config_mode_normal(_counter) && result;
    result = d_tests_sa_config_mode_verbose(_counter) && result;
    result = d_tests_sa_config_presets(_counter) && result;
    result = d_tests_sa_config_settings_stack_push_all(_counter) && result;

    return result;
}
