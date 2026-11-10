/*******************************************************************************
* djinterp [container]                                text_buffer_tests_search.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_search.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_find_char
  Tests the d_text_buffer_find_char function.
  Tests the following:
  - NULL buffer returns -1
  - character not found returns -1
  - finds first occurrence
*/
bool d_tests_text_buffer_find_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_char_null",
                              d_text_buffer_find_char(NULL, 'a') == -1,
                              "NULL buffer should return -1") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_not_found",
                                  d_text_buffer_find_char(buffer, 'z') == -1,
                                  "Missing char should return -1") && result;

        // test 3: find first 'l'
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_first",
                                  d_text_buffer_find_char(buffer, 'l') == 2,
                                  "First 'l' should be at index 2") && result;

        // test 4: find '!'
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_last_pos",
                                  d_text_buffer_find_char(buffer, '!') == 12,
                                  "'!' should be at index 12") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_find_char_from
  Tests the d_text_buffer_find_char_from function.
  Tests the following:
  - NULL buffer returns -1
  - search from offset skips earlier occurrences
  - search from beyond last occurrence returns -1
*/
bool d_tests_text_buffer_find_char_from(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_char_from");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_char_from_null",
                              d_text_buffer_find_char_from(NULL, 'a', 0) == -1,
                              "NULL buffer should return -1") && result;

    buffer = d_text_buffer_new_from_string("abcabc");

    if (buffer)
    {
        // test 2: find 'a' from index 0
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_from_0",
                                  d_text_buffer_find_char_from(buffer, 'a', 0) == 0,
                                  "First 'a' from 0 should be at 0") && result;

        // test 3: find 'a' from index 1
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_from_1",
                                  d_text_buffer_find_char_from(buffer, 'a', 1) == 3,
                                  "First 'a' from 1 should be at 3") && result;

        // test 4: find 'a' from index 4
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_char_from_past",
                                  d_text_buffer_find_char_from(buffer, 'a', 4) == -1,
                                  "'a' from index 4 should return -1") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_find_string
  Tests the d_text_buffer_find_string function.
  Tests the following:
  - NULL buffer returns -1
  - NULL search string returns -1
  - substring not found returns -1
  - finds first occurrence of substring
  - empty search string returns 0
*/
bool d_tests_text_buffer_find_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_str_null",
                              d_text_buffer_find_string(NULL, "test") == -1,
                              "NULL buffer should return -1") && result;

    buffer = d_text_buffer_new_from_string("foo bar baz foo");

    if (buffer)
    {
        // test 2: NULL search string
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_null_needle",
                                  d_text_buffer_find_string(buffer, NULL) == -1,
                                  "NULL needle should return -1") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_not_found",
                                  d_text_buffer_find_string(buffer, "xyz") == -1,
                                  "Missing substring should return -1") && result;

        // test 4: find first "foo"
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_first",
                                  d_text_buffer_find_string(buffer, "foo") == 0,
                                  "First 'foo' should be at index 0") && result;

        // test 5: find "bar"
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_middle",
                                  d_text_buffer_find_string(buffer, "bar") == 4,
                                  "'bar' should be at index 4") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_find_string_from
  Tests the d_text_buffer_find_string_from function.
  Tests the following:
  - search from offset finds second occurrence
  - search from past all occurrences returns -1
*/
bool d_tests_text_buffer_find_string_from(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_string_from");

    result = true;

    buffer = d_text_buffer_new_from_string("abc-abc-abc");

    if (buffer)
    {
        // test 1: find "abc" from index 1
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_from_skip",
                                  d_text_buffer_find_string_from(buffer, "abc", 1) == 4,
                                  "'abc' from index 1 should be at 4") && result;

        // test 2: find "abc" from index 5
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_from_skip2",
                                  d_text_buffer_find_string_from(buffer, "abc", 5) == 8,
                                  "'abc' from index 5 should be at 8") && result;

        // test 3: find "abc" from index 9
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_str_from_past",
                                  d_text_buffer_find_string_from(buffer, "abc", 9) == -1,
                                  "'abc' from index 9 should return -1") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_find_last_char
  Tests the d_text_buffer_find_last_char function.
  Tests the following:
  - NULL buffer returns -1
  - character not found returns -1
  - finds last occurrence
