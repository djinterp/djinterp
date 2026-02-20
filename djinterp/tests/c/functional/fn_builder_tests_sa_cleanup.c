#include ".\fn_builder_tests_sa.h"


/******************************************************************************
 * TEST HELPERS
 *****************************************************************************/

/*
test_helper_identity_transform
  Transformer: copies input to output unchanged.
*/
static bool
test_helper_identity_transform
(
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
static bool
test_helper_always_true
(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}


/*
d_tests_sa_fn_builder_free
  Tests d_fn_builder_free.
  Tests the following:
  - free on a valid builder with no registered functions succeeds
  - free on a builder with registered transforms succeeds
  - free on a builder with registered predicates succeeds
  - free on a builder with both transforms and predicates succeeds
  - free(NULL) is a safe no-op
  - free on a builder that was used for execute succeeds
*/
bool
d_tests_sa_fn_builder_free
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3 };
    int                  output[3];
    size_t               out_count;
    bool                 all_passed;

    all_passed = true;

    // ---- free empty builder ----
    builder = d_fn_builder_new();

    d_fn_builder_free(builder);

    all_passed &= d_assert_standalone(
        true,
        "free: empty builder freed without crash",
        "freeing a freshly-created builder should be safe",
        _test_info);

    // ---- free builder with transforms ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_identity_transform);
    d_funtional_builder_map(builder, test_helper_identity_transform);

    d_fn_builder_free(builder);

    all_passed &= d_assert_standalone(
        true,
        "free: builder with 2 transforms freed safely",
        "transforms array should be freed",
        _test_info);

    // ---- free builder with predicates ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_always_true);
    d_funtional_builder_filter(builder, test_helper_always_true);

    d_fn_builder_free(builder);

    all_passed &= d_assert_standalone(
        true,
        "free: builder with 2 predicates freed safely",
        "predicates array should be freed",
        _test_info);

    // ---- free builder with both ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_identity_transform);
    d_funtional_builder_filter(builder, test_helper_always_true);

    d_fn_builder_free(builder);

    all_passed &= d_assert_standalone(
        true,
        "free: builder with transforms + predicates freed safely",
        "both arrays and struct should be freed",
        _test_info);

    // ---- free NULL: safe no-op ----
    d_fn_builder_free(NULL);

    all_passed &= d_assert_standalone(
        true,
        "free: NULL is safe no-op",
        "passing NULL should not crash",
        _test_info);

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

    all_passed &= d_assert_standalone(
        true,
        "free: builder freed safely after execute",
        "execute should not leave builder in a broken state",
        _test_info);

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

        all_passed &= d_assert_standalone(
            true,
            "free: grown builder (20 transforms) freed safely",
            "realloc'd arrays should be freed correctly",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_fn_builder_cleanup_all
  Runs all builder cleanup tests.
  Tests the following:
  - d_fn_builder_free
*/
bool
d_tests_sa_fn_builder_cleanup_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_fn_builder_free(_test_info);

    return all_passed;
}
