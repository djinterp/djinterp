#include ".\compose_tests_sa.h"
#include ".\compose_tests_sa_helpers.h"


/*
d_tests_sa_partial_consumer_new
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
bool
d_tests_sa_partial_consumer_new
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_partial_consumer* partial;
    struct consumer_context    ctx;
    int                        addend;

    result      = true;
    ctx.sum     = 0;
    ctx.count   = 0;
    addend      = 5;

    // test 1: NULL consumer with NULL context should return NULL
    partial = d_functional_partial_consumer_new(NULL, NULL);
    result  = d_assert_standalone(
        partial == NULL,
        "partial_consumer_new_null_consumer",
        "NULL consumer should return NULL",
        _counter) && result;

    // test 2: NULL consumer with non-NULL context should return NULL
    partial = d_functional_partial_consumer_new(NULL, &ctx);
    result  = d_assert_standalone(
        partial == NULL,
        "partial_consumer_new_null_consumer_with_context",
        "NULL consumer with non-NULL context should still return NULL",
        _counter) && result;

    // test 3: successful creation with no context
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);
    result  = d_assert_standalone(
        partial != NULL,
        "partial_consumer_new_success_no_context",
        "d_functional_partial_consumer_new should succeed with NULL context",
        _counter) && result;

    if (partial)
    {
        result = d_assert_standalone(
            partial->consumer == consumer_double_in_place,
            "partial_consumer_new_consumer_set",
            "consumer should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            partial->context == NULL,
            "partial_consumer_new_context_null",
            "context should be NULL",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 4: successful creation with context
    partial = d_functional_partial_consumer_new(consumer_accumulate, &ctx);
    result  = d_assert_standalone(
        partial != NULL,
        "partial_consumer_new_success_with_context",
        "d_functional_partial_consumer_new should succeed with context",
        _counter) && result;

    if (partial)
    {
        result = d_assert_standalone(
            partial->consumer == consumer_accumulate,
            "partial_consumer_new_context_consumer_set",
            "consumer should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            partial->context == &ctx,
            "partial_consumer_new_context_set",
            "context should point to ctx variable",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 5: creation with different consumer and context
    partial = d_functional_partial_consumer_new(consumer_add_context, &addend);
    result  = d_assert_standalone(
        partial != NULL,
        "partial_consumer_new_add_context",
        "d_functional_partial_consumer_new with add_context should succeed",
        _counter) && result;

    if (partial)
    {
        result = d_assert_standalone(
            partial->consumer == consumer_add_context,
            "partial_consumer_new_add_context_consumer",
            "consumer should be consumer_add_context",
            _counter) && result;

        result = d_assert_standalone(
            partial->context == &addend,
            "partial_consumer_new_add_context_context",
            "context should point to addend",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 6: creation with no-op consumer
    partial = d_functional_partial_consumer_new(consumer_noop, NULL);
    result  = d_assert_standalone(
        partial != NULL,
        "partial_consumer_new_noop",
        "d_functional_partial_consumer_new with noop should succeed",
        _counter) && result;

    if (partial)
    {
        result = d_assert_standalone(
            partial->consumer == consumer_noop,
            "partial_consumer_new_noop_consumer_set",
            "consumer should be consumer_noop",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    return result;
}


/*
d_tests_sa_partial_consumer_apply
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
bool
d_tests_sa_partial_consumer_apply
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_partial_consumer* partial;
    struct consumer_context    ctx;
    int                        value;
    int                        addend;

    result      = true;
    ctx.sum     = 0;
    ctx.count   = 0;
    addend      = 7;

    // test 1: NULL partial should not crash
    value = 10;
    d_functional_partial_consumer_apply(NULL, &value);
    result = d_assert_standalone(
        true,
        "partial_consumer_apply_null_partial",
        "NULL partial should not crash",
        _counter) && result;

    // test 2: NULL consumer in partial should not crash
    partial = d_functional_partial_consumer_new(consumer_noop, NULL);

    if (partial)
    {
        partial->consumer = NULL;
        value             = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            true,
            "partial_consumer_apply_null_consumer",
            "NULL consumer should not crash",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 3: successful application with no context (double in place)
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = 5;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            value == 10,
            "partial_consumer_apply_double_no_context",
            "5 should be doubled to 10",
            _counter) && result;

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
        result = d_assert_standalone(
            ctx.sum == 3 && ctx.count == 1,
            "partial_consumer_apply_accumulate_first",
            "First accumulate: sum=3, count=1",
            _counter) && result;

        // second application
        value = 7;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            ctx.sum == 10 && ctx.count == 2,
            "partial_consumer_apply_accumulate_second",
            "Second accumulate: sum=10, count=2",
            _counter) && result;

        // third application
        value = 5;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            ctx.sum == 15 && ctx.count == 3,
            "partial_consumer_apply_accumulate_third",
            "Third accumulate: sum=15, count=3",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 5: context forwarding (add context value)
    partial = d_functional_partial_consumer_new(consumer_add_context, &addend);

    if (partial)
    {
        value = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            value == 17,
            "partial_consumer_apply_add_context",
            "10 + 7 should equal 17",
            _counter) && result;

        // apply again with different value
        value = 3;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            value == 10,
            "partial_consumer_apply_add_context_again",
            "3 + 7 should equal 10",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 6: NULL element handling
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        d_functional_partial_consumer_apply(partial, NULL);
        result = d_assert_standalone(
            true,
            "partial_consumer_apply_null_element",
            "NULL element should not crash",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 7: multiple partial consumers with same context
    ctx.sum   = 0;
    ctx.count = 0;

    {
        struct d_partial_consumer* partial1;
        struct d_partial_consumer* partial2;

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

            result = d_assert_standalone(
                ctx.sum == 5 && ctx.count == 2,
                "partial_consumer_apply_shared_context",
                "Both partial consumers should share context: sum=5, count=2",
                _counter) && result;

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
        result = d_assert_standalone(
            value == 42,
            "partial_consumer_apply_noop",
            "No-op consumer should not modify value",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 9: zero boundary value
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = 0;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            value == 0,
            "partial_consumer_apply_zero_boundary",
            "0 doubled should still be 0",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    // test 10: negative value handling
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        value = -5;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            value == -10,
            "partial_consumer_apply_negative",
            "-5 doubled should equal -10",
            _counter) && result;

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
        result = d_assert_standalone(
            ctx.sum == 0 && ctx.count == 1,
            "partial_consumer_apply_accumulate_zero",
            "Accumulate 0: sum=0, count=1",
            _counter) && result;

        value = -10;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            ctx.sum == -10 && ctx.count == 2,
            "partial_consumer_apply_accumulate_negative",
            "Accumulate -10: sum=-10, count=2",
            _counter) && result;

        value = 10;
        d_functional_partial_consumer_apply(partial, &value);
        result = d_assert_standalone(
            ctx.sum == 0 && ctx.count == 3,
            "partial_consumer_apply_accumulate_cancel",
            "Accumulate 10 after -10: sum=0, count=3",
            _counter) && result;

        d_functional_partial_consumer_free(partial);
    }

    return result;
}


/*
d_tests_sa_partial_consumer_free
  Tests the d_functional_partial_consumer_free function.
  Tests the following:
  - NULL pointer handling (should not crash)
  - successful free after creation
  - context not freed (should remain valid)
  - free after use (context should remain valid)
*/
bool
d_tests_sa_partial_consumer_free
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_partial_consumer* partial;
    struct consumer_context    ctx;

    result      = true;
    ctx.sum     = 100;
    ctx.count   = 5;

    // test 1: NULL pointer should not crash
    d_functional_partial_consumer_free(NULL);
    result = d_assert_standalone(
        true,
        "partial_consumer_free_null",
        "Freeing NULL should not crash",
        _counter) && result;

    // test 2: normal free after creation
    partial = d_functional_partial_consumer_new(consumer_double_in_place,
                                                NULL);

    if (partial)
    {
        d_functional_partial_consumer_free(partial);
        result = d_assert_standalone(
            true,
            "partial_consumer_free_normal",
            "Normal free should succeed",
            _counter) && result;
    }

    // test 3: free with context (context should not be freed)
    partial = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        d_functional_partial_consumer_free(partial);
        result = d_assert_standalone(
            true,
            "partial_consumer_free_with_context",
            "Free with context should succeed",
            _counter) && result;

        // context should still be valid
        result = d_assert_standalone(
            ctx.sum == 100 && ctx.count == 5,
            "partial_consumer_free_context_intact",
            "Context should not be freed/modified",
            _counter) && result;
    }

    // test 4: free after use
    ctx.sum   = 0;
    ctx.count = 0;
    partial   = d_functional_partial_consumer_new(consumer_accumulate, &ctx);

    if (partial)
    {
        int value = 10;

        d_functional_partial_consumer_apply(partial, &value);

        result = d_assert_standalone(
            ctx.sum == 10 && ctx.count == 1,
            "partial_consumer_free_after_use_state",
            "Context should have accumulated value before free",
            _counter) && result;

        d_functional_partial_consumer_free(partial);

        result = d_assert_standalone(
            ctx.sum == 10 && ctx.count == 1,
            "partial_consumer_free_after_use_intact",
            "Context should remain valid after free",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_compose_partial_all
  Aggregation function that runs all partial application tests.
*/
bool
d_tests_sa_compose_partial_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Partial Application Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_partial_consumer_new(_counter)   && result;
    result = d_tests_sa_partial_consumer_apply(_counter)  && result;
    result = d_tests_sa_partial_consumer_free(_counter)   && result;

    return result;
}
