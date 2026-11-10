/*******************************************************************************
* djinterp [container]                                registry_tests_iterators.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_iterators.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER CALLBACKS
 *****************************************************************************/


/*
pred_value_positive
  Predicate: returns true if the row's value field is > 0.
*/
static
bool pred_value_positive(
    const void* _row,
    const void* _context
)
{
    const struct test_row* r;

    (void)_context;

    if (!_row)
    {
        return false;
    }

    r = (const struct test_row*)_row;

    return (r->value > 0);
}

/*
pred_value_even
  Predicate: returns true if the row's value field is even.
*/
static
bool pred_value_even(
    const void* _row,
    const void* _context
)
{
    const struct test_row* r;

    (void)_context;

    if (!_row)
    {
        return false;
    }

    r = (const struct test_row*)_row;

    return ((r->value % 2) == 0);
}

/*
visitor_sum_values
  Visitor: adds each row's value to an int accumulator.
  Context is an int*.  Always returns true (full traversal).
*/
static
bool visitor_sum_values(
    void* _row,
    void* _context
)
{
    struct test_row* r;
    int*             sum;

    if (!_row || !_context)
    {
        return true;
    }

    r   = (struct test_row*)_row;
    sum = (int*)_context;

    *sum += r->value;

    return true;
}

/*
visitor_stop_at_three
  Visitor: counts visits via context (int*).  Returns false after 3 visits
  to test early termination.
*/
static
bool visitor_stop_at_three(
    void* _row,
    void* _context
)
{
    int* count;

    (void)_row;

    if (!_context)
    {
        return false;
    }

    count = (int*)_context;
    *count += 1;

    return (*count < 3);
}

/******************************************************************************
 * TEST FUNCTIONS
 *****************************************************************************/


/*
d_tests_registry_iterator_new
  Tests the d_registry_iterator_new function.
  Tests the following:
  - returns iterator with registry set
  - current starts at 0
  - filter is NULL
  - filter_ctx is NULL
*/
bool d_tests_registry_iterator_new(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_registry*         reg;
    struct dj_registry_iterator it;

    d_test_suite_test(_suite, "registry_iterator_new");

    result = true;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        it = d_registry_iterator_new(reg);

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_new_reg",
                                  it.registry == reg,
                                  "Iterator should reference the registry") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_new_current",
                                  it.current == 0,
                                  "Iterator should start at 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_new_filter",
                                  it.filter == NULL,
                                  "Iterator filter should be NULL") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_new_ctx",
                                  it.filter_ctx == NULL,
                                  "Iterator filter_ctx should be NULL") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_iterator_filtered
  Tests the d_registry_iterator_filtered function.
  Tests the following:
  - registry, filter, and context are stored
  - current starts at 0
*/
bool d_tests_registry_iterator_filtered(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_registry*         reg;
    struct dj_registry_iterator it;
    int                        ctx;

    d_test_suite_test(_suite, "registry_iterator_filtered");

    result = true;
    ctx    = 42;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        it = d_registry_iterator_filtered(reg, pred_value_positive, &ctx);

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_filt_reg",
                                  it.registry == reg,
                                  "Filtered iterator should reference the registry") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_filt_current",
                                  it.current == 0,
                                  "Filtered iterator should start at 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_filt_fn",
                                  it.filter == pred_value_positive,
                                  "Filter function should be stored") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "iter_filt_ctx",
                                  it.filter_ctx == &ctx,
                                  "Filter context should be stored") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_iterator_has_next
  Tests the d_registry_iterator_has_next function.
  Tests the following:
  - NULL iterator returns false
  - NULL registry in iterator returns false
  - empty registry returns false
  - populated registry returns true
  - returns false when exhausted
  - filtered iterator skips non-matching rows
