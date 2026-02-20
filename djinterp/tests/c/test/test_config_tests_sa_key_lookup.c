#include ".\test_config_tests_sa.h"


/******************************************************************************
 * XIII. KEY LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_sa_config_key_from_string_valid
  Tests d_test_config_key_from_string with valid config keys.
  Tests the following:
  - "config-enabled" resolves to D_TEST_CONFIG_ENABLED
  - "skip" resolves to D_TEST_CONFIG_SKIP
  - "timeout" resolves to D_TEST_CONFIG_TIMEOUT_MS
  - "indent-string" resolves to D_TEST_CONFIG_INDENT_STR
  - "max-failures" resolves to D_TEST_CONFIG_MAX_FAILURES
  - "priority" resolves to D_TEST_CONFIG_PRIORITY
  - "message-flags" resolves to D_TEST_CONFIG_MESSAGE_FLAGS
  - "max-indent" resolves to D_TEST_CONFIG_INDENT_MAX_LEVEL
*/
bool
d_tests_sa_config_key_from_string_valid
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: "config-enabled"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("config-enabled") ==
            (uint32_t)D_TEST_CONFIG_ENABLED,
        "key_from_string_enabled",
        "\"config-enabled\" should resolve to D_TEST_CONFIG_ENABLED",
        _counter) && result;

    // test 2: "skip"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("skip") ==
            (uint32_t)D_TEST_CONFIG_SKIP,
        "key_from_string_skip",
        "\"skip\" should resolve to D_TEST_CONFIG_SKIP",
        _counter) && result;

    // test 3: "timeout"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("timeout") ==
            (uint32_t)D_TEST_CONFIG_TIMEOUT_MS,
        "key_from_string_timeout",
        "\"timeout\" should resolve to D_TEST_CONFIG_TIMEOUT_MS",
        _counter) && result;

    // test 4: "indent-string"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("indent-string") ==
            (uint32_t)D_TEST_CONFIG_INDENT_STR,
        "key_from_string_indent",
        "\"indent-string\" should resolve to D_TEST_CONFIG_INDENT_STR",
        _counter) && result;

    // test 5: "max-failures"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("max-failures") ==
            (uint32_t)D_TEST_CONFIG_MAX_FAILURES,
        "key_from_string_max_failures",
        "\"max-failures\" should resolve to D_TEST_CONFIG_MAX_FAILURES",
        _counter) && result;

    // test 6: "priority"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("priority") ==
            (uint32_t)D_TEST_CONFIG_PRIORITY,
        "key_from_string_priority",
        "\"priority\" should resolve to D_TEST_CONFIG_PRIORITY",
        _counter) && result;

    // test 7: "message-flags"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("message-flags") ==
            (uint32_t)D_TEST_CONFIG_MESSAGE_FLAGS,
        "key_from_string_msg_flags",
        "\"message-flags\" should resolve to D_TEST_CONFIG_MESSAGE_FLAGS",
        _counter) && result;

    // test 8: "max-indent"
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("max-indent") ==
            (uint32_t)D_TEST_CONFIG_INDENT_MAX_LEVEL,
        "key_from_string_max_indent",
        "\"max-indent\" should resolve to D_TEST_CONFIG_INDENT_MAX_LEVEL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_key_from_string_null
  Tests d_test_config_key_from_string with NULL input.
  Tests the following:
  - Returns D_TEST_CONFIG_KEY_INVALID (UINT32_MAX) for NULL
