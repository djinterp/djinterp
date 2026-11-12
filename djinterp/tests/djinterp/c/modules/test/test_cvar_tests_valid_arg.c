/*******************************************************************************
* djinterp [c][test]                                 test_cvar_tests_valid_arg.c
*
* test_cvar.h's suite, block "cvar_valid_arg_all": d_test_registry_is_valid_arg
* with config and metadata keys, the wrong flag, NULL and an unknown key.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_valid_arg.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 9.  ARG VALIDATION TESTS
//==============================================================================

/*
d_tests_cvar_valid_arg_config
  Tests d_test_registry_is_valid_arg for config keys with IS_CONFIG flag.
  Tests the following:
  - "config-enabled" with IS_CONFIG returns true
  - "timeout" with IS_CONFIG returns true
  - "skip" with IS_CONFIG returns true
*/
bool
d_tests_cvar_valid_arg_config(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_valid_arg_config");

    bool result = true;

    d_test_registry_init();

    // test 1: config-enabled with IS_CONFIG
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_config_enabled",
                              d_test_registry_is_valid_arg("config-enabled",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
                              "'config-enabled' should be valid with IS_CONFIG flag") && result;

    // test 2: timeout with IS_CONFIG
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_config_timeout",
                              d_test_registry_is_valid_arg("timeout",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
                              "'timeout' should be valid with IS_CONFIG flag") && result;

    // test 3: skip with IS_CONFIG
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_config_skip",
                              d_test_registry_is_valid_arg("skip",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == true,
                              "'skip' should be valid with IS_CONFIG flag") && result;

    return result;
}

/*
d_tests_cvar_valid_arg_metadata
  Tests d_test_registry_is_valid_arg for metadata keys with IS_METADATA flag.
  Tests the following:
  - "authors" with IS_METADATA returns true
  - "name" with IS_METADATA returns true
*/
bool
d_tests_cvar_valid_arg_metadata(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_valid_arg_metadata");

    bool result = true;

    d_test_registry_init();

    // test 1: authors with IS_METADATA
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_metadata_authors",
                              d_test_registry_is_valid_arg("authors",
                                                           D_TEST_REGISTRY_FLAG_IS_METADATA) == true,
                              "'authors' should be valid with IS_METADATA flag") && result;

    // test 2: name with IS_METADATA
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_metadata_name",
                              d_test_registry_is_valid_arg("name",
                                                           D_TEST_REGISTRY_FLAG_IS_METADATA) == true,
                              "'name' should be valid with IS_METADATA flag") && result;

    return result;
}

/*
d_tests_cvar_valid_arg_wrong_flag
  Tests d_test_registry_is_valid_arg with a key that exists but has the wrong
  command flag.
  Tests the following:
  - Config key with IS_METADATA flag returns false
  - Metadata key with IS_CONFIG flag returns false
*/
bool
d_tests_cvar_valid_arg_wrong_flag(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_valid_arg_wrong_flag");

    bool result = true;

    d_test_registry_init();

    // test 1: config key with IS_METADATA flag
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_config_wrong_flag",
                              d_test_registry_is_valid_arg("config-enabled",
                                                           D_TEST_REGISTRY_FLAG_IS_METADATA) == false,
                              "'config-enabled' with IS_METADATA should return false") && result;

    // test 2: metadata key with IS_CONFIG flag
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_metadata_wrong_flag",
                              d_test_registry_is_valid_arg("authors",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
                              "'authors' with IS_CONFIG should return false") && result;

    // test 3: config key with SESSION flag (not set)
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_config_session_flag",
                              d_test_registry_is_valid_arg("timeout",
                                                           D_TEST_REGISTRY_FLAG_SESSION) == false,
                              "'timeout' with SESSION should return false") && result;

    return result;
}

/*
d_tests_cvar_valid_arg_null_key
  Tests d_test_registry_is_valid_arg with NULL key.
  Tests the following:
  - NULL key returns false
*/
bool
d_tests_cvar_valid_arg_null_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_valid_arg_null_key");

    bool result = true;

    d_test_registry_init();

    // test 1: NULL key returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_null_key",
                              d_test_registry_is_valid_arg(NULL,
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
                              "is_valid_arg(NULL, ...) should return false") && result;

    return result;
}

/*
d_tests_cvar_valid_arg_invalid_key
  Tests d_test_registry_is_valid_arg with a nonexistent key.
  Tests the following:
  - Nonexistent key returns false
  - Empty string returns false
*/
bool
d_tests_cvar_valid_arg_invalid_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_valid_arg_invalid_key");

    bool result = true;

    d_test_registry_init();

    // test 1: nonexistent key returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_invalid_key",
                              d_test_registry_is_valid_arg("nonexistent-key",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
                              "is_valid_arg('nonexistent-key', ...) should return false") && result;

    // test 2: empty string returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "valid_arg_empty_key",
                              d_test_registry_is_valid_arg("",
                                                           D_TEST_REGISTRY_FLAG_IS_CONFIG) == false,
                              "is_valid_arg('', ...) should return false") && result;

    return result;
}

/*
d_tests_cvar_valid_arg_all
  Aggregation function that runs all arg validation tests.
*/
bool
d_tests_cvar_valid_arg_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_valid_arg_all");

    bool result = true;

    result = d_tests_cvar_valid_arg_config(_suite)      && result;
    result = d_tests_cvar_valid_arg_metadata(_suite)    && result;
    result = d_tests_cvar_valid_arg_wrong_flag(_suite)  && result;
    result = d_tests_cvar_valid_arg_null_key(_suite)    && result;
    result = d_tests_cvar_valid_arg_invalid_key(_suite) && result;

    return result;
}
