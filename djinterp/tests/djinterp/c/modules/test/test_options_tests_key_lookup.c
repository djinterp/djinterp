/*******************************************************************************
* djinterp [test]                                test_options_tests_key_lookup.c
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_key_lookup.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_options_tests.h"


/******************************************************************************
 * XIII. KEY LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_options_key_from_string_valid
  Tests d_test_options_key_from_string with valid config keys.
  Tests the following:
  - "config-enabled" resolves to D_TEST_OPTIONS_ENABLED
  - "skip" resolves to D_TEST_OPTIONS_SKIP
  - "timeout" resolves to D_TEST_OPTIONS_TIMEOUT_MS
  - "indent-string" resolves to D_TEST_OPTIONS_INDENT_STR
  - "max-failures" resolves to D_TEST_OPTIONS_MAX_FAILURES
  - "priority" resolves to D_TEST_OPTIONS_PRIORITY
  - "message-flags" resolves to D_TEST_OPTIONS_MESSAGE_FLAGS
  - "max-indent" resolves to D_TEST_OPTIONS_INDENT_MAX_LEVEL
*/
bool d_tests_options_key_from_string_valid(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_key_from_string_valid");

    result = true;

    // test 1: "config-enabled"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_enabled",
                              (uint32_t)d_test_options_key_from_string("config-enabled") == (uint32_t)D_TEST_OPTIONS_ENABLED,
                              "\"config-enabled\" should resolve to D_TEST_OPTIONS_ENABLED") && result;

    // test 2: "skip"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_skip",
                              (uint32_t)d_test_options_key_from_string("skip") == (uint32_t)D_TEST_OPTIONS_SKIP,
                              "\"skip\" should resolve to D_TEST_OPTIONS_SKIP") && result;

    // test 3: "timeout"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_timeout",
                              (uint32_t)d_test_options_key_from_string("timeout") == (uint32_t)D_TEST_OPTIONS_TIMEOUT_MS,
                              "\"timeout\" should resolve to D_TEST_OPTIONS_TIMEOUT_MS") && result;

    // test 4: "indent-string"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_indent",
                              (uint32_t)d_test_options_key_from_string("indent-string") == (uint32_t)D_TEST_OPTIONS_INDENT_STR,
                              "\"indent-string\" should resolve to D_TEST_OPTIONS_INDENT_STR") && result;

    // test 5: "max-failures"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_max_failures",
                              (uint32_t)d_test_options_key_from_string("max-failures") == (uint32_t)D_TEST_OPTIONS_MAX_FAILURES,
                              "\"max-failures\" should resolve to D_TEST_OPTIONS_MAX_FAILURES") && result;

    // test 6: "priority"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_priority",
                              (uint32_t)d_test_options_key_from_string("priority") == (uint32_t)D_TEST_OPTIONS_PRIORITY,
                              "\"priority\" should resolve to D_TEST_OPTIONS_PRIORITY") && result;

    // test 7: "message-flags"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_msg_flags",
                              (uint32_t)d_test_options_key_from_string("message-flags") == (uint32_t)D_TEST_OPTIONS_MESSAGE_FLAGS,
                              "\"message-flags\" should resolve to D_TEST_OPTIONS_MESSAGE_FLAGS") && result;

    // test 8: "max-indent"
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_max_indent",
                              (uint32_t)d_test_options_key_from_string("max-indent") == (uint32_t)D_TEST_OPTIONS_INDENT_MAX_LEVEL,
                              "\"max-indent\" should resolve to D_TEST_OPTIONS_INDENT_MAX_LEVEL") && result;

    return result;
}

/*
d_tests_options_key_from_string_null
  Tests d_test_options_key_from_string with NULL input.
  Tests the following:
  - Returns D_TEST_OPTIONS_KEY_INVALID (UINT32_MAX) for NULL
*/
bool d_tests_options_key_from_string_null(
    struct dj_test_suite* _suite
)
{
    bool                result;
    enum DTestConfigKey key;

    d_test_suite_test(_suite, "config_key_from_string_null");

    result = true;
    key    = d_test_options_key_from_string(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_null",
                              (uint32_t)key == UINT32_MAX,
                              "NULL key should return INVALID (UINT32_MAX)") && result;

    return result;
}

