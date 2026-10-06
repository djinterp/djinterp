/*******************************************************************************
* djinterp [c]                                               t_functional_core.c
*
*   Conformance harness for the functional module's utilities, higher-order
* functions, searches, predicate combinators, composition, builder and
* pipeline: each function's stated behaviour, the order folds and searches
* walk in, short-circuiting, error propagation, and (under ASan) that nothing
* leaks.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                  \
*        src/djinterp/c/functional/functional_common.c                         \
*        src/djinterp/c/functional/functional.c                                \
*        src/djinterp/c/functional/predicate.c                                 \
*        src/djinterp/c/functional/compose.c                                   \
*        src/djinterp/c/functional/fn_builder.c                                \
*        src/djinterp/c/functional/pipeline.c                                  \
*        tests/djinterp/c/functional/t_functional_core.c -o t_functional_core
*
*
* path:      /tests/djinterp/c/functional/t_functional_core.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <stdlib.h>   // free
#include <string.h>   // memcmp
// djinterp
#include "../../../../inc/djinterp/c/functional/functional.h"   // unit under test
#include "../../../../inc/djinterp/c/functional/predicate.h"    // unit under test
#include "../../../../inc/djinterp/c/functional/compose.h"      // unit under test
#include "../../../../inc/djinterp/c/functional/fn_builder.h"   // unit under test
#include "../../../../inc/djinterp/c/functional/pipeline.h"     // unit under test


static int g_checks   = 0;
static int g_failures = 0;
static int g_calls    = 0;

static void
d_tests_fn_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // report a failure with what was being checked, and where
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

// D_TESTS_FN_CHECK
//   macro: records one check with its source text and line.
#define D_TESTS_FN_CHECK(_cond)                                               \
    d_tests_fn_expect((_cond), #_cond, __LINE__)

static bool
d_tests_fn_even(
    const void* _element,
    void*       _context
)
{
    (void)_context;
    ++g_calls;

    return ((*(const int*)_element % 2) == 0);
}

static bool
d_tests_fn_positive(
    const void* _element,
    void*       _context
)
{
    (void)_context;
    ++g_calls;

    return (*(const int*)_element > 0);
}

static bool
d_tests_fn_times(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int factor = (_context != NULL) ? *(const int*)_context : 10;

    *(int*)_output = *(const int*)_input * factor;

    return true;
}

static bool
d_tests_fn_plus_one(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;
    *(int*)_output = *(const int*)_input + 1;

    return true;
}

static bool
d_tests_fn_fail_on_three(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;
    *(int*)_output = *(const int*)_input;

    return (*(const int*)_input != 3);
}

static bool
d_tests_fn_digits(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;
    *(int*)_accumulated = (*(int*)_accumulated * 10) + *(const int*)_element;

    return true;
}

static void
d_tests_fn_double_it(
    void* _element,
    void* _context
)
{
    (void)_context;
    *(int*)_element *= 2;

    return;
}

static void
d_tests_fn_sum(
    const void* _element,
    void*       _context
)
{
    *(int*)_context += *(const int*)_element;

    return;
}

static int
d_tests_fn_by_value(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    return d_functional_compare_int(_a, _b, _context);
}

static void
d_tests_fn_utilities(void)
{
    const int    a = 3, b = 7;
    const size_t s1 = 5u, s2 = 5u;
    const double one = 1.0, nan_value = 0.0 / 0.0;
    const bool   yes = true, no = false;
    int          copy = 0;
    size_t       size = sizeof(int);
    const void*  pointers[2] = { &a, NULL };

    D_TESTS_FN_CHECK( d_functional_identity_transformer(&a, &copy, &size) &&
                      (copy == 3) );
    D_TESTS_FN_CHECK(!d_functional_identity_transformer(&a, &copy, NULL));
    D_TESTS_FN_CHECK( d_functional_identity_predicate(&yes, NULL) &&
                      !d_functional_identity_predicate(&no, NULL) &&
                      !d_functional_identity_predicate(NULL, NULL) );
    D_TESTS_FN_CHECK( d_functional_constant_true(NULL, NULL) &&
                      !d_functional_constant_false(NULL, NULL) );
    D_TESTS_FN_CHECK( (d_functional_compare_int(&a, &b, NULL) < 0) &&
                      (d_functional_compare_int(&b, &a, NULL) > 0) &&
                      (d_functional_compare_int(&a, &a, NULL) == 0) &&
                      (d_functional_compare_int(NULL, &a, NULL) < 0) &&
                      (d_functional_compare_int(NULL, NULL, NULL) == 0) );
    D_TESTS_FN_CHECK( (d_functional_compare_double(&one, &nan_value,
                                                   NULL) < 0) &&
                      (d_functional_compare_double(&nan_value, &one,
                                                   NULL) > 0) &&
                      (d_functional_compare_double(&nan_value, &nan_value,
                                                   NULL) == 0) );
    D_TESTS_FN_CHECK( (d_functional_compare_size_t(&s1, &s2, NULL) == 0) &&
                      d_functional_equal_size_t(&s1, &s2, NULL) &&
                      d_functional_equal_int(&a, &a, NULL) &&
                      !d_functional_equal_int(&a, &b, NULL) );
    D_TESTS_FN_CHECK( !d_functional_is_null(&pointers[0], NULL) &&
                      d_functional_is_null(&pointers[1], NULL) &&
                      d_functional_is_null(NULL, NULL) &&
                      d_functional_is_not_null(&pointers[0], NULL) );

    return;
}

static void
d_tests_fn_higher_order(void)
{
    const int values[5] = { 1, 2, 3, 4, 5 };
    int       out[5]    = { 0 };
    int       mutable_values[3] = { 1, 2, 3 };
    int       acc = 0;
    int       sum = 0;
    const int evens[3] = { 2, 4, 6 };

    D_TESTS_FN_CHECK( d_functional_map(values, out, 5u, sizeof(int),
                                       &d_tests_fn_times, NULL) &&
                      (out[4] == 50) );

    // a failing transformer stops the map where it failed
    memset(out, 0, sizeof(out));
    D_TESTS_FN_CHECK( !d_functional_map(values, out, 5u, sizeof(int),
                                        &d_tests_fn_fail_on_three, NULL) &&
                      (out[1] == 2) && (out[3] == 0) );
    D_TESTS_FN_CHECK(!d_functional_map(values, out, 5u, sizeof(int), NULL,
                                       NULL));

    // the folds walk in opposite orders
    D_TESTS_FN_CHECK( d_functional_fold_left(values, 3u, sizeof(int), &acc,
                                             &d_tests_fn_digits, NULL) &&
                      (acc == 123) );
    acc = 0;
    D_TESTS_FN_CHECK( d_functional_fold_right(values, 3u, sizeof(int), &acc,
                                              &d_tests_fn_digits, NULL) &&
                      (acc == 321) );

    d_functional_for_each(mutable_values, 3u, sizeof(int),
                          &d_tests_fn_double_it, NULL);
    d_functional_for_each_const(mutable_values, 3u, sizeof(int),
                                &d_tests_fn_sum, &sum);
    D_TESTS_FN_CHECK( (mutable_values[2] == 6) && (sum == 12) );

    // quantifiers, including their empty cases
    D_TESTS_FN_CHECK( d_functional_all(evens, 3u, sizeof(int),
                                       &d_tests_fn_even, NULL) &&
                      !d_functional_all(values, 5u, sizeof(int),
                                        &d_tests_fn_even, NULL) &&
                      d_functional_all(NULL, 0u, sizeof(int),
                                       &d_tests_fn_even, NULL) );
    D_TESTS_FN_CHECK( d_functional_any(values, 5u, sizeof(int),
                                       &d_tests_fn_even, NULL) &&
                      !d_functional_any(NULL, 0u, sizeof(int),
                                        &d_tests_fn_even, NULL) );
    D_TESTS_FN_CHECK( d_functional_none(NULL, 0u, sizeof(int),
                                        &d_tests_fn_even, NULL) &&
                      !d_functional_none(values, 5u, sizeof(int),
                                         &d_tests_fn_even, NULL) );
    D_TESTS_FN_CHECK( (d_functional_count_if(values, 5u, sizeof(int),
                                             &d_tests_fn_even, NULL) == 2u) &&
                      (d_functional_find_if(values, 5u, sizeof(int),
                                            &d_tests_fn_even, NULL) ==
                       &values[1]) );

    // searches and sortedness
    D_TESTS_FN_CHECK( (d_functional_index_of(values, 5u, sizeof(int),
                                             &d_tests_fn_even) == 1u) &&
                      (d_functional_last_index_of(values, 5u, sizeof(int),
                                                  &d_tests_fn_even) == 3u) &&
                      (d_functional_find_last(values, 5u, sizeof(int),
                                              &d_tests_fn_even) ==
                       &values[3]) &&
                      (d_functional_index_of(values, 0u, sizeof(int),
                                             &d_tests_fn_even) ==
                       (size_t)-1) );
    {
        const int unsorted[3] = { 1, 3, 2 };
        const int flat[3]     = { 2, 2, 2 };

        D_TESTS_FN_CHECK( d_functional_is_sorted(values, 5u, sizeof(int),
                                                 &d_tests_fn_by_value) &&
                          d_functional_is_sorted(flat, 3u, sizeof(int),
                                                 &d_tests_fn_by_value) &&
                          !d_functional_is_sorted(unsorted, 3u, sizeof(int),
                                                  &d_tests_fn_by_value) &&
                          !d_functional_is_sorted(values, 5u, sizeof(int),
                                                  NULL) );
    }

    return;
}

static void
d_tests_fn_combinators(void)
{
    const int two = 2, three = 3, minus = -4;
    struct d_predicate_and* both   = d_predicate_and_new(&d_tests_fn_even,
                                                         NULL,
                                                         &d_tests_fn_positive,
                                                         NULL);
    struct d_predicate_or*  either = d_predicate_or_new(&d_tests_fn_even,
                                                        NULL,
                                                        &d_tests_fn_positive,
                                                        NULL);
    struct d_predicate_xor* one_of = d_predicate_xor_new(&d_tests_fn_even,
                                                         NULL,
                                                         &d_tests_fn_positive,
                                                         NULL);
    struct d_predicate_not* odd    = d_predicate_not_new(&d_tests_fn_even,
                                                         NULL);

    D_TESTS_FN_CHECK( (both != NULL) && (either != NULL) &&
                      (one_of != NULL) && (odd != NULL) );
    D_TESTS_FN_CHECK( d_predicate_and_eval(both, &two) &&
                      !d_predicate_and_eval(both, &minus) &&
                      !d_predicate_and_eval(both, &three) );

    // and short-circuits: an odd element never reaches the second test
    g_calls = 0;
    (void)d_predicate_and_eval(both, &three);
    D_TESTS_FN_CHECK(g_calls == 1);
    g_calls = 0;
    (void)d_predicate_or_eval(either, &two);
    D_TESTS_FN_CHECK(g_calls == 1);

    D_TESTS_FN_CHECK( d_predicate_or_eval(either, &three) &&
                      d_predicate_or_eval(either, &minus) );
    D_TESTS_FN_CHECK( !d_predicate_xor_eval(one_of, &two) &&
                      d_predicate_xor_eval(one_of, &three) &&
                      d_predicate_xor_eval(one_of, &minus) );
    D_TESTS_FN_CHECK( d_predicate_not_eval(odd, &three) &&
                      !d_predicate_not_eval(odd, &two) &&
                      !d_predicate_and_eval(NULL, &two) );
    D_TESTS_FN_CHECK(d_predicate_and_new(NULL, NULL, &d_tests_fn_even,
                                         NULL) == NULL);
    free(both);
    free(either);
    free(one_of);
    free(odd);

    // composition: second(first(x)), and nothing after a failed first
    {
        int                            factor = 3;
        int                            out    = 0;
        struct d_composed_transformer* c      = d_functional_compose_new(
            &d_tests_fn_times, &factor, &d_tests_fn_plus_one, NULL,
            sizeof(int));
        struct d_composed_transformer* f      = d_functional_compose_new(
            &d_tests_fn_fail_on_three, NULL, &d_tests_fn_plus_one, NULL,
            sizeof(int));

        D_TESTS_FN_CHECK( d_functional_compose_apply(c, &three, &out) &&
                          (out == 10) );
        out = 0;
        D_TESTS_FN_CHECK( !d_functional_compose_apply(f, &three, &out) &&
                          (out == 0) );
        d_functional_compose_free(c);
        d_functional_compose_free(f);
    }

    // a partial consumer applies its bound context
    {
        int                        total   = 0;
        int                        element = 5;
        struct d_partial_consumer* partial =
            d_functional_partial_consumer_new(
                (fn_consumer)NULL, &total);

        D_TESTS_FN_CHECK(partial == NULL);
        partial = d_functional_partial_consumer_new(&d_tests_fn_double_it,
                                                    NULL);
        d_functional_partial_consumer_apply(partial, &element);
        D_TESTS_FN_CHECK(element == 10);
        d_functional_partial_consumer_free(partial);
    }

    return;
}

static void
d_tests_fn_builder(void)
{
    const int            input[6] = { 1, 2, 3, 4, 5, 6 };
    int                  output[6] = { 0 };
    size_t               written   = 99u;
    struct d_fn_builder* builder   = d_fn_builder_new();

    // WHERE even, then x10, then +1
    D_TESTS_FN_CHECK(builder != NULL);
    D_TESTS_FN_CHECK(d_funtional_builder_where(builder, &d_tests_fn_even) ==
                     builder);
    (void)d_funtional_builder_map(builder, &d_tests_fn_times);
    (void)d_funtional_builder_and_then(builder, &d_tests_fn_plus_one);
    D_TESTS_FN_CHECK( d_fn_builder_execute(builder, input, 6u, sizeof(int),
                                           output, &written) &&
                      (written == 3u) && (output[0] == 21) &&
                      (output[2] == 61) );

    // growing past the initial capacity keeps every stage, in order
    for (int i = 0; i < 10; ++i)
    {
        (void)d_funtional_builder_and_then(builder, &d_tests_fn_plus_one);
    }

    D_TESTS_FN_CHECK( d_fn_builder_execute(builder, input, 6u, sizeof(int),
                                           output, &written) &&
                      (written == 3u) && (output[0] == 31) );

    // a failing transformer stops the run and reports how far it got
    {
        const int            ramp[4] = { 2, 3, 4, 5 };
        struct d_fn_builder* failing = d_fn_builder_new();

        (void)d_funtional_builder_map(failing, &d_tests_fn_fail_on_three);
        D_TESTS_FN_CHECK( !d_fn_builder_execute(failing, ramp, 4u,
                                                sizeof(int), output,
                                                &written) &&
                          (written == 1u) );
        d_fn_builder_free(failing);
    }

    d_fn_builder_free(builder);
    d_fn_builder_free(NULL);

    return;
}

static void
d_tests_fn_pipeline(void)
{
    int                          input[6] = { 1, 2, 3, 4, 5, 6 };
    const int                    before[6] = { 1, 2, 3, 4, 5, 6 };
    int                          factor   = 2;
    int                          zero     = 0;
    size_t                       count    = 99u;
    struct d_functional_pipeline pipe;
    int*                         result;

    // filter, map and take over a view leave the caller's array alone
    pipe = d_functional_pipeline_begin(input, 6u, sizeof(int));
    pipe = d_functional_pipeline_filter(pipe, &d_tests_fn_even, NULL);
    pipe = d_functional_pipeline_map(pipe, &d_tests_fn_times, &factor);
    pipe = d_functional_pipeline_take(pipe, 2u);
    result = d_functional_pipeline_end(pipe, &count);
    D_TESTS_FN_CHECK( (result != NULL) && (count == 2u) &&
                      (result[0] == 4) && (result[1] == 8) );
    D_TESTS_FN_CHECK(memcmp(input, before, sizeof(input)) == 0);
    free(result);

    // skip on a view, then end copies the view
    pipe   = d_functional_pipeline_skip(
                 d_functional_pipeline_begin(input, 6u, sizeof(int)), 4u);
    result = d_functional_pipeline_end(pipe, &count);
    D_TESTS_FN_CHECK( (count == 2u) && (result != NULL) &&
                      (result[0] == 5) && (result != &input[4]) );
    free(result);

    // skip on an owned copy keeps the block's start
    pipe = d_functional_pipeline_begin_copy(input, 6u, sizeof(int));
    pipe = d_functional_pipeline_skip(pipe, 1u);
    pipe = d_functional_pipeline_fold(pipe, &zero, sizeof(int),
                                      &d_tests_fn_digits, NULL);
    D_TESTS_FN_CHECK( (pipe.error_code == 0) && (pipe.count == 1u) &&
                      (*(int*)pipe.data == 23456) );
    d_functional_pipeline_free(&pipe);
    D_TESTS_FN_CHECK(pipe.data == NULL);

    // for_each writes through a view, as its consumer asks
    pipe = d_functional_pipeline_begin(input, 3u, sizeof(int));
    pipe = d_functional_pipeline_for_each(pipe, &d_tests_fn_double_it, NULL);
    D_TESTS_FN_CHECK( (input[0] == 2) && (input[2] == 6) && (input[3] == 4) );

    // the first failure sticks, and end yields nothing
    pipe = d_functional_pipeline_begin_copy(before, 6u, sizeof(int));
    pipe = d_functional_pipeline_map(pipe, NULL, NULL);
    pipe = d_functional_pipeline_take(pipe, 1u);
    D_TESTS_FN_CHECK( (pipe.error_code != 0) && (pipe.count == 6u) );
    result = d_functional_pipeline_end(pipe, &count);
    D_TESTS_FN_CHECK( (result == NULL) && (count == 0u) );

    // a transformer that fails is a failure too
    pipe = d_functional_pipeline_begin_copy(before, 6u, sizeof(int));
    pipe = d_functional_pipeline_map(pipe, &d_tests_fn_fail_on_three, NULL);
    D_TESTS_FN_CHECK(pipe.error_code != 0);
    d_functional_pipeline_free(&pipe);

    return;
}

int
main(void)
{
    d_tests_fn_utilities();
    d_tests_fn_higher_order();
    d_tests_fn_combinators();
    d_tests_fn_builder();
    d_tests_fn_pipeline();

    printf("t_functional_core: %d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
