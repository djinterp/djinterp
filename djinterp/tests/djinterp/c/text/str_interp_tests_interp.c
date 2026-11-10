/*******************************************************************************
* djinterp [text]                                      str_interp_tests_interp.c
*
*
* path:      /tests/djinterp/c/text/str_interp_tests_interp.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./str_interp_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * INTERPOLATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_str_interp_interp
  Tests d_str_interp for buffer-based interpolation.
  Tests the following:
  - replaces a single specifier in input
  - replaces multiple specifiers in input
  - leaves non-matching text unchanged
  - handles input with no specifiers present
  - handles empty input string
  - longest match wins when specifiers overlap
  - replaces repeated occurrences of same specifier
  - returns ERROR_NULL_PARAM for NULL context
  - returns ERROR_NULL_PARAM for NULL buffer
  - returns ERROR_NULL_PARAM for NULL input
  - returns ERROR_BUFFER_TOO_SMALL when buffer is insufficient
  - handles adjacent specifiers with no gap
*/
bool d_tests_str_interp_interp(struct dj_test_suite* _suite)
{
    struct dj_str_interp_context* ctx;
    enum dj_str_interp_error      err;
    char                         buf[D_TEST_INTERP_BUFFER_SIZE];
    bool                         test_single;
    bool                         test_multiple;
    bool                         test_passthrough;
    bool                         test_no_specs;
    bool                         test_empty;
    bool                         test_longest_match;
    bool                         test_repeated;
    bool                         test_null_ctx;
    bool                         test_null_buf;
    bool                         test_null_input;
    bool                         test_buf_small;
    bool                         test_adjacent;

    /* --- single specifier replacement --- */
    ctx = d_str_interp_context_new();
    test_single = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "NAME", "Alice");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "Hello NAME!");
        test_single = ( (err == D_STR_INTERP_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "Hello Alice!",
                                     12)) );
        d_str_interp_context_free(ctx);
    }

    /* --- multiple specifiers --- */
    ctx = d_str_interp_context_new();
    test_multiple = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "FIRST", "Jane");
        d_str_interp_add_specifier(ctx, "LAST", "Doe");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "FIRST LAST");
        test_multiple = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "Jane Doe",
                                       8)) );
        d_str_interp_context_free(ctx);
    }

    /* --- non-matching text passes through --- */
    ctx = d_str_interp_context_new();
    test_passthrough = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "KEY", "val");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "no match here");
        test_passthrough = ( (err == D_STR_INTERP_SUCCESS) &&
                             (d_strequals(buf,
                                          d_strnlen(buf, sizeof(buf)),
                                          "no match here",
                                          13)) );
        d_str_interp_context_free(ctx);
    }

    /* --- input with no specifiers registered --- */
    ctx = d_str_interp_context_new();
    test_no_specs = false;
    if (ctx)
    {
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "plain text");
        test_no_specs = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "plain text",
                                       10)) );
        d_str_interp_context_free(ctx);
    }

    /* --- empty input string --- */
    ctx = d_str_interp_context_new();
    test_empty = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "X", "Y");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "");
        test_empty = ( (err == D_STR_INTERP_SUCCESS) &&
                       (buf[0] == '\0') );
        d_str_interp_context_free(ctx);
    }

    /* --- longest match wins --- */
    ctx = d_str_interp_context_new();
    test_longest_match = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "var", "SHORT");
        d_str_interp_add_specifier(ctx, "variable", "LONG");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "variable");
        /* "variable" should match the longer key "variable", not "var" */
        test_longest_match = ( (err == D_STR_INTERP_SUCCESS) &&
                               (d_strequals(buf,
                                            d_strnlen(buf, sizeof(buf)),
                                            "LONG",
                                            4)) );
        d_str_interp_context_free(ctx);
    }

    /* --- repeated occurrences --- */
    ctx = d_str_interp_context_new();
    test_repeated = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "X", "!");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "aXbXc");
        test_repeated = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "a!b!c",
                                       5)) );
        d_str_interp_context_free(ctx);
    }

    /* --- NULL context --- */
    err = d_str_interp(NULL, buf, sizeof(buf), "test");
    test_null_ctx = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    /* --- NULL buffer --- */
    ctx = d_str_interp_context_new();
    test_null_buf = false;
    if (ctx)
    {
        err = d_str_interp(ctx, NULL, 100, "test");
        test_null_buf = (err == D_STR_INTERP_ERROR_NULL_PARAM);
        d_str_interp_context_free(ctx);
    }

    /* --- NULL input --- */
    ctx = d_str_interp_context_new();
    test_null_input = false;
    if (ctx)
    {
        err = d_str_interp(ctx, buf, sizeof(buf), NULL);
        test_null_input = (err == D_STR_INTERP_ERROR_NULL_PARAM);
        d_str_interp_context_free(ctx);
    }

    /* --- buffer too small --- */
    ctx = d_str_interp_context_new();
    test_buf_small = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "BIG", "very long replacement");
        err = d_str_interp(ctx,
                           buf,
                           5,     /* tiny buffer */
                           "BIG");
        test_buf_small = (err == D_STR_INTERP_ERROR_BUFFER_TOO_SMALL);
        d_str_interp_context_free(ctx);
    }

    /* --- adjacent specifiers --- */
    ctx = d_str_interp_context_new();
    test_adjacent = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "AA", "1");
        d_str_interp_add_specifier(ctx, "BB", "2");
        err = d_str_interp(ctx,
                           buf,
                           sizeof(buf),
                           "AABB");
        test_adjacent = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "12",
                                       2)) );
        d_str_interp_context_free(ctx);
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_str_interp");

    D_TEST_CHECK_MSG(_suite,
                     "single",
                     test_single,
                     "replaces single specifier");
    D_TEST_CHECK_MSG(_suite,
                     "multiple",
                     test_multiple,
                     "replaces multiple specifiers");
    D_TEST_CHECK_MSG(_suite,
                     "passthrough",
                     test_passthrough,
                     "non-matching text unchanged");
    D_TEST_CHECK_MSG(_suite,
                     "no_specs",
                     test_no_specs,
                     "no specifiers passes through");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty input");
    D_TEST_CHECK_MSG(_suite,
                     "longest_match",
                     test_longest_match,
                     "longest match wins");
    D_TEST_CHECK_MSG(_suite,
                     "repeated",
                     test_repeated,
                     "replaces repeated occurrences");
    D_TEST_CHECK_MSG(_suite,
                     "null_ctx",
                     test_null_ctx,
                     "ERROR_NULL_PARAM for NULL context");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "ERROR_NULL_PARAM for NULL buffer");
    D_TEST_CHECK_MSG(_suite,
                     "null_input",
                     test_null_input,
                     "ERROR_NULL_PARAM for NULL input");
    D_TEST_CHECK_MSG(_suite,
                     "buf_small",
                     test_buf_small,
                     "ERROR_BUFFER_TOO_SMALL on overflow");
    D_TEST_CHECK_MSG(_suite,
                     "adjacent",
                     test_adjacent,
                     "handles adjacent specifiers");

    return true;
}

