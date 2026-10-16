/*******************************************************************************
* djinterp [container]                             text_buffer_tests_sa_search.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_search.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_find_char
  Tests the d_text_buffer_find_char function.
  Tests the following:
  - NULL buffer returns -1
  - character not found returns -1
  - finds first occurrence
*/
bool
d_tests_sa_text_buffer_find_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_find_char(NULL, 'a') == -1,
        "find_char_null",
        "NULL buffer should return -1",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: not found
        result = d_assert_standalone(
            d_text_buffer_find_char(buffer, 'z') == -1,
            "find_char_not_found",
            "Missing char should return -1",
            _counter) && result;

        // test 3: find first 'l'
        result = d_assert_standalone(
            d_text_buffer_find_char(buffer, 'l') == 2,
            "find_char_first",
            "First 'l' should be at index 2",
            _counter) && result;

        // test 4: find '!'
        result = d_assert_standalone(
            d_text_buffer_find_char(buffer, '!') == 12,
            "find_char_last_pos",
            "'!' should be at index 12",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_find_char_from
  Tests the d_text_buffer_find_char_from function.
  Tests the following:
  - NULL buffer returns -1
  - search from offset skips earlier occurrences
  - search from beyond last occurrence returns -1
*/
bool
d_tests_sa_text_buffer_find_char_from
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_find_char_from(NULL, 'a', 0) == -1,
        "find_char_from_null",
        "NULL buffer should return -1",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("abcabc");

    if (buffer)
    {
        // test 2: find 'a' from index 0
        result = d_assert_standalone(
            d_text_buffer_find_char_from(buffer, 'a', 0) == 0,
            "find_char_from_0",
            "First 'a' from 0 should be at 0",
            _counter) && result;

        // test 3: find 'a' from index 1
        result = d_assert_standalone(
            d_text_buffer_find_char_from(buffer, 'a', 1) == 3,
            "find_char_from_1",
            "First 'a' from 1 should be at 3",
            _counter) && result;

        // test 4: find 'a' from index 4
        result = d_assert_standalone(
            d_text_buffer_find_char_from(buffer, 'a', 4) == -1,
            "find_char_from_past",
            "'a' from index 4 should return -1",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_find_string
  Tests the d_text_buffer_find_string function.
  Tests the following:
  - NULL buffer returns -1
  - NULL search string returns -1
  - substring not found returns -1
  - finds first occurrence of substring
  - empty search string returns 0
*/
bool
d_tests_sa_text_buffer_find_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_find_string(NULL, "test") == -1,
        "find_str_null",
        "NULL buffer should return -1",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("foo bar baz foo");

    if (buffer)
    {
        // test 2: NULL search string
        result = d_assert_standalone(
            d_text_buffer_find_string(buffer, NULL) == -1,
            "find_str_null_needle",
            "NULL needle should return -1",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_text_buffer_find_string(buffer, "xyz") == -1,
            "find_str_not_found",
            "Missing substring should return -1",
            _counter) && result;

        // test 4: find first "foo"
        result = d_assert_standalone(
            d_text_buffer_find_string(buffer, "foo") == 0,
            "find_str_first",
            "First 'foo' should be at index 0",
            _counter) && result;

        // test 5: find "bar"
        result = d_assert_standalone(
            d_text_buffer_find_string(buffer, "bar") == 4,
            "find_str_middle",
            "'bar' should be at index 4",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_find_string_from
  Tests the d_text_buffer_find_string_from function.
  Tests the following:
  - search from offset finds second occurrence
  - search from past all occurrences returns -1
*/
bool
d_tests_sa_text_buffer_find_string_from
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    buffer = d_text_buffer_new_from_string("abc-abc-abc");

    if (buffer)
    {
        // test 1: find "abc" from index 1
        result = d_assert_standalone(
            d_text_buffer_find_string_from(buffer, "abc", 1) == 4,
            "find_str_from_skip",
            "'abc' from index 1 should be at 4",
            _counter) && result;

        // test 2: find "abc" from index 5
        result = d_assert_standalone(
            d_text_buffer_find_string_from(buffer, "abc", 5) == 8,
            "find_str_from_skip2",
            "'abc' from index 5 should be at 8",
            _counter) && result;

        // test 3: find "abc" from index 9
        result = d_assert_standalone(
            d_text_buffer_find_string_from(buffer, "abc", 9) == -1,
            "find_str_from_past",
            "'abc' from index 9 should return -1",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_find_last_char
  Tests the d_text_buffer_find_last_char function.
  Tests the following:
  - NULL buffer returns -1
  - character not found returns -1
  - finds last occurrence
*/
bool
d_tests_sa_text_buffer_find_last_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_find_last_char(NULL, 'a') == -1,
        "find_last_char_null",
        "NULL buffer should return -1",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("abcabc");

    if (buffer)
    {
        // test 2: find last 'a'
        result = d_assert_standalone(
            d_text_buffer_find_last_char(buffer, 'a') == 3,
            "find_last_char_found",
            "Last 'a' should be at index 3",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_text_buffer_find_last_char(buffer, 'z') == -1,
            "find_last_char_not_found",
            "Missing char should return -1",
            _counter) && result;

        // test 4: single occurrence
        result = d_assert_standalone(
            d_text_buffer_find_last_char(buffer, 'a') == 3,
            "find_last_char_single",
            "Last 'a' should be at 3",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_find_last_string
  Tests the d_text_buffer_find_last_string function.
  Tests the following:
  - NULL buffer returns -1
  - substring not found returns -1
  - finds last occurrence of substring
*/
bool
d_tests_sa_text_buffer_find_last_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_find_last_string(NULL, "test") == -1,
        "find_last_str_null",
        "NULL buffer should return -1",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("foo bar baz foo");

    if (buffer)
    {
        // test 2: find last "foo"
        result = d_assert_standalone(
            d_text_buffer_find_last_string(buffer, "foo") == 12,
            "find_last_str_found",
            "Last 'foo' should be at index 12",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_text_buffer_find_last_string(buffer, "xyz") == -1,
            "find_last_str_not_found",
            "Missing substring should return -1",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_contains_char
  Tests the d_text_buffer_contains_char function.
  Tests the following:
  - NULL buffer returns false
  - character present returns true
  - character absent returns false
*/
bool
d_tests_sa_text_buffer_contains_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_contains_char(NULL, 'a') == false,
        "contains_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: present
        result = d_assert_standalone(
            d_text_buffer_contains_char(buffer, 'e') == true,
            "contains_char_present",
            "'e' should be found",
            _counter) && result;

        // test 3: absent
        result = d_assert_standalone(
            d_text_buffer_contains_char(buffer, 'z') == false,
            "contains_char_absent",
            "'z' should not be found",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_contains_string
  Tests the d_text_buffer_contains_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - substring present returns true
  - substring absent returns false
  - empty substring returns true
*/
bool
d_tests_sa_text_buffer_contains_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_contains_string(NULL, "test") == false,
        "contains_str_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: present
        result = d_assert_standalone(
            d_text_buffer_contains_string(buffer, "World") == true,
            "contains_str_present",
            "'World' should be found",
            _counter) && result;

        // test 3: absent
        result = d_assert_standalone(
            d_text_buffer_contains_string(buffer, "xyz") == false,
            "contains_str_absent",
            "'xyz' should not be found",
            _counter) && result;

        // test 4: empty substring
        result = d_assert_standalone(
            d_text_buffer_contains_string(buffer, "") == true,
            "contains_str_empty",
            "Empty substring should be found",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_starts_with
  Tests the d_text_buffer_starts_with function.
  Tests the following:
  - NULL buffer returns false
  - NULL prefix returns false
  - matching prefix returns true
  - non-matching prefix returns false
  - empty prefix returns true
  - prefix longer than buffer returns false
*/
bool
d_tests_sa_text_buffer_starts_with
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_starts_with(NULL, "He") == false,
        "starts_with_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: matching prefix
        result = d_assert_standalone(
            d_text_buffer_starts_with(buffer, "Hello") == true,
            "starts_with_match",
            "Should start with 'Hello'",
            _counter) && result;

        // test 3: non-matching
        result = d_assert_standalone(
            d_text_buffer_starts_with(buffer, "World") == false,
            "starts_with_no_match",
            "Should not start with 'World'",
            _counter) && result;

        // test 4: empty prefix
        result = d_assert_standalone(
            d_text_buffer_starts_with(buffer, "") == true,
            "starts_with_empty",
            "Should start with empty prefix",
            _counter) && result;

        // test 5: prefix longer than buffer
        result = d_assert_standalone(
            d_text_buffer_starts_with(buffer,
                "Hello, World! And more text") == false,
            "starts_with_too_long",
            "Prefix longer than buffer should return false",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_ends_with
  Tests the d_text_buffer_ends_with function.
  Tests the following:
  - NULL buffer returns false
  - NULL suffix returns false
  - matching suffix returns true
  - non-matching suffix returns false
  - empty suffix returns true
*/
bool
d_tests_sa_text_buffer_ends_with
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_ends_with(NULL, "!") == false,
        "ends_with_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: matching suffix
        result = d_assert_standalone(
            d_text_buffer_ends_with(buffer, "World!") == true,
            "ends_with_match",
            "Should end with 'World!'",
            _counter) && result;

        // test 3: non-matching
        result = d_assert_standalone(
            d_text_buffer_ends_with(buffer, "Hello") == false,
            "ends_with_no_match",
            "Should not end with 'Hello'",
            _counter) && result;

        // test 4: empty suffix
        result = d_assert_standalone(
            d_text_buffer_ends_with(buffer, "") == true,
            "ends_with_empty",
            "Should end with empty suffix",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_count_char
  Tests the d_text_buffer_count_char function.
  Tests the following:
  - NULL buffer returns 0
  - character not present returns 0
  - correct count of occurrences
*/
bool
d_tests_sa_text_buffer_count_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_count_char(NULL, 'a') == 0,
        "count_char_null",
        "NULL buffer should return 0",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("banana");

    if (buffer)
    {
        // test 2: count 'a'
        result = d_assert_standalone(
            d_text_buffer_count_char(buffer, 'a') == 3,
            "count_char_found",
            "'a' should appear 3 times",
            _counter) && result;

        // test 3: count 'n'
        result = d_assert_standalone(
            d_text_buffer_count_char(buffer, 'n') == 2,
            "count_char_n",
            "'n' should appear 2 times",
            _counter) && result;

        // test 4: not found
        result = d_assert_standalone(
            d_text_buffer_count_char(buffer, 'z') == 0,
            "count_char_not_found",
            "'z' count should be 0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_count_string
  Tests the d_text_buffer_count_string function.
  Tests the following:
  - NULL buffer returns 0
  - NULL string returns 0
  - substring not present returns 0
  - correct count of non-overlapping occurrences
*/
bool
d_tests_sa_text_buffer_count_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_count_string(NULL, "test") == 0,
        "count_str_null",
        "NULL buffer should return 0",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("abcabcabc");

    if (buffer)
    {
        // test 2: count "abc"
        result = d_assert_standalone(
            d_text_buffer_count_string(buffer, "abc") == 3,
            "count_str_found",
            "'abc' should appear 3 times",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_text_buffer_count_string(buffer, "xyz") == 0,
            "count_str_not_found",
            "'xyz' count should be 0",
            _counter) && result;

        // test 4: single char substring
        result = d_assert_standalone(
            d_text_buffer_count_string(buffer, "a") == 3,
            "count_str_single",
            "'a' should appear 3 times",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_search_all
  Aggregation function that runs all search operation tests.
*/
bool
d_tests_sa_text_buffer_search_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Operations\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_text_buffer_find_char(_counter) && result;
    result = d_tests_sa_text_buffer_find_char_from(_counter) && result;
    result = d_tests_sa_text_buffer_find_string(_counter) && result;
    result = d_tests_sa_text_buffer_find_string_from(_counter)
             && result;
    result = d_tests_sa_text_buffer_find_last_char(_counter) && result;
    result = d_tests_sa_text_buffer_find_last_string(_counter)
             && result;
    result = d_tests_sa_text_buffer_contains_char(_counter) && result;
    result = d_tests_sa_text_buffer_contains_string(_counter) && result;
    result = d_tests_sa_text_buffer_starts_with(_counter) && result;
    result = d_tests_sa_text_buffer_ends_with(_counter) && result;
    result = d_tests_sa_text_buffer_count_char(_counter) && result;
    result = d_tests_sa_text_buffer_count_string(_counter) && result;

    return result;
}
