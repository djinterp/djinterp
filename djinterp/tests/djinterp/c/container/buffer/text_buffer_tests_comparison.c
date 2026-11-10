/*******************************************************************************
* djinterp [container]                            text_buffer_tests_comparison.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_comparison.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_compare
  Tests the d_text_buffer_compare function.
  Tests the following:
  - NULL buffers handled
  - equal buffers return 0
  - lexicographically lesser buffer returns negative
  - lexicographically greater buffer returns positive
*/
bool d_tests_text_buffer_compare(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buf1;
    struct dj_text_buffer* buf2;
    bool                  result = true;

    d_test_suite_test(_suite, "text_buffer_compare");

    buf1 = d_text_buffer_new_from_string("abc");
    buf2 = d_text_buffer_new_from_string("abc");

    if (buf1 && buf2)
    {
        // test 1: equal buffers
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_equal",
                                  d_text_buffer_compare(buf1, buf2) == 0,
                                  "Equal buffers should return 0") && result;

        d_text_buffer_free(buf2);

        // test 2: buf1 < "abd"
        buf2 = d_text_buffer_new_from_string("abd");

        if (buf2)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "compare_less",
                                      d_text_buffer_compare(buf1, buf2) < 0,
                                      "'abc' < 'abd' should return negative") && result;

            // test 3: buf2 > buf1
            result = D_TEST_CHECK_MSG(_suite,
                                      "compare_greater",
                                      d_text_buffer_compare(buf2, buf1) > 0,
                                      "'abd' > 'abc' should return positive") && result;

            d_text_buffer_free(buf2);
        }

        // test 4: shorter vs longer
        buf2 = d_text_buffer_new_from_string("abcdef");

        if (buf2)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "compare_shorter",
                                      d_text_buffer_compare(buf1, buf2) < 0,
                                      "'abc' < 'abcdef' should return negative") && result;

            d_text_buffer_free(buf2);
        }
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_text_buffer_compare_string
  Tests the d_text_buffer_compare_string function.
  Tests the following:
  - NULL buffer handled
  - NULL string handled
  - equal comparison returns 0
  - lexicographic ordering correct
*/
bool d_tests_text_buffer_compare_string(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    d_test_suite_test(_suite, "text_buffer_compare_string");

    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        // test 1: equal
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_str_equal",
                                  d_text_buffer_compare_string(buffer, "hello") == 0,
                                  "Comparing with equal string should return 0") && result;

        // test 2: less
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_str_less",
                                  d_text_buffer_compare_string(buffer, "hfllo") < 0,
                                  "'hello' < 'hfllo' should return negative") && result;

        // test 3: greater
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_str_greater",
                                  d_text_buffer_compare_string(buffer, "hallo") > 0,
                                  "'hello' > 'hallo' should return positive") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_compare_n
  Tests the d_text_buffer_compare_n function.
  Tests the following:
  - comparing first N characters when prefixes match
  - comparing first N characters when they differ
  - N = 0 returns 0
*/
bool d_tests_text_buffer_compare_n(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buf1;
    struct dj_text_buffer* buf2;
    bool                  result = true;

    d_test_suite_test(_suite, "text_buffer_compare_n");

    buf1 = d_text_buffer_new_from_string("abcXYZ");
    buf2 = d_text_buffer_new_from_string("abc123");

    if ( (buf1) &&
         (buf2) )
    {
        // test 1: first 3 chars equal
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_n_prefix_equal",
                                  d_text_buffer_compare_n(buf1, buf2, 3) == 0,
                                  "First 3 chars should be equal") && result;

        // test 2: first 4 chars differ
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_n_prefix_differ",
                                  d_text_buffer_compare_n(buf1, buf2, 4) != 0,
                                  "First 4 chars should differ") && result;

        // test 3: N = 0
        result = D_TEST_CHECK_MSG(_suite,
                                  "compare_n_zero",
                                  d_text_buffer_compare_n(buf1, buf2, 0) == 0,
                                  "N=0 should return 0") && result;

        d_text_buffer_free(buf2);
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_text_buffer_equals
  Tests the d_text_buffer_equals function.
  Tests the following:
  - NULL buffers return false
  - equal buffers return true
  - different buffers return false
  - same pointer returns true
*/
bool d_tests_text_buffer_equals(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buf1;
    struct dj_text_buffer* buf2;
    bool                  result = true;

    // test 1: NULL == NULL -> returns true
    d_test_suite_test(_suite, "text_buffer_equals");

    result = D_TEST_CHECK_MSG(_suite,
                              "equals_null_null",
                              d_text_buffer_equals(NULL, NULL) == true,
                              "NULL == NULL should return true") && result;

    buf1 = d_text_buffer_new_from_string("test");
    buf2 = d_text_buffer_new_from_string("test");

    if ( (buf1) &&
         (buf2) )
    {
        // test 2: equal
        result = D_TEST_CHECK_MSG(_suite,
                                  "equals_match",
                                  d_text_buffer_equals(buf1, buf2) == true,
                                  "Equal buffers should return true") && result;

        // test 3: same pointer
        result = D_TEST_CHECK_MSG(_suite,
                                  "equals_same_ptr",
                                  d_text_buffer_equals(buf1, buf1) == true,
                                  "Same pointer should return true") && result;

        d_text_buffer_free(buf2);

        // test 4: different
        buf2 = d_text_buffer_new_from_string("other");

        if (buf2)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "equals_differ",
                                      d_text_buffer_equals(buf1, buf2) == false,
                                      "Different buffers should return false") && result;

            d_text_buffer_free(buf2);
        }
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_text_buffer_equals_string
  Tests the d_text_buffer_equals_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - matching string returns true
  - non-matching string returns false
*/
bool d_tests_text_buffer_equals_string(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_equals_string");

    result = D_TEST_CHECK_MSG(_suite,
                              "equals_str_null_buf",
                              d_text_buffer_equals_string(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: matching
        result = D_TEST_CHECK_MSG(_suite,
                                  "equals_str_match",
                                  d_text_buffer_equals_string(buffer, "Hello") == true,
                                  "Matching string should return true") && result;

        // test 3: non-matching
        result = D_TEST_CHECK_MSG(_suite,
                                  "equals_str_no_match",
                                  d_text_buffer_equals_string(buffer, "World") == false,
                                  "Non-matching should return false") && result;

        // test 4: different length
        result = D_TEST_CHECK_MSG(_suite,
                                  "equals_str_diff_len",
                                  d_text_buffer_equals_string(buffer, "Hell") == false,
                                  "Different length should return false") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_comparison_all
  Aggregation function that runs all comparison operation tests.
*/
bool d_tests_text_buffer_comparison_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "text_buffer_comparison_all");

    return d_tests_text_buffer_compare(_suite)        &&
           d_tests_text_buffer_compare_string(_suite) &&
           d_tests_text_buffer_compare_n(_suite)      &&
           d_tests_text_buffer_equals(_suite)         &&
           d_tests_text_buffer_equals_string(_suite);
}