/*
d_tests_str_interp_alloc
  Tests d_str_interp_alloc for allocating interpolation.
  Tests the following:
  - allocates and returns correct result
  - result is null-terminated
  - caller can free the result
  - handles empty input
  - handles no specifier matches
  - returns ERROR_NULL_PARAM for NULL inputs
  - handles value longer than name (expansion)
  - handles value shorter than name (contraction)
*/
bool d_tests_str_interp_alloc(struct dj_test_suite* _suite)
{
    struct dj_str_interp_context* ctx;
    enum dj_str_interp_error      err;
    char*                        result;
    bool                         test_correct;
    bool                         test_null_term;
    bool                         test_freeable;
    bool                         test_empty;
    bool                         test_no_match;
    bool                         test_null_param;
    bool                         test_expansion;
    bool                         test_contraction;

    /* --- correct result --- */
    ctx = d_str_interp_context_new();
    test_correct  = false;
    test_null_term = false;
    test_freeable = false;
    result = NULL;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "WHO", "World");
        err = d_str_interp_alloc(ctx, "Hello WHO", &result);
        test_correct = ( (err == D_STR_INTERP_SUCCESS) &&
                         (result != NULL) &&
                         (d_strequals(result,
                                      d_strnlen(result, 256),
                                      "Hello World",
                                      11)) );

        // test 2: null-terminated
        if (result)
        {
            test_null_term = (result[11] == '\0');
        }

        // test 3: freeable
        if (result)
        {
            free(result);
            result = NULL;
            test_freeable = true;  /* no crash */
        }

        d_str_interp_context_free(ctx);
    }

    /* --- empty input --- */
    ctx = d_str_interp_context_new();
    test_empty = false;
    result = NULL;
    if (ctx)
    {
        err = d_str_interp_alloc(ctx, "", &result);
        test_empty = ( (err == D_STR_INTERP_SUCCESS) &&
                       (result != NULL)              &&
                       (result[0] == '\0') );
        if (result)
        {
            free(result);
        }

        d_str_interp_context_free(ctx);
    }

    /* --- no match --- */
    ctx = d_str_interp_context_new();
    test_no_match = false;
    result = NULL;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "ZZZ", "zzz");
        err = d_str_interp_alloc(ctx, "abc", &result);
        test_no_match = ( (err == D_STR_INTERP_SUCCESS) &&
                          (result != NULL)              &&
                          (d_strequals(result,
                                       d_strnlen(result, 256),
                                       "abc",
                                       3)) );
        if (result)
        {
            free(result);
        }

        d_str_interp_context_free(ctx);
    }

    /* --- NULL params --- */
    err = d_str_interp_alloc(NULL, "test", &result);
    test_null_param = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    /* --- expansion (value longer than name) --- */
    ctx = d_str_interp_context_new();
    test_expansion = false;
    result = NULL;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "X", "EXPANDED");
        err = d_str_interp_alloc(ctx, "aXb", &result);
        test_expansion = ( (err == D_STR_INTERP_SUCCESS) &&
                           (result != NULL)              &&
                           (d_strequals(result,
                                        d_strnlen(result, 256),
                                        "aEXPANDEDb",
                                        10)) );
        if (result)
        {
            free(result);
        }

        d_str_interp_context_free(ctx);
    }

    /* --- contraction (value shorter than name) --- */
    ctx = d_str_interp_context_new();
    test_contraction = false;
    result = NULL;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "LONGNAME", "s");
        err = d_str_interp_alloc(ctx, "aLONGNAMEb", &result);
        test_contraction = ( (err == D_STR_INTERP_SUCCESS) &&
                             (result != NULL)              &&
                             (d_strequals(result,
                                          d_strnlen(result, 256),
                                          "asb",
                                          3)) );
        if (result)
        {
            free(result);
        }

        d_str_interp_context_free(ctx);
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_str_interp_alloc");

    D_TEST_CHECK_MSG(_suite,
                     "correct",
                     test_correct,
                     "allocates correct result");
    D_TEST_CHECK_MSG(_suite,
                     "null_term",
                     test_null_term,
                     "result is null-terminated");
    D_TEST_CHECK_MSG(_suite, "freeable", test_freeable, "result can be freed");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty input");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "no-match passes through");
    D_TEST_CHECK_MSG(_suite,
                     "null_param",
                     test_null_param,
                     "ERROR_NULL_PARAM for NULL inputs");
    D_TEST_CHECK_MSG(_suite,
                     "expansion",
                     test_expansion,
                     "value longer than name (expansion)");
    D_TEST_CHECK_MSG(_suite,
                     "contraction",
                     test_contraction,
                     "value shorter than name (contraction)");

    return true;
}

