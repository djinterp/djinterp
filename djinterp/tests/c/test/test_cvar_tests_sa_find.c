#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * V. ROW FIND TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_find_valid_key
  Tests d_test_registry_find with a valid key.
  Tests the following:
  - "config-enabled" returns non-NULL
  - "authors" (metadata) returns non-NULL
  - "timeout" (config) returns non-NULL
*/
bool
d_tests_sa_cvar_find_valid_key
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: config key returns non-NULL
    row = d_test_registry_find("config-enabled");

    result = d_assert_standalone(
        row != NULL,
        "find_valid_config_key",
        "find('config-enabled') should return non-NULL",
        _counter) && result;

    // test 2: metadata key returns non-NULL
    row = d_test_registry_find("authors");

    result = d_assert_standalone(
        row != NULL,
        "find_valid_metadata_key",
        "find('authors') should return non-NULL",
        _counter) && result;

    // test 3: another config key
    row = d_test_registry_find("timeout");

    result = d_assert_standalone(
        row != NULL,
        "find_valid_timeout_key",
        "find('timeout') should return non-NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_key_matches
  Tests that d_test_registry_find returns a row whose key matches.
  Tests the following:
  - Returned row's key matches the queried key
  - Returned row's flag matches the expected enum value
*/
bool
d_tests_sa_cvar_find_key_matches
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: returned row key matches
    row = d_test_registry_find("skip");

    result = d_assert_standalone(
        row != NULL && d_strcasecmp(row->key, "skip") == 0,
        "find_key_matches_skip",
        "Returned row key should match 'skip'",
        _counter) && result;

    // test 2: returned row flag matches expected enum
    result = d_assert_standalone(
        row != NULL && row->flag == D_TEST_CONFIG_SKIP,
        "find_flag_matches_skip",
        "Returned row flag should be D_TEST_CONFIG_SKIP",
        _counter) && result;

    // test 3: metadata row key matches
    row = d_test_registry_find("name");

    result = d_assert_standalone(
        row != NULL && d_strcasecmp(row->key, "name") == 0,
        "find_key_matches_name",
        "Returned row key should match 'name'",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_null_key
  Tests d_test_registry_find with NULL key.
  Tests the following:
  - NULL key returns NULL
*/
bool
d_tests_sa_cvar_find_null_key
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: NULL key returns NULL
    row = d_test_registry_find(NULL);

    result = d_assert_standalone(
        row == NULL,
        "find_null_key",
        "find(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_invalid_key
  Tests d_test_registry_find with an invalid key.
  Tests the following:
  - Nonexistent key returns NULL
  - Empty string returns NULL
*/
bool
d_tests_sa_cvar_find_invalid_key
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: nonexistent key returns NULL
    row = d_test_registry_find("nonexistent-key-xyz");

    result = d_assert_standalone(
        row == NULL,
        "find_invalid_key",
        "find('nonexistent-key-xyz') should return NULL",
        _counter) && result;

    // test 2: empty string returns NULL
    row = d_test_registry_find("");

    result = d_assert_standalone(
        row == NULL,
        "find_empty_key",
        "find('') should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_by_flag_valid
  Tests d_test_registry_find_by_flag with valid flags.
  Tests the following:
  - Config flag (D_TEST_CONFIG_ENABLED) returns non-NULL row
  - Metadata flag (D_TEST_METADATA_AUTHORS) returns non-NULL row
  - Returned row flag matches the queried flag
*/
bool
d_tests_sa_cvar_find_by_flag_valid
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: config flag returns non-NULL
    row = d_test_registry_find_by_flag(D_TEST_CONFIG_ENABLED);

    result = d_assert_standalone(
        row != NULL,
        "find_by_flag_config_enabled",
        "find_by_flag(D_TEST_CONFIG_ENABLED) should return non-NULL",
        _counter) && result;

    // test 2: returned row flag matches queried flag
    result = d_assert_standalone(
        row != NULL && row->flag == D_TEST_CONFIG_ENABLED,
        "find_by_flag_config_enabled_matches",
        "Returned row flag should match D_TEST_CONFIG_ENABLED",
        _counter) && result;

    // test 3: metadata flag returns non-NULL
    row = d_test_registry_find_by_flag(D_TEST_METADATA_AUTHORS);

    result = d_assert_standalone(
        row != NULL,
        "find_by_flag_metadata_authors",
        "find_by_flag(D_TEST_METADATA_AUTHORS) should return non-NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_by_flag_invalid
  Tests d_test_registry_find_by_flag with invalid flag.
  Tests the following:
  - Flag value 0xFFFFFFFF returns NULL
  - Flag value 9999 returns NULL
*/
bool
d_tests_sa_cvar_find_by_flag_invalid
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: invalid flag 0xFFFFFFFF returns NULL
    row = d_test_registry_find_by_flag(0xFFFFFFFF);

    result = d_assert_standalone(
        row == NULL,
        "find_by_flag_invalid_max",
        "find_by_flag(0xFFFFFFFF) should return NULL",
        _counter) && result;

    // test 2: arbitrary invalid flag returns NULL
    row = d_test_registry_find_by_flag(9999);

    result = d_assert_standalone(
        row == NULL,
        "find_by_flag_invalid_9999",
        "find_by_flag(9999) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_find_all
  Aggregation function that runs all row find tests.
*/
bool
d_tests_sa_cvar_find_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Row Find\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_find_valid_key(_counter)      && result;
    result = d_tests_sa_cvar_find_key_matches(_counter)     && result;
    result = d_tests_sa_cvar_find_null_key(_counter)        && result;
    result = d_tests_sa_cvar_find_invalid_key(_counter)     && result;
    result = d_tests_sa_cvar_find_by_flag_valid(_counter)   && result;
    result = d_tests_sa_cvar_find_by_flag_invalid(_counter) && result;

    return result;
}
