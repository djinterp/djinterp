#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * I. CONSTANT AND TYPE TESTS
 *****************************************************************************/

/*
d_tests_sa_event_hash_constants
  Tests the hash table constant definitions.
  Tests the following:
  - D_EVENT_HASH_TABLE_DEFAULT_SIZE equals 101
  - D_EVENT_HASH_TABLE_DEFAULT_SIZE is positive
  - D_EVENT_HASH_TABLE_LOAD_FACTOR equals 0.75
  - D_EVENT_HASH_TABLE_LOAD_FACTOR is between 0 and 1
  - D_EVENT_HASH_TABLE_RESIZE_FACTOR equals 2
  - D_EVENT_HASH_TABLE_RESIZE_FACTOR is greater than 1
*/
bool
d_tests_sa_event_hash_constants
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_DEFAULT_SIZE == 101,
        "default_size_value",
        "D_EVENT_HASH_TABLE_DEFAULT_SIZE should equal 101",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_DEFAULT_SIZE > 0,
        "default_size_positive",
        "D_EVENT_HASH_TABLE_DEFAULT_SIZE should be positive",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_LOAD_FACTOR == 0.75,
        "load_factor_value",
        "D_EVENT_HASH_TABLE_LOAD_FACTOR should equal 0.75",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_LOAD_FACTOR > 0.0 &&
        D_EVENT_HASH_TABLE_LOAD_FACTOR < 1.0,
        "load_factor_range",
        "Load factor should be between 0 and 1 exclusive",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_RESIZE_FACTOR == 2,
        "resize_factor_value",
        "D_EVENT_HASH_TABLE_RESIZE_FACTOR should equal 2",
        _counter) && result;

    result = d_assert_standalone(
        D_EVENT_HASH_TABLE_RESIZE_FACTOR > 1,
        "resize_factor_above_one",
        "Resize factor should be greater than 1",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_node_struct
  Tests the d_event_hash_node structure layout.
  Tests the following:
  - key, value, next members are accessible
  - sizeof(struct d_event_hash_node) is non-zero
*/
bool
d_tests_sa_event_hash_node_struct
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_node  node;

    result = true;

    node.key   = 42;
    node.value = NULL;
    node.next  = NULL;

    result = d_assert_standalone(
        node.key == 42 && node.value == NULL && node.next == NULL,
        "hash_node_struct_members",
        "All d_event_hash_node members should be accessible",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(struct d_event_hash_node) > 0,
        "hash_node_struct_sizeof",
        "sizeof(struct d_event_hash_node) should be non-zero",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_stats_struct
  Tests the d_event_hash_stats structure layout.
  Tests the following:
  - Zero-initialized struct has all zero fields
  - All fields are assignable and readable
  - sizeof(struct d_event_hash_stats) is non-zero
*/
bool
d_tests_sa_event_hash_stats_struct
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_stats  stats;

    result = true;

    // zero-initialization via memset-style
    stats.total_buckets        = 0;
    stats.used_buckets         = 0;
    stats.total_elements       = 0;
    stats.max_chain_length     = 0;
    stats.average_chain_length = 0.0;
    stats.load_factor          = 0.0;

    result = d_assert_standalone(
        stats.total_buckets == 0 &&
        stats.used_buckets == 0 &&
        stats.total_elements == 0 &&
        stats.max_chain_length == 0,
        "hash_stats_zero_init",
        "Zero-initialized stats should have zero fields",
        _counter) && result;

    // populate with sample values
    stats.total_buckets        = 100;
    stats.used_buckets         = 75;
    stats.total_elements       = 150;
    stats.max_chain_length     = 3;
    stats.average_chain_length = 2.0;
    stats.load_factor          = 1.5;

    result = d_assert_standalone(
        stats.total_buckets == 100 &&
        stats.used_buckets == 75 &&
        stats.total_elements == 150 &&
        stats.max_chain_length == 3,
        "hash_stats_population",
        "Stats struct should store all values correctly",
        _counter) && result;

    result = d_assert_standalone(
        stats.load_factor == 1.5 &&
        stats.average_chain_length == 2.0,
        "hash_stats_doubles",
        "Stats struct should store double values correctly",
        _counter) && result;

    result = d_assert_standalone(
        sizeof(struct d_event_hash_stats) > 0,
        "hash_stats_sizeof",
        "sizeof(struct d_event_hash_stats) should be non-zero",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_constant_all
  Runs all constant and type tests.
*/
bool
d_tests_sa_event_hash_constant_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constants and Types\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_event_hash_constants(_counter)   && result;
    result = d_tests_sa_event_hash_node_struct(_counter)  && result;
    result = d_tests_sa_event_hash_stats_struct(_counter) && result;

    return result;
}