/*
d_tests_str_interp_length
  Tests d_str_interp_length for length calculation.
  Tests the following:
  - returns correct length for simple substitution
  - returns 0 for empty input
  - returns input length when no specifiers match
  - accounts for expansion (value longer than name)
  - accounts for contraction (value shorter than name)
  - returns ERROR_NULL_PARAM for NULL inputs
  - handles multiple replacements in length sum
*/
bool d_tests_str_interp_length(struct dj_test_suite* _suite)
{
    struct dj_str_interp_context* ctx;
    enum dj_str_interp_error      err;
    size_t                       len;
    bool                         test_simple;
    bool                         test_empty;
    bool                         test_no_match;
    bool                         test_expansion;
    bool                         test_contraction;
    bool                         test_null_param;
    bool                         test_multi;

    /* --- simple --- */
    ctx = d_str_interp_context_new();
    test_simple = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "AB", "XY");
        err = d_str_interp_length(ctx, "1AB2", &len);
        /* "1" + "XY" + "2" = 4 */
        test_simple = ( (err == D_STR_INTERP_SUCCESS) &&
                        (len == 4) );
        d_str_interp_context_free(ctx);
    }

    /* --- empty --- */
    ctx = d_str_interp_context_new();
    test_empty = false;
    if (ctx)
    {
        err = d_str_interp_length(ctx, "", &len);
        test_empty = ( (err == D_STR_INTERP_SUCCESS) &&
                       (len == 0) );
        d_str_interp_context_free(ctx);
    }

    /* --- no match --- */
    ctx = d_str_interp_context_new();
    test_no_match = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "ZZZ", "z");
        err = d_str_interp_length(ctx, "abcdef", &len);
        test_no_match = ( (err == D_STR_INTERP_SUCCESS) &&
                          (len == 6) );
        d_str_interp_context_free(ctx);
    }

    /* --- expansion --- */
    ctx = d_str_interp_context_new();
    test_expansion = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "X", "HUGE");
        err = d_str_interp_length(ctx, "aXb", &len);
        /* "a" + "HUGE" + "b" = 6 */
        test_expansion = ( (err == D_STR_INTERP_SUCCESS) &&
                           (len == 6) );
        d_str_interp_context_free(ctx);
    }

    /* --- contraction --- */
    ctx = d_str_interp_context_new();
    test_contraction = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "LONGKEY", "s");
        err = d_str_interp_length(ctx, "LONGKEY", &len);
        /* "s" = 1 */
        test_contraction = ( (err == D_STR_INTERP_SUCCESS) &&
                             (len == 1) );
        d_str_interp_context_free(ctx);
    }

    /* --- NULL params --- */
    err = d_str_interp_length(NULL, "abc", &len);
    test_null_param = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    /* --- multiple replacements --- */
    ctx = d_str_interp_context_new();
    test_multi = false;
    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "A", "xx");
        d_str_interp_add_specifier(ctx, "B", "yy");
        err = d_str_interp_length(ctx, "AzB", &len);
        /* "xx" + "z" + "yy" = 5 */
        test_multi = ( (err == D_STR_INTERP_SUCCESS) &&
                       (len == 5) );
        d_str_interp_context_free(ctx);
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_str_interp_length");

    D_TEST_CHECK_MSG(_suite,
                     "simple",
                     test_simple,
                     "correct length for simple case");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "returns 0 for empty input");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "returns input length when no match");
    D_TEST_CHECK_MSG(_suite,
                     "expansion",
                     test_expansion,
                     "accounts for expansion");
    D_TEST_CHECK_MSG(_suite,
                     "contraction",
                     test_contraction,
                     "accounts for contraction");
    D_TEST_CHECK_MSG(_suite,
                     "null_param",
                     test_null_param,
                     "ERROR_NULL_PARAM for NULL inputs");
    D_TEST_CHECK_MSG(_suite, "multi", test_multi, "sums multiple replacements");

    return true;
}

/*
d_tests_str_interp_interp_all
  Runs all interpolation function tests.
  Tests the following:
  - d_str_interp
  - d_str_interp_alloc
  - d_str_interp_length
*/
bool d_tests_str_interp_interp_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Interpolation Functions");

    d_tests_str_interp_interp(_suite);
    d_tests_str_interp_alloc(_suite);
    d_tests_str_interp_length(_suite);

    return true;
}
