/*******************************************************************************
* djinterp [test]                                            dstring_tests_dup.c
*
*   Unit tests for dj_string duplication functions:
*     - d_string_dup
*     - d_string_ndup
*     - d_string_substr
*
*
* path:      /tests/djinterp/c/dstring_tests_dup.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "dstring_tests.h"


/******************************************************************************
* d_tests_dstring_dup
******************************************************************************/

/*
d_tests_dstring_dup
  Tests d_string_dup() which creates a duplicate (deep copy) of a dj_string.

Test cases:
  1.  NULL string returns NULL
  2.  Duplicate empty string
  3.  Duplicate normal string
  4.  Duplicate is independent copy
  5.  Duplicate has same content
  6.  Duplicate has same size
  7.  Original unchanged after dup
  8.  Modifying duplicate doesn't affect original

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_dup(struct dj_test_suite* _suite)
{
    struct dj_string*      original;
    struct dj_string*      duplicate;

    d_test_suite_test(_suite, "d_string_dup");

    // test 1: NULL string returns NULL
    duplicate = d_string_dup(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_returns_null",
                     (duplicate) == NULL,
                     "d_string_dup(NULL) should return NULL");

    // test 2: duplicate empty string
    original = d_string_new();

    if (original)
    {
        duplicate = d_string_dup(original);

        D_TEST_CHECK_MSG(_suite,
                         "empty_dup_not_null",
                         (duplicate) != NULL,
                         "duplicating empty string should return non-NULL");

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "empty_dup_size_0",
                             (duplicate->size) == (0),
                             "duplicate of empty string should have size 0");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "empty_size_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "empty_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "empty_size_skip", false, "skip");
    }

    // test 3: duplicate normal string
    original = d_string_new_from_cstr("Hello World");

    if (original)
    {
        duplicate = d_string_dup(original);

        D_TEST_CHECK_MSG(_suite,
                         "normal_dup_not_null",
                         (duplicate) != NULL,
                         "duplicating normal string should succeed");

        if (duplicate)
        {
            // test 4: duplicate has same content
            D_TEST_CHECK_MSG(_suite,
                             "dup_same_content",
                             d_test_suite_str_equal((duplicate->text), ("Hello World")),
                             "duplicate should have same content");

            // test 5: duplicate has same size
            D_TEST_CHECK_MSG(_suite,
                             "dup_same_size",
                             (duplicate->size) == (original->size),
                             "duplicate should have same size");

            // test 6: duplicate is independent (different pointer)
            D_TEST_CHECK_MSG(_suite,
                             "dup_independent_pointer",
                             duplicate->text != original->text,
                             "duplicate text should be at different address");

            // test 7: modifying duplicate doesn't affect original
            if (duplicate->size > 0)
            {
                duplicate->text[0] = 'X';

                D_TEST_CHECK_MSG(_suite,
                                 "modify_dup_no_affect_original",
                                 original->text[0] == 'H',
                                 "modifying duplicate should not affect original");
            }
            else
            {
                D_TEST_CHECK_MSG(_suite, "modify_skip", false, "skip");
            }

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "content_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "size_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "ptr_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "modify_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "normal_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "content_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "size_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "ptr_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "modify_skip", false, "skip");
    }

    // test 8: duplicate long string
    original = d_string_new_fill(500, 'A');

    if (original)
    {
        duplicate = d_string_dup(original);

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "long_dup_size",
                             (duplicate->size) == (500),
                             "duplicate of 500-char string should have size 500");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "long_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "long_skip", false, "skip");
    }

    // test 9: duplicate string with special characters
    original = d_string_new_from_cstr("Tab\tNewline\nCR\r");

    if (original)
    {
        duplicate = d_string_dup(original);

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "special_chars_dup",
                             d_test_suite_str_equal((duplicate->text), ("Tab\tNewline\nCR\r")),
                             "special characters should be duplicated correctly");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "special_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "special_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_ndup
******************************************************************************/

