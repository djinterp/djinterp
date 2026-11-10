/*******************************************************************************
* djinterp [functional]                               fn_builder_tests_cleanup.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_cleanup.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./fn_builder_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPERS
 *****************************************************************************/

/*
test_helper_identity_transform
  Transformer: copies input to output unchanged.
*/
static
bool test_helper_identity_transform(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = *(const int*)_input;

    return true;
}


/*
test_helper_always_true
  Predicate: always returns true.
*/
static
bool test_helper_always_true(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}


/**
 * @brief Part 1 of d_tests_fn_builder_free.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - free empty builder
 *   - free builder with transforms
 *   - free builder with predicates
 *   - free builder with both
 *   - free NULL: safe no-op
 *   - free after execute
 */
static bool
d_internal_tests_fn_builder_free_part1(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                   input[]  = { 1, 2, 3 };
    int                   output[3];
    size_t                out_count;
    bool                  all_passed;

    all_passed = true;

    // ---- free empty builder ----
    builder = d_fn_builder_new();

    d_fn_builder_free(builder);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: empty builder freed without crash",
                                   true,
                                   "freeing a freshly-created builder should be safe");

    // ---- free builder with transforms ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_identity_transform);
    d_funtional_builder_map(builder, test_helper_identity_transform);

    d_fn_builder_free(builder);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: builder with 2 transforms freed safely",
                                   true,
                                   "transforms array should be freed");

    // ---- free builder with predicates ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_always_true);
    d_funtional_builder_filter(builder, test_helper_always_true);

    d_fn_builder_free(builder);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: builder with 2 predicates freed safely",
                                   true,
                                   "predicates array should be freed");

    // ---- free builder with both ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_identity_transform);
    d_funtional_builder_filter(builder, test_helper_always_true);

    d_fn_builder_free(builder);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: builder with transforms + predicates freed safely",
                                   true,
                                   "both arrays and struct should be freed");

    // ---- free NULL: safe no-op ----
    d_fn_builder_free(NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: NULL is safe no-op",
                                   true,
                                   "passing NULL should not crash");

    // ---- free after execute ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_identity_transform);
    d_funtional_builder_filter(builder, test_helper_always_true);

    d_fn_builder_execute(builder,
                         input,
                         3,
                         sizeof(int),
                         output,
                         &out_count);

    d_fn_builder_free(builder);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "free: builder freed safely after execute",
                                   true,
                                   "execute should not leave builder in a broken state");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_fn_builder_free.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - free builder that was grown
 */
static bool
d_internal_tests_fn_builder_free_part2(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    bool                  all_passed;

    all_passed = true;

    // ---- free builder that was grown ----
    {
        size_t i;

        builder = d_fn_builder_new();

        for (i = 0; i < 20; i++)
        {
            d_funtional_builder_map(builder,
                                    test_helper_identity_transform);
        }

        d_fn_builder_free(builder);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "free: grown builder (20 transforms) freed safely",
                                       true,
                                       "realloc'd arrays should be freed correctly");
    }

    return all_passed;
}


/*
d_tests_fn_builder_free
  Tests d_fn_builder_free.
  Tests the following:
  - free on a valid builder with no registered functions succeeds
  - free on a builder with registered transforms succeeds
  - free on a builder with registered predicates succeeds
  - free on a builder with both transforms and predicates succeeds
  - free(NULL) is a safe no-op
  - free on a builder that was used for execute succeeds
*/
bool d_tests_fn_builder_free(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_test(_suite, "fn_builder_free");

    all_passed = true;
    all_passed &= d_internal_tests_fn_builder_free_part1(_suite);
    all_passed &= d_internal_tests_fn_builder_free_part2(_suite);

    return all_passed;
}


/*
d_tests_fn_builder_cleanup_all
  Runs all builder cleanup tests.
  Tests the following:
  - d_fn_builder_free
*/
bool d_tests_fn_builder_cleanup_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "fn_builder_cleanup_all");

    all_passed = true;

    all_passed &= d_tests_fn_builder_free(_suite);

    return all_passed;
}
