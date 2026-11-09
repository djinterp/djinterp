/*******************************************************************************
* djinterp [test]                                         test_cvar_tests_find.c
*
* test_cvar.h's suite, block "cvar_find_all": d_test_registry_find by key
* (valid, NULL, unknown) and d_test_registry_find_by_flag (valid, unknown).
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_find.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 5.  ROW FIND TESTS
//==============================================================================

/*
d_tests_cvar_find_valid_key
  Tests d_test_registry_find with a valid key.
  Tests the following:
  - "config-enabled" returns non-NULL
  - "authors" (metadata) returns non-NULL
  - "timeout" (config) returns non-NULL
*/
bool
d_tests_cvar_find_valid_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_valid_key");

    bool result = true;

    d_test_registry_init();

    // test 1: config key returns non-NULL
    struct dj_test_registry_row* row = d_test_registry_find("config-enabled");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_valid_config_key",
                              row != NULL,
                              "find('config-enabled') should return non-NULL") && result;

    // test 2: metadata key returns non-NULL
    row = d_test_registry_find("authors");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_valid_metadata_key",
                              row != NULL,
                              "find('authors') should return non-NULL") && result;

    // test 3: another config key
    row = d_test_registry_find("timeout");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_valid_timeout_key",
                              row != NULL,
                              "find('timeout') should return non-NULL") && result;

    return result;
}

/*
d_tests_cvar_find_key_matches
  Tests that d_test_registry_find returns a row whose key matches.
  Tests the following:
  - Returned row's key matches the queried key
  - Returned row's flag matches the expected enum value
*/
bool
d_tests_cvar_find_key_matches(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_key_matches");

    bool result = true;

    d_test_registry_init();

    // test 1: returned row key matches
    struct dj_test_registry_row* row = d_test_registry_find("skip");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_key_matches_skip",
                              ( (row != NULL) &&
                                (d_strcasecmp(row->key,
                                              "skip") == 0) ),
                              "Returned row key should match 'skip'") && result;

    // test 2: returned row flag matches expected enum
    result = D_TEST_CHECK_MSG(_suite,
                              "find_flag_matches_skip",
                              ( (row != NULL) &&
                                (row->flag == D_TEST_OPTIONS_SKIP) ),
                              "Returned row flag should be D_TEST_OPTIONS_SKIP") && result;

    // test 3: metadata row key matches
    row = d_test_registry_find("name");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_key_matches_name",
                              ( (row != NULL) &&
                                (d_strcasecmp(row->key,
                                              "name") == 0) ),
                              "Returned row key should match 'name'") && result;

    return result;
}

/*
d_tests_cvar_find_null_key
  Tests d_test_registry_find with NULL key.
  Tests the following:
  - NULL key returns NULL
*/
bool
d_tests_cvar_find_null_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_null_key");

    bool result = true;

    d_test_registry_init();

    // test 1: NULL key returns NULL
    struct dj_test_registry_row* row = d_test_registry_find(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "find_null_key",
                              row == NULL,
                              "find(NULL) should return NULL") && result;

    return result;
}

/*
d_tests_cvar_find_invalid_key
  Tests d_test_registry_find with an invalid key.
  Tests the following:
  - Nonexistent key returns NULL
  - Empty string returns NULL
*/
bool
d_tests_cvar_find_invalid_key(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_invalid_key");

    bool result = true;

    d_test_registry_init();

    // test 1: nonexistent key returns NULL
    struct dj_test_registry_row* row =
        d_test_registry_find("nonexistent-key-xyz");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_invalid_key",
                              row == NULL,
                              "find('nonexistent-key-xyz') should return NULL") && result;

    // test 2: empty string returns NULL
    row = d_test_registry_find("");

    result = D_TEST_CHECK_MSG(_suite,
                              "find_empty_key",
                              row == NULL,
                              "find('') should return NULL") && result;

    return result;
}

/*
d_tests_cvar_find_by_flag_valid
  Tests d_test_registry_find_by_flag with valid flags.
  Tests the following:
  - Config flag (D_TEST_OPTIONS_ENABLED) returns non-NULL row
  - Metadata flag (D_TEST_METADATA_AUTHORS) returns non-NULL row
  - Returned row flag matches the queried flag
*/
bool
d_tests_cvar_find_by_flag_valid(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_by_flag_valid");

    bool result = true;

    d_test_registry_init();

    // test 1: config flag returns non-NULL
    struct dj_test_registry_row* row =
        d_test_registry_find_by_flag(D_TEST_OPTIONS_ENABLED);

    result = D_TEST_CHECK_MSG(_suite,
                              "find_by_flag_config_enabled",
                              row != NULL,
                              "find_by_flag(D_TEST_OPTIONS_ENABLED) should return non-NULL") && result;

    // test 2: returned row flag matches queried flag
    result = D_TEST_CHECK_MSG(_suite,
                              "find_by_flag_config_enabled_matches",
                              ( (row != NULL) &&
                                (row->flag == D_TEST_OPTIONS_ENABLED) ),
                              "Returned row flag should match D_TEST_OPTIONS_ENABLED") && result;

    // test 3: metadata flag returns non-NULL
    row = d_test_registry_find_by_flag(D_TEST_METADATA_AUTHORS);

    result = D_TEST_CHECK_MSG(_suite,
                              "find_by_flag_metadata_authors",
                              row != NULL,
                              "find_by_flag(D_TEST_METADATA_AUTHORS) should return non-NULL") && result;

    return result;
}

/*
d_tests_cvar_find_by_flag_invalid
  Tests d_test_registry_find_by_flag with invalid flag.
  Tests the following:
  - Flag value 0xFFFFFFFF returns NULL
  - Flag value 9999 returns NULL
*/
bool
d_tests_cvar_find_by_flag_invalid(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_find_by_flag_invalid");

    bool result = true;

    d_test_registry_init();

    // test 1: invalid flag 0xFFFFFFFF returns NULL
    struct dj_test_registry_row* row = d_test_registry_find_by_flag(0xFFFFFFFF);

    result = D_TEST_CHECK_MSG(_suite,
                              "find_by_flag_invalid_max",
                              row == NULL,
                              "find_by_flag(0xFFFFFFFF) should return NULL") && result;

    // test 2: arbitrary invalid flag returns NULL
    row = d_test_registry_find_by_flag(9999);

    result = D_TEST_CHECK_MSG(_suite,
                              "find_by_flag_invalid_9999",
                              row == NULL,
                              "find_by_flag(9999) should return NULL") && result;

    return result;
}

/*
d_tests_cvar_find_all
  Aggregation function that runs all row find tests.
*/
bool
d_tests_cvar_find_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_find_all");

    bool result = true;

    result = d_tests_cvar_find_valid_key(_suite)      && result;
    result = d_tests_cvar_find_key_matches(_suite)     && result;
    result = d_tests_cvar_find_null_key(_suite)        && result;
    result = d_tests_cvar_find_invalid_key(_suite)     && result;
    result = d_tests_cvar_find_by_flag_valid(_suite)   && result;
    result = d_tests_cvar_find_by_flag_invalid(_suite) && result;

    return result;
}