/*
d_tests_dstring_ndup
  Tests d_string_ndup() which creates a duplicate of at most n characters.

Test cases:
  1.  NULL string returns NULL
  2.  n = 0 returns empty string
  3.  n less than string length
  4.  n equal to string length
  5.  n greater than string length
  6.  Duplicate is null-terminated
  7.  Original unchanged

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_ndup(struct dj_test_suite* _suite)
{
    struct dj_string*      original;
    struct dj_string*      duplicate;

    d_test_suite_test(_suite, "d_string_ndup");

    // test 1: NULL string returns NULL
    duplicate = d_string_ndup(NULL, 5);
    D_TEST_CHECK_MSG(_suite,
                     "null_returns_null",
                     (duplicate) == NULL,
                     "d_string_ndup(NULL, ...) should return NULL");

    // test 2: n = 0 returns empty string
    original = d_string_new_from_cstr("Hello");

    if (original)
    {
        duplicate = d_string_ndup(original, 0);

        D_TEST_CHECK_MSG(_suite,
                         "n_0_not_null",
                         (duplicate) != NULL,
                         "d_string_ndup(..., 0) should return non-NULL");

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "n_0_size",
                             (duplicate->size) == (0),
                             "n = 0 should return empty string");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "n0_size_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "n0_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "n0_size_skip", false, "skip");
    }

    // test 3: n less than string length
    original = d_string_new_from_cstr("Hello World");

    if (original)
    {
        duplicate = d_string_ndup(original, 5);

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "n_less_content",
                             d_test_suite_str_equal((duplicate->text), ("Hello")),
                             "should duplicate only first 5 characters");

            D_TEST_CHECK_MSG(_suite,
                             "n_less_size",
                             (duplicate->size) == (5),
                             "size should be 5");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "n_less_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "n_less_sz_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "n_less_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "n_less_sz_skip", false, "skip");
    }

    // test 4: n equal to string length
    original = d_string_new_from_cstr("Exact");

    if (original)
    {
        duplicate = d_string_ndup(original, 5);

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "n_equal_content",
                             d_test_suite_str_equal((duplicate->text), ("Exact")),
                             "should duplicate entire string");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "n_equal_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "n_equal_skip", false, "skip");
    }

    // test 5: n greater than string length
    original = d_string_new_from_cstr("Short");

    if (original)
    {
        duplicate = d_string_ndup(original, 100);

        if (duplicate)
        {
            D_TEST_CHECK_MSG(_suite,
                             "n_greater_content",
                             d_test_suite_str_equal((duplicate->text), ("Short")),
                             "should duplicate entire string (capped at length)");

            D_TEST_CHECK_MSG(_suite,
                             "n_greater_size",
                             (duplicate->size) == (5),
                             "size should be original length, not n");

            d_string_free(duplicate);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "n_greater_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "n_greater_sz_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "n_greater_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "n_greater_sz_skip", false, "skip");
    }

    // test 6: original unchanged after ndup
    original = d_string_new_from_cstr("Original");

    if (original)
    {
        duplicate = d_string_ndup(original, 4);

        D_TEST_CHECK_MSG(_suite,
                         "original_unchanged",
                         d_test_suite_str_equal((original->text), ("Original")),
                         "original should be unchanged after ndup");

        if (duplicate)
        {
            d_string_free(duplicate);
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "unchanged_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_substr
******************************************************************************/

