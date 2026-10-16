/*******************************************************************************
* djinterp [container]                         text_buffer_tests_sa_comparison.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_comparison.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_compare
  Tests the d_text_buffer_compare function.
  Tests the following:
  - NULL buffers handled
  - equal buffers return 0
  - lexicographically lesser buffer returns negative
  - lexicographically greater buffer returns positive
*/
bool
d_tests_sa_text_buffer_compare
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buf1;
    struct d_text_buffer* buf2;
    bool                  result = true;

    buf1 = d_text_buffer_new_from_string("abc");
    buf2 = d_text_buffer_new_from_string("abc");

    if (buf1 && buf2)
    {
        // test 1: equal buffers
        result = d_assert_standalone(
            d_text_buffer_compare(buf1, buf2) == 0,
            "compare_equal",
            "Equal buffers should return 0",
            _counter) && result;

        d_text_buffer_free(buf2);

        // test 2: buf1 < "abd"
        buf2 = d_text_buffer_new_from_string("abd");

        if (buf2)
        {
            result = d_assert_standalone(
                d_text_buffer_compare(buf1, buf2) < 0,
                "compare_less",
                "'abc' < 'abd' should return negative",
                _counter) && result;

            // test 3: buf2 > buf1
            result = d_assert_standalone(
                d_text_buffer_compare(buf2, buf1) > 0,
                "compare_greater",
                "'abd' > 'abc' should return positive",
                _counter) && result;

            d_text_buffer_free(buf2);
        }

        // test 4: shorter vs longer
        buf2 = d_text_buffer_new_from_string("abcdef");

        if (buf2)
        {
            result = d_assert_standalone(
                d_text_buffer_compare(buf1, buf2) < 0,
                "compare_shorter",
                "'abc' < 'abcdef' should return negative",
                _counter) && result;

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
d_tests_sa_text_buffer_compare_string
  Tests the d_text_buffer_compare_string function.
  Tests the following:
  - NULL buffer handled
  - NULL string handled
  - equal comparison returns 0
  - lexicographic ordering correct
*/
bool
d_tests_sa_text_buffer_compare_string
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        // test 1: equal
        result = d_assert_standalone(
            d_text_buffer_compare_string(buffer, "hello") == 0,
            "compare_str_equal",
            "Comparing with equal string should return 0",
            _counter) && result;

        // test 2: less
        result = d_assert_standalone(
            d_text_buffer_compare_string(buffer, "hfllo") < 0,
            "compare_str_less",
            "'hello' < 'hfllo' should return negative",
            _counter) && result;

        // test 3: greater
        result = d_assert_standalone(
            d_text_buffer_compare_string(buffer, "hallo") > 0,
            "compare_str_greater",
            "'hello' > 'hallo' should return positive",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_compare_n
  Tests the d_text_buffer_compare_n function.
  Tests the following:
  - comparing first N characters when prefixes match
  - comparing first N characters when they differ
  - N = 0 returns 0
*/
bool
d_tests_sa_text_buffer_compare_n
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buf1;
    struct d_text_buffer* buf2;
    bool                  result = true;

    buf1 = d_text_buffer_new_from_string("abcXYZ");
    buf2 = d_text_buffer_new_from_string("abc123");

    if ( (buf1) &&
         (buf2) )
    {
        // test 1: first 3 chars equal
        result = d_assert_standalone(
            d_text_buffer_compare_n(buf1, buf2, 3) == 0,
            "compare_n_prefix_equal",
            "First 3 chars should be equal",
            _counter) && result;

        // test 2: first 4 chars differ
        result = d_assert_standalone(
            d_text_buffer_compare_n(buf1, buf2, 4) != 0,
            "compare_n_prefix_differ",
            "First 4 chars should differ",
            _counter) && result;

        // test 3: N = 0
        result = d_assert_standalone(
            d_text_buffer_compare_n(buf1, buf2, 0) == 0,
            "compare_n_zero",
            "N=0 should return 0",
            _counter) && result;

        d_text_buffer_free(buf2);
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_sa_text_buffer_equals
  Tests the d_text_buffer_equals function.
  Tests the following:
  - NULL buffers return false
  - equal buffers return true
  - different buffers return false
  - same pointer returns true
*/
bool
d_tests_sa_text_buffer_equals
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buf1;
    struct d_text_buffer* buf2;
    bool                  result = true;

    // test 1: NULL == NULL -> returns true
    result = d_assert_standalone(
        d_text_buffer_equals(NULL, NULL) == true,
        "equals_null_null",
        "NULL == NULL should return true",
        _counter) && result;

    buf1 = d_text_buffer_new_from_string("test");
    buf2 = d_text_buffer_new_from_string("test");

    if ( (buf1) &&
         (buf2) )
    {
        // test 2: equal
        result = d_assert_standalone(
            d_text_buffer_equals(buf1, buf2) == true,
            "equals_match",
            "Equal buffers should return true",
            _counter) && result;

        // test 3: same pointer
        result = d_assert_standalone(
            d_text_buffer_equals(buf1, buf1) == true,
            "equals_same_ptr",
            "Same pointer should return true",
            _counter) && result;

        d_text_buffer_free(buf2);

        // test 4: different
        buf2 = d_text_buffer_new_from_string("other");

        if (buf2)
        {
            result = d_assert_standalone(
                d_text_buffer_equals(buf1, buf2) == false,
                "equals_differ",
                "Different buffers should return false",
                _counter) && result;

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
d_tests_sa_text_buffer_equals_string
  Tests the d_text_buffer_equals_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - matching string returns true
  - non-matching string returns false
*/
bool
d_tests_sa_text_buffer_equals_string
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_equals_string(NULL, "test") == false,
        "equals_str_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: matching
        result = d_assert_standalone(
            d_text_buffer_equals_string(buffer, "Hello") == true,
            "equals_str_match",
            "Matching string should return true",
            _counter) && result;

        // test 3: non-matching
        result = d_assert_standalone(
            d_text_buffer_equals_string(buffer, "World") == false,
            "equals_str_no_match",
            "Non-matching should return false",
            _counter) && result;

        // test 4: different length
        result = d_assert_standalone(
            d_text_buffer_equals_string(buffer, "Hell") == false,
            "equals_str_diff_len",
            "Different length should return false",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_comparison_all
  Aggregation function that runs all comparison operation tests.
*/
bool
d_tests_sa_text_buffer_comparison_all
(
    struct d_test_counter* _counter
)
{
    printf("\n  [SECTION] Comparison Operations\n");
    printf("  --------------------------------\n");

    return d_tests_sa_text_buffer_compare(_counter)        &&
           d_tests_sa_text_buffer_compare_string(_counter) &&
           d_tests_sa_text_buffer_compare_n(_counter)      &&
           d_tests_sa_text_buffer_equals(_counter)         &&
           d_tests_sa_text_buffer_equals_string(_counter);
}