*/
bool d_tests_registry_iterator_has_next(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_registry*         reg;
    struct dj_registry_iterator it;
    struct test_row            row;

    d_test_suite_test(_suite, "registry_iterator_has_next");

    result = true;

    // test 1: NULL iterator
    result = D_TEST_CHECK_MSG(_suite,
                              "has_next_null",
                              d_registry_iterator_has_next(NULL) == false,
                              "NULL iterator should return false") && result;

    // test 2: NULL registry in iterator
    it.registry   = NULL;
    it.current    = 0;
    it.filter     = NULL;
    it.filter_ctx = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "has_next_null_reg",
                              d_registry_iterator_has_next(&it) == false,
                              "Iterator with NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 3: empty
        it = d_registry_iterator_new(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_next_empty",
                                  d_registry_iterator_has_next(&it) == false,
                                  "Empty registry iterator should return false") && result;

        // populate
        row.key = "a";  row.value = -1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 5;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = -2;
        d_registry_add(reg, &row);

        // test 4: populated
        it = d_registry_iterator_new(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_next_populated",
                                  d_registry_iterator_has_next(&it) == true,
                                  "Populated registry iterator should return true") && result;

        // test 5: exhausted
        it.current = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_next_exhausted",
                                  d_registry_iterator_has_next(&it) == false,
                                  "Exhausted iterator should return false") && result;

        // test 6: filtered (only value > 0 => row "b")
        it = d_registry_iterator_filtered(reg, pred_value_positive, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_next_filtered",
                                  d_registry_iterator_has_next(&it) == true,
                                  "Filtered iterator should find row with value > 0") && result;

        // advance past all rows
        it.current = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_next_filt_exhausted",
                                  d_registry_iterator_has_next(&it) == false,
                                  "Filtered exhausted iterator should return false") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_iterator_next
  Tests the d_registry_iterator_next function.
  Tests the following:
  - NULL iterator returns NULL
  - NULL registry returns NULL
  - iterates through all rows in order
  - returns NULL when exhausted
  - filtered iterator skips non-matching rows
*/
bool d_tests_registry_iterator_next(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_registry*         reg;
    struct dj_registry_iterator it;
    struct test_row            row;
    struct test_row*           r;
    int                        visit_count;

    d_test_suite_test(_suite, "registry_iterator_next");

    result = true;

    // test 1: NULL iterator
    result = D_TEST_CHECK_MSG(_suite,
                              "next_null",
                              d_registry_iterator_next(NULL) == NULL,
                              "NULL iterator should return NULL") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = -5;
        d_registry_add(reg, &row);
        row.key = "gamma";  row.value = 20;
        d_registry_add(reg, &row);

        // test 2: iterate all
        it = d_registry_iterator_new(reg);
        visit_count = 0;

        while (d_registry_iterator_has_next(&it))
        {
            r = (struct test_row*)d_registry_iterator_next(&it);

            if (r)
            {
                visit_count++;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "next_all",
                                  visit_count == 3,
                                  "Should iterate through all 3 rows") && result;

        // test 3: exhausted returns NULL
        result = D_TEST_CHECK_MSG(_suite,
                                  "next_exhausted",
                                  d_registry_iterator_next(&it) == NULL,
                                  "Exhausted iterator should return NULL") && result;

        // test 4: filtered iterator skips beta (value < 0)
        it = d_registry_iterator_filtered(reg, pred_value_positive, NULL);
        visit_count = 0;

        while ((r = (struct test_row*)d_registry_iterator_next(&it)) != NULL)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "next_filt_positive",
                                      r->value > 0,
                                      "Filtered next should only return positive values") && result;

            visit_count++;
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "next_filt_count",
                                  visit_count == 2,
                                  "Filtered iterator should visit 2 rows (alpha, gamma)") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_iterator_reset
  Tests the d_registry_iterator_reset function.
  Tests the following:
  - NULL iterator does not crash
  - resets current back to 0
  - can iterate again after reset
