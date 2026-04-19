#include "./str_interp_tests_sa.h"


/******************************************************************************
 * CONTEXT MANAGEMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_str_interp_context_new
  Tests d_str_interp_context_new for context creation.
  Tests the following:
  - returns non-NULL pointer
  - specifier_count is zero after creation
  - multiple contexts can be created independently
  - context is usable immediately after creation
*/
struct d_test_object*
d_tests_sa_str_interp_context_new
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx1;
    struct d_str_interp_context* ctx2;
    bool                         test_not_null;
    bool                         test_count_zero;
    bool                         test_independent;
    bool                         test_usable;
    size_t                       idx;

    // test 1: returns non-NULL
    ctx1 = d_str_interp_context_new();
    test_not_null = (ctx1 != NULL);

    // test 2: specifier_count is zero
    test_count_zero = false;
    if (ctx1)
    {
        test_count_zero = (ctx1->specifier_count == 0);
    }

    // test 3: multiple contexts are independent
    ctx2 = d_str_interp_context_new();
    test_independent = ( (ctx2 != NULL) &&
                         (ctx1 != ctx2) );
    if (ctx1 && ctx2)
    {
        // adding to ctx1 should not affect ctx2
        d_str_interp_add_specifier(ctx1, "key", "val");
        test_independent = test_independent &&
                           (ctx1->specifier_count == 1) &&
                           (ctx2->specifier_count == 0);
    }

    // test 4: context is immediately usable for add
    test_usable = false;
    if (ctx1)
    {
        enum d_str_interp_error err;
        err = d_str_interp_add_specifier(ctx1, "test", "value");
        test_usable = (err == D_STR_INTERP_SUCCESS);
    }

    // cleanup
    if (ctx1)
    {
        d_str_interp_context_free(ctx1);
    }

    if (ctx2)
    {
        d_str_interp_context_free(ctx2);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_context_new", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("not_null",
                                           test_not_null,
                                           "returns non-NULL pointer");
    group->elements[idx++] = D_ASSERT_TRUE("count_zero",
                                           test_count_zero,
                                           "specifier_count is zero");
    group->elements[idx++] = D_ASSERT_TRUE("independent",
                                           test_independent,
                                           "multiple contexts are independent");
    group->elements[idx++] = D_ASSERT_TRUE("usable",
                                           test_usable,
                                           "context is usable immediately");

    return group;
}

/*
d_tests_sa_str_interp_context_free
  Tests d_str_interp_context_free for context destruction.
  Tests the following:
  - does not crash on NULL
  - does not crash on empty context
  - frees context with specifiers without leaking
  - frees context with many specifiers
*/
struct d_test_object*
d_tests_sa_str_interp_context_free
(
    void
)
{
    struct d_test_object*       group;
    struct d_str_interp_context* ctx;
    bool                         test_null_safe;
    bool                         test_empty;
    bool                         test_with_specs;
    bool                         test_many_specs;
    size_t                       i;
    size_t                       idx;
    char                         name_buf[32];

    // test 1: does not crash on NULL
    d_str_interp_context_free(NULL);
    test_null_safe = true;  /* if we reach here, it didn't crash */

    // test 2: does not crash on empty context
    ctx = d_str_interp_context_new();
    test_empty = (ctx != NULL);
    if (ctx)
    {
        d_str_interp_context_free(ctx);
        /* if we reach here, it didn't crash */
    }

    // test 3: frees context with specifiers
    ctx = d_str_interp_context_new();
    test_with_specs = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "name", "Alice");
        d_str_interp_add_specifier(ctx, "city", "Wonderland");
        d_str_interp_add_specifier(ctx, "item", "teacup");
        test_with_specs = (ctx->specifier_count == 3);
        d_str_interp_context_free(ctx);
    }

    // test 4: frees context with many specifiers (near limit)
    ctx = d_str_interp_context_new();
    test_many_specs = false;
    if (ctx)
    {
        for (i = 0; i < D_STR_INTERP_MAX_SPECIFIERS; i++)
        {
            /* build a unique name for each specifier */
            d_memset(name_buf, 0, sizeof(name_buf));
            name_buf[0] = 'k';
            name_buf[1] = (char)('0' + (i / 10));
            name_buf[2] = (char)('0' + (i % 10));
            name_buf[3] = '\0';

            d_str_interp_add_specifier(ctx, name_buf, "v");
        }

        test_many_specs = (ctx->specifier_count == D_STR_INTERP_MAX_SPECIFIERS);
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_context_free", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_safe",
                                           test_null_safe,
                                           "does not crash on NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "frees empty context safely");
    group->elements[idx++] = D_ASSERT_TRUE("with_specs",
                                           test_with_specs,
                                           "frees context with specifiers");
    group->elements[idx++] = D_ASSERT_TRUE("many_specs",
                                           test_many_specs,
                                           "frees context at max capacity");

    return group;
}

