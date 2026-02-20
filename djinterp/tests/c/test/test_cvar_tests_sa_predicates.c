#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * XI. PREDICATE FUNCTION TESTS
 *****************************************************************************/

/*
  NOTE: The predicate functions d_test_registry_is_config_row,
  d_test_registry_is_metadata_row, and d_test_registry_is_required_row are
  declared in test_cvar.h but defined as `static inline` in test_cvar.c,
  giving them internal linkage. They cannot be called directly from external
  test code. Instead we verify the command_flags field directly, which is the
  same logic the predicates use internally. The FOREACH_CONFIG and
  FOREACH_METADATA macros (which pass these predicates to the registry
  iterator) are tested in section XII (table integrity).
*/


/*
d_tests_sa_cvar_predicate_config_true
  Tests that config entries have the IS_CONFIG command flag set.
  Tests the following:
  - "config-enabled" row has IS_CONFIG bit set
  - "timeout" row has IS_CONFIG bit set
*/
bool
d_tests_sa_cvar_predicate_config_true
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: config-enabled has IS_CONFIG
    row = d_test_registry_find("config-enabled");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0,
        "predicate_config_true_enabled",
        "'config-enabled' should have IS_CONFIG command flag",
        _counter) && result;

    // test 2: timeout has IS_CONFIG
    row = d_test_registry_find("timeout");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0,
        "predicate_config_true_timeout",
        "'timeout' should have IS_CONFIG command flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_predicate_config_false
  Tests that metadata entries do NOT have the IS_CONFIG command flag.
  Tests the following:
  - "authors" row does not have IS_CONFIG bit set
  - "name" row does not have IS_CONFIG bit set
*/
bool
d_tests_sa_cvar_predicate_config_false
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: authors does NOT have IS_CONFIG
    row = d_test_registry_find("authors");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) == 0,
        "predicate_config_false_authors",
        "'authors' should not have IS_CONFIG command flag",
        _counter) && result;

    // test 2: name does NOT have IS_CONFIG
    row = d_test_registry_find("name");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) == 0,
        "predicate_config_false_name",
        "'name' should not have IS_CONFIG command flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_predicate_metadata_true
  Tests that metadata entries have the IS_METADATA command flag set.
  Tests the following:
  - "authors" row has IS_METADATA bit set
  - "description" row has IS_METADATA bit set
*/
bool
d_tests_sa_cvar_predicate_metadata_true
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: authors has IS_METADATA
    row = d_test_registry_find("authors");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0,
        "predicate_metadata_true_authors",
        "'authors' should have IS_METADATA command flag",
        _counter) && result;

    // test 2: description has IS_METADATA
    row = d_test_registry_find("description");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0,
        "predicate_metadata_true_description",
        "'description' should have IS_METADATA command flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_predicate_metadata_false
  Tests that config entries do NOT have the IS_METADATA command flag.
  Tests the following:
  - "config-enabled" row does not have IS_METADATA bit set
  - "skip" row does not have IS_METADATA bit set
*/
bool
d_tests_sa_cvar_predicate_metadata_false
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: config-enabled does NOT have IS_METADATA
    row = d_test_registry_find("config-enabled");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) == 0,
        "predicate_metadata_false_enabled",
        "'config-enabled' should not have IS_METADATA command flag",
        _counter) && result;

    // test 2: skip does NOT have IS_METADATA
    row = d_test_registry_find("skip");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) == 0,
        "predicate_metadata_false_skip",
        "'skip' should not have IS_METADATA command flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_predicate_required_false
  Tests that no current rows have the IS_REQUIRED command flag.
  Tests the following:
  - "config-enabled" does not have IS_REQUIRED
  - "authors" does not have IS_REQUIRED
  - "name" does not have IS_REQUIRED
*/
bool
d_tests_sa_cvar_predicate_required_false
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();

    // test 1: config-enabled is NOT required
    row = d_test_registry_find("config-enabled");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_REQUIRED) == 0,
        "predicate_required_false_enabled",
        "'config-enabled' should not have IS_REQUIRED flag",
        _counter) && result;

    // test 2: authors is NOT required
    row = d_test_registry_find("authors");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_REQUIRED) == 0,
        "predicate_required_false_authors",
        "'authors' should not have IS_REQUIRED flag",
        _counter) && result;

    // test 3: name is NOT required
    row = d_test_registry_find("name");

    result = d_assert_standalone(
        row != NULL &&
            (row->command_flags & D_TEST_REGISTRY_FLAG_IS_REQUIRED) == 0,
        "predicate_required_false_name",
        "'name' should not have IS_REQUIRED flag",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_predicate_all
  Aggregation function that runs all predicate function tests.
*/
bool
d_tests_sa_cvar_predicate_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Predicate Functions\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_predicate_config_true(_counter)    && result;
    result = d_tests_sa_cvar_predicate_config_false(_counter)   && result;
    result = d_tests_sa_cvar_predicate_metadata_true(_counter)  && result;
    result = d_tests_sa_cvar_predicate_metadata_false(_counter) && result;
    result = d_tests_sa_cvar_predicate_required_false(_counter) && result;

    return result;
}
