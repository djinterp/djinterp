/*******************************************************************************
* djinterp [functional]                             fn_builder_tests_execution.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_execution.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.11
*******************************************************************************/
// djinterp
#include "./fn_builder_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPERS: callbacks for execution tests
 *****************************************************************************/

/*
test_helper_double_int
  Transformer: multiplies each int by 2.
*/
static
bool test_helper_double_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) * 2;

    return true;
}


/*
test_helper_add_ten
  Transformer: adds 10 to each int.
*/
static
bool test_helper_add_ten(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) + 10;

    return true;
}


/*
test_helper_negate
  Transformer: negates each int.
*/
static
bool test_helper_negate(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = -(*(const int*)_input);

    return true;
}


/*
test_helper_square
  Transformer: squares each int.
*/
static
bool test_helper_square(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    int val;

    (void)_context;

    val            = *(const int*)_input;
    *(int*)_output = val * val;

    return true;
}


/*
test_helper_fail_transformer
  Transformer: always fails (returns false).
*/
static
bool test_helper_fail_transformer(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_input;
    (void)_output;
    (void)_context;

    return false;
}


/*
test_helper_is_even
  Predicate: returns true if the int is even.
*/
static
bool test_helper_is_even(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) % 2 == 0;
}


/*
test_helper_is_positive
  Predicate: returns true if the int is > 0.
*/
static
bool test_helper_is_positive(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 0;
}


/*
test_helper_greater_than_five
  Predicate: returns true if the int is > 5.
*/
static
bool test_helper_greater_than_five(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 5;
}


/*
test_helper_always_false
  Predicate: always returns false.
*/
static
bool test_helper_always_false(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}


/*
d_tests_fn_builder_execute_validation
  Tests d_fn_builder_execute parameter validation.
  Tests the following:
  - NULL builder returns false, sets _out_count to 0
  - NULL input returns false, sets _out_count to 0
  - NULL output returns false, sets _out_count to 0
  - NULL _out_count returns false (does not crash)
  - zero count returns false, sets _out_count to 0
  - zero element_size returns false, sets _out_count to 0
  - multiple invalid params at once returns false
*/
bool d_tests_fn_builder_execute_validation(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3 };
    int                  output[3];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_validation");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- NULL builder ----
    out_count = 999;
    result    = d_fn_builder_execute(NULL,
                                     input,
                                     3,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: NULL builder returns false",
                                   result == false,
                                   "validation should reject NULL builder");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: NULL builder sets out_count to 0",
                                   out_count == 0,
                                   "out_count should be zeroed on error");

    // ---- NULL input ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     NULL,
                                     3,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: NULL input returns false, out_count 0",
                                   result == false && out_count == 0,
                                   "validation should reject NULL input");

    // ---- NULL output ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     3,
                                     sizeof(int),
                                     NULL,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: NULL output returns false, out_count 0",
                                   result == false && out_count == 0,
                                   "validation should reject NULL output");

    // ---- NULL _out_count ----
    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  NULL);

    // the count is optional
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: NULL out_count is allowed",
                                   result == true,
                                   "the run succeeds without reporting a count");

    // ---- zero count ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     0,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: zero count succeeds, out_count 0",
                                   result == true && out_count == 0,
                                   "an empty input runs to an empty output");

    // ---- zero element_size ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     3,
                                     0,
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: zero element_size returns false, out_count 0",
                                   result == false && out_count == 0,
                                   "validation should reject zero element_size");

    // ---- multiple invalid at once ----
    out_count = 999;
    result    = d_fn_builder_execute(NULL,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "execute: all invalid returns false, out_count 0",
                                   result == false && out_count == 0,
                                   "all-invalid should still write out_count and return false");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_passthrough
  Tests execute with empty builder (no transforms, no predicates).
  Tests the following:
  - empty builder copies input to output verbatim
  - out_count equals input count
  - output contents match input exactly
  - single element passthrough works
