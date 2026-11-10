/*******************************************************************************
* djinterp [functional]                              compose_tests_transformer.c
*
*
* path:      /tests/djinterp/c/functional/compose_tests_transformer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./compose_tests.h"
#include "./compose_tests_helpers.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_compose_new
  Tests the d_functional_compose_new constructor function.
  Tests the following:
  - NULL first transformer rejection
  - NULL second transformer rejection
  - both NULL transformers rejection
  - zero temp_size rejection
  - successful creation with valid parameters and no context
  - field initialization verification (first, second, contexts, temp_size,
    temp_buf)
  - successful creation with both contexts
  - large temp_size allocation
  - mixed context handling (one NULL, one non-NULL)
*/
bool d_tests_compose_new(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    struct dj_composed_transformer* composed;
    int                            context1;
    int                            context2;

    d_test_suite_test(_suite, "compose_new");

    result   = true;
    context1 = 5;
    context2 = 3;

    // test 1: NULL first transformer should return NULL
    composed = d_functional_compose_new(NULL,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_null_first",
                                composed == NULL,
                                "NULL first transformer should return NULL") && result;

    // test 2: NULL second transformer should return NULL
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        NULL,
                                        NULL,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_null_second",
                                composed == NULL,
                                "NULL second transformer should return NULL") && result;

    // test 3: both NULL transformers should return NULL
    composed = d_functional_compose_new(NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_both_null",
                                composed == NULL,
                                "Both NULL transformers should return NULL") && result;

    // test 4: zero temp_size should return NULL
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_zero_temp_size",
                                composed == NULL,
                                "Zero temp_size should return NULL") && result;

    // test 5: successful creation with no contexts
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_success_no_context",
                                composed != NULL,
                                "d_functional_compose_new should succeed with NULL contexts") && result;

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_first_set",
                                  composed->first == transform_double,
                                  "first transformer should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_second_set",
                                  composed->second == transform_add_10,
                                  "second transformer should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_contexts_null",
                                  composed->context1 == NULL && composed->context2 == NULL,
                                  "contexts should be NULL") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_temp_size_set",
                                  composed->temp_size == sizeof(int),
                                  "temp_size should be sizeof(int)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_temp_buf_allocated",
                                  composed->temp_buf != NULL,
                                  "temp_buf should be allocated") && result;

        d_functional_compose_free(composed);
    }

    // test 6: successful creation with contexts
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_success_with_context",
                                composed != NULL,
                                "d_functional_compose_new should succeed with contexts") && result;

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_context_first_set",
                                  composed->first == transform_multiply_by_context,
                                  "first transformer should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_context1_set",
                                  composed->context1 == &context1,
                                  "context1 should point to context1 variable") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_context2_set",
                                  composed->context2 == &context2,
                                  "context2 should point to context2 variable") && result;

        d_functional_compose_free(composed);
    }

    // test 7: successful creation with large temp_size
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        1024);
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_large_temp_size",
                                composed != NULL,
                                "d_functional_compose_new with large temp_size should succeed") && result;

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_large_temp_size_set",
                                  composed->temp_size == 1024,
                                  "temp_size should be 1024") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_large_temp_buf_allocated",
                                  composed->temp_buf != NULL,
                                  "temp_buf should be allocated for large size") && result;

        d_functional_compose_free(composed);
    }

    // test 8: creation with mixed context (one NULL, one non-NULL)
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "compose_new_mixed_context",
                                composed != NULL,
                                "d_functional_compose_new with mixed contexts should succeed") && result;

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_mixed_context1_null",
                                  composed->context1 == NULL,
                                  "context1 should be NULL") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_new_mixed_context2_set",
                                  composed->context2 == &context2,
                                  "context2 should be set") && result;

        d_functional_compose_free(composed);
    }

    return result;
}


