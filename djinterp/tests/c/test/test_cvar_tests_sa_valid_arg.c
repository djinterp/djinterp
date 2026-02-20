#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * IX. ARG VALIDATION TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_valid_arg_config
  Tests d_test_registry_is_valid_arg for config keys with IS_CONFIG flag.
  Tests the following:
  - "config-enabled" with IS_CONFIG returns true
  - "timeout" with IS_CONFIG returns true
  - "skip" with IS_CONFIG returns true
*/
bool
d_tests_sa_cvar_valid_arg_config
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: config-enabled with IS_CONFIG
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("config-enabled",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
        "valid_arg_config_enabled",
        "'config-enabled' should be valid with IS_CONFIG flag",
        _counter) && result;

    // test 2: timeout with IS_CONFIG
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("timeout",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
        "valid_arg_config_timeout",
        "'timeout' should be valid with IS_CONFIG flag",
        _counter) && result;

    // test 3: skip with IS_CONFIG
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("skip",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
        "valid_arg_config_skip",
        "'skip' should be valid with IS_CONFIG flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_valid_arg_metadata
  Tests d_test_registry_is_valid_arg for metadata keys with IS_METADATA flag.
  Tests the following:
  - "authors" with IS_METADATA returns true
  - "name" with IS_METADATA returns true
*/
bool
d_tests_sa_cvar_valid_arg_metadata
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: authors with IS_METADATA
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("authors",
                                     D_TEST_REGISTRY_FLAG_IS_METADATA) == true,
        "valid_arg_metadata_authors",
        "'authors' should be valid with IS_METADATA flag",
        _counter) && result;

    // test 2: name with IS_METADATA
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("name",
                                     D_TEST_REGISTRY_FLAG_IS_METADATA) == true,
        "valid_arg_metadata_name",
        "'name' should be valid with IS_METADATA flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_valid_arg_wrong_flag
  Tests d_test_registry_is_valid_arg with a key that exists but has the wrong
  command flag.
  Tests the following:
  - Config key with IS_METADATA flag returns false
  - Metadata key with IS_CONFIG flag returns false
*/
bool
d_tests_sa_cvar_valid_arg_wrong_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: config key with IS_METADATA flag
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("config-enabled",
                                     D_TEST_REGISTRY_FLAG_IS_METADATA) == false,
        "valid_arg_config_wrong_flag",
        "'config-enabled' with IS_METADATA should return false",
        _counter) && result;

    // test 2: metadata key with IS_CONFIG flag
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("authors",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
        "valid_arg_metadata_wrong_flag",
        "'authors' with IS_CONFIG should return false",
        _counter) && result;

    // test 3: config key with SESSION flag (not set)
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("timeout",
                                     D_TEST_REGISTRY_FLAG_SESSION) == false,
        "valid_arg_config_session_flag",
        "'timeout' with SESSION should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_valid_arg_null_key
  Tests d_test_registry_is_valid_arg with NULL key.
  Tests the following:
  - NULL key returns false
*/
bool
d_tests_sa_cvar_valid_arg_null_key
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: NULL key returns false
    result = d_assert_standalone(
        d_test_registry_is_valid_arg(NULL,
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
        "valid_arg_null_key",
        "is_valid_arg(NULL, ...) should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_valid_arg_invalid_key
  Tests d_test_registry_is_valid_arg with a nonexistent key.
  Tests the following:
  - Nonexistent key returns false
  - Empty string returns false
*/
bool
d_tests_sa_cvar_valid_arg_invalid_key
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: nonexistent key returns false
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("nonexistent-key",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
        "valid_arg_invalid_key",
        "is_valid_arg('nonexistent-key', ...) should return false",
        _counter) && result;

    // test 2: empty string returns false
    result = d_assert_standalone(
        d_test_registry_is_valid_arg("",
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
        "valid_arg_empty_key",
        "is_valid_arg('', ...) should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_valid_arg_all
  Aggregation function that runs all arg validation tests.
*/
bool
d_tests_sa_cvar_valid_arg_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Arg Validation\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_valid_arg_config(_counter)      && result;
    result = d_tests_sa_cvar_valid_arg_metadata(_counter)    && result;
    result = d_tests_sa_cvar_valid_arg_wrong_flag(_counter)  && result;
    result = d_tests_sa_cvar_valid_arg_null_key(_counter)    && result;
    result = d_tests_sa_cvar_valid_arg_invalid_key(_counter) && result;

    return result;
}