*/
bool
d_tests_sa_config_key_from_string_null
(
    struct d_test_counter* _counter
)
{
    bool                result;
    enum DTestConfigKey key;

    result = true;
    key    = d_test_config_key_from_string(NULL);

    result = d_assert_standalone(
        (uint32_t)key == UINT32_MAX,
        "key_from_string_null",
        "NULL key should return INVALID (UINT32_MAX)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_key_from_string_invalid
  Tests d_test_config_key_from_string with invalid or metadata-only keys.
  Tests the following:
  - Non-existent key returns INVALID
  - Metadata-only keys (not config) return INVALID
  - Empty string returns INVALID
*/
bool
d_tests_sa_config_key_from_string_invalid
(
    struct d_test_counter* _counter
)
{
    bool                result;
    enum DTestConfigKey key;

    result = true;

    // test 1: non-existent key
    key = d_test_config_key_from_string("this-key-does-not-exist");

    result = d_assert_standalone(
        (uint32_t)key == UINT32_MAX,
        "key_from_string_nonexistent",
        "Non-existent key should return INVALID",
        _counter) && result;

    // test 2: metadata-only key should return INVALID
    key = d_test_config_key_from_string("description");

    result = d_assert_standalone(
        (uint32_t)key == UINT32_MAX,
        "key_from_string_metadata_only",
        "Metadata-only key should return INVALID",
        _counter) && result;

    // test 3: another metadata key
    key = d_test_config_key_from_string("authors");

    result = d_assert_standalone(
        (uint32_t)key == UINT32_MAX,
        "key_from_string_authors",
        "\"authors\" (metadata) should return INVALID",
        _counter) && result;

    // test 4: "name" is metadata, not config
    key = d_test_config_key_from_string("name");

    result = d_assert_standalone(
        (uint32_t)key == UINT32_MAX,
        "key_from_string_name_metadata",
        "\"name\" (metadata) should return INVALID",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_key_from_string_alias
  Tests d_test_config_key_from_string with alias keys.
  Tests the following:
  - "enabled" alias resolves to D_TEST_CONFIG_ENABLED
  - "indent" alias resolves to D_TEST_CONFIG_INDENT_STR
  - "indent-max" alias resolves to D_TEST_CONFIG_INDENT_MAX_LEVEL
  - "indent-level" alias resolves to D_TEST_CONFIG_INDENT_MAX_LEVEL
  - "timeout-ms" alias resolves to D_TEST_CONFIG_TIMEOUT_MS
*/
bool
d_tests_sa_config_key_from_string_alias
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: "enabled" alias
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("enabled") ==
            (uint32_t)D_TEST_CONFIG_ENABLED,
        "key_from_alias_enabled",
        "\"enabled\" alias should resolve to CONFIG_ENABLED",
        _counter) && result;

    // test 2: "indent" alias
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("indent") ==
            (uint32_t)D_TEST_CONFIG_INDENT_STR,
        "key_from_alias_indent",
        "\"indent\" alias should resolve to CONFIG_INDENT_STR",
        _counter) && result;

    // test 3: "indent-max" alias
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("indent-max") ==
            (uint32_t)D_TEST_CONFIG_INDENT_MAX_LEVEL,
        "key_from_alias_indent_max",
        "\"indent-max\" alias should resolve to INDENT_MAX_LEVEL",
        _counter) && result;

    // test 4: "indent-level" alias
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("indent-level") ==
            (uint32_t)D_TEST_CONFIG_INDENT_MAX_LEVEL,
        "key_from_alias_indent_level",
        "\"indent-level\" alias should resolve to INDENT_MAX_LEVEL",
        _counter) && result;

    // test 5: "timeout-ms" alias
    result = d_assert_standalone(
        (uint32_t)d_test_config_key_from_string("timeout-ms") ==
            (uint32_t)D_TEST_CONFIG_TIMEOUT_MS,
        "key_from_alias_timeout_ms",
        "\"timeout-ms\" alias should resolve to TIMEOUT_MS",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_key_lookup_all
  Aggregation function that runs all key lookup tests.
*/
bool
d_tests_sa_config_key_lookup_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Key Lookup\n");
    printf("  ----------------------\n");

    result = d_tests_sa_config_key_from_string_valid(_counter) && result;
    result = d_tests_sa_config_key_from_string_null(_counter) && result;
    result = d_tests_sa_config_key_from_string_invalid(_counter) && result;
    result = d_tests_sa_config_key_from_string_alias(_counter) && result;

    return result;
}