/*
d_tests_compose_apply
  Tests the d_functional_compose_apply function.
  Tests the following:
  - NULL composed rejection
  - NULL input rejection
  - NULL output rejection
  - NULL first transformer in struct rejection
  - NULL second transformer in struct rejection
  - NULL temp_buf in struct rejection
  - successful composition (double then add 10)
  - correct order of operations (add 10 then double vs double then add 10)
  - context forwarding to first transformer
  - context forwarding to second transformer
  - both contexts simultaneously
  - first transformer failure handling
  - second transformer failure handling
  - multiple applications with same composed transformer (reuse)
  - zero boundary value
  - negative input value
  - identity composition
*/
bool d_tests_compose_apply(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    struct dj_composed_transformer* composed;
    int                            input;
    int                            output;
    int                            context1;
    int                            context2;

    d_test_suite_test(_suite, "compose_apply");

    result   = true;
    context1 = 3;
    context2 = 7;

    // test 1: NULL composed should return false
    input  = 5;
    output = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "compose_apply_null_composed",
                              d_functional_compose_apply(NULL, &input, &output) == false,
                              "NULL composed should return false") && result;

    // test 2: NULL input should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_null_input",
                                  d_functional_compose_apply(composed, NULL, &output) == false,
                                  "NULL input should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 3: NULL output should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_null_output",
                                  d_functional_compose_apply(composed, &input, NULL) == false,
                                  "NULL output should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 4: NULL first transformer in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // corrupt the first field
        composed->first = NULL;
        input            = 5;
        output           = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_null_first_in_struct",
                                  d_functional_compose_apply(composed, &input, &output) == false,
                                  "NULL first in struct should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 5: NULL second transformer in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // corrupt the second field
        composed->second = NULL;
        input             = 5;
        output            = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_null_second_in_struct",
                                  d_functional_compose_apply(composed, &input, &output) == false,
                                  "NULL second in struct should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 6: NULL temp_buf in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // save and corrupt temp_buf, restore for proper free
        void* saved_buf    = composed->temp_buf;
        composed->temp_buf = NULL;
        input              = 5;
        output             = 999;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_null_temp_buf",
                                  d_functional_compose_apply(composed, &input, &output) == false,
                                  "NULL temp_buf should return false") && result;

        // restore for clean free
        composed->temp_buf = saved_buf;
        d_functional_compose_free(composed);
    }

    // test 7: successful composition - double then add 10
    // input: 5 -> double -> 10 -> add 10 -> 20
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_double_add10_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Composition should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_double_add10_result",
                                  output == 20,
                                  "5 doubled then +10 should equal 20") && result;

        d_functional_compose_free(composed);
    }

    // test 8: verify order matters - add 10 then double
    // input: 5 -> add 10 -> 15 -> double -> 30
    composed = d_functional_compose_new(transform_add_10,
                                        NULL,
                                        transform_double,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_add10_double_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Reverse composition should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_add10_double_result",
                                  output == 30,
                                  "5 +10 then doubled should equal 30") && result;

        d_functional_compose_free(composed);
    }

    // test 9: context forwarding to first transformer
    // input: 4 -> multiply by 3 -> 12 -> add 10 -> 22
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 4;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_context1_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Composition with context1 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_context1_result",
                                  output == 22,
                                  "4 * 3 + 10 should equal 22") && result;

        d_functional_compose_free(composed);
    }

    // test 10: context forwarding to second transformer
    // input: 5 -> double -> 10 -> add 7 -> 17
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_context2_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Composition with context2 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_context2_result",
                                  output == 17,
                                  "(5 * 2) + 7 should equal 17") && result;

        d_functional_compose_free(composed);
    }

    // test 11: both contexts
    // input: 2 -> multiply by 3 -> 6 -> add 7 -> 13
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));

    if (composed)
    {
        input  = 2;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_both_contexts_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Composition with both contexts should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_both_contexts_result",
                                  output == 13,
                                  "(2 * 3) + 7 should equal 13") && result;

        d_functional_compose_free(composed);
    }

    // test 12: first transformer failure
    composed = d_functional_compose_new(transform_always_fails,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_first_fails",
                                  d_functional_compose_apply(composed, &input, &output) == false,
                                  "First transformer failure should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 13: second transformer failure
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_always_fails,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_second_fails",
                                  d_functional_compose_apply(composed, &input, &output) == false,
                                  "Second transformer failure should return false") && result;

        d_functional_compose_free(composed);
    }

    // test 14: multiple applications with same composed transformer
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // first application: 3 -> 6 -> 16
        input  = 3;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_first",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "First application should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_first_result",
                                  output == 16,
                                  "3 doubled +10 should equal 16") && result;

        // second application: 7 -> 14 -> 24
        input  = 7;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_second",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Second application should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_second_result",
                                  output == 24,
                                  "7 doubled +10 should equal 24") && result;

        // third application: 0 -> 0 -> 10 (verifies temp buffer is clean)
        input  = 0;
        output = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_third",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Third application with 0 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_reuse_third_result",
                                  output == 10,
                                  "0 doubled +10 should equal 10 (no temp buffer leakage)") && result;

        d_functional_compose_free(composed);
    }

    // test 15: zero boundary value
    // input: 0 -> double -> 0 -> add 10 -> 10
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 0;
        output = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_zero_boundary_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Zero input should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_zero_boundary_result",
                                  output == 10,
                                  "0 doubled +10 should equal 10") && result;

        d_functional_compose_free(composed);
    }

    // test 16: negative input value
    // input: -3 -> double -> -6 -> add 10 -> 4
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = -3;
        output = 999;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_negative_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Negative input should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_negative_result",
                                  output == 4,
                                  "-3 doubled +10 should equal 4") && result;

        d_functional_compose_free(composed);
    }

    // test 17: identity composition (identity . identity = identity)
    composed = d_functional_compose_new(transform_identity,
                                        NULL,
                                        transform_identity,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 42;
        output = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_identity_success",
                                  d_functional_compose_apply(composed, &input, &output) == true,
                                  "Identity composition should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_apply_identity_result",
                                  output == 42,
                                  "identity(identity(42)) should equal 42") && result;

        d_functional_compose_free(composed);
    }

    // test 18: three-stage composition via nesting
    // input: 3 -> square -> 9 -> double -> 18
    {
        struct dj_composed_transformer* inner;

        inner = d_functional_compose_new(transform_square,
                                         NULL,
                                         transform_double,
                                         NULL,
                                         sizeof(int));

        if (inner)
        {
            // verify inner works: 3 -> 9 -> 18
            input  = 3;
            output = 0;
            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_apply_nested_inner_success",
                                      d_functional_compose_apply(inner, &input, &output) == true,
                                      "Inner composition (square then double) should succeed") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_apply_nested_inner_result",
                                      output == 18,
                                      "3 squared then doubled should equal 18") && result;

            d_functional_compose_free(inner);
        }
    }

    return result;
}