*/
bool d_tests_text_buffer_find_last_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_last_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_last_char_null",
                              d_text_buffer_find_last_char(NULL, 'a') == -1,
                              "NULL buffer should return -1") && result;

    buffer = d_text_buffer_new_from_string("abcabc");

    if (buffer)
    {
        // test 2: find last 'a'
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_char_found",
                                  d_text_buffer_find_last_char(buffer, 'a') == 3,
                                  "Last 'a' should be at index 3") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_char_not_found",
                                  d_text_buffer_find_last_char(buffer, 'z') == -1,
                                  "Missing char should return -1") && result;

        // test 4: single occurrence
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_char_single",
                                  d_text_buffer_find_last_char(buffer, 'a') == 3,
                                  "Last 'a' should be at 3") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_find_last_string
  Tests the d_text_buffer_find_last_string function.
  Tests the following:
  - NULL buffer returns -1
  - substring not found returns -1
  - finds last occurrence of substring
*/
bool d_tests_text_buffer_find_last_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_find_last_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_last_str_null",
                              d_text_buffer_find_last_string(NULL, "test") == -1,
                              "NULL buffer should return -1") && result;

    buffer = d_text_buffer_new_from_string("foo bar baz foo");

    if (buffer)
    {
        // test 2: find last "foo"
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_str_found",
                                  d_text_buffer_find_last_string(buffer, "foo") == 12,
                                  "Last 'foo' should be at index 12") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_last_str_not_found",
                                  d_text_buffer_find_last_string(buffer, "xyz") == -1,
                                  "Missing substring should return -1") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_contains_char
  Tests the d_text_buffer_contains_char function.
  Tests the following:
  - NULL buffer returns false
  - character present returns true
  - character absent returns false
*/
bool d_tests_text_buffer_contains_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_contains_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_char_null",
                              d_text_buffer_contains_char(NULL, 'a') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: present
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_char_present",
                                  d_text_buffer_contains_char(buffer, 'e') == true,
                                  "'e' should be found") && result;

        // test 3: absent
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_char_absent",
                                  d_text_buffer_contains_char(buffer, 'z') == false,
                                  "'z' should not be found") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_contains_string
  Tests the d_text_buffer_contains_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - substring present returns true
  - substring absent returns false
  - empty substring returns true
*/
bool d_tests_text_buffer_contains_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_contains_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_str_null",
                              d_text_buffer_contains_string(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: present
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_str_present",
                                  d_text_buffer_contains_string(buffer, "World") == true,
                                  "'World' should be found") && result;

        // test 3: absent
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_str_absent",
                                  d_text_buffer_contains_string(buffer, "xyz") == false,
                                  "'xyz' should not be found") && result;

        // test 4: empty substring
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_str_empty",
                                  d_text_buffer_contains_string(buffer, "") == true,
                                  "Empty substring should be found") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_starts_with
  Tests the d_text_buffer_starts_with function.
  Tests the following:
  - NULL buffer returns false
  - NULL prefix returns false
  - matching prefix returns true
  - non-matching prefix returns false
  - empty prefix returns true
  - prefix longer than buffer returns false
