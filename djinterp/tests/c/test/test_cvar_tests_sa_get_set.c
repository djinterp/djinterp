#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * VII. VALUE GET/SET TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_get_default_value
  Tests that registry values have expected defaults after reset_all.
  NOTE: The flag-based API (d_test_registry_get) is ambiguous for flags that
  collide between DTestConfigKey and DTestMetadataFlag (both enums start at 0).
  Use key-based macros for unambiguous access; flag-based access is tested only
  for flags whose numeric values are unique in the row array.
  Tests the following:
  - config-enabled default is true (bool)
  - skip default is false (bool)
  - timeout default is D_TEST_DEFAULT_TIMEOUT (size_t, via key)
*/
bool
d_tests_sa_cvar_get_default_value
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: config-enabled default is true (key-based)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "get_default_enabled_true",
        "'config-enabled' default should be true",
        _counter) && result;

    // test 2: skip default is false (key-based)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("skip") == false,
        "get_default_skip_false",
        "'skip' default should be false",
        _counter) && result;

    // test 3: timeout default is D_TEST_DEFAULT_TIMEOUT (key-based)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
        "get_default_timeout",
        "'timeout' default should be D_TEST_DEFAULT_TIMEOUT",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_set_then_get
  Tests that d_test_registry_set changes the value and get reads it back.
  Tests the following:
  - Set a size_t value and get it back
  - Set a uint32_t value and get it back
*/
bool
d_tests_sa_cvar_set_then_get
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;
    bool               set_ok;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: set max-failures to 42 and read back
    val.z  = 42;
    set_ok = d_test_registry_set(D_TEST_CONFIG_MAX_FAILURES, val);

    result = d_assert_standalone(
        set_ok == true,
        "set_max_failures_ok",
        "set(D_TEST_CONFIG_MAX_FAILURES, 42) should return true",
        _counter) && result;

    val = d_test_registry_get(D_TEST_CONFIG_MAX_FAILURES);

    result = d_assert_standalone(
        val.z == 42,
        "get_max_failures_42",
        "get(D_TEST_CONFIG_MAX_FAILURES) should return 42 after set",
        _counter) && result;

    // test 2: set message-flags and read back
    val.u32 = 0xABCD1234;
    set_ok  = d_test_registry_set(D_TEST_CONFIG_MESSAGE_FLAGS, val);

    result = d_assert_standalone(
        set_ok == true,
        "set_message_flags_ok",
        "set(D_TEST_CONFIG_MESSAGE_FLAGS) should return true",
        _counter) && result;

    val = d_test_registry_get(D_TEST_CONFIG_MESSAGE_FLAGS);

    result = d_assert_standalone(
        val.u32 == 0xABCD1234,
        "get_message_flags_readback",
        "get(D_TEST_CONFIG_MESSAGE_FLAGS) should return 0xABCD1234",
        _counter) && result;

    // cleanup
    d_test_registry_reset_all();

    return result;
}


/*
d_tests_sa_cvar_set_invalid_flag
  Tests that d_test_registry_set with an invalid flag returns false.
  Tests the following:
  - Invalid flag (9999) returns false
  - Invalid flag (0xFFFFFFFF) returns false
*/
bool
d_tests_sa_cvar_set_invalid_flag
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;
    bool               set_ok;

    result = true;

    d_test_registry_init();

    // test 1: invalid flag 9999
    val.u32 = 1;
    set_ok  = d_test_registry_set(9999, val);

    result = d_assert_standalone(
        set_ok == false,
        "set_invalid_flag_9999",
        "set(9999, ...) should return false",
        _counter) && result;

    // test 2: invalid flag 0xFFFFFFFF
    set_ok = d_test_registry_set(0xFFFFFFFF, val);

    result = d_assert_standalone(
        set_ok == false,
        "set_invalid_flag_max",
        "set(0xFFFFFFFF, ...) should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_get_invalid_flag
  Tests that d_test_registry_get with an invalid flag returns a zeroed value.
  Tests the following:
  - Invalid flag returns value with ptr == NULL
*/
bool
d_tests_sa_cvar_get_invalid_flag
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    d_test_registry_init();

    // test 1: invalid flag returns zeroed union (ptr == NULL)
    val = d_test_registry_get(9999);

    result = d_assert_standalone(
        val.ptr == NULL,
        "get_invalid_flag_null",
        "get(9999) should return value with ptr == NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_set_get_bool
  Tests roundtrip set/get of a boolean value.
  Tests the following:
  - Set config-enabled to false, get returns false
  - Set config-enabled back to true, get returns true
*/
bool
d_tests_sa_cvar_set_get_bool
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: set enabled to false
    val.b = false;
    d_test_registry_set(D_TEST_CONFIG_ENABLED, val);

    val = d_test_registry_get(D_TEST_CONFIG_ENABLED);

    result = d_assert_standalone(
        val.b == false,
        "set_get_bool_false",
        "After set(ENABLED, false), get should return false",
        _counter) && result;

    // test 2: set enabled back to true
    val.b = true;
    d_test_registry_set(D_TEST_CONFIG_ENABLED, val);

    val = d_test_registry_get(D_TEST_CONFIG_ENABLED);

    result = d_assert_standalone(
        val.b == true,
        "set_get_bool_true",
        "After set(ENABLED, true), get should return true",
        _counter) && result;

    // cleanup
    d_test_registry_reset_all();

    return result;
}


/*
d_tests_sa_cvar_get_set_all
  Aggregation function that runs all value get/set tests.
*/
bool
d_tests_sa_cvar_get_set_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Value Get/Set\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_get_default_value(_counter)  && result;
    result = d_tests_sa_cvar_set_then_get(_counter)       && result;
    result = d_tests_sa_cvar_set_invalid_flag(_counter)   && result;
    result = d_tests_sa_cvar_get_invalid_flag(_counter)   && result;
    result = d_tests_sa_cvar_set_get_bool(_counter)       && result;

    return result;
}
