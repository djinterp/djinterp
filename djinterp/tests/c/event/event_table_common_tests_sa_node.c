#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * IV. NODE MANAGEMENT TESTS
 *****************************************************************************/

// d_tests_sa_etc_node_dummy_cb
//   helper: minimal callback for listener construction in node tests.
//   NOTE: the original tests used d_event_listener_new_default(id, NULL)
// which always returns NULL because NULL callback is rejected.  This
// dummy callback fixes that bug.
static void
d_tests_sa_etc_node_dummy_cb(void* _args)
{
    (void)_args;

    return;
}


/*
d_tests_sa_event_hash_node_new_valid
  Tests d_event_hash_node_new with valid parameters.
  Tests the following:
  - Returns non-NULL node
  - key is set correctly
  - value stores the listener pointer
  - next is initialized to NULL
*/
bool
d_tests_sa_event_hash_node_new_valid
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_listener*  lis;
    struct d_event_hash_node* node;

    result = true;
    lis    = d_event_listener_new_default(
                 1001,
                 (fn_callback)d_tests_sa_etc_node_dummy_cb);

    if (lis)
    {
        node = d_event_hash_node_new(1001, lis);

        result = d_assert_standalone(
            node != NULL,
            "node_new_non_null",
            "d_event_hash_node_new should return non-NULL",
            _counter) && result;

        if (node)
        {
            result = d_assert_standalone(
                node->key == 1001 &&
                node->value == lis &&
                node->next == NULL,
                "node_new_fields",
                "Node should have key=1001, correct value, next=NULL",
                _counter) && result;

            d_event_hash_node_free(node);
        }

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_hash_node_new_null_value
  Tests d_event_hash_node_new with NULL listener.
  Tests the following:
  - Node is created with NULL value
*/
bool
d_tests_sa_event_hash_node_new_null_value
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_node* node;

    result = true;
    node   = d_event_hash_node_new(2002, NULL);

    result = d_assert_standalone(
        node != NULL && node->value == NULL,
        "node_new_null_value",
        "Node should be created with NULL value",
        _counter) && result;

    if (node)
    {
        d_event_hash_node_free(node);
    }

    return result;
}

/*
d_tests_sa_event_hash_node_new_ids
  Tests d_event_hash_node_new with various key values.
  Tests the following:
  - Zero key is accepted
  - Negative key is accepted
  - Max key value is accepted
*/
bool
d_tests_sa_event_hash_node_new_ids
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_node* node;

    result = true;

    // zero key
    node = d_event_hash_node_new(0, NULL);

    result = d_assert_standalone(
        node != NULL && node->key == 0,
        "node_new_zero_key",
        "Node should accept zero key",
        _counter) && result;

    if (node)
    {
        d_event_hash_node_free(node);
    }

    // negative key
    node = d_event_hash_node_new(-5, NULL);

    result = d_assert_standalone(
        node != NULL && node->key == -5,
        "node_new_neg_key",
        "Node should accept negative key",
        _counter) && result;

    if (node)
    {
        d_event_hash_node_free(node);
    }

    // max key
    node = d_event_hash_node_new((d_event_id)(-1), NULL);

    result = d_assert_standalone(
        node != NULL && node->key == (d_event_id)(-1),
        "node_new_max_key",
        "Node should accept max key value",
        _counter) && result;

    if (node)
    {
        d_event_hash_node_free(node);
    }

    return result;
}

