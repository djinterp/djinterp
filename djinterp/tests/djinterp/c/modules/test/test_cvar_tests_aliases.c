/*******************************************************************************
* djinterp [test]                                      test_cvar_tests_aliases.c
*
* test_cvar.h's suite, block "cvar_alias_all": the aliases enabled, indent,
* indent-max and indent-level, timeout-ms, framework, module and submodule.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_aliases.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 6.  ALIAS LOOKUP TESTS
//==============================================================================

/*
d_tests_cvar_alias_enabled
  Tests the "enabled" alias for "config-enabled".
  Tests the following:
  - "enabled" resolves to a non-NULL row
  - Resolved row has the same flag as the "config-enabled" row
*/
bool
d_tests_cvar_alias_enabled(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_alias_enabled");

    bool result = true;

    d_test_registry_init();

    struct dj_test_registry_row* alias_row = d_test_registry_find("enabled");
    struct dj_test_registry_row* primary_row =
        d_test_registry_find("config-enabled");

    // test 1: alias resolves to non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_enabled_non_null",
                              alias_row != NULL,
                              "'enabled' alias should resolve to a row") && result;

    // test 2: alias resolves to the same row as primary key
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_enabled_same_flag",
                              ( (alias_row != NULL)   &&
                                (primary_row != NULL) &&
                                (alias_row->flag == primary_row->flag) ),
                              "'enabled' should resolve to same flag as 'config-enabled'") && result;

    return result;
}

/*
d_tests_cvar_alias_indent
  Tests the "indent" alias for "indent-string".
  Tests the following:
  - "indent" resolves to a non-NULL row
  - Resolved row flag matches D_TEST_OPTIONS_INDENT_STR
*/
bool
d_tests_cvar_alias_indent(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_alias_indent");

    bool result = true;

    d_test_registry_init();

    struct dj_test_registry_row* alias_row = d_test_registry_find("indent");

    // test 1: alias resolves to non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_non_null",
                              alias_row != NULL,
                              "'indent' alias should resolve to a row") && result;

    // test 2: flag matches D_TEST_OPTIONS_INDENT_STR
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_flag_matches",
                              ( (alias_row != NULL) &&
                                (alias_row->flag == D_TEST_OPTIONS_INDENT_STR) ),
                              "'indent' should map to D_TEST_OPTIONS_INDENT_STR") && result;

    return result;
}

/*
d_tests_cvar_alias_max_indent
  Tests the "indent-max" and "indent-level" aliases for "max-indent".
  Tests the following:
  - "indent-max" resolves to the "max-indent" row
  - "indent-level" resolves to the "max-indent" row
  - Both aliases resolve to the same flag
*/
bool
d_tests_cvar_alias_max_indent(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_alias_max_indent");

    bool result = true;

    d_test_registry_init();

    struct dj_test_registry_row* primary_row =
        d_test_registry_find("max-indent");
    struct dj_test_registry_row* alias_max = d_test_registry_find("indent-max");
    struct dj_test_registry_row* alias_level =
        d_test_registry_find("indent-level");

    // test 1: "indent-max" resolves to non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_max_non_null",
                              alias_max != NULL,
                              "'indent-max' alias should resolve to a row") && result;

    // test 2: "indent-level" resolves to non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_level_non_null",
                              alias_level != NULL,
                              "'indent-level' alias should resolve to a row") && result;

    // test 3: both aliases match the primary row flag
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_max_same_flag",
                              ( (alias_max != NULL)   &&
                                (primary_row != NULL) &&
                                (alias_max->flag == primary_row->flag) ),
                              "'indent-max' should map to same flag as 'max-indent'") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "alias_indent_level_same_flag",
                              ( (alias_level != NULL) &&
                                (primary_row != NULL) &&
                                (alias_level->flag == primary_row->flag) ),
                              "'indent-level' should map to same flag as 'max-indent'") && result;

    return result;
}

/*
d_tests_cvar_alias_timeout
  Tests the "timeout-ms" alias for "timeout".
  Tests the following:
  - "timeout-ms" resolves to the "timeout" row
  - Resolved flag matches D_TEST_OPTIONS_TIMEOUT_MS
*/
bool
d_tests_cvar_alias_timeout(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_alias_timeout");

    bool result = true;

    d_test_registry_init();

    struct dj_test_registry_row* alias_row = d_test_registry_find("timeout-ms");

    // test 1: alias resolves to non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_timeout_ms_non_null",
                              alias_row != NULL,
                              "'timeout-ms' alias should resolve to a row") && result;

    // test 2: flag matches D_TEST_OPTIONS_TIMEOUT_MS
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_timeout_ms_flag_matches",
                              ( (alias_row != NULL) &&
                                (alias_row->flag == D_TEST_OPTIONS_TIMEOUT_MS) ),
                              "'timeout-ms' should map to D_TEST_OPTIONS_TIMEOUT_MS") && result;

    return result;
}

/*
d_tests_cvar_alias_name_shortcuts
  Tests the "framework", "module", "submodule" aliases.
  Tests the following:
  - "framework" resolves to D_TEST_METADATA_FRAMEWORK_NAME
  - "module" resolves to D_TEST_METADATA_MODULE_NAME
  - "submodule" resolves to D_TEST_METADATA_SUBMODULE_NAME
*/
bool
d_tests_cvar_alias_name_shortcuts(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_alias_name_shortcuts");

    bool result = true;

    d_test_registry_init();

    // test 1: "framework" alias
    struct dj_test_registry_row* row = d_test_registry_find("framework");

    result = D_TEST_CHECK_MSG(_suite,
                              "alias_framework_flag",
                              ( (row != NULL) &&
                                (row->flag == D_TEST_METADATA_FRAMEWORK_NAME) ),
                              "'framework' should map to D_TEST_METADATA_FRAMEWORK_NAME") && result;

    // test 2: "module" alias
    row = d_test_registry_find("module");

    result = D_TEST_CHECK_MSG(_suite,
                              "alias_module_flag",
                              ( (row != NULL) &&
                                (row->flag == D_TEST_METADATA_MODULE_NAME) ),
                              "'module' should map to D_TEST_METADATA_MODULE_NAME") && result;

    // test 3: "submodule" alias
    row = d_test_registry_find("submodule");

    result = D_TEST_CHECK_MSG(_suite,
                              "alias_submodule_flag",
                              ( (row != NULL) &&
                                (row->flag == D_TEST_METADATA_SUBMODULE_NAME) ),
                              "'submodule' should map to D_TEST_METADATA_SUBMODULE_NAME") && result;

    return result;
}

/*
d_tests_cvar_alias_all
  Aggregation function that runs all alias lookup tests.
*/
bool
d_tests_cvar_alias_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_alias_all");

    bool result = true;

    result = d_tests_cvar_alias_enabled(_suite)        && result;
    result = d_tests_cvar_alias_indent(_suite)         && result;
    result = d_tests_cvar_alias_max_indent(_suite)     && result;
    result = d_tests_cvar_alias_timeout(_suite)        && result;
    result = d_tests_cvar_alias_name_shortcuts(_suite) && result;

    return result;
}
