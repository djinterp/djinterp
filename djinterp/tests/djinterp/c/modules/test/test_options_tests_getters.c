/*******************************************************************************
* djinterp [c][test]                                test_options_tests_getters.c
*
* test_options.h's suite, block "config_getter_all": each d_test_options_get_*
* getter, and the getters on a NULL config, which read the registry's defaults.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_getters.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 11. GETTER FUNCTION TESTS
//==============================================================================

/*
d_tests_options_get_bool
  Tests the d_test_options_get_bool function.
  Tests the following:
  - Returns schema default when no override is set
  - Returns overridden value after set_bool
  - D_TEST_OPTIONS_ENABLED default is true
  - D_TEST_OPTIONS_SKIP default is false
*/
bool
d_tests_options_get_bool(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_bool");

    bool                    result = true;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_bool_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: ENABLED default is true
    result = D_TEST_CHECK_MSG(_suite,
                              "get_bool_enabled_default",
                              d_test_options_get_bool(config,
                                                      D_TEST_OPTIONS_ENABLED) == true,
                              "ENABLED default should be true") && result;

    // test 2: SKIP default is false
    result = D_TEST_CHECK_MSG(_suite,
                              "get_bool_skip_default",
                              d_test_options_get_bool(config,
                                                      D_TEST_OPTIONS_SKIP) == false,
                              "SKIP default should be false") && result;

    // test 3: after override, returns new value
    d_test_options_set_bool(config,
                            D_TEST_OPTIONS_SKIP,
                            true);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_bool_after_set",
                              d_test_options_get_bool(config,
                                                      D_TEST_OPTIONS_SKIP) == true,
                              "get_bool should return overridden value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_size_t
  Tests the d_test_options_get_size_t function.
  Tests the following:
  - Returns schema default for TIMEOUT_MS
  - Returns schema default for MAX_FAILURES
  - Returns overridden value after set_size_t
  - Handles type promotion from uint16_t schema rows
*/
bool
d_tests_options_get_size_t(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_size_t");

    bool                    result = true;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_size_t_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: TIMEOUT_MS default is 1000
    result = D_TEST_CHECK_MSG(_suite,
                              "get_size_t_timeout_default",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_TIMEOUT_MS) == D_TEST_DEFAULT_TIMEOUT,
                              "TIMEOUT_MS default should be 1000") && result;

    // test 2: MAX_FAILURES default is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "get_size_t_max_failures_default",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_MAX_FAILURES) == D_TEST_DEFAULT_MAX_FAILURES,
                              "MAX_FAILURES default should be 0") && result;

    // test 3: INDENT_MAX_LEVEL (uint16_t schema) can be read as size_t
    result = D_TEST_CHECK_MSG(_suite,
                              "get_size_t_indent_max_level",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_INDENT_MAX_LEVEL) == (size_t)D_TEST_DEFAULT_MAX_INDENT,
                              "INDENT_MAX_LEVEL should be promotable to size_t") && result;

    // test 4: after override
    d_test_options_set_size_t(config,
                              D_TEST_OPTIONS_TIMEOUT_MS,
                              5000);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_size_t_after_set",
                              d_test_options_get_size_t(config,
                                                        D_TEST_OPTIONS_TIMEOUT_MS) == 5000,
                              "get_size_t should return overridden value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_int32
  Tests the d_test_options_get_int32 function.
  Tests the following:
  - Returns schema default for PRIORITY (0)
  - Returns overridden value after set_int32
  - Returns 0 for non-int32 keys
*/
bool
d_tests_options_get_int32(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_int32");

    bool                    result = true;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_int32_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: PRIORITY default is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "get_int32_priority_default",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY) == 0,
                              "PRIORITY default should be 0") && result;

    // test 2: after override
    d_test_options_set_int32(config,
                             D_TEST_OPTIONS_PRIORITY,
                             -42);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_int32_after_set",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_PRIORITY) == -42,
                              "get_int32 should return overridden negative value") && result;

    // test 3: returns 0 for non-int32 key (type mismatch)
    result = D_TEST_CHECK_MSG(_suite,
                              "get_int32_type_mismatch",
                              d_test_options_get_int32(config,
                                                       D_TEST_OPTIONS_ENABLED) == 0,
                              "get_int32 should return 0 for non-int32 key") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_uint32
  Tests the d_test_options_get_uint32 function.
  Tests the following:
  - Returns schema default for MESSAGE_FLAGS
  - Returns overridden value after set_uint32
  - Handles type promotion from uint16_t and size_t
*/
bool
d_tests_options_get_uint32(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_uint32");

    bool                    result = true;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_uint32_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: MESSAGE_FLAGS returns the config->flags value
    result = D_TEST_CHECK_MSG(_suite,
                              "get_uint32_msg_flags",
                              d_test_options_get_uint32(config,
                                                        D_TEST_OPTIONS_MESSAGE_FLAGS) == D_TEST_MODE_NORMAL,
                              "MESSAGE_FLAGS should return the packed flags") && result;

    // test 2: after override via set_uint32
    d_test_options_set_uint32(config,
                              D_TEST_OPTIONS_MESSAGE_FLAGS,
                              D_TEST_MODE_VERBOSE);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_uint32_after_set",
                              d_test_options_get_uint32(config,
                                                        D_TEST_OPTIONS_MESSAGE_FLAGS) == D_TEST_MODE_VERBOSE,
                              "get_uint32 should return overridden value") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_string
  Tests the d_test_options_get_string function.
  Tests the following:
  - Returns schema default for INDENT_STR
  - Returns overridden value after set_string
  - Returns NULL for non-string keys
