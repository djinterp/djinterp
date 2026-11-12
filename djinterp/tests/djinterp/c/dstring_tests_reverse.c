/*******************************************************************************
* djinterp [c]                                           dstring_tests_reverse.c
*
*   Unit tests for dj_string reversal functions:
*     - d_string_reverse
*     - d_string_reversed
*
*
* path:      /tests/djinterp/c/dstring_tests_reverse.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "dstring_tests.h"


/******************************************************************************
* d_tests_dstring_reverse
******************************************************************************/

/*
d_tests_dstring_reverse
  Tests d_string_reverse() which reverses a string in-place.

Test cases:
  1.  NULL string returns false
  2.  Empty string (no-op, should succeed)
  3.  Single character string (no change)
  4.  Two character string
  5.  Odd-length string
  6.  Even-length string
  7.  Palindrome (should equal itself after reverse)
  8.  String with spaces
  9.  String with special characters
  10. String with numbers
  11. Double reverse returns original
  12. Long string reversal
  13. Unicode-like bytes (high ASCII)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_reverse(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;
    char                  original[256];

    d_test_suite_test(_suite, "d_string_reverse");

    // test 1: NULL string returns false
    result = d_string_reverse(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_false",
                     !(result),
                     "d_string_reverse(NULL) should return false");

    // test 2: empty string (no-op)
    str = d_string_new();

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "empty_string_succeeds",
                         result,
                         "reverse empty string should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "empty_string_still_empty",
                         (str->size) == (0),
                         "empty string should remain empty after reverse");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "empty_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "empty_size_skipped", false, "skipped");
    }

    // test 3: single character string (no change)
    str = d_string_new_from_cstr("X");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "single_char_succeeds",
                         result,
                         "reverse single char should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "single_char_unchanged",
                         d_test_suite_str_equal((str->text), ("X")),
                         "single char should be unchanged after reverse");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "single_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "single_content_skipped", false, "skipped");
    }

    // test 4: two character string
    str = d_string_new_from_cstr("AB");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "two_char_succeeds",
                         result,
                         "reverse two char string should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "two_char_reversed",
                         d_test_suite_str_equal((str->text), ("BA")),
                         "'AB' should become 'BA'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "two_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "two_content_skipped", false, "skipped");
    }

    // test 5: odd-length string
    str = d_string_new_from_cstr("ABCDE");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "odd_length_succeeds",
                         result,
                         "reverse odd-length string should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "odd_length_reversed",
                         d_test_suite_str_equal((str->text), ("EDCBA")),
                         "'ABCDE' should become 'EDCBA'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "odd_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "odd_content_skipped", false, "skipped");
    }

    // test 6: even-length string
    str = d_string_new_from_cstr("ABCDEF");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "even_length_reversed",
                         d_test_suite_str_equal((str->text), ("FEDCBA")),
                         "'ABCDEF' should become 'FEDCBA'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "even_skipped", false, "skipped");
    }

    // test 7: palindrome (should equal itself)
    str = d_string_new_from_cstr("racecar");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "palindrome_unchanged",
                         d_test_suite_str_equal((str->text), ("racecar")),
                         "palindrome should equal itself after reverse");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "palindrome_skipped", false, "skipped");
    }

    // test 8: string with spaces
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "spaces_reversed",
                         d_test_suite_str_equal((str->text), ("dlroW olleH")),
                         "'Hello World' should become 'dlroW olleH'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "spaces_skipped", false, "skipped");
    }

    // test 9: string with special characters
    str = d_string_new_from_cstr("a!b@c#d$");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "special_chars_reversed",
                         d_test_suite_str_equal((str->text), ("$d#c@b!a")),
                         "special characters should be reversed correctly");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "special_skipped", false, "skipped");
    }

    // test 10: string with numbers
    str = d_string_new_from_cstr("12345");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "numbers_reversed",
                         d_test_suite_str_equal((str->text), ("54321")),
                         "'12345' should become '54321'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "numbers_skipped", false, "skipped");
    }

    // test 11: double reverse returns original
    const char STR_TESTING[] = "Testing";
    str = d_string_new_from_cstr(STR_TESTING);

    if (str)
    {
        // save original
        d_strcpy_s(original, sizeof(original), str->text);

        // reverse twice
        d_string_reverse(str);
        d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "double_reverse_original",
                         d_test_suite_str_equal((str->text), (original)),
                         "double reverse should return original string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "double_skipped", false, "skipped");
    }

    // test 12: long string reversal
    str = d_string_new_from_cstr("The quick brown fox jumps over the lazy dog");

    if (str)
    {
        result = d_string_reverse(str);

        D_TEST_CHECK_MSG(_suite,
                         "long_string_reversed",
                         d_test_suite_str_equal((str->text), ("god yzal eht revo spmuj xof nworb kciuq ehT")),
                         "long string should be reversed correctly");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "long_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_reversed
******************************************************************************/