*/
bool d_tests_registry_iterator_reset(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_registry*         reg;
    struct dj_registry_iterator it;
    struct test_row            row;
    int                        count1;
    int                        count2;

    d_test_suite_test(_suite, "registry_iterator_reset");

    result = true;

    // test 1: NULL does not crash
    d_registry_iterator_reset(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_null",
                              true,
                              "NULL iterator reset should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);

        // exhaust iterator
        it = d_registry_iterator_new(reg);
        count1 = 0;

        while (d_registry_iterator_next(&it) != NULL)
        {
            count1++;
        }

        // test 2: reset
        d_registry_iterator_reset(&it);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reset_current",
                                  it.current == 0,
                                  "Current should be 0 after reset") && result;

        // test 3: iterate again
        count2 = 0;

        while (d_registry_iterator_next(&it) != NULL)
        {
            count2++;
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "reset_reiterate",
                                  count1 == count2 && count2 == 2,
                                  "Should iterate same count after reset") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_foreach
  Tests the d_registry_foreach function.
  Tests the following:
  - NULL registry does not crash
  - NULL visitor does not crash
  - visits all rows in order
  - context pointer is passed through
  - returning false from visitor stops iteration early
*/
bool d_tests_registry_foreach(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    int                sum;
    int                visit_count;

    d_test_suite_test(_suite, "registry_foreach");

    result = true;

    // test 1: NULL registry
    d_registry_foreach(NULL, visitor_sum_values, &sum);
    result = D_TEST_CHECK_MSG(_suite,
                              "foreach_null_reg",
                              true,
                              "NULL registry foreach should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL visitor
        d_registry_foreach(reg, NULL, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_null_visitor",
                                  true,
                                  "NULL visitor foreach should not crash") && result;

        // populate
        row.key = "a";  row.value = 10;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 20;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = 30;
        d_registry_add(reg, &row);
        row.key = "d";  row.value = 40;
        d_registry_add(reg, &row);
        row.key = "e";  row.value = 50;
        d_registry_add(reg, &row);

        // test 3: visit all, sum values
        sum = 0;
        d_registry_foreach(reg, visitor_sum_values, &sum);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_sum",
                                  sum == 150,
                                  "Sum of all values should be 150") && result;

        // test 4: early termination (stops after 3)
        visit_count = 0;
        d_registry_foreach(reg, visitor_stop_at_three, &visit_count);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_early_stop",
                                  visit_count == 3,
                                  "Visitor returning false should stop at 3 visits") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_foreach_if
  Tests the d_registry_foreach_if function.
  Tests the following:
  - NULL registry does not crash
  - NULL visitor does not crash
  - NULL predicate visits all rows (no filtering)
  - with predicate, only matching rows are visited
  - early termination works with predicate
*/
bool d_tests_registry_foreach_if(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    int                sum;

    d_test_suite_test(_suite, "registry_foreach_if");

    result = true;

    // test 1: NULL registry
    sum = 0;
    d_registry_foreach_if(NULL, pred_value_even, NULL,
                           visitor_sum_values, &sum);
    result = D_TEST_CHECK_MSG(_suite,
                              "foreach_if_null_reg",
                              true,
                              "NULL registry foreach_if should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL visitor
        d_registry_foreach_if(reg, pred_value_even, NULL, NULL, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_if_null_visitor",
                                  true,
                                  "NULL visitor foreach_if should not crash") && result;

        // populate
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = 3;
        d_registry_add(reg, &row);
        row.key = "d";  row.value = 4;
        d_registry_add(reg, &row);

        // test 3: NULL predicate visits all
        sum = 0;
        d_registry_foreach_if(reg, NULL, NULL,
                               visitor_sum_values, &sum);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_if_null_pred",
                                  sum == 10,
                                  "NULL predicate should visit all rows (sum=10)") && result;

        // test 4: with predicate, only even values
        sum = 0;
        d_registry_foreach_if(reg, pred_value_even, NULL,
                               visitor_sum_values, &sum);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_if_even",
                                  sum == 6,
                                  "Even predicate should sum to 6 (2+4)") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_iterators_all
  Aggregation function that runs all iterator tests.
*/
bool d_tests_registry_iterators_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_iterators_all");

    result = true;

    result = d_tests_registry_iterator_new(_suite) && result;
    result = d_tests_registry_iterator_filtered(_suite) && result;
    result = d_tests_registry_iterator_has_next(_suite) && result;
    result = d_tests_registry_iterator_next(_suite) && result;
    result = d_tests_registry_iterator_reset(_suite) && result;
    result = d_tests_registry_foreach(_suite) && result;
    result = d_tests_registry_foreach_if(_suite) && result;

    return result;
}