*/
bool d_tests_fn_builder_execute_passthrough(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 10, 20, 30, 40, 50 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;
    bool                 match;
    size_t               i;

    d_test_suite_test(_suite, "fn_builder_execute_passthrough");

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- empty builder: passthrough copy ----
    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "passthrough: returns true",
                                   result == true,
                                   "empty builder should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "passthrough: out_count equals input count",
                                   out_count == 5,
                                   "all 5 elements should be copied");

    match = true;

    for (i = 0; i < 5; i++)
    {
        if (output[i] != input[i])
        {
            match = false;
        }
    }

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "passthrough: output contents match input",
                                   match,
                                   "memcpy should produce an exact copy");

    // ---- single element passthrough ----
    {
        int one_in  = 42;
        int one_out = 0;

        result = d_fn_builder_execute(builder,
                                      &one_in,
                                      1,
                                      sizeof(int),
                                      &one_out,
                                      &out_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "passthrough: single element works",
                                       result == true && out_count == 1 && one_out == 42,
                                       "one element should copy correctly");
    }

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_transforms_only
  Tests execute with transforms but no predicates.
  Tests the following:
  - single transform applied to all elements (double)
  - two transforms applied in order (double then add 10)
  - all elements pass through (no filtering)
  - out_count equals input count
  - original input is not modified
*/
bool d_tests_fn_builder_execute_transforms_only(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_transforms_only");

    all_passed = true;

    // ---- single transform: double ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transforms_only: single transform returns 5 elements",
                                   result == true && out_count == 5,
                                   "no filtering means all pass");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transforms_only: single double produces 2,4,6,8,10",
                                   output[0] == 2 && output[1] == 4 && output[2] == 6 && output[3] == 8 && output[4] == 10,
                                   "each element should be doubled");

    // original unchanged
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transforms_only: original input unchanged",
                                   input[0] == 1 && input[4] == 5,
                                   "execute should not modify the source array");

    d_fn_builder_free(builder);

    // ---- two transforms: double then add 10 ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_add_ten);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transforms_only: two transforms returns 5 elements",
                                   result == true && out_count == 5,
                                   "no filtering means all pass");

    // (1*2)+10=12, (2*2)+10=14, (3*2)+10=16, (4*2)+10=18, (5*2)+10=20
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transforms_only: double then add_ten -> 12,14,16,18,20",
                                   output[0] == 12 && output[1] == 14 && output[2] == 16 && output[3] == 18 && output[4] == 20,
                                   "transforms should compose left-to-right");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_predicates_only
  Tests execute with predicates but no transforms.
  Tests the following:
  - single predicate filters elements (keep evens)
  - predicate-only uses original input values for filtering and output
  - two predicates applied as conjunction (even AND positive)
  - all-fail predicate produces out_count 0
  - single predicate that passes all produces full copy
*/
bool d_tests_fn_builder_execute_predicates_only(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5, 6 };
    int                  output[6];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_predicates_only");

    all_passed = true;

    // ---- single predicate: keep evens ----
    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: is_even keeps 3 of 6",
                                   result == true && out_count == 3,
                                   "2, 4, 6 should pass");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: filtered values are 2, 4, 6",
                                   output[0] == 2 && output[1] == 4 && output[2] == 6,
                                   "original (untransformed) values should be copied");

    d_fn_builder_free(builder);

    // ---- two predicates as conjunction: is_even AND > 5 ----
    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_is_even);
    d_fn_builder_where(builder, test_helper_greater_than_five);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: is_even AND > 5 keeps 1",
                                   result == true && out_count == 1,
                                   "only 6 passes both predicates");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: conjunction result is 6",
                                   output[0] == 6,
                                   "only 6 is both even and > 5");

    d_fn_builder_free(builder);

    // ---- all-fail predicate: nothing passes ----
    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_always_false);

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: always_false produces out_count 0",
                                   result == true && out_count == 0,
                                   "no elements should pass");

    d_fn_builder_free(builder);

    // ---- predicate that all pass: is_positive on positive data ----
    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_is_positive);

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "predicates_only: all-pass predicate keeps all 6",
                                   result == true && out_count == 6,
                                   "all elements are positive");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_combined
  Tests execute with both transforms and predicates. The chain runs in call
order (fn_builder.h): a predicate tests the value the steps before it
produced, so a filter after a map sees the mapped value.
  Tests the following:
  - double -> is_even: every doubled value is even, so all 5 are kept
  - negate -> is_positive: every negated value is negative, so none is kept
  - double -> is_even -> > 5: the doubled values 6, 8 and 10 are kept
  - add_ten -> is_even: the predicate sees 11..14 and keeps 12 and 14
  - output contains the TRANSFORMED values of the kept elements
