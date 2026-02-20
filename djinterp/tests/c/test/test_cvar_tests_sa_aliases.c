#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * VI. ALIAS LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_alias_enabled
  Tests the "enabled" alias for "config-enabled".
  Tests the following:
  - "enabled" resolves to a non-NULL row
  - Resolved row has the same flag as the "config-enabled" row
*/
bool
d_tests_sa_cvar_alias_enabled
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* alias_row;
    struct d_test_registry_row* primary_row;

    result = true;

    d_test_registry_init();

    alias_row   = d_test_registry_find("enabled");
    primary_row = d_test_registry_find("config-enabled");

    // test 1: alias resolves to non-NULL
    result = d_assert_standalone(
        alias_row != NULL,
        "alias_enabled_non_null",
        "'enabled' alias should resolve to a row",
        _counter) && result;

    // test 2: alias resolves to the same row as primary key
    result = d_assert_standalone(
        alias_row != NULL && primary_row != NULL &&
            alias_row->flag == primary_row->flag,
        "alias_enabled_same_flag",
        "'enabled' should resolve to same flag as 'config-enabled'",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_alias_indent
  Tests the "indent" alias for "indent-string".
  Tests the following:
  - "indent" resolves to a non-NULL row
  - Resolved row flag matches D_TEST_CONFIG_INDENT_STR
*/
bool
d_tests_sa_cvar_alias_indent
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* alias_row;

    result = true;

    d_test_registry_init();

    alias_row = d_test_registry_find("indent");

    // test 1: alias resolves to non-NULL
    result = d_assert_standalone(
        alias_row != NULL,
        "alias_indent_non_null",
        "'indent' alias should resolve to a row",
        _counter) && result;

    // test 2: flag matches D_TEST_CONFIG_INDENT_STR
    result = d_assert_standalone(
        alias_row != NULL &&
            alias_row->flag == D_TEST_CONFIG_INDENT_STR,
        "alias_indent_flag_matches",
        "'indent' should map to D_TEST_CONFIG_INDENT_STR",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_alias_max_indent
  Tests the "indent-max" and "indent-level" aliases for "max-indent".
  Tests the following:
  - "indent-max" resolves to the "max-indent" row
  - "indent-level" resolves to the "max-indent" row
  - Both aliases resolve to the same flag
*/
bool
d_tests_sa_cvar_alias_max_indent
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* primary_row;
    struct d_test_registry_row* alias_max;
    struct d_test_registry_row* alias_level;

    result = true;

    d_test_registry_init();

    primary_row = d_test_registry_find("max-indent");
    alias_max   = d_test_registry_find("indent-max");
    alias_level = d_test_registry_find("indent-level");

    // test 1: "indent-max" resolves to non-NULL
    result = d_assert_standalone(
        alias_max != NULL,
        "alias_indent_max_non_null",
        "'indent-max' alias should resolve to a row",
        _counter) && result;

    // test 2: "indent-level" resolves to non-NULL
    result = d_assert_standalone(
        alias_level != NULL,
        "alias_indent_level_non_null",
        "'indent-level' alias should resolve to a row",
        _counter) && result;

    // test 3: both aliases match the primary row flag
    result = d_assert_standalone(
        alias_max != NULL && primary_row != NULL &&
            alias_max->flag == primary_row->flag,
        "alias_indent_max_same_flag",
        "'indent-max' should map to same flag as 'max-indent'",
        _counter) && result;

    result = d_assert_standalone(
        alias_level != NULL && primary_row != NULL &&
            alias_level->flag == primary_row->flag,
        "alias_indent_level_same_flag",
        "'indent-level' should map to same flag as 'max-indent'",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_alias_timeout
  Tests the "timeout-ms" alias for "timeout".
  Tests the following:
  - "timeout-ms" resolves to the "timeout" row
  - Resolved flag matches D_TEST_CONFIG_TIMEOUT_MS
*/
bool
d_tests_sa_cvar_alias_timeout
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* alias_row;

    result = true;

    d_test_registry_init();

    alias_row = d_test_registry_find("timeout-ms");

    // test 1: alias resolves to non-NULL
    result = d_assert_standalone(
        alias_row != NULL,
        "alias_timeout_ms_non_null",
        "'timeout-ms' alias should resolve to a row",
        _counter) && result;

    // test 2: flag matches D_TEST_CONFIG_TIMEOUT_MS
    result = d_assert_standalone(
        alias_row != NULL &&
            alias_row->flag == D_TEST_CONFIG_TIMEOUT_MS,
        "alias_timeout_ms_flag_matches",
        "'timeout-ms' should map to D_TEST_CONFIG_TIMEOUT_MS",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_alias_name_shortcuts
  Tests the "framework", "module", "submodule" aliases.
  Tests the following:
  - "framework" resolves to D_TEST_METADATA_FRAMEWORK_NAME
  - "module" resolves to D_TEST_METADATA_MODULE_NAME
  - "submodule" resolves to D_TEST_METADATA_SUBMODULE_NAME
*/
bool
d_tests_sa_cvar_alias_name_shortcuts
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: "framework" alias
    row = d_test_registry_find("framework");

    result = d_assert_standalone(
        row != NULL &&
            row->flag == D_TEST_METADATA_FRAMEWORK_NAME,
        "alias_framework_flag",
        "'framework' should map to D_TEST_METADATA_FRAMEWORK_NAME",
        _counter) && result;

    // test 2: "module" alias
    row = d_test_registry_find("module");

    result = d_assert_standalone(
        row != NULL &&
            row->flag == D_TEST_METADATA_MODULE_NAME,
        "alias_module_flag",
        "'module' should map to D_TEST_METADATA_MODULE_NAME",
        _counter) && result;

    // test 3: "submodule" alias
    row = d_test_registry_find("submodule");

    result = d_assert_standalone(
        row != NULL &&
            row->flag == D_TEST_METADATA_SUBMODULE_NAME,
        "alias_submodule_flag",
        "'submodule' should map to D_TEST_METADATA_SUBMODULE_NAME",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_alias_all
  Aggregation function that runs all alias lookup tests.
*/
bool
d_tests_sa_cvar_alias_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Alias Lookup\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_alias_enabled(_counter)        && result;
    result = d_tests_sa_cvar_alias_indent(_counter)         && result;
    result = d_tests_sa_cvar_alias_max_indent(_counter)     && result;
    result = d_tests_sa_cvar_alias_timeout(_counter)        && result;
    result = d_tests_sa_cvar_alias_name_shortcuts(_counter) && result;

    return result;
}
