#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * III. ROW STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_row_struct_key
  Tests the key member of d_test_registry_row.
  Tests the following:
  - key member is accessible
  - key is the first member (offset 0)
*/
bool
d_tests_sa_cvar_row_struct_key
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row  row;

    result = true;

    // test 1: key member is accessible
    row.key = "test-key";

    result = d_assert_standalone(
        row.key != NULL,
        "row_struct_key_accessible",
        "key member should be accessible",
        _counter) && result;

    // test 2: key is the first member (offset 0)
    result = d_assert_standalone(
        offsetof(struct d_test_registry_row, key) == 0,
        "row_struct_key_offset_zero",
        "key must be the first member (offset 0) for registry lookup",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_struct_flag
  Tests the flag member of d_test_registry_row.
  Tests the following:
  - flag member is accessible
  - flag member stores uint32_t values
*/
bool
d_tests_sa_cvar_row_struct_flag
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row  row;

    result = true;

    // test 1: flag member is accessible
    row.flag = D_TEST_CONFIG_ENABLED;

    result = d_assert_standalone(
        row.flag == D_TEST_CONFIG_ENABLED,
        "row_struct_flag_accessible",
        "flag member should store DTestConfigKey value",
        _counter) && result;

    // test 2: flag member stores metadata flag
    row.flag = D_TEST_METADATA_AUTHORS;

    result = d_assert_standalone(
        row.flag == D_TEST_METADATA_AUTHORS,
        "row_struct_flag_metadata",
        "flag member should store DTestMetadataFlag value",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_struct_command_flags
  Tests the command_flags member of d_test_registry_row.
  Tests the following:
  - command_flags member is accessible
  - command_flags stores DTestRegistryRowFlag values
*/
bool
d_tests_sa_cvar_row_struct_command_flags
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row  row;

    result = true;

    // test 1: command_flags is accessible with IS_CONFIG
    row.command_flags = D_TEST_REGISTRY_FLAG_IS_CONFIG;

    result = d_assert_standalone(
        row.command_flags == D_TEST_REGISTRY_FLAG_IS_CONFIG,
        "row_struct_command_flags_config",
        "command_flags should store IS_CONFIG",
        _counter) && result;

    // test 2: command_flags supports bitwise combination
    row.command_flags = D_TEST_REGISTRY_FLAG_IS_CONFIG |
                        D_TEST_REGISTRY_FLAG_IS_REQUIRED;

    result = d_assert_standalone(
        (row.command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0,
        "row_struct_command_flags_bitwise",
        "command_flags should support bitwise combinations",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_struct_value_type
  Tests the value_type member of d_test_registry_row.
  Tests the following:
  - value_type member is accessible
  - value_type stores d_type_info values
*/
bool
d_tests_sa_cvar_row_struct_value_type
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row  row;

    result = true;

    // test 1: value_type is accessible
    row.value_type = D_TYPE_INFO_BOOL;

    result = d_assert_standalone(
        row.value_type == D_TYPE_INFO_BOOL,
        "row_struct_value_type_bool",
        "value_type should store D_TYPE_INFO_BOOL",
        _counter) && result;

    // test 2: value_type stores string type
    row.value_type = D_TYPE_INFO_STRING;

    result = d_assert_standalone(
        row.value_type == D_TYPE_INFO_STRING,
        "row_struct_value_type_string",
        "value_type should store D_TYPE_INFO_STRING",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_struct_value_help
  Tests the value and help members of d_test_registry_row.
  Tests the following:
  - value member (d_test_value union) is accessible
  - help member stores a string pointer
*/
bool
d_tests_sa_cvar_row_struct_value_help
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row  row;

    result = true;

    // test 1: value union is accessible through row
    row.value.b = true;

    result = d_assert_standalone(
        row.value.b == true,
        "row_struct_value_accessible",
        "value union member should be accessible through row",
        _counter) && result;

    // test 2: help member stores a string
    row.help = "Some help text";

    result = d_assert_standalone(
        row.help != NULL,
        "row_struct_help_accessible",
        "help member should store a string pointer",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_struct_all
  Aggregation function that runs all row structure tests.
*/
bool
d_tests_sa_cvar_row_struct_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Row Structure\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_row_struct_key(_counter)           && result;
    result = d_tests_sa_cvar_row_struct_flag(_counter)          && result;
    result = d_tests_sa_cvar_row_struct_command_flags(_counter)  && result;
    result = d_tests_sa_cvar_row_struct_value_type(_counter)     && result;
    result = d_tests_sa_cvar_row_struct_value_help(_counter)     && result;

    return result;
}