/*
d_tests_dstring_reversed
  Tests d_string_reversed() which returns a new reversed copy of the string,
  leaving the original unchanged.

Test cases:
  1.  NULL string returns NULL
  2.  Empty string returns empty copy
  3.  Single character string
  4.  Normal string reversal
  5.  Original string unchanged
  6.  Returned string is independent copy
  7.  Palindrome
  8.  String with mixed content
  9.  Long string
  10. Size preserved in new string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_reversed(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      reversed;

    d_test_suite_test(_suite, "d_string_reversed");

    // test 1: NULL string returns NULL
    reversed = d_string_reversed(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_null",
                     (reversed) == NULL,
                     "d_string_reversed(NULL) should return NULL");

    // test 2: empty string returns empty copy
    str = d_string_new();

    if (str)
    {
        reversed = d_string_reversed(str);

        D_TEST_CHECK_MSG(_suite,
                         "empty_returns_non_null",
                         (reversed) != NULL,
                         "reversed empty string should return valid dj_string");

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "empty_reversed_size_zero",
                             (reversed->size) == (0),
                             "reversed empty string should have size 0");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "empty_size_skipped", false, "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "empty_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "empty_size_skipped", false, "skipped");
    }

    // test 3: single character string
    str = d_string_new_from_cstr("Z");

    if (str)
    {
        reversed = d_string_reversed(str);

        D_TEST_CHECK_MSG(_suite,
                         "single_char_non_null",
                         (reversed) != NULL,
                         "reversed single char should succeed");

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "single_char_content",
                             d_test_suite_str_equal((reversed->text), ("Z")),
                             "single char reversed should be same");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite,
                             "single_content_skipped",
                             false,
                             "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "single_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "single_content_skipped", false, "skipped");
    }

    // test 4: normal string reversal
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        reversed = d_string_reversed(str);

        D_TEST_CHECK_MSG(_suite,
                         "normal_reversed_non_null",
                         (reversed) != NULL,
                         "reversed normal string should succeed");

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "normal_reversed_content",
                             d_test_suite_str_equal((reversed->text), ("olleH")),
                             "'Hello' reversed should be 'olleH'");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite,
                             "normal_content_skipped",
                             false,
                             "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "normal_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "normal_content_skipped", false, "skipped");
    }

    // test 5: original string unchanged
    str = d_string_new_from_cstr("Original");

    if (str)
    {
        reversed = d_string_reversed(str);

        D_TEST_CHECK_MSG(_suite,
                         "original_unchanged",
                         d_test_suite_str_equal((str->text), ("Original")),
                         "original string should be unchanged after reversed()");

        if (reversed)
        {
            d_string_free(reversed);
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "original_skipped", false, "skipped");
    }

    // test 6: returned string is independent copy
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        reversed = d_string_reversed(str);

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "independent_copy",
                             reversed->text != str->text,
                             "reversed should return independent copy (different pointer)");

            // modify reversed, original should be unaffected
            reversed->text[0] = 'X';

            D_TEST_CHECK_MSG(_suite,
                             "modification_isolated",
                             str->text[0] == 'T',
                             "modifying reversed copy should not affect original");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "independent_skipped", false, "skipped");
            D_TEST_CHECK_MSG(_suite, "modification_skipped", false, "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "independent_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "modification_skipped", false, "skipped");
    }

    // test 7: palindrome
    str = d_string_new_from_cstr("level");

    if (str)
    {
        reversed = d_string_reversed(str);

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "palindrome_same",
                             d_test_suite_str_equal((reversed->text), ("level")),
                             "palindrome reversed should equal original content");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "palindrome_skipped", false, "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "palindrome_skipped", false, "skipped");
    }

    // test 8: string with mixed content
    str = d_string_new_from_cstr("a1 b2 c3!");

    if (str)
    {
        reversed = d_string_reversed(str);

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "mixed_content_reversed",
                             d_test_suite_str_equal((reversed->text), ("!3c 2b 1a")),
                             "mixed content should be reversed correctly");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "mixed_skipped", false, "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "mixed_skipped", false, "skipped");
    }

    // test 9: size preserved
    str = d_string_new_from_cstr("SizeTest");

    if (str)
    {
        reversed = d_string_reversed(str);

        if (reversed)
        {
            D_TEST_CHECK_MSG(_suite,
                             "size_preserved",
                             (reversed->size) == (str->size),
                             "reversed string should have same size as original");

            d_string_free(reversed);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "size_skipped", false, "skipped");
        }

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "size_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_reversal_all
******************************************************************************/

/*
d_tests_dstring_reversal_all
  Runs all reversal tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all reversal test results.
*/
bool d_tests_dstring_reversal_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "`dstring` reversal");

    // run all reversal tests
    d_tests_dstring_reverse(_suite);
    d_tests_dstring_reversed(_suite);

    return true;
}
