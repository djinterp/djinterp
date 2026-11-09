/*******************************************************************************
* djinterp [test]                                 test_cvar_tests_typed_macros.c
*
* test_cvar.h's suite, block "cvar_typed_macro_all": the D_TEST_REGISTRY_* typed
* access macros, also with an unknown key.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_typed_macros.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 10. TYPED ACCESS MACRO TESTS
//==============================================================================

/*
d_tests_cvar_macro_get_row
  Tests D_TEST_REGISTRY_GET macro.
  Tests the following:
  - Valid key returns non-NULL row pointer
  - Returned row has correct key
*/
bool
d_tests_cvar_macro_get_row(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_get_row");

    bool result = true;

    d_test_registry_init();

    // test 1: valid key returns non-NULL
    struct dj_test_registry_row* row = D_TEST_REGISTRY_GET("config-enabled");

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_get_row_non_null",
                              row != NULL,
                              "D_TEST_REGISTRY_GET('config-enabled') should return non-NULL") && result;

    // test 2: returned row has matching key
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_get_row_key_matches",
                              ( (row != NULL) &&
                                (d_strcasecmp(row->key,
                                              "config-enabled") == 0) ),
                              "Returned row key should match 'config-enabled'") && result;

    return result;
}

/*
d_tests_cvar_macro_value_bool
  Tests D_TEST_REGISTRY_VALUE_BOOL macro.
  Tests the following:
  - Returns true for config-enabled (default)
  - Returns false for skip (default)
*/
bool
d_tests_cvar_macro_value_bool(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_value_bool");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: config-enabled default is true
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_bool_enabled",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "D_TEST_REGISTRY_VALUE_BOOL('config-enabled') should be true") && result;

    // test 2: skip default is false
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_bool_skip",
                              D_TEST_REGISTRY_VALUE_BOOL("skip") == false,
                              "D_TEST_REGISTRY_VALUE_BOOL('skip') should be false") && result;

    return result;
}

/*
d_tests_cvar_macro_value_size_t
  Tests D_TEST_REGISTRY_VALUE_SIZE_T macro.
  Tests the following:
  - timeout returns D_TEST_DEFAULT_TIMEOUT
  - max-failures returns D_TEST_DEFAULT_MAX_FAILURES
*/
bool
d_tests_cvar_macro_value_size_t(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_value_size_t");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: timeout returns default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_size_t_timeout",
                              D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
                              "D_TEST_REGISTRY_VALUE_SIZE_T('timeout') should be default timeout") && result;

    // test 2: max-failures returns default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_size_t_max_failures",
                              D_TEST_REGISTRY_VALUE_SIZE_T("max-failures") == D_TEST_DEFAULT_MAX_FAILURES,
                              "D_TEST_REGISTRY_VALUE_SIZE_T('max-failures') should be default") && result;

    return result;
}

/*
d_tests_cvar_macro_value_numeric
  Tests D_TEST_REGISTRY_VALUE_UINT32, UINT16, and INT32 macros.
  Tests the following:
  - message-flags returns uint32 default (0)
  - max-indent returns uint16 default (D_TEST_DEFAULT_MAX_INDENT)
  - priority returns int32 default (0)
*/
bool
d_tests_cvar_macro_value_numeric(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_value_numeric");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: message-flags uint32 default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_uint32_msg_flags",
                              D_TEST_REGISTRY_VALUE_UINT32("message-flags") == 0,
                              "D_TEST_REGISTRY_VALUE_UINT32('message-flags') should be 0") && result;

    // test 2: max-indent uint16 default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_uint16_max_indent",
                              D_TEST_REGISTRY_VALUE_UINT16("max-indent") == D_TEST_DEFAULT_MAX_INDENT,
                              "D_TEST_REGISTRY_VALUE_UINT16('max-indent') should be default") && result;

    // test 3: priority int32 default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_int32_priority",
                              D_TEST_REGISTRY_VALUE_INT32("priority") == 0,
                              "D_TEST_REGISTRY_VALUE_INT32('priority') should be 0") && result;

    return result;
}