/*
d_tests_compose_free
  Tests the d_functional_compose_free function.
  Tests the following:
  - NULL pointer handling (should not crash)
  - successful free after creation
  - free with contexts (contexts should remain valid)
  - free after use (contexts should remain valid)
*/
bool d_tests_compose_free(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    struct dj_composed_transformer* composed;

    d_test_suite_test(_suite, "compose_free");

    result = true;

    // test 1: NULL pointer should not crash
    d_functional_compose_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "compose_free_null",
                              true,
                              "Freeing NULL should not crash") && result;

    // test 2: normal free after creation
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        d_functional_compose_free(composed);
        result = D_TEST_CHECK_MSG(_suite,
                                  "compose_free_normal",
                                  true,
                                  "Normal free should succeed") && result;
    }

    // test 3: free with contexts (contexts should not be freed)
    {
        int ctx1 = 5;
        int ctx2 = 10;

        composed = d_functional_compose_new(transform_multiply_by_context,
                                            &ctx1,
                                            transform_add_context,
                                            &ctx2,
                                            sizeof(int));

        if (composed)
        {
            d_functional_compose_free(composed);
            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_free_with_contexts",
                                      true,
                                      "Free with contexts should succeed") && result;

            // contexts should still be valid
            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_free_contexts_intact",
                                      ctx1 == 5 && ctx2 == 10,
                                      "Contexts should not be freed/modified") && result;
        }
    }

    // test 4: free after use (contexts should remain valid)
    {
        int ctx1 = 3;
        int ctx2 = 7;
        int input;
        int output;

        composed = d_functional_compose_new(transform_multiply_by_context,
                                            &ctx1,
                                            transform_add_context,
                                            &ctx2,
                                            sizeof(int));

        if (composed)
        {
            // use before freeing
            input  = 4;
            output = 0;
            d_functional_compose_apply(composed, &input, &output);

            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_free_after_use_result",
                                      output == 19,
                                      "(4 * 3) + 7 should equal 19 before free") && result;

            d_functional_compose_free(composed);

            // contexts should still be valid after free
            result = D_TEST_CHECK_MSG(_suite,
                                      "compose_free_after_use_contexts_intact",
                                      ctx1 == 3 && ctx2 == 7,
                                      "Contexts should remain valid after use and free") && result;
        }
    }

    return result;
}


/*
d_tests_compose_transformer_all
  Aggregation function that runs all transformer composition tests.
*/
bool d_tests_compose_transformer_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "compose_transformer_all");

    result = true;

    result = d_tests_compose_new(_suite)   && result;
    result = d_tests_compose_apply(_suite)  && result;
    result = d_tests_compose_free(_suite)   && result;

    return result;
}
