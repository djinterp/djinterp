/*******************************************************************************
* djinterp [test]                                   test_options_tests_setters.c
*
* test_options.h's suite, block "config_setter_all": each d_test_options_set_*
* setter, on a NULL config, and with a key of the wrong type.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_setters.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 12. SETTER FUNCTION TESTS
//==============================================================================

/*
d_tests_options_set_bool
  Tests the d_test_options_set_bool function.
  Tests the following:
  - Returns true on success
  - Value is persisted and retrievable via get_bool
  - Can toggle value back and forth
*/
bool
d_tests_options_set_bool(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_bool");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_bool_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: set returns true on success
    result = D_TEST_CHECK_MSG(_suite,
                              "set_bool_returns_true",
                              d_test_options_set_bool(config,
                                                      D_TEST_OPTIONS_SKIP,
                                                      true) == true,
                              "set_bool should return true on success") && result;

    // test 2: value persisted
    result = D_TEST_CHECK_MSG(_suite,
                              "set_bool_persisted",
                              d_test_options_get_bool(config,
                                                      D_TEST_OPTIONS_SKIP) == true,
                              "set_bool value should be retrievable via get_bool") && result;

    // test 3: toggle back
    d_test_options_set_bool(config,
                            D_TEST_OPTIONS_SKIP,
                            false);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_bool_toggle",
                              d_test_options_get_bool(config,
                                                      D_TEST_OPTIONS_SKIP) == false,
                              "set_bool should allow toggling value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_size_t
  Tests the d_test_options_set_size_t function.
  Tests the following:
  - Returns true on success for size_t keys
  - Returns true for uint16_t keys (compatible type)
  - Value is persisted correctly
  - Large values are handled
*/
bool
d_tests_options_set_size_t(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_size_t");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_size_t_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: set TIMEOUT_MS (size_t schema type)
    result = D_TEST_CHECK_MSG(_suite,
                              "set_size_t_timeout_success",
                              d_test_options_set_size_t(config,
                                                        D_TEST_OPTIONS_TIMEOUT_MS,
                                                        2500) == true,
                              "set_size_t should succeed for TIMEOUT_MS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_size_t_timeout_value",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_TIMEOUT_MS) == 2500,
                              "TIMEOUT_MS should reflect set value") && result;

    // test 2: set INDENT_MAX_LEVEL (uint16_t schema type)
    result = D_TEST_CHECK_MSG(_suite,
                              "set_size_t_indent_max_success",
                              d_test_options_set_size_t(config,
                                                        D_TEST_OPTIONS_INDENT_MAX_LEVEL,
                                                        20) == true,
                              "set_size_t should succeed for uint16_t key") && result;

    // test 3: set MAX_FAILURES
    d_test_options_set_size_t(config,
                              D_TEST_OPTIONS_MAX_FAILURES,
                              100);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_size_t_max_failures",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_MAX_FAILURES) == 100,
                              "MAX_FAILURES should reflect set value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_int32
  Tests the d_test_options_set_int32 function.
  Tests the following:
  - Returns true on success for PRIORITY
  - Negative values are handled correctly
  - Zero value works
*/
bool
d_tests_options_set_int32(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_int32");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_int32_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: positive value
    result = D_TEST_CHECK_MSG(_suite,
                              "set_int32_positive",
                              d_test_options_set_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY,
                                                       10) == true,
                              "set_int32 should succeed for positive value") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_int32_positive_value",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY) == 10,
                              "PRIORITY should reflect positive set value") && result;

    // test 2: negative value
    d_test_options_set_int32(config,
                             D_TEST_OPTIONS_PRIORITY,
                             -99);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_int32_negative",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY) == -99,
                              "PRIORITY should reflect negative set value") && result;

    // test 3: zero value
    d_test_options_set_int32(config,
                             D_TEST_OPTIONS_PRIORITY,
                             0);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_int32_zero",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY) == 0,
                              "PRIORITY should reflect zero set value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_uint32
  Tests the d_test_options_set_uint32 function.
  Tests the following:
  - MESSAGE_FLAGS modifies config->flags directly
  - Other uint32_t keys use the override map
