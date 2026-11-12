/*******************************************************************************
* djinterp [c][test]                                 test_options_tests_struct.c
*
* test_options.h's suite, block "config_struct_all": dj_test_options' members,
* its size and its layout.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_struct.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 9.  CORE STRUCTURE TESTS
//==============================================================================

/*
d_tests_options_struct_members
  Tests the dj_test_options structure members.
  Tests the following:
  - flags member is accessible and writable
  - settings member is accessible
  - stage_hooks member is accessible
  - Members have correct types (by assignment compatibility)
*/
bool
d_tests_options_struct_members(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: flags member is accessible
    d_test_suite_test(_suite,
                      "config_struct_members");

    struct dj_test_options config_stack = {0};

    config_stack.flags = 0x12345678u;

    result = D_TEST_CHECK_MSG(_suite,
                              "struct_flags_accessible",
                              config_stack.flags == 0x12345678u,
                              "flags member should be accessible and writable") && result;

    // test 2: settings member is accessible
    config_stack.settings = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "struct_settings_accessible",
                              config_stack.settings == NULL,
                              "settings member should be accessible") && result;

    // test 3: stage_hooks member is accessible
    config_stack.stage_hooks = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "struct_stage_hooks_accessible",
                              config_stack.stage_hooks == NULL,
                              "stage_hooks member should be accessible") && result;

    return result;
}

/*
d_tests_options_struct_size
  Tests the dj_test_options structure size and layout.
  Tests the following:
  - Structure has non-zero size
  - Size is at least large enough for its members
*/
bool
d_tests_options_struct_size(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_struct_size");

    const size_t config_size = sizeof(struct dj_test_options);

    // test 1: non-zero size
    result = D_TEST_CHECK_MSG(_suite,
                              "struct_size_nonzero",
                              config_size > 0,
                              "dj_test_config should have non-zero size") && result;

    // test 2: at least sizeof(uint32_t) + 2 pointers
    result = D_TEST_CHECK_MSG(_suite,
                              "struct_size_minimum",
                              config_size >= (sizeof(uint32_t) + sizeof(struct dj_min_enum_map*) + sizeof(struct dj_min_enum_map*)),
                              "dj_test_config should fit flags + two pointers") && result;

    return result;
}

/*
d_tests_options_struct_all
  Aggregation function that runs all core structure tests.
*/
bool
d_tests_options_struct_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "config_struct_all");

    bool result = true;

    result = d_tests_options_struct_members(_suite) && result;
    result = d_tests_options_struct_size(_suite) && result;

    return result;
}
