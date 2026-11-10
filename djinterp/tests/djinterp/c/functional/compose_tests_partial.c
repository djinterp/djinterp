/*******************************************************************************
* djinterp [functional]                                  compose_tests_partial.c
*
*
* path:      /tests/djinterp/c/functional/compose_tests_partial.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./compose_tests.h"
#include "./compose_tests_helpers.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/**
 * @brief Part 1 of d_tests_partial_consumer_new.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL consumer with NULL context should return NULL
 *   - test 2: NULL consumer with non-NULL context should return NULL
 *   - test 3: successful creation with no context
 *   - test 4: successful creation with context
 *   - test 5: creation with different consumer and context
 */
static bool
d_internal_tests_partial_consumer_new_part1(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_partial_consumer* partial;
    struct consumer_context     ctx;
    int                         addend;

    result = true;

    ctx.sum     = 0;
    ctx.count   = 0;
    addend      = 5;

    // test 1: NULL consumer with NULL context should return NULL
    partial = d_functional_partial_consumer_new(NULL, NULL);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_null_consumer",
                               partial == NULL,
                               "NULL consumer should return NULL") && result;

    // test 2: NULL consumer with non-NULL context should return NULL
    partial = d_functional_partial_consumer_new(NULL, &ctx);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_null_consumer_with_context",
                               partial == NULL,
                               "NULL consumer with non-NULL context should still return NULL") && result;

    // test 3: successful creation with no context
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_success_no_context",
                               partial != NULL,
                               "d_functional_partial_consumer_new should succeed with NULL context") && result;

    if (partial)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_consumer_set",
                                  partial->consumer == consumer_double_in_place,
                                  "consumer should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_context_null",
                                  partial->context == NULL,
                                  "context should be NULL") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 4: successful creation with context
    partial = d_functional_partial_consumer_new(consumer_accumulate, &ctx);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_success_with_context",
                               partial != NULL,
                               "d_functional_partial_consumer_new should succeed with context") && result;

    if (partial)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_context_consumer_set",
                                  partial->consumer == consumer_accumulate,
                                  "consumer should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_context_set",
                                  partial->context == &ctx,
                                  "context should point to ctx variable") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 5: creation with different consumer and context
    partial = d_functional_partial_consumer_new(consumer_add_context, &addend);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_add_context",
                               partial != NULL,
                               "d_functional_partial_consumer_new with add_context should succeed") && result;

    if (partial)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_add_context_consumer",
                                  partial->consumer == consumer_add_context,
                                  "consumer should be consumer_add_context") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_add_context_context",
                                  partial->context == &addend,
                                  "context should point to addend") && result;

        d_functional_partial_consumer_free(partial);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_partial_consumer_new.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 6: creation with no-op consumer
 */
static bool
d_internal_tests_partial_consumer_new_part2(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_partial_consumer* partial;

    result = true;

    // test 6: creation with no-op consumer
    partial = d_functional_partial_consumer_new(consumer_noop, NULL);
    result  = D_TEST_CHECK_MSG(_suite,
                               "partial_consumer_new_noop",
                               partial != NULL,
                               "d_functional_partial_consumer_new with noop should succeed") && result;

    if (partial)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_new_noop_consumer_set",
                                  partial->consumer == consumer_noop,
                                  "consumer should be consumer_noop") && result;

        d_functional_partial_consumer_free(partial);
    }

    return result;
}


/*
d_tests_partial_consumer_new
  Tests the d_functional_partial_consumer_new constructor function.
  Tests the following:
  - NULL consumer rejection (with NULL context)
  - NULL consumer rejection (with non-NULL context)
  - successful creation with valid consumer and no context
  - consumer and context field initialization
  - successful creation with context
  - creation with different consumer and context types
  - creation with no-op consumer
*/
bool d_tests_partial_consumer_new(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "partial_consumer_new");

    result = true;
    result &= d_internal_tests_partial_consumer_new_part1(_suite);
    result &= d_internal_tests_partial_consumer_new_part2(_suite);

    return result;
}