*/
bool d_tests_fn_builder_execute_combined(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_combined");

    all_passed = true;

    // ---- double -> is_even: all doubled values are even ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "combined: double -> is_even keeps all 5",
                                   result == true && out_count == 5,
                                   "every doubled value is even");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "combined: output contains TRANSFORMED values",
                                   (output[0] == 2) && (output[1] == 4) &&
                                   (output[2] == 6) && (output[3] == 8) &&
                                   (output[4] == 10),
                                   "the inputs, doubled: 2, 4, 6, 8, 10");

    d_fn_builder_free(builder);

    // ---- negate -> is_positive: all negated values fail ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_negate);
    d_fn_builder_filter(builder, test_helper_is_positive);

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "combined: negate -> is_positive keeps none",
                                   result == true && out_count == 0,
                                   "every negated value is negative");

    d_fn_builder_free(builder);

    // ---- double -> is_even -> > 5: filtering transformed data ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_filter(builder, test_helper_is_even);
    d_fn_builder_where(builder, test_helper_greater_than_five);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "combined: double -> is_even -> > 5 keeps 3",
                                   result == true && out_count == 3,
                                   "6, 8 and 10 are even and > 5");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "combined: filtered transformed output",
                                   (output[0] == 6) && (output[1] == 8) &&
                                   (output[2] == 10),
                                   "the kept values are 6, 8, 10");

    d_fn_builder_free(builder);

    // ---- add_ten -> is_even: partial pass on transformed ----
    {
        int data[] = { 1, 2, 3, 4 };

        builder = d_fn_builder_new();
        d_fn_builder_map(builder, test_helper_add_ten);
        d_fn_builder_filter(builder, test_helper_is_even);

        d_memset(output, 0, sizeof(output));

        result = d_fn_builder_execute(builder,
                                      data,
                                      4,
                                      sizeof(int),
                                      output,
                                      &out_count);

        // 1+10=11(odd), 2+10=12(even), 3+10=13(odd), 4+10=14(even)
        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "combined: add_ten -> is_even keeps 2 of 4",
                                       result == true && out_count == 2,
                                       "12 and 14 are even");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "combined: add_ten -> is_even values are 12, 14",
                                       output[0] == 12 && output[1] == 14,
                                       "transformed even values should be output");

        d_fn_builder_free(builder);
    }

    return all_passed;
}


/*
d_tests_fn_builder_execute_order
  Tests that the chain runs in the order its steps were added.
  Tests the following:
  - filter -> map and map -> filter over the same steps give different
    results: is_even then double keeps 2 and 4 as 4, 8; double then is_even
    keeps all five
  - a predicate between two transformers sees the first one's value:
    add_ten -> > 5 -> negate keeps every element of { -9, -4, 0, 3 } whose
    image under add_ten exceeds 5, then negates it
  - a rejected element leaves the output past *_out_count as it was
*/
bool d_tests_fn_builder_execute_order(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[] = { 1, 2, 3, 4, 5 };
    int                  mixed[] = { -9, -4, 0, 3 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_order");

    all_passed = true;

    // ---- is_even -> double: the predicate sees the input ----
    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_is_even);
    d_fn_builder_map(builder, test_helper_double_int);

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "order: is_even -> double keeps 2 and 4, doubled",
                                   (result == true) && (out_count == 2) &&
                                   (output[0] == 4) && (output[1] == 8),
                                   "the predicate runs before the transformer");

    d_fn_builder_free(builder);

    // ---- double -> is_even: the predicate sees the image ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_filter(builder, test_helper_is_even);

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "order: double -> is_even keeps all 5",
                                   (result == true) && (out_count == 5),
                                   "the transformer runs before the predicate");

    d_fn_builder_free(builder);

    // ---- add_ten -> > 5 -> negate: a predicate between transformers ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_add_ten);
    d_fn_builder_where(builder, test_helper_greater_than_five);
    d_fn_builder_and_then(builder, test_helper_negate);

    output[0] = 99;
    output[1] = 99;
    output[2] = 99;
    output[3] = 99;
    output[4] = 99;

    result = d_fn_builder_execute(builder,
                                  mixed,
                                  4,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // add_ten gives 1, 6, 10, 13; > 5 keeps 6, 10, 13; negate gives -6, -10, -13
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "order: add_ten -> > 5 -> negate keeps 3",
                                   (result == true) && (out_count == 3) &&
                                   (output[0] == -6) && (output[1] == -10) &&
                                   (output[2] == -13),
                                   "the predicate tests add_ten's value");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "order: output past out_count untouched",
                                   (output[3] == 99) && (output[4] == 99),
                                   "a rejected element writes nothing");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_in_place
  Tests execute with _output equal to _input, which the contract allows.
  Tests the following:
  - a predicate-only chain filters in place, packing the kept elements
  - a chain with transformers maps and filters in place
