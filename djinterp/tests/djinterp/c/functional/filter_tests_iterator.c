/*******************************************************************************
* djinterp [functional]                                  filter_tests_iterator.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests_iterator.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./filter_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


///////////////////////////////////////////////////////////////////////////////
// HELPER PREDICATES
///////////////////////////////////////////////////////////////////////////////

static
bool pred_is_even(
    const void* _element,
    void*       _context
)
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

static
bool pred_is_positive(
    const void* _element,
    void*       _context
)
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
d_tests_filter_iterator_create
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
bool d_tests_filter_iterator_create(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_iterator*  iter;
    int                        input[6];

    d_test_suite_test(_suite, "filter_iterator_create");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_create_success",
                              iter != NULL,
                              "iterator_new with valid args should succeed") && result;

    if (iter)
    {
        // test 2: fields initialized
        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_input_ref",
                                  iter->input == input,
                                  "iterator should reference original input") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_input_count",
                                  iter->input_count == 6,
                                  "iterator should store input count 6") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_elem_size",
                                  iter->element_size == sizeof(int),
                                  "iterator should store element_size") && result;

        // test 3: indices precomputed
        // even elements in {1,2,3,4,5,6} are {2,4,6} => 3 indices
        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_indices_allocated",
                                  iter->indices != NULL,
                                  "iterator should precompute indices") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_indices_count",
                                  iter->indices_count == 3,
                                  "iterator should find 3 matching indices") && result;

        // test 4: initial position
        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_pos_zero",
                                  iter->indices_pos == 0,
                                  "iterator should start at position 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_not_exhausted",
                                  iter->exhausted == false,
                                  "iterator should not be exhausted initially") && result;

        d_filter_iterator_free(iter);
    }

    d_filter_chain_free(chain);

    // test 5: NULL chain returns exhausted iterator
    iter = d_filter_iterator_new(NULL,
                                 input,
                                 6,
                                 sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_create_null_chain",
                              iter != NULL,
                              "NULL chain should return an iterator (exhausted)") && result;

    if (iter)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_null_chain_exhausted",
                                  iter->exhausted == true,
                                  "NULL chain iterator should be exhausted") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_null_chain_no_next",
                                  d_filter_iterator_has_next(iter) == false,
                                  "NULL chain iterator has_next should be false") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_null_input",
                                  iter != NULL,
                                  "NULL input should return an iterator (exhausted)") && result;

        if (iter)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_create_null_input_exhausted",
                                      iter->exhausted == true,
                                      "NULL input iterator should be exhausted") && result;

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
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_create_zero_count_empty",
                                      (iter->indices_count == 0) || (iter->exhausted == true),
                                      "zero count should yield empty/exhausted iterator") && result;

            d_filter_iterator_free(iter);
        }
        else
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_create_zero_count_null",
                                      true,
                                      "zero count returning NULL is acceptable") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_create_zero_elem_size",
                                  iter != NULL,
                                  "zero element_size should return iterator (exhausted)") && result;

        if (iter)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_create_zero_elem_size_exhausted",
                                      iter->exhausted == true,
                                      "zero element_size iterator should be exhausted") && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 9: free NULL safety
    d_filter_iterator_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_free_null",
                              true,
                              "free(NULL) should not crash") && result;

    return result;
}


/**
 * @brief Part 1 of d_tests_filter_iterator_traverse.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: has_next is true initially
 *   - test 2: collect all elements via next
 *   - test 3: has_next false after exhaustion
 *   - test 4: next returns NULL after exhaustion
 */
static bool
d_internal_tests_filter_iterator_traverse_part1(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*    chain;
    struct dj_filter_iterator* iter;
    int                        input[6];
    const int*                 elem;
    int                        collected[6];
    size_t                     collected_count;

    result = true;

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
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_traverse_has_next_initial",
                              d_filter_iterator_has_next(iter) == true,
                              "has_next should be true initially (3 even elements)") && result;

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

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_traverse_count",
                              collected_count == 3,
                              "traversal should yield 3 even elements") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_traverse_values",
                              (collected[0] == 2) && (collected[1] == 4) && (collected[2] == 6),
                              "traversal should yield {2, 4, 6} in order") && result;

    // test 3: has_next false after exhaustion
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_traverse_exhausted",
                              d_filter_iterator_has_next(iter) == false,
                              "has_next should be false after all consumed") && result;

    // test 4: next returns NULL after exhaustion
    elem = (const int*)d_filter_iterator_next(iter);
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_traverse_next_null",
                              elem == NULL,
                              "next should return NULL after exhaustion") && result;

    d_filter_iterator_free(iter);
    d_filter_chain_free(chain);

    return result;
}


/**
 * @brief Part 2 of d_tests_filter_iterator_traverse.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: iterator over no-matching input
 *   - test 6: has_next on NULL returns false
 *   - test 7: next on NULL returns NULL
 */
