/*******************************************************************************
* djinterp [test]                                   test_cvar_tests_row_struct.c
*
* test_cvar.h's suite, block "cvar_row_struct_all": dj_test_registry_row's
* members, the key first, then flag, command_flags, value_type, value and help.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_row_struct.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 3.  ROW STRUCTURE TESTS
//==============================================================================

/*
d_tests_cvar_row_struct_key
  Tests the key member of dj_test_registry_row.
  Tests the following:
  - key member is accessible
  - key is the first member (offset 0)
*/
bool
d_tests_cvar_row_struct_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_struct_key");

    bool                        result = true;
    struct dj_test_registry_row row    = {0};

    // test 1: key member is accessible
    row.key = "test-key";

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_key_accessible",
                              row.key != NULL,
                              "key member should be accessible") && result;

    // test 2: key is the first member (offset 0)
    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_key_offset_zero",
                              offsetof(struct dj_test_registry_row,
                                       key) == 0,
                              "key must be the first member (offset 0) for registry lookup") && result;

    return result;
}

/*
d_tests_cvar_row_struct_flag
  Tests the flag member of dj_test_registry_row.
  Tests the following:
  - flag member is accessible
  - flag member stores uint32_t values
*/
bool
d_tests_cvar_row_struct_flag(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_struct_flag");

    bool                        result = true;
    struct dj_test_registry_row row    = {0};

    // test 1: flag member is accessible
    row.flag = D_TEST_OPTIONS_ENABLED;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_flag_accessible",
                              row.flag == D_TEST_OPTIONS_ENABLED,
                              "flag member should store DTestConfigKey value") && result;

    // test 2: flag member stores metadata flag
    row.flag = D_TEST_METADATA_AUTHORS;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_flag_metadata",
                              row.flag == D_TEST_METADATA_AUTHORS,
                              "flag member should store DTestMetadataFlag value") && result;

    return result;
}

/*
d_tests_cvar_row_struct_command_flags
  Tests the command_flags member of dj_test_registry_row.
  Tests the following:
  - command_flags member is accessible
  - command_flags stores DTestRegistryRowFlag values
*/
bool
d_tests_cvar_row_struct_command_flags(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_struct_command_flags");

    bool                        result = true;
    struct dj_test_registry_row row    = {0};

    // test 1: command_flags is accessible with IS_CONFIG
    row.command_flags = D_TEST_REGISTRY_FLAG_IS_CONFIG;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_command_flags_config",
                              row.command_flags == D_TEST_REGISTRY_FLAG_IS_CONFIG,
                              "command_flags should store IS_CONFIG") && result;

    // test 2: command_flags supports bitwise combination
    row.command_flags = D_TEST_REGISTRY_FLAG_IS_CONFIG |
                        D_TEST_REGISTRY_FLAG_IS_REQUIRED;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_command_flags_bitwise",
                              (row.command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0,
                              "command_flags should support bitwise combinations") && result;

    return result;
}

/*
d_tests_cvar_row_struct_value_type
  Tests the value_type member of dj_test_registry_row.
  Tests the following:
  - value_type member is accessible
  - value_type stores d_type_info values
*/
bool
d_tests_cvar_row_struct_value_type(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_struct_value_type");

    bool                        result = true;
    struct dj_test_registry_row row    = {0};

    // test 1: value_type is accessible
    row.value_type = D_TYPE_INFO_BOOL;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_value_type_bool",
                              row.value_type == D_TYPE_INFO_BOOL,
                              "value_type should store D_TYPE_INFO_BOOL") && result;

    // test 2: value_type stores string type
    row.value_type = D_TYPE_INFO_STRING;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_value_type_string",
                              row.value_type == D_TYPE_INFO_STRING,
                              "value_type should store D_TYPE_INFO_STRING") && result;

    return result;
}

/*
d_tests_cvar_row_struct_value_help
  Tests the value and help members of dj_test_registry_row.
  Tests the following:
  - value member (dj_test_value union) is accessible
  - help member stores a string pointer
*/
bool
d_tests_cvar_row_struct_value_help(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_struct_value_help");

    bool                        result = true;
    struct dj_test_registry_row row    = {0};

    // test 1: value union is accessible through row
    row.value.b = true;

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_value_accessible",
                              row.value.b == true,
                              "value union member should be accessible through row") && result;

    // test 2: help member stores a string
    row.help = "Some help text";

    result = D_TEST_CHECK_MSG(_suite,
                              "row_struct_help_accessible",
                              row.help != NULL,
                              "help member should store a string pointer") && result;

    return result;
}

/*
d_tests_cvar_row_struct_all
  Aggregation function that runs all row structure tests.
*/
bool
d_tests_cvar_row_struct_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_row_struct_all");

    bool result = true;

    result = d_tests_cvar_row_struct_key(_suite)           && result;
    result = d_tests_cvar_row_struct_flag(_suite)          && result;
    result = d_tests_cvar_row_struct_command_flags(_suite)  && result;
    result = d_tests_cvar_row_struct_value_type(_suite)     && result;
    result = d_tests_cvar_row_struct_value_help(_suite)     && result;

    return result;
}
