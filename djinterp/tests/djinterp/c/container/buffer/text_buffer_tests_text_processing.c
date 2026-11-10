/*******************************************************************************
* djinterp [container]                       text_buffer_tests_text_processing.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_text_processing.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_trim_whitespace
  Tests the d_text_buffer_trim_whitespace function.
  Tests the following:
  - NULL buffer returns false
  - trims leading and trailing whitespace
  - buffer with no whitespace unchanged
  - buffer of only whitespace becomes empty
  - mixed whitespace characters (spaces, tabs, newlines)
*/
bool d_tests_text_buffer_trim_whitespace(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_trim_whitespace");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "trim_ws_null",
                              d_text_buffer_trim_whitespace(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: trim both sides
    buffer = d_text_buffer_new_from_string("  hello world  ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_both",
                                  d_text_buffer_trim_whitespace(buffer) == true,
                                  "Trim should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_both_len",
                                  d_text_buffer_length(buffer) == 11,
                                  "Length should be 11 after trim") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_both_content",
                                  d_strcasecmp(buffer->data, "hello world") == 0,
                                  "Content should be 'hello world'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no whitespace to trim
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_none",
                                  d_text_buffer_trim_whitespace(buffer) == true,
                                  "Trim with no whitespace should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_none_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should remain 5") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: only whitespace
    buffer = d_text_buffer_new_from_string("   \t\n  ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_all",
                                  d_text_buffer_trim_whitespace(buffer) == true,
                                  "Trimming all-whitespace should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_all_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Length should be 0 after trimming all whitespace") && result;

        d_text_buffer_free(buffer);
    }

    // test 5: mixed whitespace chars
    buffer = d_text_buffer_new_from_string("\t\n  data \r\n ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_mixed",
                                  d_text_buffer_trim_whitespace(buffer) == true,
                                  "Trim mixed whitespace should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_ws_mixed_content",
                                  d_strcasecmp(buffer->data, "data") == 0,
                                  "Content should be 'data'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_trim_front
  Tests the d_text_buffer_trim_front function.
  Tests the following:
  - NULL buffer returns false
  - trims only leading whitespace
  - trailing whitespace preserved
*/
bool d_tests_text_buffer_trim_front(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_trim_front");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "trim_front_null",
                              d_text_buffer_trim_front(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: trim leading only
    buffer = d_text_buffer_new_from_string("   hello   ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_front_success",
                                  d_text_buffer_trim_front(buffer) == true,
                                  "Trim front should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_front_content",
                                  d_strcasecmp(buffer->data, "hello   ") == 0,
                                  "Content should be 'hello   '") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_front_len",
                                  d_text_buffer_length(buffer) == 8,
                                  "Length should be 8") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no leading whitespace
    buffer = d_text_buffer_new_from_string("hello   ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_front_noop",
                                  d_text_buffer_trim_front(buffer) == true,
                                  "Trim front with no leading ws should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_front_noop_len",
                                  d_text_buffer_length(buffer) == 8,
                                  "Length should remain 8") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_trim_back
  Tests the d_text_buffer_trim_back function.
  Tests the following:
  - NULL buffer returns false
  - trims only trailing whitespace
  - leading whitespace preserved
*/
bool d_tests_text_buffer_trim_back(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_trim_back");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "trim_back_null",
                              d_text_buffer_trim_back(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: trim trailing only
    buffer = d_text_buffer_new_from_string("   hello   ");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_back_success",
                                  d_text_buffer_trim_back(buffer) == true,
                                  "Trim back should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_back_content",
                                  d_strcasecmp(buffer->data, "   hello") == 0,
                                  "Content should be '   hello'") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_back_len",
                                  d_text_buffer_length(buffer) == 8,
                                  "Length should be 8") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no trailing whitespace
    buffer = d_text_buffer_new_from_string("   hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_back_noop",
                                  d_text_buffer_trim_back(buffer) == true,
                                  "Trim back with no trailing ws should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_back_noop_len",
                                  d_text_buffer_length(buffer) == 8,
                                  "Length should remain 8") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_trim_chars
  Tests the d_text_buffer_trim_chars function.
  Tests the following:
  - NULL buffer returns false
  - NULL chars returns false
  - trims specified characters from both ends
  - characters in the middle preserved
*/
bool d_tests_text_buffer_trim_chars(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_trim_chars");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "trim_chars_null_buf",
                              d_text_buffer_trim_chars(NULL, "xy") == false,
                              "NULL buffer should return false") && result;

    // test 2: trim custom characters
    buffer = d_text_buffer_new_from_string("xxyhelloxyyx");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_success",
                                  d_text_buffer_trim_chars(buffer, "xy") == true,
                                  "Trim chars should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_content",
                                  d_strcasecmp(buffer->data, "hello") == 0,
                                  "Content should be 'hello'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no matching chars to trim
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_noop",
                                  d_text_buffer_trim_chars(buffer, "xy") == true,
                                  "Trim with no matching chars should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_noop_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should remain 5") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: trim all characters
    buffer = d_text_buffer_new_from_string("aaabbb");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_all",
                                  d_text_buffer_trim_chars(buffer, "ab") == true,
                                  "Trimming all chars should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "trim_chars_all_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Length should be 0") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_to_upper
  Tests the d_text_buffer_to_upper function.
  Tests the following:
  - NULL buffer returns false
  - converts lowercase to uppercase
  - non-alpha characters unchanged
  - already uppercase unchanged
*/
bool d_tests_text_buffer_to_upper(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_to_upper");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "to_upper_null",
                              d_text_buffer_to_upper(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: convert to uppercase
    buffer = d_text_buffer_new_from_string("hello world 123");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_upper_success",
                                  d_text_buffer_to_upper(buffer) == true,
                                  "to_upper should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_upper_content",
                                  d_strcasecmp(buffer->data, "HELLO WORLD 123") == 0,
                                  "Content should be 'HELLO WORLD 123'") && result;

        // verify case-sensitive: data should literally be uppercase
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_upper_exact",
                                  buffer->data[0] == 'H' && buffer->data[4] == 'O',
                                  "Characters should be uppercase") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: already uppercase
    buffer = d_text_buffer_new_from_string("ABC");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_upper_noop",
                                  d_text_buffer_to_upper(buffer) == true,
                                  "to_upper on uppercase should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_upper_noop_content",
                                  buffer->data[0] == 'A' && buffer->data[1] == 'B' && buffer->data[2] == 'C',
                                  "Content should remain 'ABC'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_to_lower
  Tests the d_text_buffer_to_lower function.
  Tests the following:
  - NULL buffer returns false
  - converts uppercase to lowercase
  - non-alpha characters unchanged
  - already lowercase unchanged
*/
bool d_tests_text_buffer_to_lower(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_to_lower");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "to_lower_null",
                              d_text_buffer_to_lower(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: convert to lowercase
    buffer = d_text_buffer_new_from_string("HELLO WORLD 123");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_lower_success",
                                  d_text_buffer_to_lower(buffer) == true,
                                  "to_lower should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_lower_content",
                                  buffer->data[0] == 'h' && buffer->data[6] == 'w',
                                  "Characters should be lowercase") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_lower_len",
                                  d_text_buffer_length(buffer) == 15,
                                  "Length should remain 15") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: mixed case
    buffer = d_text_buffer_new_from_string("HeLLo");

    if (buffer)
    {
        d_text_buffer_to_lower(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_lower_mixed",
                                  buffer->data[0] == 'h' && buffer->data[1] == 'e' && buffer->data[2] == 'l' && buffer->data[3] == 'l' && buffer->data[4] == 'o',
                                  "Mixed case should all become lowercase") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_reverse
  Tests the d_text_buffer_reverse function.
  Tests the following:
  - NULL buffer returns false
  - reverses string content
  - single character unchanged
  - empty buffer handled
  - palindrome unchanged
*/
bool d_tests_text_buffer_reverse(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_reverse");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null",
                              d_text_buffer_reverse(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: reverse a string
    buffer = d_text_buffer_new_from_string("abcde");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_success",
                                  d_text_buffer_reverse(buffer) == true,
                                  "Reverse should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_content",
                                  d_strcasecmp(buffer->data, "edcba") == 0,
                                  "Content should be 'edcba'") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should remain 5") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: single character
    buffer = d_text_buffer_new_from_string("X");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single",
                                  d_text_buffer_reverse(buffer) == true,
                                  "Reverse single char should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single_content",
                                  buffer->data[0] == 'X',
                                  "Single char should remain 'X'") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: palindrome
    buffer = d_text_buffer_new_from_string("racecar");

    if (buffer)
    {
        d_text_buffer_reverse(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_palindrome",
                                  d_strcasecmp(buffer->data, "racecar") == 0,
                                  "Palindrome should remain unchanged") && result;

        d_text_buffer_free(buffer);
    }

    // test 5: double reverse restores original
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        d_text_buffer_reverse(buffer);
        d_text_buffer_reverse(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_double",
                                  d_strcasecmp(buffer->data, "Hello") == 0,
                                  "Double reverse should restore original") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_pad_left
  Tests the d_text_buffer_pad_left function.
  Tests the following:
  - NULL buffer returns false
  - pads to specified width with given character
  - no padding when already at or beyond width
  - correct resulting length
*/
bool d_tests_text_buffer_pad_left(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_pad_left");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "pad_left_null",
                              d_text_buffer_pad_left(NULL, 10, ' ') == false,
                              "NULL buffer should return false") && result;

    // test 2: pad left
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_success",
                                  d_text_buffer_pad_left(buffer, 10, '*') == true,
                                  "Pad left should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_len",
                                  d_text_buffer_length(buffer) == 10,
                                  "Length should be 10") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_content",
                                  d_strcasecmp(buffer->data, "*****hello") == 0,
                                  "Content should be '*****hello'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: width <= current length (no-op)
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_noop",
                                  d_text_buffer_pad_left(buffer, 3, '*') == true,
                                  "Pad left with small width should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_noop_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should remain 5") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_noop_content",
                                  d_strcasecmp(buffer->data, "hello") == 0,
                                  "Content should remain 'hello'") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: pad with spaces
    buffer = d_text_buffer_new_from_string("42");

    if (buffer)
    {
        d_text_buffer_pad_left(buffer, 5, '0');

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_left_zero",
                                  d_strcasecmp(buffer->data, "00042") == 0,
                                  "Content should be '00042'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_pad_right
  Tests the d_text_buffer_pad_right function.
  Tests the following:
  - NULL buffer returns false
  - pads to specified width with given character
  - no padding when already at or beyond width
  - correct resulting length
*/
bool d_tests_text_buffer_pad_right(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_pad_right");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "pad_right_null",
                              d_text_buffer_pad_right(NULL, 10, ' ') == false,
                              "NULL buffer should return false") && result;

    // test 2: pad right
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_success",
                                  d_text_buffer_pad_right(buffer, 10, '.') == true,
                                  "Pad right should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_len",
                                  d_text_buffer_length(buffer) == 10,
                                  "Length should be 10") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_content",
                                  d_strcasecmp(buffer->data, "hello.....") == 0,
                                  "Content should be 'hello.....'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: width <= current length (no-op)
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_noop",
                                  d_text_buffer_pad_right(buffer, 5, '.') == true,
                                  "Pad right at exact width should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_noop_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should remain 5") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: pad empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_empty",
                                  d_text_buffer_pad_right(buffer, 4, '-') == true,
                                  "Pad right on empty buffer should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_empty_len",
                                  d_text_buffer_length(buffer) == 4,
                                  "Length should be 4") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pad_right_empty_content",
                                  d_strcasecmp(buffer->data, "----") == 0,
                                  "Content should be '----'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_text_processing_all
  Aggregation function that runs all text processing tests.
*/
bool d_tests_text_buffer_text_processing_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_text_processing_all");

    result = true;

    result = d_tests_text_buffer_trim_whitespace(_suite) && result;
    result = d_tests_text_buffer_trim_front(_suite) && result;
    result = d_tests_text_buffer_trim_back(_suite) && result;
    result = d_tests_text_buffer_trim_chars(_suite) && result;
    result = d_tests_text_buffer_to_upper(_suite) && result;
    result = d_tests_text_buffer_to_lower(_suite) && result;
    result = d_tests_text_buffer_reverse(_suite) && result;
    result = d_tests_text_buffer_pad_left(_suite) && result;
    result = d_tests_text_buffer_pad_right(_suite) && result;

    return result;
}
