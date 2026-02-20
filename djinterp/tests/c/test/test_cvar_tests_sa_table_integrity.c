#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * XII. REGISTRY TABLE INTEGRITY TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_table_keys_non_null
  Tests that all registry rows have non-NULL key strings.
  Tests the following:
  - Every row iterated via D_TEST_REGISTRY_FOREACH has a non-NULL key
*/
bool
d_tests_sa_cvar_table_keys_non_null
(
    struct d_test_counter* _counter
)
{
    bool   result;
    bool   all_non_null;
    size_t count;

    result = true;

    d_test_registry_init();

    all_non_null = true;
    count        = 0;

    D_TEST_REGISTRY_FOREACH(row)
    {
        if (row->key == NULL)
        {
            all_non_null = false;
        }

        count++;
    }

    // test 1: all keys are non-NULL
    result = d_assert_standalone(
        all_non_null,
        "table_keys_all_non_null",
        "All registry rows should have non-NULL key strings",
        _counter) && result;

    // test 2: iteration visited at least one row
    result = d_assert_standalone(
        count > 0,
        "table_keys_iterated",
        "D_TEST_REGISTRY_FOREACH should iterate at least one row",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_table_help_non_null
  Tests that all registry rows have non-NULL help text.
  Tests the following:
  - Every row has a non-NULL help member
*/
bool
d_tests_sa_cvar_table_help_non_null
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool all_non_null;

    result = true;

    d_test_registry_init();

    all_non_null = true;

    D_TEST_REGISTRY_FOREACH(row)
    {
        if (row->help == NULL)
        {
            all_non_null = false;
        }
    }

    // test 1: all help strings are non-NULL
    result = d_assert_standalone(
        all_non_null,
        "table_help_all_non_null",
        "All registry rows should have non-NULL help text",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_table_config_flags
  Tests that config rows have the IS_CONFIG command flag set.
  Tests the following:
  - All rows with IS_CONFIG counted via D_TEST_REGISTRY_FOREACH
  - Config count is 8
*/
bool
d_tests_sa_cvar_table_config_flags
(
    struct d_test_counter* _counter
)
{
    bool   result;
    bool   all_valid;
    size_t config_count;

    result = true;

    d_test_registry_init();

    all_valid    = true;
    config_count = 0;

    D_TEST_REGISTRY_FOREACH(row)
    {
        if ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0)
        {
            config_count++;

            // verify the row also has a non-NULL key
            if (row->key == NULL)
            {
                all_valid = false;
            }
        }
    }

    // test 1: all config rows are valid
    result = d_assert_standalone(
        all_valid,
        "table_config_flags_set",
        "All config rows should have valid key strings",
        _counter) && result;

    // test 2: config count is 8
    result = d_assert_standalone(
        config_count == 8,
        "table_config_count_8",
        "Should have exactly 8 config rows",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_table_metadata_flags
  Tests that metadata rows have the IS_METADATA command flag set.
  Tests the following:
  - All rows with IS_METADATA counted via D_TEST_REGISTRY_FOREACH
  - Metadata count is 17
*/
bool
d_tests_sa_cvar_table_metadata_flags
(
    struct d_test_counter* _counter
)
{
    bool   result;
    bool   all_valid;
    size_t metadata_count;

    result = true;

    d_test_registry_init();

    all_valid      = true;
    metadata_count = 0;

    D_TEST_REGISTRY_FOREACH(row)
    {
        if ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0)
        {
            metadata_count++;

            // verify the row also has a non-NULL key
            if (row->key == NULL)
            {
                all_valid = false;
            }
        }
    }

    // test 1: all metadata rows are valid
    result = d_assert_standalone(
        all_valid,
        "table_metadata_flags_set",
        "All metadata rows should have valid key strings",
        _counter) && result;

    // test 2: metadata count is 17
    result = d_assert_standalone(
        metadata_count == 17,
        "table_metadata_count_17",
        "Should have exactly 17 metadata rows",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_table_row_counts
  Tests the total row count and config/metadata breakdown.
  Tests the following:
  - Total row count is 25
  - Config (8) + metadata (17) = 25
*/
bool
d_tests_sa_cvar_table_row_counts
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t total_count;
    size_t config_count;
    size_t metadata_count;

    result = true;

    d_test_registry_init();

    total_count    = 0;
    config_count   = 0;
    metadata_count = 0;

    D_TEST_REGISTRY_FOREACH(row)
    {
        total_count++;

        if ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0)
        {
            config_count++;
        }

        if ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0)
        {
            metadata_count++;
        }
    }

    // test 1: total count is 25
    result = d_assert_standalone(
        total_count == 25,
        "table_total_count_25",
        "Total registry row count should be 25",
        _counter) && result;

    // test 2: config + metadata = total
    result = d_assert_standalone(
        config_count + metadata_count == total_count,
        "table_config_plus_metadata",
        "Config count + metadata count should equal total count",
        _counter) && result;

    // test 3: explicit breakdown
    result = d_assert_standalone(
        config_count == 8 && metadata_count == 17,
        "table_breakdown_8_17",
        "Should have 8 config rows and 17 metadata rows",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_table_integrity_all
  Aggregation function that runs all table integrity tests.
*/
bool
d_tests_sa_cvar_table_integrity_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Registry Table Integrity\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_table_keys_non_null(_counter)  && result;
    result = d_tests_sa_cvar_table_help_non_null(_counter)  && result;
    result = d_tests_sa_cvar_table_config_flags(_counter)   && result;
    result = d_tests_sa_cvar_table_metadata_flags(_counter) && result;
    result = d_tests_sa_cvar_table_row_counts(_counter)     && result;

    return result;
}