*/
bool
d_tests_options_set_uint32(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_uint32");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_SILENT);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_uint32_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: MESSAGE_FLAGS modifies flags directly
    result = D_TEST_CHECK_MSG(_suite,
                              "set_uint32_msg_flags_success",
                              d_test_options_set_uint32(config,
                                                        D_TEST_OPTIONS_MESSAGE_FLAGS,
                                                        D_TEST_MODE_VERBOSE) == true,
                              "set_uint32 for MESSAGE_FLAGS should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_uint32_msg_flags_direct",
                              config->flags == D_TEST_MODE_VERBOSE,
                              "MESSAGE_FLAGS should modify config->flags directly") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_string
  Tests the d_test_options_set_string function.
  Tests the following:
  - Returns true on success for INDENT_STR
  - Value is persisted and retrievable
  - Rejects non-string keys
*/
bool
d_tests_options_set_string(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_string");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_string_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: set INDENT_STR
    result = D_TEST_CHECK_MSG(_suite,
                              "set_string_indent_success",
                              d_test_options_set_string(config,
                                                        D_TEST_OPTIONS_INDENT_STR,
                                                        "    ") == true,
                              "set_string should succeed for INDENT_STR") && result;

    // test 2: value is retrievable
    const char* indent = d_test_options_get_string(config,
                                                   D_TEST_OPTIONS_INDENT_STR);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_string_indent_retrievable",
                              indent != NULL,
                              "Overridden indent should be retrievable") && result;

    // test 3: rejects non-string key
    result = D_TEST_CHECK_MSG(_suite,
                              "set_string_type_mismatch",
                              d_test_options_set_string(config,
                                                        D_TEST_OPTIONS_ENABLED,
                                                        "invalid") == false,
                              "set_string should reject non-string key") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_ptr
  Tests the d_test_options_set_ptr function.
  Tests the following:
  - Returns true on success
  - Value is persisted and retrievable
*/
bool
d_tests_options_set_ptr(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_ptr");

    int                     dummy  = 123;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_ptr_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: set and retrieve pointer
    result = D_TEST_CHECK_MSG(_suite,
                              "set_ptr_success",
                              d_test_options_set_ptr(config,
                                                     D_TEST_OPTIONS_INDENT_STR,
                                                     &dummy) == true,
                              "set_ptr should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_ptr_value",
                              d_test_options_get_ptr(config,
                                                     D_TEST_OPTIONS_INDENT_STR) == &dummy,
                              "get_ptr should return the set pointer") && result;

    // test 2: set NULL pointer
    d_test_options_set_ptr(config,
                           D_TEST_OPTIONS_INDENT_STR,
                           NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "set_ptr_null",
                              d_test_options_get_ptr(config,
                                                     D_TEST_OPTIONS_INDENT_STR) == NULL,
                              "get_ptr should return NULL after setting NULL") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_set_null_config
  Tests setter functions with NULL config.
  Tests the following:
  - All setters return false for NULL config
*/
bool
d_tests_options_set_null_config(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_null_config");

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_bool",
                              d_test_options_set_bool(NULL,
                                                      D_TEST_OPTIONS_SKIP,
                                                      true) == false,
                              "set_bool(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_size_t",
                              d_test_options_set_size_t(NULL,
                                                        D_TEST_OPTIONS_TIMEOUT_MS,
                                                        1) == false,
                              "set_size_t(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_int32",
                              d_test_options_set_int32(NULL,
                                                       D_TEST_OPTIONS_PRIORITY,
                                                       1) == false,
                              "set_int32(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_uint32",
                              d_test_options_set_uint32(NULL,
                                                        D_TEST_OPTIONS_MESSAGE_FLAGS,
                                                        1) == false,
                              "set_uint32(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_string",
                              d_test_options_set_string(NULL,
                                                        D_TEST_OPTIONS_INDENT_STR,
                                                        "x") == false,
                              "set_string(NULL) should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_ptr",
                              d_test_options_set_ptr(NULL,
                                                     D_TEST_OPTIONS_INDENT_STR,
                                                     NULL) == false,
                              "set_ptr(NULL) should return false") && result;

    return result;
}

/*
d_tests_options_set_type_mismatch
  Tests setter functions with type-mismatched keys.
  Tests the following:
  - set_bool rejects non-bool keys
  - set_int32 rejects non-int32 keys
  - set_string rejects non-string keys
*/
bool
d_tests_options_set_type_mismatch(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_set_type_mismatch");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "set_mismatch_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: set_bool on a size_t key
    result = D_TEST_CHECK_MSG(_suite,
                              "set_mismatch_bool_on_size_t",
                              d_test_options_set_bool(config,
                                                      D_TEST_OPTIONS_TIMEOUT_MS,
                                                      true) == false,
                              "set_bool should reject size_t key") && result;

    // test 2: set_int32 on a bool key
    result = D_TEST_CHECK_MSG(_suite,
                              "set_mismatch_int32_on_bool",
                              d_test_options_set_int32(config,
                                                       D_TEST_OPTIONS_ENABLED,
                                                       42) == false,
                              "set_int32 should reject bool key") && result;

    // test 3: set_string on a bool key
    result = D_TEST_CHECK_MSG(_suite,
                              "set_mismatch_string_on_bool",
                              d_test_options_set_string(config,
                                                        D_TEST_OPTIONS_SKIP,
                                                        "hello") == false,
                              "set_string should reject bool key") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_setter_all
  Aggregation function that runs all setter function tests.
*/
bool
d_tests_options_setter_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_block(_suite,
                       "config_setter_all");

    result = d_tests_options_set_bool(_suite)          && result;
    result = d_tests_options_set_size_t(_suite)        && result;
    result = d_tests_options_set_int32(_suite)         && result;
    result = d_tests_options_set_uint32(_suite)        && result;
    result = d_tests_options_set_string(_suite)        && result;
    result = d_tests_options_set_ptr(_suite)           && result;
    result = d_tests_options_set_null_config(_suite)   && result;
    result = d_tests_options_set_type_mismatch(_suite) && result;

    return result;
}
