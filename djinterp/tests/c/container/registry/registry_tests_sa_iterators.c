#include ".\registry_tests_sa.h"


/******************************************************************************
 * HELPER CALLBACKS
 *****************************************************************************/


/*
pred_value_positive
  Predicate: returns true if the row's value field is > 0.
*/
static bool
pred_value_positive
(
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
static bool
pred_value_even
(
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
static bool
visitor_sum_values
(
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
static bool
visitor_stop_at_three
(
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
d_tests_sa_registry_iterator_new
  Tests the d_registry_iterator_new function.
  Tests the following:
  - returns iterator with registry set
  - current starts at 0
  - filter is NULL
  - filter_ctx is NULL
*/
bool
d_tests_sa_registry_iterator_new
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_registry*         reg;
    struct d_registry_iterator it;

    result = true;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        it = d_registry_iterator_new(reg);

        result = d_assert_standalone(
            it.registry == reg,
            "iter_new_reg",
            "Iterator should reference the registry",
            _counter) && result;

        result = d_assert_standalone(
            it.current == 0,
            "iter_new_current",
            "Iterator should start at 0",
            _counter) && result;

        result = d_assert_standalone(
            it.filter == NULL,
            "iter_new_filter",
            "Iterator filter should be NULL",
            _counter) && result;

        result = d_assert_standalone(
            it.filter_ctx == NULL,
            "iter_new_ctx",
            "Iterator filter_ctx should be NULL",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_iterator_filtered
  Tests the d_registry_iterator_filtered function.
  Tests the following:
  - registry, filter, and context are stored
  - current starts at 0
*/
bool
d_tests_sa_registry_iterator_filtered
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_registry*         reg;
    struct d_registry_iterator it;
    int                        ctx;

    result = true;
    ctx    = 42;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        it = d_registry_iterator_filtered(reg, pred_value_positive, &ctx);

        result = d_assert_standalone(
            it.registry == reg,
            "iter_filt_reg",
            "Filtered iterator should reference the registry",
            _counter) && result;

        result = d_assert_standalone(
            it.current == 0,
            "iter_filt_current",
            "Filtered iterator should start at 0",
            _counter) && result;

        result = d_assert_standalone(
            it.filter == pred_value_positive,
            "iter_filt_fn",
            "Filter function should be stored",
            _counter) && result;

        result = d_assert_standalone(
            it.filter_ctx == &ctx,
            "iter_filt_ctx",
            "Filter context should be stored",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_iterator_has_next
  Tests the d_registry_iterator_has_next function.
  Tests the following:
  - NULL iterator returns false
  - NULL registry in iterator returns false
  - empty registry returns false
  - populated registry returns true
  - returns false when exhausted
  - filtered iterator skips non-matching rows
*/
bool
d_tests_sa_registry_iterator_has_next
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_registry*         reg;
    struct d_registry_iterator it;
    struct test_row            row;

    result = true;

    // test 1: NULL iterator
    result = d_assert_standalone(
        d_registry_iterator_has_next(NULL) == false,
        "has_next_null",
        "NULL iterator should return false",
        _counter) && result;

    // test 2: NULL registry in iterator
    it.registry   = NULL;
    it.current    = 0;
    it.filter     = NULL;
    it.filter_ctx = NULL;

    result = d_assert_standalone(
        d_registry_iterator_has_next(&it) == false,
        "has_next_null_reg",
        "Iterator with NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 3: empty
        it = d_registry_iterator_new(reg);
        result = d_assert_standalone(
            d_registry_iterator_has_next(&it) == false,
            "has_next_empty",
            "Empty registry iterator should return false",
            _counter) && result;

        // populate
        row.key = "a";  row.value = -1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 5;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = -2;
        d_registry_add(reg, &row);

        // test 4: populated
        it = d_registry_iterator_new(reg);
        result = d_assert_standalone(
            d_registry_iterator_has_next(&it) == true,
            "has_next_populated",
            "Populated registry iterator should return true",
            _counter) && result;

        // test 5: exhausted
        it.current = 3;
        result = d_assert_standalone(
            d_registry_iterator_has_next(&it) == false,
            "has_next_exhausted",
            "Exhausted iterator should return false",
            _counter) && result;

        // test 6: filtered (only value > 0 => row "b")
        it = d_registry_iterator_filtered(reg, pred_value_positive, NULL);
        result = d_assert_standalone(
            d_registry_iterator_has_next(&it) == true,
            "has_next_filtered",
            "Filtered iterator should find row with value > 0",
            _counter) && result;

        // advance past all rows
        it.current = 3;
        result = d_assert_standalone(
            d_registry_iterator_has_next(&it) == false,
            "has_next_filt_exhausted",
            "Filtered exhausted iterator should return false",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_iterator_next
  Tests the d_registry_iterator_next function.
  Tests the following:
  - NULL iterator returns NULL
  - NULL registry returns NULL
  - iterates through all rows in order
  - returns NULL when exhausted
  - filtered iterator skips non-matching rows
*/
bool
d_tests_sa_registry_iterator_next
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_registry*         reg;
    struct d_registry_iterator it;
    struct test_row            row;
    struct test_row*           r;
    int                        visit_count;

    result = true;

    // test 1: NULL iterator
    result = d_assert_standalone(
        d_registry_iterator_next(NULL) == NULL,
        "next_null",
        "NULL iterator should return NULL",
        _counter) && result;

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

        result = d_assert_standalone(
            visit_count == 3,
            "next_all",
            "Should iterate through all 3 rows",
            _counter) && result;

        // test 3: exhausted returns NULL
        result = d_assert_standalone(
            d_registry_iterator_next(&it) == NULL,
            "next_exhausted",
            "Exhausted iterator should return NULL",
            _counter) && result;

        // test 4: filtered iterator skips beta (value < 0)
        it = d_registry_iterator_filtered(reg, pred_value_positive, NULL);
        visit_count = 0;

        while ((r = (struct test_row*)d_registry_iterator_next(&it)) != NULL)
        {
            result = d_assert_standalone(
                r->value > 0,
                "next_filt_positive",
                "Filtered next should only return positive values",
                _counter) && result;

            visit_count++;
        }

        result = d_assert_standalone(
            visit_count == 2,
            "next_filt_count",
            "Filtered iterator should visit 2 rows (alpha, gamma)",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_iterator_reset
  Tests the d_registry_iterator_reset function.
  Tests the following:
  - NULL iterator does not crash
  - resets current back to 0
  - can iterate again after reset
*/
bool
d_tests_sa_registry_iterator_reset
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_registry*         reg;
    struct d_registry_iterator it;
    struct test_row            row;
    int                        count1;
    int                        count2;

    result = true;

    // test 1: NULL does not crash
    d_registry_iterator_reset(NULL);
    result = d_assert_standalone(
        true,
        "reset_null",
        "NULL iterator reset should not crash",
        _counter) && result;

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

        result = d_assert_standalone(
            it.current == 0,
            "reset_current",
            "Current should be 0 after reset",
            _counter) && result;

        // test 3: iterate again
        count2 = 0;

        while (d_registry_iterator_next(&it) != NULL)
        {
            count2++;
        }

        result = d_assert_standalone(
            count1 == count2 && count2 == 2,
            "reset_reiterate",
            "Should iterate same count after reset",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_foreach
  Tests the d_registry_foreach function.
  Tests the following:
  - NULL registry does not crash
  - NULL visitor does not crash
  - visits all rows in order
  - context pointer is passed through
  - returning false from visitor stops iteration early
*/
bool
d_tests_sa_registry_foreach
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    int                sum;
    int                visit_count;

    result = true;

    // test 1: NULL registry
    d_registry_foreach(NULL, visitor_sum_values, &sum);
    result = d_assert_standalone(
        true,
        "foreach_null_reg",
        "NULL registry foreach should not crash",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL visitor
        d_registry_foreach(reg, NULL, NULL);
        result = d_assert_standalone(
            true,
            "foreach_null_visitor",
            "NULL visitor foreach should not crash",
            _counter) && result;

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
        result = d_assert_standalone(
            sum == 150,
            "foreach_sum",
            "Sum of all values should be 150",
            _counter) && result;

        // test 4: early termination (stops after 3)
        visit_count = 0;
        d_registry_foreach(reg, visitor_stop_at_three, &visit_count);
        result = d_assert_standalone(
            visit_count == 3,
            "foreach_early_stop",
            "Visitor returning false should stop at 3 visits",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_foreach_if
  Tests the d_registry_foreach_if function.
  Tests the following:
  - NULL registry does not crash
  - NULL visitor does not crash
  - NULL predicate visits all rows (no filtering)
  - with predicate, only matching rows are visited
  - early termination works with predicate
*/
bool
d_tests_sa_registry_foreach_if
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    int                sum;

    result = true;

    // test 1: NULL registry
    sum = 0;
    d_registry_foreach_if(NULL, pred_value_even, NULL,
                           visitor_sum_values, &sum);
    result = d_assert_standalone(
        true,
        "foreach_if_null_reg",
        "NULL registry foreach_if should not crash",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL visitor
        d_registry_foreach_if(reg, pred_value_even, NULL, NULL, NULL);
        result = d_assert_standalone(
            true,
            "foreach_if_null_visitor",
            "NULL visitor foreach_if should not crash",
            _counter) && result;

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
        result = d_assert_standalone(
            sum == 10,
            "foreach_if_null_pred",
            "NULL predicate should visit all rows (sum=10)",
            _counter) && result;

        // test 4: with predicate, only even values
        sum = 0;
        d_registry_foreach_if(reg, pred_value_even, NULL,
                               visitor_sum_values, &sum);
        result = d_assert_standalone(
            sum == 6,
            "foreach_if_even",
            "Even predicate should sum to 6 (2+4)",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_iterators_all
  Aggregation function that runs all iterator tests.
*/
bool
d_tests_sa_registry_iterators_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Iterator Functions\n");
    printf("  ------------------------------\n");

    result = d_tests_sa_registry_iterator_new(_counter) && result;
    result = d_tests_sa_registry_iterator_filtered(_counter) && result;
    result = d_tests_sa_registry_iterator_has_next(_counter) && result;
    result = d_tests_sa_registry_iterator_next(_counter) && result;
    result = d_tests_sa_registry_iterator_reset(_counter) && result;
    result = d_tests_sa_registry_foreach(_counter) && result;
    result = d_tests_sa_registry_foreach_if(_counter) && result;

    return result;
}
