#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * VI. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/

// d_tests_sa_etc_adv_dummy_cb
//   helper: minimal callback for listener construction in advanced tests.
static void
d_tests_sa_etc_adv_dummy_cb(void* _args)
{
    (void)_args;

    return;
}


/*
d_tests_sa_event_hash_integration
  Tests a manual hash table build-and-lookup workflow.
  Tests the following:
  - All inserted nodes are reachable via bucket traversal
  - Lookup by hash + key finds inserted node
*/
bool
d_tests_sa_event_hash_integration
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    const size_t              table_size = 10;
    struct d_event_hash_node* buckets[10];
    struct d_event_listener*  listeners[20];
    int                       created_count;
    int                       inserted_count;
    int                       total_nodes;
    size_t                    i;
    int                       k;

    result         = true;
    created_count  = 0;
    inserted_count = 0;

    for (i = 0; i < table_size; i++)
    {
        buckets[i] = NULL;
    }

    // create listeners with real callback
    for (k = 0; k < 20; k++)
    {
        listeners[k] = d_event_listener_new_default(
                            1000 + k,
                            (fn_callback)d_tests_sa_etc_adv_dummy_cb);

        if (listeners[k])
        {
            created_count++;
        }
    }

    // hash and insert into buckets
    for (k = 0; k < 20; k++)
    {
        if (listeners[k])
        {
            size_t                    hash;
            struct d_event_hash_node* new_node;

            hash     = d_event_hash_function(listeners[k]->id, table_size);
            new_node = d_event_hash_node_new(listeners[k]->id, listeners[k]);

            if (new_node)
            {
                new_node->next = buckets[hash];
                buckets[hash]  = new_node;
                inserted_count++;
            }
        }
    }

    // verify all nodes are reachable
    total_nodes = 0;

    for (i = 0; i < table_size; i++)
    {
        struct d_event_hash_node* cur;

        cur = buckets[i];

        while (cur)
        {
            total_nodes++;
            cur = cur->next;
        }
    }

    result = d_assert_standalone(
        total_nodes == inserted_count,
        "integration_all_reachable",
        "All inserted nodes should be reachable via traversal",
        _counter) && result;

    // verify lookup for id 1010
    {
        d_event_id                search_id;
        size_t                    search_hash;
        struct d_event_hash_node* cur;
        bool                      found;
        bool                      should_exist;

        search_id    = 1010;
        should_exist = (listeners[10] != NULL);
        search_hash  = d_event_hash_function(search_id, table_size);
        cur          = buckets[search_hash];
        found        = false;

        while (cur)
        {
            if (cur->key == search_id)
            {
                found = true;
                break;
            }

            cur = cur->next;
        }

        result = d_assert_standalone(
            found == should_exist,
            "integration_lookup",
            "Lookup via hash should find inserted node",
            _counter) && result;
    }

    // cleanup nodes
    for (i = 0; i < table_size; i++)
    {
        struct d_event_hash_node* cur;

        cur = buckets[i];

        while (cur)
        {
            struct d_event_hash_node* next;

            next = cur->next;
            d_event_hash_node_free(cur);
            cur = next;
        }
    }

    // cleanup listeners
    for (k = 0; k < 20; k++)
    {
        if (listeners[k])
        {
            d_event_listener_free(listeners[k]);
        }
    }

    return result;
}

/*
d_tests_sa_event_hash_stress_prime
  Stress-tests prime generation with many consecutive values.
  Tests the following:
  - 100 consecutive primes scale correctly (last > first)
*/
bool
d_tests_sa_event_hash_stress_prime
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t primes[100];
    size_t current;
    int    i;

    result  = true;
    current = 2;

    for (i = 0; i < 100; i++)
    {
        primes[i] = d_event_hash_next_prime(current);
        current   = primes[i] + 1;
    }

    result = d_assert_standalone(
        primes[99] > primes[0],
        "stress_prime_scaling",
        "100th prime should be greater than first",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_stress_hash
  Stress-tests hash function with many IDs.
  Tests the following:
  - 10000 IDs all hash within bounds
*/
bool
d_tests_sa_event_hash_stress_hash
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool all_in_bounds;
    int  i;

    result       = true;
    all_in_bounds = true;

    for (i = 0; i < 10000; i++)
    {
        if (d_event_hash_function(i, 1000) >= 1000)
        {
            all_in_bounds = false;
            break;
        }
    }

    result = d_assert_standalone(
        all_in_bounds,
        "stress_hash_bounds",
        "All 10000 IDs should hash within bounds",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_edge_cases
  Tests edge cases across multiple functions.
  Tests the following:
  - Table size 1 always produces hash 0
  - Large prime is actually prime and >= 10000
  - Node with max event ID is created correctly
*/
bool
d_tests_sa_event_hash_edge_cases
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    size_t                    hash;
    size_t                    large_prime;
    bool                      is_prime;
    size_t                    i;
    d_event_id                max_id;
    struct d_event_hash_node* node;

    result = true;

    // table size 1
    hash = d_event_hash_function(12345, 1);

    result = d_assert_standalone(
        hash == 0,
        "edge_table_size_1",
        "Table size 1 should always produce hash 0",
        _counter) && result;

    // large prime
    large_prime = d_event_hash_next_prime(10000);
    is_prime    = true;

    for (i = 2; i * i <= large_prime; i++)
    {
        if (large_prime % i == 0)
        {
            is_prime = false;
            break;
        }
    }

    result = d_assert_standalone(
        is_prime && large_prime >= 10000,
        "edge_large_prime",
        "Prime after 10000 should be valid and >= 10000",
        _counter) && result;

    // node with max ID
    max_id = (d_event_id)(-1);
    node   = d_event_hash_node_new(max_id, NULL);

    result = d_assert_standalone(
        node != NULL && node->key == max_id,
        "edge_max_id_node",
        "Node should accept max event ID",
        _counter) && result;

    if (node)
    {
        d_event_hash_node_free(node);
    }

    return result;
}

/*
d_tests_sa_event_hash_advanced_all
  Runs all integration, stress, and edge case tests.
*/
bool
d_tests_sa_event_hash_advanced_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Integration, Stress, and Edge Cases\n");
    printf("  ------------------------------------------------\n");

    result = d_tests_sa_event_hash_integration(_counter)  && result;
    result = d_tests_sa_event_hash_stress_prime(_counter)  && result;
    result = d_tests_sa_event_hash_stress_hash(_counter)   && result;
    result = d_tests_sa_event_hash_edge_cases(_counter)    && result;

    return result;
}
