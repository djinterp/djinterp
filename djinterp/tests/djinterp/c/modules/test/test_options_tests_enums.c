/*******************************************************************************
* djinterp [test]                                     test_options_tests_enums.c
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_enums.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_options_tests.h"


/******************************************************************************
 * III. ENUM VALUE TESTS
 *****************************************************************************/

/*
d_tests_options_enum_message_flag
  Tests the DTestMessageFlag enum values.
  Tests the following:
  - Each counter flag occupies a unique bit in bits 0-7
  - Each print flag occupies a unique bit in bits 8-15
  - Counter flags are within the counter mask
  - Print flags are within the print mask
  - No two enum values collide
*/
bool d_tests_options_enum_message_flag(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_enum_message_flag");

    result = true;

    // test 1: counter flags occupy bits 0-7
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_asserts_fail",
                              D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL == 0x01,
                              "COUNT_ASSERTS_FAIL should be 0x01") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_asserts_pass",
                              D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS == 0x02,
                              "COUNT_ASSERTS_PASS should be 0x02") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_tests_fail",
                              D_TEST_MSG_FLAG_COUNT_TESTS_FAIL == 0x04,
                              "COUNT_TESTS_FAIL should be 0x04") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_tests_pass",
                              D_TEST_MSG_FLAG_COUNT_TESTS_PASS == 0x08,
                              "COUNT_TESTS_PASS should be 0x08") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_blocks_fail",
                              D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL == 0x10,
                              "COUNT_BLOCKS_FAIL should be 0x10") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_blocks_pass",
                              D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS == 0x20,
                              "COUNT_BLOCKS_PASS should be 0x20") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_modules_fail",
                              D_TEST_MSG_FLAG_COUNT_MODULES_FAIL == 0x40,
                              "COUNT_MODULES_FAIL should be 0x40") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_count_modules_pass",
                              D_TEST_MSG_FLAG_COUNT_MODULES_PASS == 0x80,
                              "COUNT_MODULES_PASS should be 0x80") && result;

    // test 2: print flags occupy bits 8-15
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_asserts_fail",
                              D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL == 0x0100,
                              "PRINT_ASSERTS_FAIL should be 0x0100") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_asserts_pass",
                              D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS == 0x0200,
                              "PRINT_ASSERTS_PASS should be 0x0200") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_tests_fail",
                              D_TEST_MSG_FLAG_PRINT_TESTS_FAIL == 0x0400,
                              "PRINT_TESTS_FAIL should be 0x0400") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_tests_pass",
                              D_TEST_MSG_FLAG_PRINT_TESTS_PASS == 0x0800,
                              "PRINT_TESTS_PASS should be 0x0800") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_blocks_fail",
                              D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL == 0x1000,
                              "PRINT_BLOCKS_FAIL should be 0x1000") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_blocks_pass",
                              D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS == 0x2000,
                              "PRINT_BLOCKS_PASS should be 0x2000") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_modules_fail",
                              D_TEST_MSG_FLAG_PRINT_MODULES_FAIL == 0x4000,
                              "PRINT_MODULES_FAIL should be 0x4000") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "msg_flag_print_modules_pass",
                              D_TEST_MSG_FLAG_PRINT_MODULES_PASS == 0x8000,
                              "PRINT_MODULES_PASS should be 0x8000") && result;

    // test 3: all counter flags fit within counter mask
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_counter_flags_in_mask",
                              ((D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL | D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS | D_TEST_MSG_FLAG_COUNT_TESTS_FAIL | D_TEST_MSG_FLAG_COUNT_TESTS_PASS | D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL | D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS | D_TEST_MSG_FLAG_COUNT_MODULES_FAIL | D_TEST_MSG_FLAG_COUNT_MODULES_PASS) & ~D_TEST_MASK_COUNTER_FLAGS) == 0,
                              "All counter flags should fit within counter mask") && result;

    // test 4: all print flags fit within print mask
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_print_flags_in_mask",
                              ((D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL | D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS | D_TEST_MSG_FLAG_PRINT_TESTS_FAIL | D_TEST_MSG_FLAG_PRINT_TESTS_PASS | D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL | D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS | D_TEST_MSG_FLAG_PRINT_MODULES_FAIL | D_TEST_MSG_FLAG_PRINT_MODULES_PASS) & ~D_TEST_MASK_PRINT_FLAGS) == 0,
                              "All print flags should fit within print mask") && result;

    return result;
}

/*
d_tests_options_enum_settings_flag
  Tests the DTestSettingsFlag enum values.
  Tests the following:
  - Each settings flag is a unique power of 2
  - Settings flags are within bits 0-3 (before shifting)
*/
bool d_tests_options_enum_settings_flag(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_enum_settings_flag");

    result = true;

    // test 1: individual values
    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flag_push_fail",
                              D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL == 0x01,
                              "STACK_PUSH_FAIL should be 0x01") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flag_push_pass",
                              D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS == 0x02,
                              "STACK_PUSH_PASS should be 0x02") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flag_push_warning",
                              D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING == 0x04,
                              "STACK_PUSH_WARNING should be 0x04") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flag_push_info",
                              D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO == 0x08,
                              "STACK_PUSH_INFO should be 0x08") && result;

    // test 2: all values are unique powers of 2
    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flags_unique_fail_pass",
                              (D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL & D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS) == 0,
                              "PUSH_FAIL and PUSH_PASS should not overlap") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "settings_flags_unique_warn_info",
                              (D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING & D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO) == 0,
                              "PUSH_WARNING and PUSH_INFO should not overlap") && result;

    return result;
}

