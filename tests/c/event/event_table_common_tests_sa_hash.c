#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * II. HASH FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_event_hash_fnv_bounds
  Tests d_event_hash_function returns values within table bounds.
  Tests the following:
  - Hash of typical ID is less than table size
  - Hash of zero ID is within bounds
  - Hash of negative ID is within bounds
  - Hash of max ID is within bounds
*/
bool
d_tests_sa_event_hash_fnv_bounds
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t hash;

    result = true;

    hash = d_event_hash_function(1001, 101);

    result = d_assert_standalone(
        hash < 101,
        "fnv_bounds_typical",
        "FNV hash should be within table bounds",
        _counter) && result;

    hash = d_event_hash_function(0, 101);

    result = d_assert_standalone(
        hash < 101,
        "fnv_bounds_zero",
        "FNV hash of zero ID should be within bounds",
        _counter) && result;

    hash = d_event_hash_function(-1, 101);

    result = d_assert_standalone(
        hash < 101,
        "fnv_bounds_negative",
        "FNV hash of negative ID should be within bounds",
        _counter) && result;

    hash = d_event_hash_function((d_event_id)(-1), 101);

    result = d_assert_standalone(
        hash < 101,
        "fnv_bounds_max",
        "FNV hash of max ID should be within bounds",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_fnv_consistency
  Tests d_event_hash_function is deterministic.
  Tests the following:
  - Same input produces same output on repeated calls
*/
bool
d_tests_sa_event_hash_fnv_consistency
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t hash_a;
    size_t hash_b;

    result = true;
    hash_a = d_event_hash_function(1001, 101);
    hash_b = d_event_hash_function(1001, 101);

    result = d_assert_standalone(
        hash_a == hash_b,
        "fnv_consistency",
        "Same input should always produce same hash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_fnv_differentiation
  Tests d_event_hash_function differentiates similar IDs.
  Tests the following:
  - Adjacent IDs produce different hashes
*/
bool
d_tests_sa_event_hash_fnv_differentiation
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t hash_a;
    size_t hash_b;

    result = true;
    hash_a = d_event_hash_function(1001, 101);
    hash_b = d_event_hash_function(1002, 101);

    result = d_assert_standalone(
        hash_a != hash_b,
        "fnv_differentiation",
        "Adjacent IDs should produce different hashes",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_fnv_edge_ids
  Tests d_event_hash_function with edge-case event IDs.
  Tests the following:
  - Zero event ID produces valid hash
  - Negative event ID produces valid hash
  - Max event ID produces valid hash
*/
bool
d_tests_sa_event_hash_fnv_edge_ids
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t h_zero;
    size_t h_neg;
    size_t h_max;

    result = true;
    h_zero = d_event_hash_function(0, 101);
    h_neg  = d_event_hash_function(-1, 101);
    h_max  = d_event_hash_function((d_event_id)(-1), 101);

    result = d_assert_standalone(
        h_zero < 101 && h_neg < 101 && h_max < 101,
        "fnv_edge_ids",
        "Edge IDs should all produce valid hashes",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_fnv_table_sizes
  Tests d_event_hash_function adapts to different table sizes.
  Tests the following:
  - Small table size produces hash within range
  - Large table size produces hash within range
  - Table size 1 always returns 0
*/
bool
d_tests_sa_event_hash_fnv_table_sizes
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t h_small;
    size_t h_large;
    size_t h_one;

    result  = true;
    h_small = d_event_hash_function(1001, 10);
    h_large = d_event_hash_function(1001, 10000);
    h_one   = d_event_hash_function(1001, 1);

    result = d_assert_standalone(
        h_small < 10 && h_large < 10000,
        "fnv_table_sizes",
        "Hash should adapt to different table sizes",
        _counter) && result;

    result = d_assert_standalone(
        h_one == 0,
        "fnv_table_size_one",
        "Table size 1 should always return hash 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_simple_bounds
  Tests d_event_hash_simple returns values within table bounds.
  Tests the following:
  - Hash of typical ID is less than table size
  - Hash with power-of-2 table size is within bounds
*/
bool
d_tests_sa_event_hash_simple_bounds
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t hash;

    result = true;
    hash   = d_event_hash_simple(12345, 101);

    result = d_assert_standalone(
        hash < 101,
        "simple_bounds_typical",
        "Simple hash should be within table bounds",
        _counter) && result;

    hash = d_event_hash_simple(12345, 128);

    result = d_assert_standalone(
        hash < 128,
        "simple_bounds_power_of_2",
        "Simple hash should work with power-of-2 size",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_simple_consistency
  Tests d_event_hash_simple is deterministic.
  Tests the following:
  - Same input produces same output
*/
bool
d_tests_sa_event_hash_simple_consistency
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t hash_a;
    size_t hash_b;

    result = true;
    hash_a = d_event_hash_simple(12345, 101);
    hash_b = d_event_hash_simple(12345, 101);

    result = d_assert_standalone(
        hash_a == hash_b,
        "simple_consistency",
        "Same input should always produce same hash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_simple_edge_ids
  Tests d_event_hash_simple with edge-case IDs.
  Tests the following:
  - Zero event ID produces valid hash
  - Negative event ID produces valid hash
*/
bool
d_tests_sa_event_hash_simple_edge_ids
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t h_zero;
    size_t h_neg;

    result = true;
    h_zero = d_event_hash_simple(0, 101);
    h_neg  = d_event_hash_simple(-1, 101);

    result = d_assert_standalone(
        h_zero < 101 && h_neg < 101,
        "simple_edge_ids",
        "Edge IDs should produce valid hashes",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_collision
  Tests hash collision behavior with varying table sizes.
  Tests the following:
  - Small table produces collisions
  - Large table minimizes collisions
*/
bool
d_tests_sa_event_hash_collision
(
    struct d_test_counter* _counter
)
{
    bool       result;
    size_t     collision_count;
    d_event_id ids[100];
    size_t     hashes[100];
    int        i;
    int        j;

    result = true;

    // generate IDs and hash into small table
    for (i = 0; i < 100; i++)
    {
        ids[i]    = i * 7 + 1000;
        hashes[i] = d_event_hash_function(ids[i], 10);
    }

    collision_count = 0;

    for (i = 0; i < 100; i++)
    {
        for (j = i + 1; j < 100; j++)
        {
            if ( (hashes[i] == hashes[j]) &&
                 (ids[i] != ids[j]) )
            {
                collision_count++;
                break;
            }
        }
    }

    result = d_assert_standalone(
        collision_count > 0,
        "collision_small_table",
        "Small table should produce collisions",
        _counter) && result;

    // re-hash into large table
    collision_count = 0;

    for (i = 0; i < 100; i++)
    {
        hashes[i] = d_event_hash_function(ids[i], 10000);
    }

    for (i = 0; i < 100; i++)
    {
        for (j = i + 1; j < 100; j++)
        {
            if (hashes[i] == hashes[j])
            {
                collision_count++;
            }
        }
    }

    result = d_assert_standalone(
        collision_count < 5,
        "collision_large_table",
        "Large table should minimize collisions",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_distribution
  Tests hash distribution quality across buckets.
  Tests the following:
  - Few empty buckets with many inputs (good spread)
  - No severely overloaded bucket (no clustering)
*/
bool
d_tests_sa_event_hash_distribution
(
    struct d_test_counter* _counter
)
{
    bool       result;
    size_t     bucket_counts[100];
    size_t     empty_buckets;
    size_t     max_count;
    size_t     expected_avg;
    d_event_id i;
    size_t     b;

    result = true;

    // zero the counts
    for (b = 0; b < 100; b++)
    {
        bucket_counts[b] = 0;
    }

    // hash 1000 sequential IDs into 100 buckets
    for (i = 0; i < 1000; i++)
    {
        bucket_counts[d_event_hash_function(i, 100)]++;
    }

    // count empty buckets
    empty_buckets = 0;

    for (b = 0; b < 100; b++)
    {
        if (bucket_counts[b] == 0)
        {
            empty_buckets++;
        }
    }

    result = d_assert_standalone(
        empty_buckets < 10,
        "distribution_empty_buckets",
        "Most buckets should have at least one value",
        _counter) && result;

    // find max bucket count
    max_count    = 0;
    expected_avg = 1000 / 100;

    for (b = 0; b < 100; b++)
    {
        if (bucket_counts[b] > max_count)
        {
            max_count = bucket_counts[b];
        }
    }

    result = d_assert_standalone(
        max_count < expected_avg * 3,
        "distribution_no_clustering",
        "No bucket should have excessive elements",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_all
  Runs all hash function tests.
*/
bool
d_tests_sa_event_hash_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Hash Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_event_hash_fnv_bounds(_counter)          && result;
    result = d_tests_sa_event_hash_fnv_consistency(_counter)     && result;
    result = d_tests_sa_event_hash_fnv_differentiation(_counter) && result;
    result = d_tests_sa_event_hash_fnv_edge_ids(_counter)        && result;
    result = d_tests_sa_event_hash_fnv_table_sizes(_counter)     && result;
    result = d_tests_sa_event_hash_simple_bounds(_counter)       && result;
    result = d_tests_sa_event_hash_simple_consistency(_counter)  && result;
    result = d_tests_sa_event_hash_simple_edge_ids(_counter)     && result;
    result = d_tests_sa_event_hash_collision(_counter)           && result;
    result = d_tests_sa_event_hash_distribution(_counter)        && result;

    return result;
}