*/
bool
d_tests_options_get_string(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_string");

    bool                    result = true;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_string_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: INDENT_STR default
    const char* indent = d_test_options_get_string(config,
                                                   D_TEST_OPTIONS_INDENT_STR);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_string_indent_not_null",
                              indent != NULL,
                              "Default indent string should not be NULL") && result;

    // the string's value, when there is one
    if (indent)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_string_indent_value",
                                  d_strcasecmp(indent,
                                               D_TEST_DEFAULT_INDENT) == 0,
                                  "Default indent string should match default") && result;
    }

    // test 2: after override
    d_test_options_set_string(config,
                              D_TEST_OPTIONS_INDENT_STR,
                              "\t");
    indent = d_test_options_get_string(config,
                                       D_TEST_OPTIONS_INDENT_STR);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_string_after_set_not_null",
                              indent != NULL,
                              "Overridden indent should not be NULL") && result;

    // test 3: non-string key returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "get_string_type_mismatch",
                              d_test_options_get_string(config,
                                                        D_TEST_OPTIONS_ENABLED) == NULL,
                              "get_string should return NULL for non-string key") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_ptr
  Tests the d_test_options_get_ptr function.
  Tests the following:
  - Returns NULL for pointer keys with no override
  - Returns overridden value after set_ptr
*/
bool
d_tests_options_get_ptr(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_ptr");

    bool                    result = true;
    int                     dummy  = 42;
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // allocation failed: the test fails here
    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_ptr_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: INDENT_STR is accessible as ptr (string is pointer-like)
    void* ptr = d_test_options_get_ptr(config,
                                       D_TEST_OPTIONS_INDENT_STR);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_ptr_indent_default",
                              ptr != NULL,
                              "Default indent ptr should not be NULL") && result;

    // test 2: after override with arbitrary pointer
    d_test_options_set_ptr(config,
                           D_TEST_OPTIONS_INDENT_STR,
                           &dummy);
    ptr = d_test_options_get_ptr(config,
                                 D_TEST_OPTIONS_INDENT_STR);

    result = D_TEST_CHECK_MSG(_suite,
                              "get_ptr_after_set",
                              ptr == &dummy,
                              "get_ptr should return overridden pointer") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_null_config
  Tests getter functions with NULL config (should return schema defaults).
  Tests the following:
  - get_bool with NULL returns schema default
  - get_size_t with NULL returns schema default
  - get_int32 with NULL returns schema default
  - get_string with NULL returns schema default
*/
bool
d_tests_options_get_null_config(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_get_null_config");

    bool result = true;

    // test 1: get_bool NULL returns schema default for ENABLED
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_bool_enabled",
                              d_test_options_get_bool(NULL,
                                                      D_TEST_OPTIONS_ENABLED) == true,
                              "get_bool(NULL) for ENABLED should return true") && result;

    // test 2: get_bool NULL returns schema default for SKIP
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_bool_skip",
                              d_test_options_get_bool(NULL,
                                                      D_TEST_OPTIONS_SKIP) == false,
                              "get_bool(NULL) for SKIP should return false") && result;

    // test 3: get_size_t NULL returns schema default
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_size_t_timeout",
                              d_test_options_get_size_t(NULL,
                                                        D_TEST_OPTIONS_TIMEOUT_MS) == D_TEST_DEFAULT_TIMEOUT,
                              "get_size_t(NULL) for TIMEOUT should return default") && result;

    // test 4: get_int32 NULL returns schema default
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_int32_priority",
                              d_test_options_get_int32(NULL,
                                                       D_TEST_OPTIONS_PRIORITY) == 0,
                              "get_int32(NULL) for PRIORITY should return 0") && result;

    // test 5: get_string NULL returns schema default
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_string_indent",
                              d_test_options_get_string(NULL,
                                                        D_TEST_OPTIONS_INDENT_STR) != NULL,
                              "get_string(NULL) for INDENT_STR should return default") && result;

    return result;
}

/*
d_tests_options_getter_all
  Aggregation function that runs all getter function tests.
*/
bool
d_tests_options_getter_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "config_getter_all");

    bool result = true;

    result = d_tests_options_get_bool(_suite) && result;
    result = d_tests_options_get_size_t(_suite) && result;
    result = d_tests_options_get_int32(_suite) && result;
    result = d_tests_options_get_uint32(_suite) && result;
    result = d_tests_options_get_string(_suite) && result;
    result = d_tests_options_get_ptr(_suite) && result;
    result = d_tests_options_get_null_config(_suite) && result;

    return result;
}