*/
bool d_tests_text_buffer_starts_with(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_starts_with");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "starts_with_null",
                              d_text_buffer_starts_with(NULL, "He") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: matching prefix
        result = D_TEST_CHECK_MSG(_suite,
                                  "starts_with_match",
                                  d_text_buffer_starts_with(buffer, "Hello") == true,
                                  "Should start with 'Hello'") && result;

        // test 3: non-matching
        result = D_TEST_CHECK_MSG(_suite,
                                  "starts_with_no_match",
                                  d_text_buffer_starts_with(buffer, "World") == false,
                                  "Should not start with 'World'") && result;

        // test 4: empty prefix
        result = D_TEST_CHECK_MSG(_suite,
                                  "starts_with_empty",
                                  d_text_buffer_starts_with(buffer, "") == true,
                                  "Should start with empty prefix") && result;

        // test 5: prefix longer than buffer
        result = D_TEST_CHECK_MSG(_suite,
                                  "starts_with_too_long",
                                  d_text_buffer_starts_with(buffer, "Hello, World! And more text") == false,
                                  "Prefix longer than buffer should return false") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_ends_with
  Tests the d_text_buffer_ends_with function.
  Tests the following:
  - NULL buffer returns false
  - NULL suffix returns false
  - matching suffix returns true
  - non-matching suffix returns false
  - empty suffix returns true
*/
bool d_tests_text_buffer_ends_with(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_ends_with");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "ends_with_null",
                              d_text_buffer_ends_with(NULL, "!") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: matching suffix
        result = D_TEST_CHECK_MSG(_suite,
                                  "ends_with_match",
                                  d_text_buffer_ends_with(buffer, "World!") == true,
                                  "Should end with 'World!'") && result;

        // test 3: non-matching
        result = D_TEST_CHECK_MSG(_suite,
                                  "ends_with_no_match",
                                  d_text_buffer_ends_with(buffer, "Hello") == false,
                                  "Should not end with 'Hello'") && result;

        // test 4: empty suffix
        result = D_TEST_CHECK_MSG(_suite,
                                  "ends_with_empty",
                                  d_text_buffer_ends_with(buffer, "") == true,
                                  "Should end with empty suffix") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_count_char
  Tests the d_text_buffer_count_char function.
  Tests the following:
  - NULL buffer returns 0
  - character not present returns 0
  - correct count of occurrences
*/
bool d_tests_text_buffer_count_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_count_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "count_char_null",
                              d_text_buffer_count_char(NULL, 'a') == 0,
                              "NULL buffer should return 0") && result;

    buffer = d_text_buffer_new_from_string("banana");

    if (buffer)
    {
        // test 2: count 'a'
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_char_found",
                                  d_text_buffer_count_char(buffer, 'a') == 3,
                                  "'a' should appear 3 times") && result;

        // test 3: count 'n'
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_char_n",
                                  d_text_buffer_count_char(buffer, 'n') == 2,
                                  "'n' should appear 2 times") && result;

        // test 4: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_char_not_found",
                                  d_text_buffer_count_char(buffer, 'z') == 0,
                                  "'z' count should be 0") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_count_string
  Tests the d_text_buffer_count_string function.
  Tests the following:
  - NULL buffer returns 0
  - NULL string returns 0
  - substring not present returns 0
  - correct count of non-overlapping occurrences
*/
bool d_tests_text_buffer_count_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_count_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "count_str_null",
                              d_text_buffer_count_string(NULL, "test") == 0,
                              "NULL buffer should return 0") && result;

    buffer = d_text_buffer_new_from_string("abcabcabc");

    if (buffer)
    {
        // test 2: count "abc"
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_str_found",
                                  d_text_buffer_count_string(buffer, "abc") == 3,
                                  "'abc' should appear 3 times") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_str_not_found",
                                  d_text_buffer_count_string(buffer, "xyz") == 0,
                                  "'xyz' count should be 0") && result;

        // test 4: single char substring
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_str_single",
                                  d_text_buffer_count_string(buffer, "a") == 3,
                                  "'a' should appear 3 times") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_search_all
  Aggregation function that runs all search operation tests.
*/
bool d_tests_text_buffer_search_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_search_all");

    result = true;

    result = d_tests_text_buffer_find_char(_suite) && result;
    result = d_tests_text_buffer_find_char_from(_suite) && result;
    result = d_tests_text_buffer_find_string(_suite) && result;
    result = d_tests_text_buffer_find_string_from(_suite)
             && result;
    result = d_tests_text_buffer_find_last_char(_suite) && result;
    result = d_tests_text_buffer_find_last_string(_suite)
             && result;
    result = d_tests_text_buffer_contains_char(_suite) && result;
    result = d_tests_text_buffer_contains_string(_suite) && result;
    result = d_tests_text_buffer_starts_with(_suite) && result;
    result = d_tests_text_buffer_ends_with(_suite) && result;
    result = d_tests_text_buffer_count_char(_suite) && result;
    result = d_tests_text_buffer_count_string(_suite) && result;

    return result;
}
