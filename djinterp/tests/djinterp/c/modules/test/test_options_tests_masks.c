/*******************************************************************************
* djinterp [c][test]                                  test_options_tests_masks.c
*
* test_options.h's suite, block "config_mask_all": each D_TEST_MASK_* mask, and
* that the message and settings masks do not overlap.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_masks.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 2.  MASK DEFINITION TESTS
//==============================================================================

/*
d_tests_options_mask_message
  Tests the D_TEST_MASK_MESSAGE_FLAGS constant.
  Tests the following:
  - Value is 0x0000FFFF
  - Covers all 16 lower bits
  - Does not overlap with settings mask
*/
bool
d_tests_options_mask_message(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: value is 0x0000FFFF
    d_test_suite_test(_suite,
                      "config_mask_message");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_message_value",
                              D_TEST_MASK_MESSAGE_FLAGS == 0x0000FFFFu,
                              "D_TEST_MASK_MESSAGE_FLAGS should be 0x0000FFFF") && result;

    // test 2: all 16 lower bits are set
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_message_lower_bits",
                              (D_TEST_MASK_MESSAGE_FLAGS & 0x0000FFFFu) == 0x0000FFFFu,
                              "Message mask should cover all lower 16 bits") && result;

    // test 3: no upper bits set
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_message_no_upper",
                              (D_TEST_MASK_MESSAGE_FLAGS & 0xFFFF0000u) == 0,
                              "Message mask should not set any upper bits") && result;

    return result;
}

/*
d_tests_options_mask_settings
  Tests the D_TEST_MASK_SETTINGS_FLAGS constant.
  Tests the following:
  - Value is 0xFFFF0000
  - Covers all 16 upper bits
  - No lower bits set
*/
bool
d_tests_options_mask_settings(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: value is 0xFFFF0000
    d_test_suite_test(_suite,
                      "config_mask_settings");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_settings_value",
                              D_TEST_MASK_SETTINGS_FLAGS == 0xFFFF0000u,
                              "D_TEST_MASK_SETTINGS_FLAGS should be 0xFFFF0000") && result;

    // test 2: no lower bits set
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_settings_no_lower",
                              (D_TEST_MASK_SETTINGS_FLAGS & 0x0000FFFFu) == 0,
                              "Settings mask should not set any lower bits") && result;

    return result;
}

/*
d_tests_options_mask_counter
  Tests the D_TEST_MASK_COUNTER_FLAGS constant.
  Tests the following:
  - Value is 0x000000FF (bits 0-7)
  - Is a subset of message flags mask
*/
bool
d_tests_options_mask_counter(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: value is 0x000000FF
    d_test_suite_test(_suite,
                      "config_mask_counter");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_counter_value",
                              D_TEST_MASK_COUNTER_FLAGS == 0x000000FFu,
                              "D_TEST_MASK_COUNTER_FLAGS should be 0x000000FF") && result;

    // test 2: is a subset of message mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_counter_subset_of_message",
                              (D_TEST_MASK_COUNTER_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) == D_TEST_MASK_COUNTER_FLAGS,
                              "Counter mask should be a subset of message mask") && result;

    return result;
}

/*
d_tests_options_mask_print
  Tests the D_TEST_MASK_PRINT_FLAGS constant.
  Tests the following:
  - Value is 0x0000FF00 (bits 8-15)
  - Is a subset of message flags mask
  - Does not overlap with counter mask
*/
bool
d_tests_options_mask_print(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: value is 0x0000FF00
    d_test_suite_test(_suite,
                      "config_mask_print");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_print_value",
                              D_TEST_MASK_PRINT_FLAGS == 0x0000FF00u,
                              "D_TEST_MASK_PRINT_FLAGS should be 0x0000FF00") && result;

    // test 2: is a subset of message mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_print_subset_of_message",
                              (D_TEST_MASK_PRINT_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) == D_TEST_MASK_PRINT_FLAGS,
                              "Print mask should be a subset of message mask") && result;

    // test 3: no overlap with counter mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_print_no_overlap_counter",
                              (D_TEST_MASK_PRINT_FLAGS & D_TEST_MASK_COUNTER_FLAGS) == 0,
                              "Print mask should not overlap with counter mask") && result;

    return result;
}

/*
d_tests_options_mask_stack
  Tests the D_TEST_MASK_STACK_FLAGS constant.
  Tests the following:
  - Value is 0x000F0000 (bits 16-19)
  - Is a subset of settings flags mask
  - Does not overlap with message mask
*/
bool
d_tests_options_mask_stack(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: value is 0x000F0000
    d_test_suite_test(_suite,
                      "config_mask_stack");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_stack_value",
                              D_TEST_MASK_STACK_FLAGS == 0x000F0000u,
                              "D_TEST_MASK_STACK_FLAGS should be 0x000F0000") && result;

    // test 2: is a subset of settings mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_stack_subset_of_settings",
                              (D_TEST_MASK_STACK_FLAGS & D_TEST_MASK_SETTINGS_FLAGS) == D_TEST_MASK_STACK_FLAGS,
                              "Stack mask should be a subset of settings mask") && result;

    // test 3: no overlap with message mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_stack_no_overlap_message",
                              (D_TEST_MASK_STACK_FLAGS & D_TEST_MASK_MESSAGE_FLAGS) == 0,
                              "Stack mask should not overlap with message mask") && result;

    return result;
}

/*
d_tests_options_mask_no_overlap
  Tests that masks are mutually exclusive where expected.
  Tests the following:
  - Message and settings masks fully partition the 32-bit space
  - Counter and print masks partition the message space
*/
bool
d_tests_options_mask_no_overlap(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: message and settings together cover all 32 bits
    d_test_suite_test(_suite,
                      "config_mask_no_overlap");

    result = D_TEST_CHECK_MSG(_suite,
                              "mask_full_coverage",
                              (D_TEST_MASK_MESSAGE_FLAGS | D_TEST_MASK_SETTINGS_FLAGS) == 0xFFFFFFFFu,
                              "Message + settings masks should cover all 32 bits") && result;

    // test 2: message and settings do not overlap
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_no_overlap_msg_settings",
                              (D_TEST_MASK_MESSAGE_FLAGS & D_TEST_MASK_SETTINGS_FLAGS) == 0,
                              "Message and settings masks should not overlap") && result;

    // test 3: counter and print together cover the message mask
    result = D_TEST_CHECK_MSG(_suite,
                              "mask_counter_print_cover_message",
                              (D_TEST_MASK_COUNTER_FLAGS | D_TEST_MASK_PRINT_FLAGS) == D_TEST_MASK_MESSAGE_FLAGS,
                              "Counter + print masks should equal message mask") && result;

    return result;
}

/*
d_tests_options_mask_all
  Aggregation function that runs all mask definition tests.
*/
bool
d_tests_options_mask_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_block(_suite,
                       "config_mask_all");

    result = d_tests_options_mask_message(_suite)    && result;
    result = d_tests_options_mask_settings(_suite)   && result;
    result = d_tests_options_mask_counter(_suite)    && result;
    result = d_tests_options_mask_print(_suite)      && result;
    result = d_tests_options_mask_stack(_suite)      && result;
    result = d_tests_options_mask_no_overlap(_suite) && result;

    return result;
}