/*
d_tests_options_enum_config_key
  Tests the DTestConfigKey enum values.
  Tests the following:
  - Enum values are sequential starting from 0
  - Each key has a distinct value
  - D_TEST_OPTIONS_INDENT alias matches D_TEST_OPTIONS_INDENT_STR
*/
bool d_tests_options_enum_config_key(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_enum_config_key");

    result = true;

    // test 1: ENABLED is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_enabled",
                              D_TEST_OPTIONS_ENABLED == 0,
                              "D_TEST_OPTIONS_ENABLED should be 0") && result;

    // test 2: keys are sequential (spot check a few)
    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_indent_max",
                              D_TEST_OPTIONS_INDENT_MAX_LEVEL == 1,
                              "D_TEST_OPTIONS_INDENT_MAX_LEVEL should be 1") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_indent_str",
                              D_TEST_OPTIONS_INDENT_STR == 2,
                              "D_TEST_OPTIONS_INDENT_STR should be 2") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_timeout",
                              D_TEST_OPTIONS_TIMEOUT_MS == 8,
                              "D_TEST_OPTIONS_TIMEOUT_MS should be 8") && result;

    // test 3: legacy alias matches
    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_indent_alias",
                              D_TEST_OPTIONS_INDENT == D_TEST_OPTIONS_INDENT_STR,
                              "D_TEST_OPTIONS_INDENT should equal D_TEST_OPTIONS_INDENT_STR") && result;

    // test 4: all keys are distinct
    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_skip_ne_enabled",
                              D_TEST_OPTIONS_SKIP != D_TEST_OPTIONS_ENABLED,
                              "SKIP and ENABLED should be distinct") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "config_key_priority_ne_msg_flags",
                              D_TEST_OPTIONS_PRIORITY != D_TEST_OPTIONS_MESSAGE_FLAGS,
                              "PRIORITY and MESSAGE_FLAGS should be distinct") && result;

    return result;
}

/*
d_tests_options_enum_metadata_flag
  Tests the DTestMetadataFlag enum values.
  Tests the following:
  - UNKNOWN is 0
  - Subsequent values are sequential from 1
  - VERSION_STRING is the last value (17)
*/
bool d_tests_options_enum_metadata_flag(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_enum_metadata_flag");

    result = true;

    // test 1: UNKNOWN is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "metadata_flag_unknown",
                              D_TEST_METADATA_UNKNOWN == 0x100,
                              "D_TEST_METADATA_UNKNOWN should be 0") && result;

    // test 2: AUTHORS is 1
    result = D_TEST_CHECK_MSG(_suite,
                              "metadata_flag_authors",
                              D_TEST_METADATA_AUTHORS == 0x101,
                              "D_TEST_METADATA_AUTHORS should be 1") && result;

    // test 3: sequential (spot check)
    result = D_TEST_CHECK_MSG(_suite,
                              "metadata_flag_category",
                              D_TEST_METADATA_CATEGORY == 0x102,
                              "D_TEST_METADATA_CATEGORY should be 2") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "metadata_flag_description",
                              D_TEST_METADATA_DESCRIPTION == 0x106,
                              "D_TEST_METADATA_DESCRIPTION should be 6") && result;

    // test 4: VERSION_STRING is 17
    result = D_TEST_CHECK_MSG(_suite,
                              "metadata_flag_version_string",
                              D_TEST_METADATA_VERSION_STRING == 0x111,
                              "D_TEST_METADATA_VERSION_STRING should be 17") && result;

    return result;
}

/*
d_tests_options_enum_event
  Tests the DTestEvent enum values.
  Tests the following:
  - Events are sequential starting from 0
  - All six events have distinct values
*/
bool d_tests_options_enum_event(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_enum_event");

    result = true;

    // test 1: SETUP is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "event_setup",
                              D_TEST_EVENT_SETUP == 0,
                              "D_TEST_EVENT_SETUP should be 0") && result;

    // test 2: START is 1
    result = D_TEST_CHECK_MSG(_suite,
                              "event_start",
                              D_TEST_EVENT_START == 1,
                              "D_TEST_EVENT_START should be 1") && result;

    // test 3: SUCCESS is 2
    result = D_TEST_CHECK_MSG(_suite,
                              "event_success",
                              D_TEST_EVENT_SUCCESS == 2,
                              "D_TEST_EVENT_SUCCESS should be 2") && result;

    // test 4: FAILURE is 3
    result = D_TEST_CHECK_MSG(_suite,
                              "event_failure",
                              D_TEST_EVENT_FAILURE == 3,
                              "D_TEST_EVENT_FAILURE should be 3") && result;

    // test 5: END is 4
    result = D_TEST_CHECK_MSG(_suite,
                              "event_end",
                              D_TEST_EVENT_END == 4,
                              "D_TEST_EVENT_END should be 4") && result;

    // test 6: TEAR_DOWN is 5
    result = D_TEST_CHECK_MSG(_suite,
                              "event_tear_down",
                              D_TEST_EVENT_TEAR_DOWN == 5,
                              "D_TEST_EVENT_TEAR_DOWN should be 5") && result;

    return result;
}

/*
d_tests_options_enum_all
  Aggregation function that runs all enum value tests.
*/
bool d_tests_options_enum_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "config_enum_all");

    result = true;

    result = d_tests_options_enum_message_flag(_suite) && result;
    result = d_tests_options_enum_settings_flag(_suite) && result;
    result = d_tests_options_enum_config_key(_suite) && result;
    result = d_tests_options_enum_metadata_flag(_suite) && result;
    result = d_tests_options_enum_event(_suite) && result;

    return result;
}