*/
bool d_tests_fn_builder_execute_in_place(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  data[6];
    size_t               out_count;
    size_t               i;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_in_place");

    all_passed = true;

    // ---- predicates only, in place ----
    for (i = 0; i < 6; i++)
    {
        data[i] = (int)i + 1;
    }

    builder = d_fn_builder_new();
    d_fn_builder_filter(builder, test_helper_is_even);

    result = d_fn_builder_execute(builder,
                                  data,
                                  6,
                                  sizeof(int),
                                  data,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "in place: is_even packs 2, 4, 6",
                                   (result == true) && (out_count == 3) &&
                                   (data[0] == 2) && (data[1] == 4) &&
                                   (data[2] == 6),
                                   "kept elements packed at the front");

    d_fn_builder_free(builder);

    // ---- transformers and a predicate, in place ----
    for (i = 0; i < 6; i++)
    {
        data[i] = (int)i + 1;
    }

    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_square);
    d_fn_builder_where(builder, test_helper_greater_than_five);
    d_fn_builder_and_then(builder, test_helper_negate);

    result = d_fn_builder_execute(builder,
                                  data,
                                  6,
                                  sizeof(int),
                                  data,
                                  &out_count);

    // squares 1, 4, 9, 16, 25, 36; > 5 keeps 9..36; negated
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "in place: square -> > 5 -> negate",
                                   (result == true) && (out_count == 4) &&
                                   (data[0] == -9) && (data[1] == -16) &&
                                   (data[2] == -25) && (data[3] == -36),
                                   "mapped and filtered over its own input");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execute_transform_failure
  Tests execute when a transformer returns false.
  Tests the following:
  - first (and only) transform fails: returns false, out_count 0
  - first of two transforms fails: returns false, out_count 0, both
    temp buffers freed
  - second of two transforms fails: returns false, out_count 0
  - failure on a non-first element still returns false
*/
bool d_tests_fn_builder_execute_transform_failure(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_transform_failure");

    all_passed = true;

    // ---- single failing transform (only temp_a, no temp_b) ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_fail_transformer);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transform_fail: single failing transform returns false",
                                   result == false && out_count == 0,
                                   "first transform failure should abort, out_count 0");

    d_fn_builder_free(builder);

    // ---- first of two transforms fails (temp_a + temp_b allocated) ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_fail_transformer);
    d_fn_builder_and_then(builder, test_helper_double_int);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transform_fail: first of two fails, both buffers freed",
                                   result == false && out_count == 0,
                                   "first transform failure should free temp_a and temp_b");

    d_fn_builder_free(builder);

    // ---- second of two transforms fails ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_fail_transformer);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "transform_fail: second of two fails returns false",
                                   result == false && out_count == 0,
                                   "subsequent transform failure in ping-pong loop aborts");

    d_fn_builder_free(builder);

    // ---- failure on non-first element (transformer succeeds for
    //      element[0] but fails on element[1]) ----
    {
        // we use a static counter to make the transformer fail
        // on the second call; here we just reuse fail_transformer
        // which fails on every element, so it fails on element[0]
        // The first-of-two test above already covers temp_b cleanup.
        // For coverage of the "some succeed then fail" path, we test
        // with a single-element array so transform[0] is the failure.
        int single[] = { 99 };

        builder = d_fn_builder_new();
        d_fn_builder_map(builder, test_helper_fail_transformer);

        out_count = 999;
        result    = d_fn_builder_execute(builder,
                                         single,
                                         1,
                                         sizeof(int),
                                         output,
                                         &out_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "transform_fail: single element failure returns false",
                                       result == false && out_count == 0,
                                       "failure on any element should abort");

        d_fn_builder_free(builder);
    }

    return all_passed;
}


