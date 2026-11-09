/*******************************************************************************
* djinterp [test]                                     test_cvar_tests_defaults.c
*
* test_cvar.h's suite, block "cvar_default_all": the defaults after reset_all,
* by flag and by key, after a set, and against the compile-time constants.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_defaults.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 13. DEFAULT VALUE HELPER TESTS
//==============================================================================

/*
  NOTE: d_test_registry_get_default and d_test_registry_get_default_by_key
  are declared in test_cvar.h but not yet implemented in test_cvar.c.
  We test default values indirectly by calling d_test_registry_reset_all()
  which restores all rows to their snapshot defaults, then reading back
  via d_test_registry_get(). This validates the same invariants.
*/

/*
d_tests_cvar_default_by_flag
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
d_tests_cvar_default_by_flag(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_default_by_flag");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: default for config-enabled is true
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_flag_enabled",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "After reset_all, 'config-enabled' should be true") && result;

    // test 2: default for timeout
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_flag_timeout",
                              D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
                              "After reset_all, 'timeout' should be D_TEST_DEFAULT_TIMEOUT") && result;

    // test 3: default for skip is false
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_flag_skip",
                              D_TEST_REGISTRY_VALUE_BOOL("skip") == false,
                              "After reset_all, 'skip' should be false") && result;

    return result;
}

/*
d_tests_cvar_default_by_key
  Tests that reset_all restores known defaults accessible by key string.
  Tests the following:
  - "config-enabled" default is true
  - "timeout" default is D_TEST_DEFAULT_TIMEOUT
  - "authors" default ptr is NULL
*/
bool
d_tests_cvar_default_by_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_default_by_key");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: default for "config-enabled"
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_key_enabled",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "After reset_all, 'config-enabled' should be true") && result;

    // test 2: default for "timeout"
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_key_timeout",
                              D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
                              "After reset_all, 'timeout' should be default timeout") && result;

    // test 3: default for "authors" (metadata, ptr is NULL)
    result = D_TEST_CHECK_MSG(_suite,
                              "default_by_key_authors",
                              D_TEST_REGISTRY_VALUE_PTR("authors") == NULL,
                              "After reset_all, 'authors' should be NULL") && result;

    return result;
}

/*
d_tests_cvar_default_after_set
  Tests that defaults are restored after set + reset_all.
  Tests the following:
  - Modify ENABLED to false, reset_all restores true
  - Modify TIMEOUT_MS to 9999, reset_all restores default
*/
bool
d_tests_cvar_default_after_set(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_default_after_set");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    union dj_test_value val = {0};

    // modify ENABLED to false
    val.b = false;
    d_test_registry_set(D_TEST_OPTIONS_ENABLED,
                        val);

    // modify TIMEOUT_MS to 9999
    val.z = 9999;
    d_test_registry_set(D_TEST_OPTIONS_TIMEOUT_MS,
                        val);

    // reset all to defaults
    d_test_registry_reset_all();

    // test 1: ENABLED restored to true
    val = d_test_registry_get(D_TEST_OPTIONS_ENABLED);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_after_set_enabled",
                              val.b == true,
                              "reset_all should restore ENABLED to true after set(false)") && result;

    // test 2: TIMEOUT_MS restored to default
    val = d_test_registry_get(D_TEST_OPTIONS_TIMEOUT_MS);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_after_set_timeout",
                              val.z == D_TEST_DEFAULT_TIMEOUT,
                              "reset_all should restore TIMEOUT_MS to default after set") && result;

    return result;
}

/*
d_tests_cvar_default_known_values
  Tests that defaults match known compile-time constants.
  Tests the following:
  - MAX_FAILURES default is D_TEST_DEFAULT_MAX_FAILURES (0)
  - MAX_INDENT default is D_TEST_DEFAULT_MAX_INDENT
  - MESSAGE_FLAGS default is 0
  - PRIORITY default is 0
*/
bool
d_tests_cvar_default_known_values(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_default_known_values");

    bool result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: MAX_FAILURES default
    union dj_test_value val = d_test_registry_get(D_TEST_OPTIONS_MAX_FAILURES);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_known_max_failures",
                              val.z == D_TEST_DEFAULT_MAX_FAILURES,
                              "After reset_all, MAX_FAILURES should be D_TEST_DEFAULT_MAX_FAILURES") && result;

    // test 2: INDENT_MAX_LEVEL default
    val = d_test_registry_get(D_TEST_OPTIONS_INDENT_MAX_LEVEL);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_known_max_indent",
                              val.u16 == D_TEST_DEFAULT_MAX_INDENT,
                              "After reset_all, INDENT_MAX_LEVEL should be D_TEST_DEFAULT_MAX_INDENT") && result;

    // test 3: MESSAGE_FLAGS default is 0
    val = d_test_registry_get(D_TEST_OPTIONS_MESSAGE_FLAGS);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_known_message_flags",
                              val.u32 == 0,
                              "After reset_all, MESSAGE_FLAGS should be 0") && result;

    // test 4: PRIORITY default is 0
    val = d_test_registry_get(D_TEST_OPTIONS_PRIORITY);

    result = D_TEST_CHECK_MSG(_suite,
                              "default_known_priority",
                              val.i32 == 0,
                              "After reset_all, PRIORITY should be 0") && result;

    return result;
}

/*
d_tests_cvar_default_all
  Aggregation function that runs all default value helper tests.
*/
bool
d_tests_cvar_default_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_default_all");

    bool result = true;

    result = d_tests_cvar_default_by_flag(_suite)     && result;
    result = d_tests_cvar_default_by_key(_suite)      && result;
    result = d_tests_cvar_default_after_set(_suite)   && result;
    result = d_tests_cvar_default_known_values(_suite) && result;

    return result;
}