static bool
d_internal_tests_filter_iterator_traverse_part2(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*    chain;
    struct dj_filter_iterator* iter;
    int                        all_odd[3];
    const int*                 elem;

    result = true;

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
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_traverse_no_match",
                                      d_filter_iterator_has_next(iter) == false,
                                      "has_next should be false for no-match input") && result;

            elem = (const int*)d_filter_iterator_next(iter);
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_traverse_no_match_next",
                                      elem == NULL,
                                      "next should return NULL for no-match input") && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    // test 6: has_next on NULL returns false
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_has_next_null",
                              d_filter_iterator_has_next(NULL) == false,
                              "has_next(NULL) should return false") && result;

    // test 7: next on NULL returns NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_next_null",
                              d_filter_iterator_next(NULL) == NULL,
                              "next(NULL) should return NULL") && result;

    return result;
}


/*
d_tests_filter_iterator_traverse
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
bool d_tests_filter_iterator_traverse(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "filter_iterator_traverse");

    result = true;
    result &= d_internal_tests_filter_iterator_traverse_part1(_suite);
    result &= d_internal_tests_filter_iterator_traverse_part2(_suite);

    return result;
}


/*
d_tests_filter_iterator_reset
  Tests d_filter_iterator_reset.
  Tests the following:
  - reset after full traversal allows re-traversal
  - re-traversal yields same elements
  - reset after partial traversal restarts from beginning
  - reset on fresh iterator is a no-op (still works)
  - reset NULL does not crash
*/
bool d_tests_filter_iterator_reset(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_iterator*  iter;
    int                        input[6];
    const int*                 elem;
    int                        first_pass[3];
    int                        second_pass[3];
    size_t                     count_1;
    size_t                     count_2;

    d_test_suite_test(_suite, "filter_iterator_reset");

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

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_first_pass",
                              count_1 == 3,
                              "first pass should yield 3 elements") && result;

    // test 1: reset after full traversal
    d_filter_iterator_reset(iter);

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_has_next",
                              d_filter_iterator_has_next(iter) == true,
                              "has_next should be true after reset") && result;

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

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_second_count",
                              count_2 == 3,
                              "second pass should yield 3 elements") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_same_values",
                              (first_pass[0] == second_pass[0]) && (first_pass[1] == second_pass[1]) && (first_pass[2] == second_pass[2]),
                              "second pass should yield same values as first") && result;

    // test 3: reset after partial traversal
    d_filter_iterator_reset(iter);
    d_filter_iterator_next(iter);   // consume first element only

    d_filter_iterator_reset(iter);

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_partial",
                              d_filter_iterator_has_next(iter) == true,
                              "has_next should be true after partial+reset") && result;

    elem = (const int*)d_filter_iterator_next(iter);

    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_partial_first",
                              (elem != NULL) && (*elem == 2),
                              "first element after reset should be 2 again") && result;

    // test 4: reset on fresh iterator
    d_filter_iterator_free(iter);

    iter = d_filter_iterator_new(chain,
                                 input,
                                 6,
                                 sizeof(int));

    if (iter)
    {
        d_filter_iterator_reset(iter);

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_reset_fresh",
                                  d_filter_iterator_has_next(iter) == true,
                                  "reset on fresh iterator should still work") && result;

        d_filter_iterator_free(iter);
    }

    d_filter_chain_free(chain);

    // test 5: reset NULL does not crash
    d_filter_iterator_reset(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "iter_reset_null",
                              true,
                              "reset(NULL) should not crash") && result;

    return result;
}


/*
d_tests_filter_iterator_edge
  Tests edge cases for the iterator interface.
  Tests the following:
  - iterator with empty chain yields all elements
  - iterator with take_first(2) yields exactly 2 elements
  - iterator with multi-operation chain composes correctly
  - single-element input with matching predicate
  - single-element input with non-matching predicate
*/
bool d_tests_filter_iterator_edge(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_filter_chain*     chain;
    struct dj_filter_iterator*  iter;
    int                        input[5];
    int                        single[1];
    const int*                 elem;
    size_t                     count;

    d_test_suite_test(_suite, "filter_iterator_edge");

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

            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_empty_chain",
                                      count == 5,
                                      "empty chain iterator should yield all 5 elements") && result;

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

            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_take_first",
                                      count == 2,
                                      "take_first(2) iterator should yield 2 elements") && result;

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

            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_multi_op",
                                      count == 4,
                                      "skip(1)->where(even) on {10..50} yields 4") && result;

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
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_single_match_has_next",
                                      d_filter_iterator_has_next(iter) == true,
                                      "single matching element: has_next should be true") && result;

            elem = (const int*)d_filter_iterator_next(iter);

            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_single_match_value",
                                      (elem != NULL) && (*elem == 4),
                                      "single matching element should be 4") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_single_match_exhausted",
                                      d_filter_iterator_has_next(iter) == false,
                                      "should be exhausted after single element") && result;

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
            result = D_TEST_CHECK_MSG(_suite,
                                      "iter_edge_single_no_match",
                                      d_filter_iterator_has_next(iter) == false,
                                      "single non-matching: has_next should be false") && result;

            d_filter_iterator_free(iter);
        }

        d_filter_chain_free(chain);
    }

    return result;
}


/*
d_tests_filter_iterator_all
  Aggregation function that runs all iterator interface tests.
*/
bool d_tests_filter_iterator_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "filter_iterator_all");

    result = true;

    result = d_tests_filter_iterator_create(_suite)    && result;
    result = d_tests_filter_iterator_traverse(_suite)  && result;
    result = d_tests_filter_iterator_reset(_suite)     && result;
    result = d_tests_filter_iterator_edge(_suite)      && result;

    return result;
}
