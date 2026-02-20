#include ".\test_config_tests_sa.h"


/******************************************************************************
 * XI. GETTER FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_config_get_bool
  Tests the d_test_config_get_bool function.
  Tests the following:
  - Returns schema default when no override is set
  - Returns overridden value after set_bool
  - D_TEST_CONFIG_ENABLED default is true
  - D_TEST_CONFIG_SKIP default is false
*/
bool
d_tests_sa_config_get_bool
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_bool_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: ENABLED default is true
    result = d_assert_standalone(
        d_test_config_get_bool(config, D_TEST_CONFIG_ENABLED) == true,
        "get_bool_enabled_default",
        "ENABLED default should be true",
        _counter) && result;

    // test 2: SKIP default is false
    result = d_assert_standalone(
        d_test_config_get_bool(config, D_TEST_CONFIG_SKIP) == false,
        "get_bool_skip_default",
        "SKIP default should be false",
        _counter) && result;

    // test 3: after override, returns new value
    d_test_config_set_bool(config, D_TEST_CONFIG_SKIP, true);

    result = d_assert_standalone(
        d_test_config_get_bool(config, D_TEST_CONFIG_SKIP) == true,
        "get_bool_after_set",
        "get_bool should return overridden value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_size_t
  Tests the d_test_config_get_size_t function.
  Tests the following:
  - Returns schema default for TIMEOUT_MS
  - Returns schema default for MAX_FAILURES
  - Returns overridden value after set_size_t
  - Handles type promotion from uint16_t schema rows
*/
bool
d_tests_sa_config_get_size_t
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_size_t_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: TIMEOUT_MS default is 1000
    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_TIMEOUT_MS) ==
            D_TEST_DEFAULT_TIMEOUT,
        "get_size_t_timeout_default",
        "TIMEOUT_MS default should be 1000",
        _counter) && result;

    // test 2: MAX_FAILURES default is 0
    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_MAX_FAILURES) ==
            D_TEST_DEFAULT_MAX_FAILURES,
        "get_size_t_max_failures_default",
        "MAX_FAILURES default should be 0",
        _counter) && result;

    // test 3: INDENT_MAX_LEVEL (uint16_t schema) can be read as size_t
    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_INDENT_MAX_LEVEL) ==
            (size_t)D_TEST_DEFAULT_MAX_INDENT,
        "get_size_t_indent_max_level",
        "INDENT_MAX_LEVEL should be promotable to size_t",
        _counter) && result;

    // test 4: after override
    d_test_config_set_size_t(config, D_TEST_CONFIG_TIMEOUT_MS, 5000);

    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_TIMEOUT_MS) == 5000,
        "get_size_t_after_set",
        "get_size_t should return overridden value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_int32
  Tests the d_test_config_get_int32 function.
  Tests the following:
  - Returns schema default for PRIORITY (0)
  - Returns overridden value after set_int32
  - Returns 0 for non-int32 keys
*/
bool
d_tests_sa_config_get_int32
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_int32_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: PRIORITY default is 0
    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_PRIORITY) == 0,
        "get_int32_priority_default",
        "PRIORITY default should be 0",
        _counter) && result;

    // test 2: after override
    d_test_config_set_int32(config, D_TEST_CONFIG_PRIORITY, -42);

    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_PRIORITY) == -42,
        "get_int32_after_set",
        "get_int32 should return overridden negative value",
        _counter) && result;

    // test 3: returns 0 for non-int32 key (type mismatch)
    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_ENABLED) == 0,
        "get_int32_type_mismatch",
        "get_int32 should return 0 for non-int32 key",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_uint32
  Tests the d_test_config_get_uint32 function.
  Tests the following:
  - Returns schema default for MESSAGE_FLAGS
  - Returns overridden value after set_uint32
  - Handles type promotion from uint16_t and size_t
