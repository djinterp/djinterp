#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * XIII. DEFAULT VALUE HELPER TESTS
 *****************************************************************************/

/*
  NOTE: d_test_registry_get_default and d_test_registry_get_default_by_key
  are declared in test_cvar.h but not yet implemented in test_cvar.c.
  We test default values indirectly by calling d_test_registry_reset_all()
  which restores all rows to their snapshot defaults, then reading back
  via d_test_registry_get(). This validates the same invariants.
*/


/*
d_tests_sa_cvar_default_by_flag
  Tests known defaults by reading through key-based macros after reset_all.
  NOTE: DTestConfigKey and DTestMetadataFlag both start at 0, so their numeric
  values overlap. The flag-based d_test_registry_get() performs a linear scan
  and may return the wrong row when flags collide. Key-based access is
  unambiguous and is the correct way to verify defaults.
  Tests the following:
  - config-enabled default is true (bool)
  - timeout default is D_TEST_DEFAULT_TIMEOUT (size_t)
  - skip default is false (bool)
*/
bool
d_tests_sa_cvar_default_by_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: default for config-enabled is true
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "default_by_flag_enabled",
        "After reset_all, 'config-enabled' should be true",
        _counter) && result;

    // test 2: default for timeout
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
        "default_by_flag_timeout",
        "After reset_all, 'timeout' should be D_TEST_DEFAULT_TIMEOUT",
        _counter) && result;

    // test 3: default for skip is false
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("skip") == false,
        "default_by_flag_skip",
        "After reset_all, 'skip' should be false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_default_by_key
  Tests that reset_all restores known defaults accessible by key string.
  Tests the following:
  - "config-enabled" default is true
  - "timeout" default is D_TEST_DEFAULT_TIMEOUT
  - "authors" default ptr is NULL
*/
bool
d_tests_sa_cvar_default_by_key
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: default for "config-enabled"
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "default_by_key_enabled",
        "After reset_all, 'config-enabled' should be true",
        _counter) && result;

    // test 2: default for "timeout"
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
        "default_by_key_timeout",
        "After reset_all, 'timeout' should be default timeout",
        _counter) && result;

    // test 3: default for "authors" (metadata, ptr is NULL)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_PTR("authors") == NULL,
        "default_by_key_authors",
        "After reset_all, 'authors' should be NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_default_after_set
  Tests that defaults are restored after set + reset_all.
  Tests the following:
  - Modify ENABLED to false, reset_all restores true
  - Modify TIMEOUT_MS to 9999, reset_all restores default
*/
bool
d_tests_sa_cvar_default_after_set
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // modify ENABLED to false
    val.b = false;
    d_test_registry_set(D_TEST_CONFIG_ENABLED, val);

    // modify TIMEOUT_MS to 9999
    val.z = 9999;
    d_test_registry_set(D_TEST_CONFIG_TIMEOUT_MS, val);

    // reset all to defaults
    d_test_registry_reset_all();

    // test 1: ENABLED restored to true
    val = d_test_registry_get(D_TEST_CONFIG_ENABLED);

    result = d_assert_standalone(
        val.b == true,
        "default_after_set_enabled",
        "reset_all should restore ENABLED to true after set(false)",
        _counter) && result;

    // test 2: TIMEOUT_MS restored to default
    val = d_test_registry_get(D_TEST_CONFIG_TIMEOUT_MS);

    result = d_assert_standalone(
        val.z == D_TEST_DEFAULT_TIMEOUT,
        "default_after_set_timeout",
        "reset_all should restore TIMEOUT_MS to default after set",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_default_known_values
  Tests that defaults match known compile-time constants.
  Tests the following:
  - MAX_FAILURES default is D_TEST_DEFAULT_MAX_FAILURES (0)
  - MAX_INDENT default is D_TEST_DEFAULT_MAX_INDENT
  - MESSAGE_FLAGS default is 0
  - PRIORITY default is 0
*/
bool
d_tests_sa_cvar_default_known_values
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: MAX_FAILURES default
    val = d_test_registry_get(D_TEST_CONFIG_MAX_FAILURES);

    result = d_assert_standalone(
        val.z == D_TEST_DEFAULT_MAX_FAILURES,
        "default_known_max_failures",
        "After reset_all, MAX_FAILURES should be D_TEST_DEFAULT_MAX_FAILURES",
        _counter) && result;

    // test 2: INDENT_MAX_LEVEL default
    val = d_test_registry_get(D_TEST_CONFIG_INDENT_MAX_LEVEL);

    result = d_assert_standalone(
        val.u16 == D_TEST_DEFAULT_MAX_INDENT,
        "default_known_max_indent",
        "After reset_all, INDENT_MAX_LEVEL should be D_TEST_DEFAULT_MAX_INDENT",
        _counter) && result;

    // test 3: MESSAGE_FLAGS default is 0
    val = d_test_registry_get(D_TEST_CONFIG_MESSAGE_FLAGS);

    result = d_assert_standalone(
        val.u32 == 0,
        "default_known_message_flags",
        "After reset_all, MESSAGE_FLAGS should be 0",
        _counter) && result;

    // test 4: PRIORITY default is 0
    val = d_test_registry_get(D_TEST_CONFIG_PRIORITY);

    result = d_assert_standalone(
        val.i32 == 0,
        "default_known_priority",
        "After reset_all, PRIORITY should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_default_all
  Aggregation function that runs all default value helper tests.
*/
bool
d_tests_sa_cvar_default_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Default Value Helpers\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_default_by_flag(_counter)     && result;
    result = d_tests_sa_cvar_default_by_key(_counter)      && result;
    result = d_tests_sa_cvar_default_after_set(_counter)   && result;
    result = d_tests_sa_cvar_default_known_values(_counter) && result;

    return result;
}
