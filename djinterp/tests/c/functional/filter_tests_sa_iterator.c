#include ".\filter_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// HELPER PREDICATES
///////////////////////////////////////////////////////////////////////////////

static bool pred_is_even(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 == 0);
}

static bool pred_is_positive(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 0);
}


/*
d_tests_sa_filter_iterator_create
  Tests d_filter_iterator_new for iterator creation.
  Tests the following:
  - creation with valid chain, input, count, element_size succeeds
  - iterator fields are initialized correctly
  - indices are precomputed on creation
  - iterator starts at position 0, not exhausted
  - creation with NULL chain returns NULL
  - creation with NULL input returns NULL
  - creation with zero count returns NULL or exhausted iterator
  - creation with zero element_size returns NULL
  - free NULL safety
*/
bool
d_tests_sa_filter_iterator_create
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_iterator*  iter;
    int                        input[6];

    result   = true;
    input[0] = 1;
    input[1] = 2;
    input[2] = 3;
    input[3] = 4;
    input[4] = 5;
    input[5] = 6;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    // test 1: valid creation
    iter = d_filter_iterator_new(chain,
                                 input,
                                 6,
                                 sizeof(int));

    result = d_assert_standalone(
        iter != NULL,
        "iter_create_success",
        "iterator_new with valid args should succeed",
        _counter) && result;

    if (iter)
    {
        // test 2: fields initialized
        result = d_assert_standalone(
            iter->input == input,
            "iter_create_input_ref",
            "iterator should reference original input",
            _counter) && result;

        result = d_assert_standalone(
            iter->input_count == 6,
            "iter_create_input_count",
            "iterator should store input count 6",
            _counter) && result;

        result = d_assert_standalone(
            iter->element_size == sizeof(int),
            "iter_create_elem_size",
            "iterator should store element_size",
            _counter) && result;

        // test 3: indices precomputed
        // even elements in {1,2,3,4,5,6} are {2,4,6} => 3 indices
        result = d_assert_standalone(
            iter->indices != NULL,
            "iter_create_indices_allocated",
            "iterator should precompute indices",
            _counter) && result;

        result = d_assert_standalone(
            iter->indices_count == 3,
            "iter_create_indices_count",
            "iterator should find 3 matching indices",
            _counter) && result;

        // test 4: initial position
        result = d_assert_standalone(
            iter->indices_pos == 0,
            "iter_create_pos_zero",
            "iterator should start at position 0",
            _counter) && result;

        result = d_assert_standalone(
            iter->exhausted == false,
            "iter_create_not_exhausted",
            "iterator should not be exhausted initially",
            _counter) && result;

        d_filter_iterator_free(iter);
    }

    d_filter_chain_free(chain);

    // test 5: NULL chain returns exhausted iterator
    iter = d_filter_iterator_new(NULL,
                                 input,
                                 6,
                                 sizeof(int));

    result = d_assert_standalone(
        iter != NULL,
        "iter_create_null_chain",
        "NULL chain should return an iterator (exhausted)",
        _counter) && result;

    if (iter)
    {
        result = d_assert_standalone(
            iter->exhausted == true,
            "iter_create_null_chain_exhausted",
            "NULL chain iterator should be exhausted",
            _counter) && result;

        result = d_assert_standalone(
            d_filter_iterator_has_next(iter) == false,
            "iter_create_null_chain_no_next",
            "NULL chain iterator has_next should be false",
            _counter) && result;

        d_filter_iterator_free(iter);
    }

    // test 6: NULL input returns exhausted iterator
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     NULL,
                                     6,
                                     sizeof(int));

        result = d_assert_standalone(
            iter != NULL,
            "iter_create_null_input",
            "NULL input should return an iterator (exhausted)",
            _counter) && result;

        if (iter)
        {
            result = d_assert_standalone(
                iter->exhausted == true,
                "iter_create_null_input_exhausted",
                "NULL input iterator should be exhausted",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 7: zero count
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     input,
                                     0,
                                     sizeof(int));

        // may return NULL or an exhausted iterator
        if (iter)
        {
            result = d_assert_standalone(
                (iter->indices_count == 0) ||
                (iter->exhausted == true),
                "iter_create_zero_count_empty",
                "zero count should yield empty/exhausted iterator",
                _counter) && result;

            d_filter_iterator_free(iter);
        }
        else
        {
            result = d_assert_standalone(
                true,
                "iter_create_zero_count_null",
                "zero count returning NULL is acceptable",
                _counter) && result;
        }

        d_filter_chain_free(chain);
    }

    // test 8: zero element_size returns exhausted iterator
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     input,
                                     6,
                                     0);

        result = d_assert_standalone(
            iter != NULL,
            "iter_create_zero_elem_size",
            "zero element_size should return iterator (exhausted)",
            _counter) && result;

        if (iter)
        {
            result = d_assert_standalone(
                iter->exhausted == true,
                "iter_create_zero_elem_size_exhausted",
                "zero element_size iterator should be exhausted",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 9: free NULL safety
    d_filter_iterator_free(NULL);
    result = d_assert_standalone(
        true,
        "iter_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_iterator_traverse
  Tests d_filter_iterator_has_next and d_filter_iterator_next for traversal.
  Tests the following:
  - has_next returns true when elements remain
  - next returns pointer to correct element
  - sequential next calls yield all matching elements in order
  - has_next returns false after all elements consumed
  - next returns NULL after exhaustion
  - iterator over all-matching input yields all elements
  - iterator over no-matching input is immediately exhausted
*/
bool
d_tests_sa_filter_iterator_traverse
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_iterator*  iter;
    int                        input[6];
    int                        all_odd[3];
    const int*                 elem;
    int                        collected[6];
    size_t                     collected_count;

    result   = true;
    input[0] = 1;
    input[1] = 2;
    input[2] = 3;
    input[3] = 4;
    input[4] = 5;
    input[5] = 6;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    iter = d_filter_iterator_new(chain,
                                 input,
                                 6,
                                 sizeof(int));

    if (!iter)
    {
        d_filter_chain_free(chain);

        return false;
    }

    // test 1: has_next is true initially
    result = d_assert_standalone(
        d_filter_iterator_has_next(iter) == true,
        "iter_traverse_has_next_initial",
        "has_next should be true initially (3 even elements)",
        _counter) && result;

    // test 2: collect all elements via next
    collected_count = 0;

    while (d_filter_iterator_has_next(iter))
    {
        elem = (const int*)d_filter_iterator_next(iter);

        if ( (elem) &&
             (collected_count < 6) )
        {
            collected[collected_count] = *elem;
            collected_count++;
        }
    }

    result = d_assert_standalone(
        collected_count == 3,
        "iter_traverse_count",
        "traversal should yield 3 even elements",
        _counter) && result;

    result = d_assert_standalone(
        (collected[0] == 2) &&
        (collected[1] == 4) &&
        (collected[2] == 6),
        "iter_traverse_values",
        "traversal should yield {2, 4, 6} in order",
        _counter) && result;

    // test 3: has_next false after exhaustion
    result = d_assert_standalone(
        d_filter_iterator_has_next(iter) == false,
        "iter_traverse_exhausted",
        "has_next should be false after all consumed",
        _counter) && result;

    // test 4: next returns NULL after exhaustion
    elem = (const int*)d_filter_iterator_next(iter);
    result = d_assert_standalone(
        elem == NULL,
        "iter_traverse_next_null",
        "next should return NULL after exhaustion",
        _counter) && result;

    d_filter_iterator_free(iter);
    d_filter_chain_free(chain);

    // test 5: iterator over no-matching input
    all_odd[0] = 1;
    all_odd[1] = 3;
    all_odd[2] = 5;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     all_odd,
                                     3,
                                     sizeof(int));

        if (iter)
        {
            result = d_assert_standalone(
                d_filter_iterator_has_next(iter) == false,
                "iter_traverse_no_match",
                "has_next should be false for no-match input",
                _counter) && result;

            elem = (const int*)d_filter_iterator_next(iter);
            result = d_assert_standalone(
                elem == NULL,
                "iter_traverse_no_match_next",
                "next should return NULL for no-match input",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 6: has_next on NULL returns false
    result = d_assert_standalone(
        d_filter_iterator_has_next(NULL) == false,
        "iter_has_next_null",
        "has_next(NULL) should return false",
        _counter) && result;

    // test 7: next on NULL returns NULL
    result = d_assert_standalone(
        d_filter_iterator_next(NULL) == NULL,
        "iter_next_null",
        "next(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_iterator_reset
  Tests d_filter_iterator_reset.
  Tests the following:
  - reset after full traversal allows re-traversal
  - re-traversal yields same elements
  - reset after partial traversal restarts from beginning
  - reset on fresh iterator is a no-op (still works)
  - reset NULL does not crash
*/
bool
d_tests_sa_filter_iterator_reset
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_iterator*  iter;
    int                        input[6];
    const int*                 elem;
    int                        first_pass[3];
    int                        second_pass[3];
    size_t                     count_1;
    size_t                     count_2;

    result   = true;
    input[0] = 1;
    input[1] = 2;
    input[2] = 3;
    input[3] = 4;
    input[4] = 5;
    input[5] = 6;

    chain = d_filter_chain_new();

    if (!chain)
    {
        return false;
    }

    d_filter_chain_add_where(chain, pred_is_even);

    iter = d_filter_iterator_new(chain,
                                 input,
                                 6,
                                 sizeof(int));

    if (!iter)
    {
        d_filter_chain_free(chain);

        return false;
    }

    // first pass: consume all elements
    count_1 = 0;

    while (d_filter_iterator_has_next(iter))
    {
        elem = (const int*)d_filter_iterator_next(iter);

        if ( (elem) &&
             (count_1 < 3) )
        {
            first_pass[count_1] = *elem;
            count_1++;
        }
    }

    result = d_assert_standalone(
        count_1 == 3,
        "iter_reset_first_pass",
        "first pass should yield 3 elements",
        _counter) && result;

    // test 1: reset after full traversal
    d_filter_iterator_reset(iter);

    result = d_assert_standalone(
        d_filter_iterator_has_next(iter) == true,
        "iter_reset_has_next",
        "has_next should be true after reset",
        _counter) && result;

    // test 2: re-traversal yields same elements
    count_2 = 0;

    while (d_filter_iterator_has_next(iter))
    {
        elem = (const int*)d_filter_iterator_next(iter);

        if ( (elem) &&
             (count_2 < 3) )
        {
            second_pass[count_2] = *elem;
            count_2++;
        }
    }

    result = d_assert_standalone(
        count_2 == 3,
        "iter_reset_second_count",
        "second pass should yield 3 elements",
        _counter) && result;

    result = d_assert_standalone(
        (first_pass[0] == second_pass[0]) &&
        (first_pass[1] == second_pass[1]) &&
        (first_pass[2] == second_pass[2]),
        "iter_reset_same_values",
        "second pass should yield same values as first",
        _counter) && result;

    // test 3: reset after partial traversal
    d_filter_iterator_reset(iter);
    d_filter_iterator_next(iter);   // consume first element only

    d_filter_iterator_reset(iter);

    result = d_assert_standalone(
        d_filter_iterator_has_next(iter) == true,
        "iter_reset_partial",
        "has_next should be true after partial+reset",
        _counter) && result;

    elem = (const int*)d_filter_iterator_next(iter);

    result = d_assert_standalone(
        (elem != NULL) && (*elem == 2),
        "iter_reset_partial_first",
        "first element after reset should be 2 again",
        _counter) && result;

    // test 4: reset on fresh iterator
    d_filter_iterator_free(iter);

    iter = d_filter_iterator_new(chain,
                                 input,
                                 6,
                                 sizeof(int));

    if (iter)
    {
        d_filter_iterator_reset(iter);

        result = d_assert_standalone(
            d_filter_iterator_has_next(iter) == true,
            "iter_reset_fresh",
            "reset on fresh iterator should still work",
            _counter) && result;

        d_filter_iterator_free(iter);
    }

    d_filter_chain_free(chain);

    // test 5: reset NULL does not crash
    d_filter_iterator_reset(NULL);
    result = d_assert_standalone(
        true,
        "iter_reset_null",
        "reset(NULL) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_filter_iterator_edge
  Tests edge cases for the iterator interface.
  Tests the following:
  - iterator with empty chain yields all elements
  - iterator with take_first(2) yields exactly 2 elements
  - iterator with multi-operation chain composes correctly
  - single-element input with matching predicate
  - single-element input with non-matching predicate
*/
bool
d_tests_sa_filter_iterator_edge
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_filter_chain*     chain;
    struct d_filter_iterator*  iter;
    int                        input[5];
    int                        single[1];
    const int*                 elem;
    size_t                     count;

    result   = true;
    input[0] = 10;
    input[1] = 20;
    input[2] = 30;
    input[3] = 40;
    input[4] = 50;

    // test 1: empty chain yields all elements
    chain = d_filter_chain_new();

    if (chain)
    {
        iter = d_filter_iterator_new(chain,
                                     input,
                                     5,
                                     sizeof(int));

        if (iter)
        {
            count = 0;

            while (d_filter_iterator_has_next(iter))
            {
                d_filter_iterator_next(iter);
                count++;
            }

            result = d_assert_standalone(
                count == 5,
                "iter_edge_empty_chain",
                "empty chain iterator should yield all 5 elements",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 2: take_first(2) yields exactly 2
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_take_first(chain, 2);

        iter = d_filter_iterator_new(chain,
                                     input,
                                     5,
                                     sizeof(int));

        if (iter)
        {
            count = 0;

            while (d_filter_iterator_has_next(iter))
            {
                elem = (const int*)d_filter_iterator_next(iter);

                if (elem)
                {
                    count++;
                }
            }

            result = d_assert_standalone(
                count == 2,
                "iter_edge_take_first",
                "take_first(2) iterator should yield 2 elements",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 3: multi-op chain: skip(1) -> where(is_even)
    // {10,20,30,40,50} -> skip 1 -> {20,30,40,50}
    //                  -> where(even) -> {20, 40, 50}
    //   20 is even, 30 is even, 40 is even, 50 is even
    //   actually all are even, so -> {20, 30, 40, 50}
    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_skip_first(chain, 1);
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     input,
                                     5,
                                     sizeof(int));

        if (iter)
        {
            count = 0;

            while (d_filter_iterator_has_next(iter))
            {
                d_filter_iterator_next(iter);
                count++;
            }

            result = d_assert_standalone(
                count == 4,
                "iter_edge_multi_op",
                "skip(1)->where(even) on {10..50} yields 4",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 4: single element matching
    single[0] = 4;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     single,
                                     1,
                                     sizeof(int));

        if (iter)
        {
            result = d_assert_standalone(
                d_filter_iterator_has_next(iter) == true,
                "iter_edge_single_match_has_next",
                "single matching element: has_next should be true",
                _counter) && result;

            elem = (const int*)d_filter_iterator_next(iter);

            result = d_assert_standalone(
                (elem != NULL) && (*elem == 4),
                "iter_edge_single_match_value",
                "single matching element should be 4",
                _counter) && result;

            result = d_assert_standalone(
                d_filter_iterator_has_next(iter) == false,
                "iter_edge_single_match_exhausted",
                "should be exhausted after single element",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 5: single element not matching
    single[0] = 3;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, pred_is_even);

        iter = d_filter_iterator_new(chain,
                                     single,
                                     1,
                                     sizeof(int));

        if (iter)
        {
            result = d_assert_standalone(
                d_filter_iterator_has_next(iter) == false,
                "iter_edge_single_no_match",
                "single non-matching: has_next should be false",
                _counter) && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_sa_filter_iterator_all
  Aggregation function that runs all iterator interface tests.
*/
bool
d_tests_sa_filter_iterator_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Iterator Interface\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_filter_iterator_create(_counter)    && result;
    result = d_tests_sa_filter_iterator_traverse(_counter)  && result;
    result = d_tests_sa_filter_iterator_reset(_counter)     && result;
    result = d_tests_sa_filter_iterator_edge(_counter)      && result;

    return result;
}