/*
d_tests_options_key_from_string_invalid
  Tests d_test_options_key_from_string with invalid or metadata-only keys.
  Tests the following:
  - Non-existent key returns INVALID
  - Metadata-only keys (not config) return INVALID
  - Empty string returns INVALID
*/
bool d_tests_options_key_from_string_invalid(
    struct dj_test_suite* _suite
)
{
    bool                result;
    enum DTestConfigKey key;

    d_test_suite_test(_suite, "config_key_from_string_invalid");

    result = true;

    // test 1: non-existent key
    key = d_test_options_key_from_string("this-key-does-not-exist");

    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_nonexistent",
                              (uint32_t)key == UINT32_MAX,
                              "Non-existent key should return INVALID") && result;

    // test 2: metadata-only key should return INVALID
    key = d_test_options_key_from_string("description");

    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_metadata_only",
                              (uint32_t)key == UINT32_MAX,
                              "Metadata-only key should return INVALID") && result;

    // test 3: another metadata key
    key = d_test_options_key_from_string("authors");

    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_authors",
                              (uint32_t)key == UINT32_MAX,
                              "\"authors\" (metadata) should return INVALID") && result;

    // test 4: "name" is metadata, not config
    key = d_test_options_key_from_string("name");

    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_string_name_metadata",
                              (uint32_t)key == UINT32_MAX,
                              "\"name\" (metadata) should return INVALID") && result;

    return result;
}

/*
d_tests_options_key_from_string_alias
  Tests d_test_options_key_from_string with alias keys.
  Tests the following:
  - "enabled" alias resolves to D_TEST_OPTIONS_ENABLED
  - "indent" alias resolves to D_TEST_OPTIONS_INDENT_STR
  - "indent-max" alias resolves to D_TEST_OPTIONS_INDENT_MAX_LEVEL
  - "indent-level" alias resolves to D_TEST_OPTIONS_INDENT_MAX_LEVEL
  - "timeout-ms" alias resolves to D_TEST_OPTIONS_TIMEOUT_MS
*/
bool d_tests_options_key_from_string_alias(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_key_from_string_alias");

    result = true;

    // test 1: "enabled" alias
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_alias_enabled",
                              (uint32_t)d_test_options_key_from_string("enabled") == (uint32_t)D_TEST_OPTIONS_ENABLED,
                              "\"enabled\" alias should resolve to CONFIG_ENABLED") && result;

    // test 2: "indent" alias
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_alias_indent",
                              (uint32_t)d_test_options_key_from_string("indent") == (uint32_t)D_TEST_OPTIONS_INDENT_STR,
                              "\"indent\" alias should resolve to CONFIG_INDENT_STR") && result;

    // test 3: "indent-max" alias
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_alias_indent_max",
                              (uint32_t)d_test_options_key_from_string("indent-max") == (uint32_t)D_TEST_OPTIONS_INDENT_MAX_LEVEL,
                              "\"indent-max\" alias should resolve to INDENT_MAX_LEVEL") && result;

    // test 4: "indent-level" alias
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_alias_indent_level",
                              (uint32_t)d_test_options_key_from_string("indent-level") == (uint32_t)D_TEST_OPTIONS_INDENT_MAX_LEVEL,
                              "\"indent-level\" alias should resolve to INDENT_MAX_LEVEL") && result;

    // test 5: "timeout-ms" alias
    result = D_TEST_CHECK_MSG(_suite,
                              "key_from_alias_timeout_ms",
                              (uint32_t)d_test_options_key_from_string("timeout-ms") == (uint32_t)D_TEST_OPTIONS_TIMEOUT_MS,
                              "\"timeout-ms\" alias should resolve to TIMEOUT_MS") && result;

    return result;
}

/*
d_tests_options_key_lookup_all
  Aggregation function that runs all key lookup tests.
*/
bool d_tests_options_key_lookup_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "config_key_lookup_all");

    result = true;

    result = d_tests_options_key_from_string_valid(_suite) && result;
    result = d_tests_options_key_from_string_null(_suite) && result;
    result = d_tests_options_key_from_string_invalid(_suite) && result;
    result = d_tests_options_key_from_string_alias(_suite) && result;

    return result;
}