*/
bool
d_tests_sa_config_get_uint32
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_uint32_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: MESSAGE_FLAGS returns the config->flags value
    result = d_assert_standalone(
        d_test_config_get_uint32(config, D_TEST_CONFIG_MESSAGE_FLAGS) ==
            D_TEST_MODE_NORMAL,
        "get_uint32_msg_flags",
        "MESSAGE_FLAGS should return the packed flags",
        _counter) && result;

    // test 2: after override via set_uint32
    d_test_config_set_uint32(config,
                             D_TEST_CONFIG_MESSAGE_FLAGS,
                             D_TEST_MODE_VERBOSE);

    result = d_assert_standalone(
        d_test_config_get_uint32(config, D_TEST_CONFIG_MESSAGE_FLAGS) ==
            D_TEST_MODE_VERBOSE,
        "get_uint32_after_set",
        "get_uint32 should return overridden value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_string
  Tests the d_test_config_get_string function.
  Tests the following:
  - Returns schema default for INDENT_STR
  - Returns overridden value after set_string
  - Returns NULL for non-string keys
*/
bool
d_tests_sa_config_get_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    const char*           indent;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_string_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: INDENT_STR default
    indent = d_test_config_get_string(config, D_TEST_CONFIG_INDENT_STR);

    result = d_assert_standalone(
        indent != NULL,
        "get_string_indent_not_null",
        "Default indent string should not be NULL",
        _counter) && result;

    if (indent)
    {
        result = d_assert_standalone(
            d_strcasecmp(indent, D_TEST_DEFAULT_INDENT) == 0,
            "get_string_indent_value",
            "Default indent string should match default",
            _counter) && result;
    }

    // test 2: after override
    d_test_config_set_string(config, D_TEST_CONFIG_INDENT_STR, "\t");
    indent = d_test_config_get_string(config, D_TEST_CONFIG_INDENT_STR);

    result = d_assert_standalone(
        indent != NULL,
        "get_string_after_set_not_null",
        "Overridden indent should not be NULL",
        _counter) && result;

    // test 3: non-string key returns NULL
    result = d_assert_standalone(
        d_test_config_get_string(config, D_TEST_CONFIG_ENABLED) == NULL,
        "get_string_type_mismatch",
        "get_string should return NULL for non-string key",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_ptr
  Tests the d_test_config_get_ptr function.
  Tests the following:
  - Returns NULL for pointer keys with no override
  - Returns overridden value after set_ptr
*/
bool
d_tests_sa_config_get_ptr
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    int                   dummy;
    void*                 ptr;

    result = true;
    dummy  = 42;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_ptr_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: INDENT_STR is accessible as ptr (string is pointer-like)
    ptr = d_test_config_get_ptr(config, D_TEST_CONFIG_INDENT_STR);

    result = d_assert_standalone(
        ptr != NULL,
        "get_ptr_indent_default",
        "Default indent ptr should not be NULL",
        _counter) && result;

    // test 2: after override with arbitrary pointer
    d_test_config_set_ptr(config, D_TEST_CONFIG_INDENT_STR, &dummy);
    ptr = d_test_config_get_ptr(config, D_TEST_CONFIG_INDENT_STR);

    result = d_assert_standalone(
        ptr == &dummy,
        "get_ptr_after_set",
        "get_ptr should return overridden pointer",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_null_config
  Tests getter functions with NULL config (should return schema defaults).
  Tests the following:
  - get_bool with NULL returns schema default
  - get_size_t with NULL returns schema default
  - get_int32 with NULL returns schema default
  - get_string with NULL returns schema default
*/
bool
d_tests_sa_config_get_null_config
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: get_bool NULL returns schema default for ENABLED
    result = d_assert_standalone(
        d_test_config_get_bool(NULL, D_TEST_CONFIG_ENABLED) == true,
        "get_null_bool_enabled",
        "get_bool(NULL) for ENABLED should return true",
        _counter) && result;

    // test 2: get_bool NULL returns schema default for SKIP
    result = d_assert_standalone(
        d_test_config_get_bool(NULL, D_TEST_CONFIG_SKIP) == false,
        "get_null_bool_skip",
        "get_bool(NULL) for SKIP should return false",
        _counter) && result;

    // test 3: get_size_t NULL returns schema default
    result = d_assert_standalone(
        d_test_config_get_size_t(NULL, D_TEST_CONFIG_TIMEOUT_MS) ==
            D_TEST_DEFAULT_TIMEOUT,
        "get_null_size_t_timeout",
        "get_size_t(NULL) for TIMEOUT should return default",
        _counter) && result;

    // test 4: get_int32 NULL returns schema default
    result = d_assert_standalone(
        d_test_config_get_int32(NULL, D_TEST_CONFIG_PRIORITY) == 0,
        "get_null_int32_priority",
        "get_int32(NULL) for PRIORITY should return 0",
        _counter) && result;

    // test 5: get_string NULL returns schema default
    result = d_assert_standalone(
        d_test_config_get_string(NULL, D_TEST_CONFIG_INDENT_STR) != NULL,
        "get_null_string_indent",
        "get_string(NULL) for INDENT_STR should return default",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_getter_all
  Aggregation function that runs all getter function tests.
*/
bool
d_tests_sa_config_getter_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Getter Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_config_get_bool(_counter) && result;
    result = d_tests_sa_config_get_size_t(_counter) && result;
    result = d_tests_sa_config_get_int32(_counter) && result;
    result = d_tests_sa_config_get_uint32(_counter) && result;
    result = d_tests_sa_config_get_string(_counter) && result;
    result = d_tests_sa_config_get_ptr(_counter) && result;
    result = d_tests_sa_config_get_null_config(_counter) && result;

    return result;
}