/*
d_tests_partial_consumer_apply
  Tests the d_functional_partial_consumer_apply function.
  Tests the following:
  - NULL partial rejection (should not crash)
  - NULL consumer in partial struct handling (should not crash)
  - successful application with no context (double in place)
  - successful application with context (accumulate)
  - multiple applications with accumulation
  - context forwarding verification (add context value)
  - NULL element handling (should not crash)
  - multiple partial consumers sharing context
  - no-op consumer preserves value
  - zero boundary value
  - negative value handling
*/
bool d_tests_partial_consumer_apply(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_partial_consumer* partial;
    struct consumer_context    ctx;
    int                        value;
    int                        addend;

    d_test_suite_test(_suite, "partial_consumer_apply");

    result      = true;
    ctx.sum     = 0;
    ctx.count   = 0;
    addend      = 7;

    // test 1: NULL partial should not crash
    value = 10;
    d_functional_partial_consumer_apply(NULL, &value);
    result = D_TEST_CHECK_MSG(_suite,
                              "partial_consumer_apply_null_partial",
                              true,
                              "NULL partial should not crash") && result;

    // test 2: NULL consumer in partial should not crash
    partial = d_functional_partial_consumer_new(consumer_noop, NULL);

    if (partial)
    {
        partial->consumer = NULL;
        value             = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_null_consumer",
                                  true,
                                  "NULL consumer should not crash") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 3: successful application with no context (double in place)
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = 5;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_double_no_context",
                                  value == 10,
                                  "5 should be doubled to 10") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 4: successful application with context (accumulate)
    ctx.sum   = 0;
    ctx.count = 0;
    partial   = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        value = 3;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_first",
                                  ctx.sum == 3 && ctx.count == 1,
                                  "First accumulate: sum=3, count=1") && result;

        // second application
        value = 7;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_second",
                                  ctx.sum == 10 && ctx.count == 2,
                                  "Second accumulate: sum=10, count=2") && result;

        // third application
        value = 5;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_third",
                                  ctx.sum == 15 && ctx.count == 3,
                                  "Third accumulate: sum=15, count=3") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 5: context forwarding (add context value)
    partial = d_functional_partial_consumer_new(consumer_add_context, &addend);

    if (partial)
    {
        value = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_add_context",
                                  value == 17,
                                  "10 + 7 should equal 17") && result;

        // apply again with different value
        value = 3;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_add_context_again",
                                  value == 10,
                                  "3 + 7 should equal 10") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 6: NULL element handling
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        d_functional_partial_consumer_apply(partial, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_null_element",
                                  true,
                                  "NULL element should not crash") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 7: multiple partial consumers with same context
    ctx.sum   = 0;
    ctx.count = 0;

    {
        struct dj_partial_consumer* partial1;
        struct dj_partial_consumer* partial2;

        partial1 = d_functional_partial_consumer_new(consumer_accumulate,
                                                     &ctx);
        partial2 = d_functional_partial_consumer_new(consumer_accumulate,
                                                     &ctx);

        if ( (partial1) &&
             (partial2) )
        {
            value = 2;
            d_functional_partial_consumer_apply(partial1, &value);

            value = 3;
            d_functional_partial_consumer_apply(partial2, &value);

            result = D_TEST_CHECK_MSG(_suite,
                                      "partial_consumer_apply_shared_context",
                                      ctx.sum == 5 && ctx.count == 2,
                                      "Both partial consumers should share context: sum=5, count=2") && result;

            d_functional_partial_consumer_free(partial1);
            d_functional_partial_consumer_free(partial2);
        }
        else
        {
            // clean up whichever succeeded
            if (partial1)
            {
                d_functional_partial_consumer_free(partial1);
            }

            if (partial2)
            {
                d_functional_partial_consumer_free(partial2);
            }
        }
    }

    // test 8: no-op consumer
    partial = d_functional_partial_consumer_new(consumer_noop, NULL);

    if (partial)
    {
        value = 42;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_noop",
                                  value == 42,
                                  "No-op consumer should not modify value") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 9: zero boundary value
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = 0;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_zero_boundary",
                                  value == 0,
                                  "0 doubled should still be 0") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 10: negative value handling
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = -5;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_negative",
                                  value == -10,
                                  "-5 doubled should equal -10") && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 11: accumulate with zero and negative values
    ctx.sum   = 0;
    ctx.count = 0;
    partial   = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        value = 0;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_zero",
                                  ctx.sum == 0 && ctx.count == 1,
                                  "Accumulate 0: sum=0, count=1") && result;

        value = -10;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_negative",
                                  ctx.sum == -10 && ctx.count == 2,
                                  "Accumulate -10: sum=-10, count=2") && result;

        value = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_apply_accumulate_cancel",
                                  ctx.sum == 0 && ctx.count == 3,
                                  "Accumulate 10 after -10: sum=0, count=3") && result;

        d_functional_partial_consumer_free(partial);
    }

    return result;
}


/*
d_tests_partial_consumer_free
  Tests the d_functional_partial_consumer_free function.
  Tests the following:
  - NULL pointer handling (should not crash)
  - successful free after creation
  - context not freed (should remain valid)
  - free after use (context should remain valid)
*/
bool d_tests_partial_consumer_free(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_partial_consumer* partial;
    struct consumer_context    ctx;

    d_test_suite_test(_suite, "partial_consumer_free");

    result      = true;
    ctx.sum     = 100;
    ctx.count   = 5;

    // test 1: NULL pointer should not crash
    d_functional_partial_consumer_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "partial_consumer_free_null",
                              true,
                              "Freeing NULL should not crash") && result;

    // test 2: normal free after creation
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        d_functional_partial_consumer_free(partial);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_free_normal",
                                  true,
                                  "Normal free should succeed") && result;
    }

    // test 3: free with context (context should not be freed)
    partial = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        d_functional_partial_consumer_free(partial);
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_free_with_context",
                                  true,
                                  "Free with context should succeed") && result;

        // context should still be valid
        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_free_context_intact",
                                  ctx.sum == 100 && ctx.count == 5,
                                  "Context should not be freed/modified") && result;
    }

    // test 4: free after use
    ctx.sum   = 0;
    ctx.count = 0;
    partial   = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        int value = 10;

        d_functional_partial_consumer_apply(partial, &value);

        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_free_after_use_state",
                                  ctx.sum == 10 && ctx.count == 1,
                                  "Context should have accumulated value before free") && result;

        d_functional_partial_consumer_free(partial);

        result = D_TEST_CHECK_MSG(_suite,
                                  "partial_consumer_free_after_use_intact",
                                  ctx.sum == 10 && ctx.count == 1,
                                  "Context should remain valid after free") && result;
    }

    return result;
}


/*
d_tests_compose_partial_all
  Aggregation function that runs all partial application tests.
*/
bool d_tests_compose_partial_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "compose_partial_all");

    result = true;

    result = d_tests_partial_consumer_new(_suite)   && result;
    result = d_tests_partial_consumer_apply(_suite)  && result;
    result = d_tests_partial_consumer_free(_suite)   && result;

    return result;
}