/*
d_tests_cvar_macro_value_ptr
  Tests D_TEST_REGISTRY_VALUE_PTR macro.
  Tests the following:
  - indent-string returns D_TEST_DEFAULT_INDENT (non-NULL)
  - authors returns NULL (metadata default)
*/
bool
d_tests_cvar_macro_value_ptr(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_value_ptr");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: indent-string returns non-NULL default
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_ptr_indent",
                              D_TEST_REGISTRY_VALUE_PTR("indent-string") != NULL,
                              "D_TEST_REGISTRY_VALUE_PTR('indent-string') should be non-NULL") && result;

    // test 2: authors returns NULL (metadata default)
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_ptr_authors",
                              D_TEST_REGISTRY_VALUE_PTR("authors") == NULL,
                              "D_TEST_REGISTRY_VALUE_PTR('authors') should be NULL") && result;

    return result;
}

/*
d_tests_cvar_macro_metadata_fields
  Tests D_TEST_REGISTRY_HELP, D_TEST_REGISTRY_FLAG, and D_TEST_REGISTRY_TYPE.
  Tests the following:
  - HELP returns non-NULL for valid key
  - FLAG returns expected enum for config-enabled
  - TYPE returns expected d_type_info for config-enabled
*/
bool
d_tests_cvar_macro_metadata_fields(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_metadata_fields");

    bool result = true;

    d_test_registry_init();

    // test 1: HELP returns non-NULL for config-enabled
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_help_non_null",
                              D_TEST_REGISTRY_HELP("config-enabled") != NULL,
                              "D_TEST_REGISTRY_HELP('config-enabled') should be non-NULL") && result;

    // test 2: FLAG returns D_TEST_OPTIONS_ENABLED
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_flag_config_enabled",
                              D_TEST_REGISTRY_FLAG("config-enabled") == D_TEST_OPTIONS_ENABLED,
                              "D_TEST_REGISTRY_FLAG('config-enabled') should be D_TEST_OPTIONS_ENABLED") && result;

    // test 3: TYPE returns D_TYPE_INFO_BOOL for config-enabled
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_type_config_enabled",
                              D_TEST_REGISTRY_TYPE("config-enabled") == D_TYPE_INFO_BOOL,
                              "D_TEST_REGISTRY_TYPE('config-enabled') should be D_TYPE_INFO_BOOL") && result;

    return result;
}

/*
d_tests_cvar_macro_invalid_key
  Tests typed access macros with an invalid key.
  Tests the following:
  - D_TEST_REGISTRY_GET returns NULL for invalid key
  - VALUE_BOOL returns false for invalid key
  - VALUE_SIZE_T returns 0 for invalid key
  - VALUE_PTR returns NULL for invalid key
  - HELP returns NULL for invalid key
*/
bool
d_tests_cvar_macro_invalid_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_macro_invalid_key");

    bool result = true;

    d_test_registry_init();

    // test 1: GET returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_get_invalid_null",
                              D_TEST_REGISTRY_GET("no-such-key") == NULL,
                              "D_TEST_REGISTRY_GET('no-such-key') should be NULL") && result;

    // test 2: VALUE_BOOL returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_bool_invalid",
                              D_TEST_REGISTRY_VALUE_BOOL("no-such-key") == false,
                              "D_TEST_REGISTRY_VALUE_BOOL('no-such-key') should be false") && result;

    // test 3: VALUE_SIZE_T returns 0
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_size_t_invalid",
                              D_TEST_REGISTRY_VALUE_SIZE_T("no-such-key") == 0,
                              "D_TEST_REGISTRY_VALUE_SIZE_T('no-such-key') should be 0") && result;

    // test 4: VALUE_PTR returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_value_ptr_invalid",
                              D_TEST_REGISTRY_VALUE_PTR("no-such-key") == NULL,
                              "D_TEST_REGISTRY_VALUE_PTR('no-such-key') should be NULL") && result;

    // test 5: HELP returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_help_invalid",
                              D_TEST_REGISTRY_HELP("no-such-key") == NULL,
                              "D_TEST_REGISTRY_HELP('no-such-key') should be NULL") && result;

    return result;
}

/*
d_tests_cvar_typed_macro_all
  Aggregation function that runs all typed access macro tests.
*/
bool
d_tests_cvar_typed_macro_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_typed_macro_all");

    bool result = true;

    result = d_tests_cvar_macro_get_row(_suite)         && result;
    result = d_tests_cvar_macro_value_bool(_suite)      && result;
    result = d_tests_cvar_macro_value_size_t(_suite)    && result;
    result = d_tests_cvar_macro_value_numeric(_suite)   && result;
    result = d_tests_cvar_macro_value_ptr(_suite)       && result;
    result = d_tests_cvar_macro_metadata_fields(_suite) && result;
    result = d_tests_cvar_macro_invalid_key(_suite)     && result;

    return result;
}