/*
d_tests_sa_event_hash_node_free_valid
  Tests d_event_hash_node_free with a valid node.
  Tests the following:
  - Freeing a valid node does not crash
*/
bool
d_tests_sa_event_hash_node_free_valid
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_node* node;

    result = true;
    node   = d_event_hash_node_new(100, NULL);

    if (node)
    {
        d_event_hash_node_free(node);
    }

    result = d_assert_standalone(
        true,
        "node_free_valid",
        "Freeing valid node should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_node_free_null
  Tests d_event_hash_node_free with NULL.
  Tests the following:
  - d_event_hash_node_free(NULL) does not crash
*/
bool
d_tests_sa_event_hash_node_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_hash_node_free(NULL);

    result = d_assert_standalone(
        true,
        "node_free_null",
        "d_event_hash_node_free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_node_free_preserves_listener
  Tests that d_event_hash_node_free does not free the listener.
  Tests the following:
  - Listener remains valid after node is freed
*/
bool
d_tests_sa_event_hash_node_free_preserves_listener
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_listener*  lis;
    struct d_event_hash_node* node;

    result = true;
    lis    = d_event_listener_new_default(
                 1001,
                 (fn_callback)d_tests_sa_etc_node_dummy_cb);

    if (lis)
    {
        node = d_event_hash_node_new(1001, lis);

        if (node)
        {
            d_event_hash_node_free(node);
        }

        // listener should still be valid
        result = d_assert_standalone(
            lis->id == 1001,
            "node_free_preserves_listener",
            "Freeing node should not free the listener",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_hash_node_chaining
  Tests manual chaining of hash nodes.
  Tests the following:
  - Nodes can be linked via next pointers
  - Chain is traversable
  - Chain has correct length
  - Last node has NULL next
*/
bool
d_tests_sa_event_hash_node_chaining
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_listener*  l1;
    struct d_event_listener*  l2;
    struct d_event_listener*  l3;
    struct d_event_hash_node* n1;
    struct d_event_hash_node* n2;
    struct d_event_hash_node* n3;

    result = true;
    l1     = d_event_listener_new_default(
                 1001, (fn_callback)d_tests_sa_etc_node_dummy_cb);
    l2     = d_event_listener_new_default(
                 1002, (fn_callback)d_tests_sa_etc_node_dummy_cb);
    l3     = d_event_listener_new_default(
                 1003, (fn_callback)d_tests_sa_etc_node_dummy_cb);

    if ( (l1) && (l2) && (l3) )
    {
        n1 = d_event_hash_node_new(1001, l1);
        n2 = d_event_hash_node_new(1002, l2);
        n3 = d_event_hash_node_new(1003, l3);

        if ( (n1) && (n2) && (n3) )
        {
            // chain nodes
            n1->next = n2;
            n2->next = n3;

            result = d_assert_standalone(
                n1->next == n2 && n1->next->next == n3,
                "node_chain_links",
                "Chain should be traversable",
                _counter) && result;

            result = d_assert_standalone(
                n3->next == NULL,
                "node_chain_terminates",
                "Last node should have NULL next",
                _counter) && result;

            // count chain length
            {
                struct d_event_hash_node* cur;
                int                       count;

                count = 0;
                cur   = n1;

                while (cur)
                {
                    count++;
                    cur = cur->next;
                }

                result = d_assert_standalone(
                    count == 3,
                    "node_chain_length",
                    "Chain should contain 3 nodes",
                    _counter) && result;
            }

            d_event_hash_node_free(n1);
            d_event_hash_node_free(n2);
            d_event_hash_node_free(n3);
        }
        else
        {
            if (n1)
            {
                d_event_hash_node_free(n1);
            }

            if (n2)
            {
                d_event_hash_node_free(n2);
            }

            if (n3)
            {
                d_event_hash_node_free(n3);
            }
        }
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l2)
    {
        d_event_listener_free(l2);
    }

    if (l3)
    {
        d_event_listener_free(l3);
    }

    return result;
}

/*
d_tests_sa_event_hash_node_all
  Runs all node management tests.
*/
bool
d_tests_sa_event_hash_node_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Node Management\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_event_hash_node_new_valid(_counter)              && result;
    result = d_tests_sa_event_hash_node_new_null_value(_counter)         && result;
    result = d_tests_sa_event_hash_node_new_ids(_counter)                && result;
    result = d_tests_sa_event_hash_node_free_valid(_counter)             && result;
    result = d_tests_sa_event_hash_node_free_null(_counter)              && result;
    result = d_tests_sa_event_hash_node_free_preserves_listener(_counter) && result;
    result = d_tests_sa_event_hash_node_chaining(_counter)               && result;

    return result;
}
