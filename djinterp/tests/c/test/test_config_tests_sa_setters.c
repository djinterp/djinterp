#include ".\test_config_tests_sa.h"


/******************************************************************************
 * XII. SETTER FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_config_set_bool
  Tests the d_test_config_set_bool function.
  Tests the following:
  - Returns true on success
  - Value is persisted and retrievable via get_bool
  - Can toggle value back and forth
*/
bool
d_tests_sa_config_set_bool
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
                                   "set_bool_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: set returns true on success
    result = d_assert_standalone(
        d_test_config_set_bool(config, D_TEST_CONFIG_SKIP, true) == true,
        "set_bool_returns_true",
        "set_bool should return true on success",
        _counter) && result;

    // test 2: value persisted
    result = d_assert_standalone(
        d_test_config_get_bool(config, D_TEST_CONFIG_SKIP) == true,
        "set_bool_persisted",
        "set_bool value should be retrievable via get_bool",
        _counter) && result;

    // test 3: toggle back
    d_test_config_set_bool(config, D_TEST_CONFIG_SKIP, false);

    result = d_assert_standalone(
        d_test_config_get_bool(config, D_TEST_CONFIG_SKIP) == false,
        "set_bool_toggle",
        "set_bool should allow toggling value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_size_t
  Tests the d_test_config_set_size_t function.
  Tests the following:
  - Returns true on success for size_t keys
  - Returns true for uint16_t keys (compatible type)
  - Value is persisted correctly
  - Large values are handled
*/
bool
d_tests_sa_config_set_size_t
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
                                   "set_size_t_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: set TIMEOUT_MS (size_t schema type)
    result = d_assert_standalone(
        d_test_config_set_size_t(config,
                                 D_TEST_CONFIG_TIMEOUT_MS,
                                 2500) == true,
        "set_size_t_timeout_success",
        "set_size_t should succeed for TIMEOUT_MS",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_TIMEOUT_MS) == 2500,
        "set_size_t_timeout_value",
        "TIMEOUT_MS should reflect set value",
        _counter) && result;

    // test 2: set INDENT_MAX_LEVEL (uint16_t schema type)
    result = d_assert_standalone(
        d_test_config_set_size_t(config,
                                 D_TEST_CONFIG_INDENT_MAX_LEVEL,
                                 20) == true,
        "set_size_t_indent_max_success",
        "set_size_t should succeed for uint16_t key",
        _counter) && result;

    // test 3: set MAX_FAILURES
    d_test_config_set_size_t(config, D_TEST_CONFIG_MAX_FAILURES, 100);

    result = d_assert_standalone(
        d_test_config_get_size_t(config, D_TEST_CONFIG_MAX_FAILURES) == 100,
        "set_size_t_max_failures",
        "MAX_FAILURES should reflect set value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_int32
  Tests the d_test_config_set_int32 function.
  Tests the following:
  - Returns true on success for PRIORITY
  - Negative values are handled correctly
  - Zero value works
*/
bool
d_tests_sa_config_set_int32
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
                                   "set_int32_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: positive value
    result = d_assert_standalone(
        d_test_config_set_int32(config, D_TEST_CONFIG_PRIORITY, 10) == true,
        "set_int32_positive",
        "set_int32 should succeed for positive value",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_PRIORITY) == 10,
        "set_int32_positive_value",
        "PRIORITY should reflect positive set value",
        _counter) && result;

    // test 2: negative value
    d_test_config_set_int32(config, D_TEST_CONFIG_PRIORITY, -99);

    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_PRIORITY) == -99,
        "set_int32_negative",
        "PRIORITY should reflect negative set value",
        _counter) && result;

    // test 3: zero value
    d_test_config_set_int32(config, D_TEST_CONFIG_PRIORITY, 0);

    result = d_assert_standalone(
        d_test_config_get_int32(config, D_TEST_CONFIG_PRIORITY) == 0,
        "set_int32_zero",
        "PRIORITY should reflect zero set value",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_uint32
  Tests the d_test_config_set_uint32 function.
  Tests the following:
  - MESSAGE_FLAGS modifies config->flags directly
  - Other uint32_t keys use the override map
*/
bool
d_tests_sa_config_set_uint32
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_SILENT);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "set_uint32_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: MESSAGE_FLAGS modifies flags directly
    result = d_assert_standalone(
        d_test_config_set_uint32(config,
                                 D_TEST_CONFIG_MESSAGE_FLAGS,
                                 D_TEST_MODE_VERBOSE) == true,
        "set_uint32_msg_flags_success",
        "set_uint32 for MESSAGE_FLAGS should succeed",
        _counter) && result;

    result = d_assert_standalone(
        config->flags == D_TEST_MODE_VERBOSE,
        "set_uint32_msg_flags_direct",
        "MESSAGE_FLAGS should modify config->flags directly",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_string
  Tests the d_test_config_set_string function.
  Tests the following:
  - Returns true on success for INDENT_STR
  - Value is persisted and retrievable
  - Rejects non-string keys
*/
bool
d_tests_sa_config_set_string
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
                                   "set_string_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: set INDENT_STR
    result = d_assert_standalone(
        d_test_config_set_string(config,
                                 D_TEST_CONFIG_INDENT_STR,
                                 "    ") == true,
        "set_string_indent_success",
        "set_string should succeed for INDENT_STR",
        _counter) && result;

    // test 2: value is retrievable
    indent = d_test_config_get_string(config, D_TEST_CONFIG_INDENT_STR);

    result = d_assert_standalone(
        indent != NULL,
        "set_string_indent_retrievable",
        "Overridden indent should be retrievable",
        _counter) && result;

    // test 3: rejects non-string key
    result = d_assert_standalone(
        d_test_config_set_string(config,
                                 D_TEST_CONFIG_ENABLED,
                                 "invalid") == false,
        "set_string_type_mismatch",
        "set_string should reject non-string key",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_ptr
  Tests the d_test_config_set_ptr function.
  Tests the following:
  - Returns true on success
  - Value is persisted and retrievable
*/
bool
d_tests_sa_config_set_ptr
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    int                   dummy;

    result = true;
    dummy  = 123;
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "set_ptr_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: set and retrieve pointer
    result = d_assert_standalone(
        d_test_config_set_ptr(config,
                              D_TEST_CONFIG_INDENT_STR,
                              &dummy) == true,
        "set_ptr_success",
        "set_ptr should succeed",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_get_ptr(config, D_TEST_CONFIG_INDENT_STR) == &dummy,
        "set_ptr_value",
        "get_ptr should return the set pointer",
        _counter) && result;

    // test 2: set NULL pointer
    d_test_config_set_ptr(config, D_TEST_CONFIG_INDENT_STR, NULL);

    result = d_assert_standalone(
        d_test_config_get_ptr(config, D_TEST_CONFIG_INDENT_STR) == NULL,
        "set_ptr_null",
        "get_ptr should return NULL after setting NULL",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_set_null_config
  Tests setter functions with NULL config.
  Tests the following:
  - All setters return false for NULL config
*/
bool
d_tests_sa_config_set_null_config
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_test_config_set_bool(NULL, D_TEST_CONFIG_SKIP, true) == false,
        "set_null_bool",
        "set_bool(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_set_size_t(NULL, D_TEST_CONFIG_TIMEOUT_MS, 1) == false,
        "set_null_size_t",
        "set_size_t(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_set_int32(NULL, D_TEST_CONFIG_PRIORITY, 1) == false,
        "set_null_int32",
        "set_int32(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_set_uint32(NULL, D_TEST_CONFIG_MESSAGE_FLAGS, 1) == false,
        "set_null_uint32",
        "set_uint32(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_set_string(NULL, D_TEST_CONFIG_INDENT_STR, "x") == false,
        "set_null_string",
        "set_string(NULL) should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_test_config_set_ptr(NULL, D_TEST_CONFIG_INDENT_STR, NULL) == false,
        "set_null_ptr",
        "set_ptr(NULL) should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_set_type_mismatch
  Tests setter functions with type-mismatched keys.
  Tests the following:
  - set_bool rejects non-bool keys
  - set_int32 rejects non-int32 keys
  - set_string rejects non-string keys
*/
bool
d_tests_sa_config_set_type_mismatch
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
                                   "set_mismatch_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: set_bool on a size_t key
    result = d_assert_standalone(
        d_test_config_set_bool(config,
                               D_TEST_CONFIG_TIMEOUT_MS,
                               true) == false,
        "set_mismatch_bool_on_size_t",
        "set_bool should reject size_t key",
        _counter) && result;

    // test 2: set_int32 on a bool key
    result = d_assert_standalone(
        d_test_config_set_int32(config,
                                D_TEST_CONFIG_ENABLED,
                                42) == false,
        "set_mismatch_int32_on_bool",
        "set_int32 should reject bool key",
        _counter) && result;

    // test 3: set_string on a bool key
    result = d_assert_standalone(
        d_test_config_set_string(config,
                                 D_TEST_CONFIG_SKIP,
                                 "hello") == false,
        "set_mismatch_string_on_bool",
        "set_string should reject bool key",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_setter_all
  Aggregation function that runs all setter function tests.
*/
bool
d_tests_sa_config_setter_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Setter Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_config_set_bool(_counter) && result;
    result = d_tests_sa_config_set_size_t(_counter) && result;
    result = d_tests_sa_config_set_int32(_counter) && result;
    result = d_tests_sa_config_set_uint32(_counter) && result;
    result = d_tests_sa_config_set_string(_counter) && result;
    result = d_tests_sa_config_set_ptr(_counter) && result;
    result = d_tests_sa_config_set_null_config(_counter) && result;
    result = d_tests_sa_config_set_type_mismatch(_counter) && result;

    return result;
}
