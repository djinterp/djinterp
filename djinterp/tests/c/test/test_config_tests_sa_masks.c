#include ".\test_config_tests_sa.h"


/******************************************************************************
 * II. MASK DEFINITION TESTS
 *****************************************************************************/

/*
d_tests_sa_config_mask_message
  Tests the D_TEST_MASK_MESSAGE_FLAGS constant.
  Tests the following:
  - Value is 0x0000FFFF
  - Covers all 16 lower bits
  - Does not overlap with settings mask
*/
bool
d_tests_sa_config_mask_message
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 0x0000FFFF
    result = d_assert_standalone(
        D_TEST_MASK_MESSAGE_FLAGS == 0x0000FFFFu,
        "mask_message_value",
        "D_TEST_MASK_MESSAGE_FLAGS should be 0x0000FFFF",
        _counter) && result;

    // test 2: all 16 lower bits are set
    result = d_assert_standalone(
        (D_TEST_MASK_MESSAGE_FLAGS & 0x0000FFFFu) == 0x0000FFFFu,
        "mask_message_lower_bits",
        "Message mask should cover all lower 16 bits",
        _counter) && result;

    // test 3: no upper bits set
    result = d_assert_standalone(
        (D_TEST_MASK_MESSAGE_FLAGS & 0xFFFF0000u) == 0,
        "mask_message_no_upper",
        "Message mask should not set any upper bits",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_settings
  Tests the D_TEST_MASK_SETTINGS_FLAGS constant.
  Tests the following:
  - Value is 0xFFFF0000
  - Covers all 16 upper bits
  - No lower bits set
*/
bool
d_tests_sa_config_mask_settings
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 0xFFFF0000
    result = d_assert_standalone(
        D_TEST_MASK_SETTINGS_FLAGS == 0xFFFF0000u,
        "mask_settings_value",
        "D_TEST_MASK_SETTINGS_FLAGS should be 0xFFFF0000",
        _counter) && result;

    // test 2: no lower bits set
    result = d_assert_standalone(
        (D_TEST_MASK_SETTINGS_FLAGS & 0x0000FFFFu) == 0,
        "mask_settings_no_lower",
        "Settings mask should not set any lower bits",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_counter
  Tests the D_TEST_MASK_COUNTER_FLAGS constant.
  Tests the following:
  - Value is 0x000000FF (bits 0-7)
  - Is a subset of message flags mask
*/
bool
d_tests_sa_config_mask_counter
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 0x000000FF
    result = d_assert_standalone(
        D_TEST_MASK_COUNTER_FLAGS == 0x000000FFu,
        "mask_counter_value",
        "D_TEST_MASK_COUNTER_FLAGS should be 0x000000FF",
        _counter) && result;

    // test 2: is a subset of message mask
    result = d_assert_standalone(
        (D_TEST_MASK_COUNTER_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) ==
             D_TEST_MASK_COUNTER_FLAGS,
        "mask_counter_subset_of_message",
        "Counter mask should be a subset of message mask",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_print
  Tests the D_TEST_MASK_PRINT_FLAGS constant.
  Tests the following:
  - Value is 0x0000FF00 (bits 8-15)
  - Is a subset of message flags mask
  - Does not overlap with counter mask
*/
bool
d_tests_sa_config_mask_print
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 0x0000FF00
    result = d_assert_standalone(
        D_TEST_MASK_PRINT_FLAGS == 0x0000FF00u,
        "mask_print_value",
        "D_TEST_MASK_PRINT_FLAGS should be 0x0000FF00",
        _counter) && result;

    // test 2: is a subset of message mask
    result = d_assert_standalone(
        (D_TEST_MASK_PRINT_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) ==
             D_TEST_MASK_PRINT_FLAGS,
        "mask_print_subset_of_message",
        "Print mask should be a subset of message mask",
        _counter) && result;

    // test 3: no overlap with counter mask
    result = d_assert_standalone(
        (D_TEST_MASK_PRINT_FLAGS & D_TEST_MASK_COUNTER_FLAGS) == 0,
        "mask_print_no_overlap_counter",
        "Print mask should not overlap with counter mask",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_stack
  Tests the D_TEST_MASK_STACK_FLAGS constant.
  Tests the following:
  - Value is 0x000F0000 (bits 16-19)
  - Is a subset of settings flags mask
  - Does not overlap with message mask
*/
bool
d_tests_sa_config_mask_stack
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: value is 0x000F0000
    result = d_assert_standalone(
        D_TEST_MASK_STACK_FLAGS == 0x000F0000u,
        "mask_stack_value",
        "D_TEST_MASK_STACK_FLAGS should be 0x000F0000",
        _counter) && result;

    // test 2: is a subset of settings mask
    result = d_assert_standalone(
        (D_TEST_MASK_STACK_FLAGS & D_TEST_MASK_SETTINGS_FLAGS) ==
             D_TEST_MASK_STACK_FLAGS,
        "mask_stack_subset_of_settings",
        "Stack mask should be a subset of settings mask",
        _counter) && result;

    // test 3: no overlap with message mask
    result = d_assert_standalone(
        (D_TEST_MASK_STACK_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) == 0,
        "mask_stack_no_overlap_message",
        "Stack mask should not overlap with message mask",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_no_overlap
  Tests that masks are mutually exclusive where expected.
  Tests the following:
  - Message and settings masks fully partition the 32-bit space
  - Counter and print masks partition the message space
*/
bool
d_tests_sa_config_mask_no_overlap
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: message and settings together cover all 32 bits
    result = d_assert_standalone(
        (D_TEST_MASK_MESSAGE_FLAGS | D_TEST_MASK_SETTINGS_FLAGS) ==
             0xFFFFFFFFu,
        "mask_full_coverage",
        "Message + settings masks should cover all 32 bits",
        _counter) && result;

    // test 2: message and settings do not overlap
    result = d_assert_standalone(
        (D_TEST_MASK_MESSAGE_FLAGS & D_TEST_MASK_SETTINGS_FLAGS) == 0,
        "mask_no_overlap_msg_settings",
        "Message and settings masks should not overlap",
        _counter) && result;

    // test 3: counter and print together cover the message mask
    result = d_assert_standalone(
        (D_TEST_MASK_COUNTER_FLAGS | D_TEST_MASK_PRINT_FLAGS) ==
             D_TEST_MASK_MESSAGE_FLAGS,
        "mask_counter_print_cover_message",
        "Counter + print masks should equal message mask",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_mask_all
  Aggregation function that runs all mask definition tests.
*/
bool
d_tests_sa_config_mask_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Mask Definitions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_config_mask_message(_counter) && result;
    result = d_tests_sa_config_mask_settings(_counter) && result;
    result = d_tests_sa_config_mask_counter(_counter) && result;
    result = d_tests_sa_config_mask_print(_counter) && result;
    result = d_tests_sa_config_mask_stack(_counter) && result;
    result = d_tests_sa_config_mask_no_overlap(_counter) && result;

    return result;
}