/*
d_tests_dstring_substr
  Tests d_string_substr() which extracts a substring from a dj_string.

Test cases:
  1.  NULL string returns NULL
  2.  Start beyond string length returns empty
  3.  Start at 0, length 0 returns empty
  4.  Normal substring extraction
  5.  Start at middle, extract to end
  6.  Length exceeds available characters
  7.  Extract single character
  8.  Extract entire string
  9.  Original unchanged

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_substr(struct dj_test_suite* _suite)
{
    struct dj_string*      original;
    struct dj_string*      substring;

    d_test_suite_test(_suite, "d_string_substr");

    // test 1: NULL string returns NULL
    substring = d_string_substr(NULL, 0, 5);
    D_TEST_CHECK_MSG(_suite,
                     "null_returns_null",
                     (substring) == NULL,
                     "d_string_substr(NULL, ...) should return NULL");

    // test 2: start beyond string length
    original = d_string_new_from_cstr("Hello");

    if (original)
    {
        substring = d_string_substr(original, 100, 5);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "start_beyond_empty",
                             (substring->size) == (0),
                             "start beyond length should return empty string");

            d_string_free(substring);
        }
        else
        {
            // could also return NULL for invalid range
            D_TEST_CHECK_MSG(_suite,
                             "start_beyond_null_ok",
                             true,
                             "start beyond length may return NULL");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "beyond_skip", false, "skip");
    }

    // test 3: start at 0, length 0 returns empty
    original = d_string_new_from_cstr("Hello");

    if (original)
    {
        substring = d_string_substr(original, 0, 0);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "length_0_empty",
                             (substring->size) == (0),
                             "length 0 should return empty string");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite,
                             "length_0_null_ok",
                             true,
                             "length 0 may return NULL");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "len0_skip", false, "skip");
    }

    // test 4: normal substring extraction
    original = d_string_new_from_cstr("Hello World");

    if (original)
    {
        substring = d_string_substr(original, 6, 5);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "normal_substr_content",
                             d_test_suite_str_equal((substring->text), ("World")),
                             "substring from position 6, length 5 should be 'World'");

            D_TEST_CHECK_MSG(_suite,
                             "normal_substr_size",
                             (substring->size) == (5),
                             "substring size should be 5");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "normal_skip", false, "skip");
            D_TEST_CHECK_MSG(_suite, "normal_sz_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "normal_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "normal_sz_skip", false, "skip");
    }

    // test 5: start at middle, extract to end
    original = d_string_new_from_cstr("ABCDEFGHIJ");

    if (original)
    {
        // large length to extract to end
        substring = d_string_substr(original, 5, 100);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "extract_to_end",
                             d_test_suite_str_equal((substring->text), ("FGHIJ")),
                             "should extract from position 5 to end");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "to_end_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "to_end_skip", false, "skip");
    }

    // test 6: extract single character
    original = d_string_new_from_cstr("Testing");

    if (original)
    {
        substring = d_string_substr(original, 3, 1);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "single_char_substr",
                             d_test_suite_str_equal((substring->text), ("t")),
                             "single character extraction");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "single_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "single_skip", false, "skip");
    }

    // test 7: extract entire string
    original = d_string_new_from_cstr("Complete");

    if (original)
    {
        substring = d_string_substr(original, 0, 8);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "entire_string",
                             d_test_suite_str_equal((substring->text), ("Complete")),
                             "extracting entire string");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "entire_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "entire_skip", false, "skip");
    }

    // test 8: original unchanged after substr
    original = d_string_new_from_cstr("Unchanged");

    if (original)
    {
        substring = d_string_substr(original, 2, 3);

        D_TEST_CHECK_MSG(_suite,
                         "original_preserved",
                         d_test_suite_str_equal((original->text), ("Unchanged")),
                         "original should be unchanged after substr");

        if (substring)
        {
            d_string_free(substring);
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "preserved_skip", false, "skip");
    }

    // test 9: substring is independent copy
    original = d_string_new_from_cstr("Independent");

    if (original)
    {
        substring = d_string_substr(original, 0, 4);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "substr_independent",
                             substring->text != original->text,
                             "substring should be at different memory address");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "ind_skip", false, "skip");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ind_skip", false, "skip");
    }

    // test 10: extract from empty string
    original = d_string_new();

    if (original)
    {
        substring = d_string_substr(original, 0, 5);

        if (substring)
        {
            D_TEST_CHECK_MSG(_suite,
                             "empty_source_substr",
                             (substring->size) == (0),
                             "substring of empty string should be empty");

            d_string_free(substring);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite,
                             "empty_source_null_ok",
                             true,
                             "may return NULL for empty");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "empty_src_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_dup_all
******************************************************************************/

/*
d_tests_dstring_dup_all
  Runs all duplication tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all duplication test results.
*/
bool d_tests_dstring_dup_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string Duplication Functions");

    // run all duplication tests
    d_tests_dstring_dup(_suite);
    d_tests_dstring_ndup(_suite);
    d_tests_dstring_substr(_suite);

    return true;
}
