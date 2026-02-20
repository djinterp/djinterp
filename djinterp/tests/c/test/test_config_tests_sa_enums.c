#include ".\test_config_tests_sa.h"


/******************************************************************************
 * III. ENUM VALUE TESTS
 *****************************************************************************/

/*
d_tests_sa_config_enum_message_flag
  Tests the DTestMessageFlag enum values.
  Tests the following:
  - Each counter flag occupies a unique bit in bits 0-7
  - Each print flag occupies a unique bit in bits 8-15
  - Counter flags are within the counter mask
  - Print flags are within the print mask
  - No two enum values collide
*/
bool
d_tests_sa_config_enum_message_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: counter flags occupy bits 0-7
    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL == 0x01,
        "msg_flag_count_asserts_fail",
        "COUNT_ASSERTS_FAIL should be 0x01",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS == 0x02,
        "msg_flag_count_asserts_pass",
        "COUNT_ASSERTS_PASS should be 0x02",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_TESTS_FAIL == 0x04,
        "msg_flag_count_tests_fail",
        "COUNT_TESTS_FAIL should be 0x04",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_TESTS_PASS == 0x08,
        "msg_flag_count_tests_pass",
        "COUNT_TESTS_PASS should be 0x08",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL == 0x10,
        "msg_flag_count_blocks_fail",
        "COUNT_BLOCKS_FAIL should be 0x10",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS == 0x20,
        "msg_flag_count_blocks_pass",
        "COUNT_BLOCKS_PASS should be 0x20",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_MODULES_FAIL == 0x40,
        "msg_flag_count_modules_fail",
        "COUNT_MODULES_FAIL should be 0x40",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_COUNT_MODULES_PASS == 0x80,
        "msg_flag_count_modules_pass",
        "COUNT_MODULES_PASS should be 0x80",
        _counter) && result;

    // test 2: print flags occupy bits 8-15
    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL == 0x0100,
        "msg_flag_print_asserts_fail",
        "PRINT_ASSERTS_FAIL should be 0x0100",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS == 0x0200,
        "msg_flag_print_asserts_pass",
        "PRINT_ASSERTS_PASS should be 0x0200",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_TESTS_FAIL == 0x0400,
        "msg_flag_print_tests_fail",
        "PRINT_TESTS_FAIL should be 0x0400",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_TESTS_PASS == 0x0800,
        "msg_flag_print_tests_pass",
        "PRINT_TESTS_PASS should be 0x0800",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL == 0x1000,
        "msg_flag_print_blocks_fail",
        "PRINT_BLOCKS_FAIL should be 0x1000",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS == 0x2000,
        "msg_flag_print_blocks_pass",
        "PRINT_BLOCKS_PASS should be 0x2000",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_MODULES_FAIL == 0x4000,
        "msg_flag_print_modules_fail",
        "PRINT_MODULES_FAIL should be 0x4000",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_FLAG_PRINT_MODULES_PASS == 0x8000,
        "msg_flag_print_modules_pass",
        "PRINT_MODULES_PASS should be 0x8000",
        _counter) && result;

    // test 3: all counter flags fit within counter mask
    result = d_assert_standalone(
        ((D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
          D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |
          D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   |
          D_TEST_MSG_FLAG_COUNT_TESTS_PASS   |
          D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  |
          D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS  |
          D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |
          D_TEST_MSG_FLAG_COUNT_MODULES_PASS) & ~D_TEST_MASK_COUNTER_FLAGS) == 0,
        "msg_counter_flags_in_mask",
        "All counter flags should fit within counter mask",
        _counter) && result;

    // test 4: all print flags fit within print mask
    result = d_assert_standalone(
        ((D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL |
          D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS |
          D_TEST_MSG_FLAG_PRINT_TESTS_FAIL   |
          D_TEST_MSG_FLAG_PRINT_TESTS_PASS   |
          D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL  |
          D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS  |
          D_TEST_MSG_FLAG_PRINT_MODULES_FAIL |
          D_TEST_MSG_FLAG_PRINT_MODULES_PASS) & ~D_TEST_MASK_PRINT_FLAGS) == 0,
        "msg_print_flags_in_mask",
        "All print flags should fit within print mask",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_enum_settings_flag
  Tests the DTestSettingsFlag enum values.
  Tests the following:
  - Each settings flag is a unique power of 2
  - Settings flags are within bits 0-3 (before shifting)
*/
bool
d_tests_sa_config_enum_settings_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: individual values
    result = d_assert_standalone(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL == 0x01,
        "settings_flag_push_fail",
        "STACK_PUSH_FAIL should be 0x01",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS == 0x02,
        "settings_flag_push_pass",
        "STACK_PUSH_PASS should be 0x02",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING == 0x04,
        "settings_flag_push_warning",
        "STACK_PUSH_WARNING should be 0x04",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO == 0x08,
        "settings_flag_push_info",
        "STACK_PUSH_INFO should be 0x08",
        _counter) && result;

    // test 2: all values are unique powers of 2
    result = d_assert_standalone(
        (D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL &
         D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS) == 0,
        "settings_flags_unique_fail_pass",
        "PUSH_FAIL and PUSH_PASS should not overlap",
        _counter) && result;

    result = d_assert_standalone(
        (D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING &
         D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO) == 0,
        "settings_flags_unique_warn_info",
        "PUSH_WARNING and PUSH_INFO should not overlap",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_enum_config_key
  Tests the DTestConfigKey enum values.
  Tests the following:
  - Enum values are sequential starting from 0
  - Each key has a distinct value
  - D_TEST_CONFIG_INDENT alias matches D_TEST_CONFIG_INDENT_STR
*/
bool
d_tests_sa_config_enum_config_key
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: ENABLED is 0
    result = d_assert_standalone(
        D_TEST_CONFIG_ENABLED == 0,
        "config_key_enabled",
        "D_TEST_CONFIG_ENABLED should be 0",
        _counter) && result;

    // test 2: keys are sequential (spot check a few)
    result = d_assert_standalone(
        D_TEST_CONFIG_INDENT_MAX_LEVEL == 1,
        "config_key_indent_max",
        "D_TEST_CONFIG_INDENT_MAX_LEVEL should be 1",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_INDENT_STR == 2,
        "config_key_indent_str",
        "D_TEST_CONFIG_INDENT_STR should be 2",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_TIMEOUT_MS == 8,
        "config_key_timeout",
        "D_TEST_CONFIG_TIMEOUT_MS should be 8",
        _counter) && result;

    // test 3: legacy alias matches
    result = d_assert_standalone(
        D_TEST_CONFIG_INDENT == D_TEST_CONFIG_INDENT_STR,
        "config_key_indent_alias",
        "D_TEST_CONFIG_INDENT should equal D_TEST_CONFIG_INDENT_STR",
        _counter) && result;

    // test 4: all keys are distinct
    result = d_assert_standalone(
        D_TEST_CONFIG_SKIP != D_TEST_CONFIG_ENABLED,
        "config_key_skip_ne_enabled",
        "SKIP and ENABLED should be distinct",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_CONFIG_PRIORITY != D_TEST_CONFIG_MESSAGE_FLAGS,
        "config_key_priority_ne_msg_flags",
        "PRIORITY and MESSAGE_FLAGS should be distinct",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_enum_metadata_flag
  Tests the DTestMetadataFlag enum values.
  Tests the following:
  - UNKNOWN is 0
  - Subsequent values are sequential from 1
  - VERSION_STRING is the last value (17)
*/
bool
d_tests_sa_config_enum_metadata_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: UNKNOWN is 0
    result = d_assert_standalone(
        D_TEST_METADATA_UNKNOWN == 0,
        "metadata_flag_unknown",
        "D_TEST_METADATA_UNKNOWN should be 0",
        _counter) && result;

    // test 2: AUTHORS is 1
    result = d_assert_standalone(
        D_TEST_METADATA_AUTHORS == 1,
        "metadata_flag_authors",
        "D_TEST_METADATA_AUTHORS should be 1",
        _counter) && result;

    // test 3: sequential (spot check)
    result = d_assert_standalone(
        D_TEST_METADATA_CATEGORY == 2,
        "metadata_flag_category",
        "D_TEST_METADATA_CATEGORY should be 2",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_METADATA_DESCRIPTION == 6,
        "metadata_flag_description",
        "D_TEST_METADATA_DESCRIPTION should be 6",
        _counter) && result;

    // test 4: VERSION_STRING is 17
    result = d_assert_standalone(
        D_TEST_METADATA_VERSION_STRING == 17,
        "metadata_flag_version_string",
        "D_TEST_METADATA_VERSION_STRING should be 17",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_enum_event
  Tests the DTestEvent enum values.
  Tests the following:
  - Events are sequential starting from 0
  - All six events have distinct values
*/
bool
d_tests_sa_config_enum_event
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: SETUP is 0
    result = d_assert_standalone(
        D_TEST_EVENT_SETUP == 0,
        "event_setup",
        "D_TEST_EVENT_SETUP should be 0",
        _counter) && result;

    // test 2: START is 1
    result = d_assert_standalone(
        D_TEST_EVENT_START == 1,
        "event_start",
        "D_TEST_EVENT_START should be 1",
        _counter) && result;

    // test 3: SUCCESS is 2
    result = d_assert_standalone(
        D_TEST_EVENT_SUCCESS == 2,
        "event_success",
        "D_TEST_EVENT_SUCCESS should be 2",
        _counter) && result;

    // test 4: FAILURE is 3
    result = d_assert_standalone(
        D_TEST_EVENT_FAILURE == 3,
        "event_failure",
        "D_TEST_EVENT_FAILURE should be 3",
        _counter) && result;

    // test 5: END is 4
    result = d_assert_standalone(
        D_TEST_EVENT_END == 4,
        "event_end",
        "D_TEST_EVENT_END should be 4",
        _counter) && result;

    // test 6: TEAR_DOWN is 5
    result = d_assert_standalone(
        D_TEST_EVENT_TEAR_DOWN == 5,
        "event_tear_down",
        "D_TEST_EVENT_TEAR_DOWN should be 5",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_enum_all
  Aggregation function that runs all enum value tests.
*/
bool
d_tests_sa_config_enum_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Enum Values\n");
    printf("  -----------------------\n");

    result = d_tests_sa_config_enum_message_flag(_counter) && result;
    result = d_tests_sa_config_enum_settings_flag(_counter) && result;
    result = d_tests_sa_config_enum_config_key(_counter) && result;
    result = d_tests_sa_config_enum_metadata_flag(_counter) && result;
    result = d_tests_sa_config_enum_event(_counter) && result;

    return result;
}