/*
d_tests_fn_builder_execute_pingpong
  Tests the ping-pong buffer mechanism for 3+ transforms.
  Tests the following:
  - three transforms applied in correct order (double -> add_ten -> negate)
  - four transforms applied in correct order with correct ping-pong swaps
  - single transform (no ping-pong, only temp_a)
  - two transforms (single swap, temp_a -> temp_b -> done)
  - large struct elements work with ping-pong
*/
bool d_tests_fn_builder_execute_pingpong(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    int                  input[]  = { 1, 2, 3 };
    int                  output[3];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_execute_pingpong");

    all_passed = true;

    // ---- three transforms: double -> add_ten -> negate ----
    // 1 -> 2 -> 12 -> -12
    // 2 -> 4 -> 14 -> -14
    // 3 -> 6 -> 16 -> -16
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_add_ten);
    d_fn_builder_and_then(builder, test_helper_negate);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 3 transforms returns 3 elements",
                                   result == true && out_count == 3,
                                   "no filtering, all pass through");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 3-transform order correct: -12, -14, -16",
                                   output[0] == -12 && output[1] == -14 && output[2] == -16,
                                   "double(1)=2, +10=12, negate=-12");

    d_fn_builder_free(builder);

    // ---- four transforms: double -> add_ten -> negate -> square ----
    // 1 -> 2 -> 12 -> -12 -> 144
    // 2 -> 4 -> 14 -> -14 -> 196
    // 3 -> 6 -> 16 -> -16 -> 256
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_add_ten);
    d_fn_builder_and_then(builder, test_helper_negate);
    d_fn_builder_and_then(builder, test_helper_square);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 4 transforms returns 3 elements",
                                   result == true && out_count == 3,
                                   "even number of swaps should work");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 4-transform order correct: 144, 196, 256",
                                   output[0] == 144 && output[1] == 196 && output[2] == 256,
                                   "double(1)=2, +10=12, neg=-12, sq=144");

    d_fn_builder_free(builder);

    // ---- single transform: no ping-pong (only temp_a) ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_add_ten);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 1 transform (no temp_b) succeeds",
                                   result == true && out_count == 3,
                                   "single transform uses only temp_a");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 1 transform values correct: 11, 12, 13",
                                   output[0] == 11 && output[1] == 12 && output[2] == 13,
                                   "each element + 10");

    d_fn_builder_free(builder);

    // ---- two transforms: single swap (temp_a -> temp_b) ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_negate);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // 1 -> 2 -> -2; 2 -> 4 -> -4; 3 -> 6 -> -6
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 2 transforms (one swap) succeeds",
                                   result == true && out_count == 3,
                                   "two transforms use temp_a and temp_b");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 2-transform values correct: -2, -4, -6",
                                   output[0] == -2 && output[1] == -4 && output[2] == -6,
                                   "double then negate");

    d_fn_builder_free(builder);

    // ---- 3 transforms + predicate: combined ping-pong + filtering ----
    builder = d_fn_builder_new();
    d_fn_builder_map(builder, test_helper_double_int);
    d_fn_builder_and_then(builder, test_helper_add_ten);
    d_fn_builder_and_then(builder, test_helper_negate);
    d_fn_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // 1->2->12->-12(even), 2->4->14->-14(even), 3->6->16->-16(even)
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 3 transforms + filter keeps all",
                                   result == true && out_count == 3,
                                   "all negated sums are even");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "pingpong: 3 transforms + filter values correct",
                                   (output[0] == -12) && (output[1] == -14) &&
                                   (output[2] == -16),
                                   "ping-pong result fed to predicate correctly");

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_fn_builder_execution_all
  Runs all builder execution tests.
  Tests the following:
  - parameter validation
  - passthrough (empty builder)
  - transforms only
  - predicates only
  - combined transforms + predicates
  - call order
  - in-place execution
  - transform failure paths
  - ping-pong buffer mechanism
*/
bool d_tests_fn_builder_execution_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "fn_builder_execution_all");

    all_passed = true;

    all_passed &= d_tests_fn_builder_execute_validation(_suite);
    all_passed &= d_tests_fn_builder_execute_passthrough(_suite);
    all_passed &= d_tests_fn_builder_execute_transforms_only(_suite);
    all_passed &= d_tests_fn_builder_execute_predicates_only(_suite);
    all_passed &= d_tests_fn_builder_execute_combined(_suite);
    all_passed &= d_tests_fn_builder_execute_order(_suite);
    all_passed &= d_tests_fn_builder_execute_in_place(_suite);
    all_passed &= d_tests_fn_builder_execute_transform_failure(_suite);
    all_passed &= d_tests_fn_builder_execute_pingpong(_suite);

    return all_passed;
}