/*
d_tests_sa_str_interp_context_clear
  Tests d_str_interp_context_clear for clearing specifiers.
  Tests the following:
  - clears all specifiers
  - specifier_count becomes zero
  - context is reusable after clear
  - does not crash on NULL
  - does not crash on already-empty context
  - previously-added specifiers are gone after clear
*/
struct d_test_object*
d_tests_sa_str_interp_context_clear
(
    void
)
{
    struct d_test_object*       group;
    struct d_str_interp_context* ctx;
    bool                         test_clears;
    bool                         test_count_zero;
    bool                         test_reusable;
    bool                         test_null_safe;
    bool                         test_empty_clear;
    bool                         test_specs_gone;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    // populate context
    test_clears    = false;
    test_count_zero = false;
    test_reusable  = false;
    test_specs_gone = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "alpha", "one");
        d_str_interp_add_specifier(ctx, "beta", "two");
        d_str_interp_add_specifier(ctx, "gamma", "three");

        // test 1: clear succeeds (count was 3)
        test_clears = (ctx->specifier_count == 3);

        d_str_interp_context_clear(ctx);

        // test 2: count is zero after clear
        test_count_zero = (ctx->specifier_count == 0);

        // test 3: context is reusable
        enum d_str_interp_error err;
        err = d_str_interp_add_specifier(ctx, "delta", "four");
        test_reusable = ( (err == D_STR_INTERP_SUCCESS) &&
                          (ctx->specifier_count == 1) );

        // test 6: old specifiers are gone
        test_specs_gone = ( (!d_str_interp_has_specifier(ctx, "alpha")) &&
                            (!d_str_interp_has_specifier(ctx, "beta"))  &&
                            (!d_str_interp_has_specifier(ctx, "gamma")) &&
                            ( d_str_interp_has_specifier(ctx, "delta")) );

        d_str_interp_context_free(ctx);
    }

    // test 4: does not crash on NULL
    d_str_interp_context_clear(NULL);
    test_null_safe = true;

    // test 5: does not crash on empty context
    ctx = d_str_interp_context_new();
    test_empty_clear = false;
    if (ctx)
    {
        d_str_interp_context_clear(ctx);
        test_empty_clear = (ctx->specifier_count == 0);
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_context_clear", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("clears",
                                           test_clears,
                                           "had specifiers before clear");
    group->elements[idx++] = D_ASSERT_TRUE("count_zero",
                                           test_count_zero,
                                           "specifier_count is zero after clear");
    group->elements[idx++] = D_ASSERT_TRUE("reusable",
                                           test_reusable,
                                           "context is reusable after clear");
    group->elements[idx++] = D_ASSERT_TRUE("null_safe",
                                           test_null_safe,
                                           "does not crash on NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty_clear",
                                           test_empty_clear,
                                           "clears already-empty context safely");
    group->elements[idx++] = D_ASSERT_TRUE("specs_gone",
                                           test_specs_gone,
                                           "old specifiers are gone, new present");

    return group;
}

/*
d_tests_sa_str_interp_context_all
  Runs all context management tests.
  Tests the following:
  - d_str_interp_context_new
  - d_str_interp_context_free
  - d_str_interp_context_clear
*/
struct d_test_object*
d_tests_sa_str_interp_context_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Context Management", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_str_interp_context_new();
    group->elements[idx++] = d_tests_sa_str_interp_context_free();
    group->elements[idx++] = d_tests_sa_str_interp_context_clear();

    return group;
}
