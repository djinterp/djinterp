/*******************************************************************************
* djinterp [container]                    text_buffer_tests_sa_text_processing.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_text_processing.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_trim_whitespace
  Tests the d_text_buffer_trim_whitespace function.
  Tests the following:
  - NULL buffer returns false
  - trims leading and trailing whitespace
  - buffer with no whitespace unchanged
  - buffer of only whitespace becomes empty
  - mixed whitespace characters (spaces, tabs, newlines)
*/
bool
d_tests_sa_text_buffer_trim_whitespace
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_trim_whitespace(NULL) == false,
        "trim_ws_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: trim both sides
    buffer = d_text_buffer_new_from_string("  hello world  ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_whitespace(buffer) == true,
            "trim_ws_both",
            "Trim should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 11,
            "trim_ws_both_len",
            "Length should be 11 after trim",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "hello world") == 0,
            "trim_ws_both_content",
            "Content should be 'hello world'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no whitespace to trim
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_whitespace(buffer) == true,
            "trim_ws_none",
            "Trim with no whitespace should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "trim_ws_none_len",
            "Length should remain 5",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: only whitespace
    buffer = d_text_buffer_new_from_string("   \t\n  ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_whitespace(buffer) == true,
            "trim_ws_all",
            "Trimming all-whitespace should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "trim_ws_all_empty",
            "Length should be 0 after trimming all whitespace",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 5: mixed whitespace chars
    buffer = d_text_buffer_new_from_string("\t\n  data \r\n ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_whitespace(buffer) == true,
            "trim_ws_mixed",
            "Trim mixed whitespace should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "data") == 0,
            "trim_ws_mixed_content",
            "Content should be 'data'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_trim_front
  Tests the d_text_buffer_trim_front function.
  Tests the following:
  - NULL buffer returns false
  - trims only leading whitespace
  - trailing whitespace preserved
*/
bool
d_tests_sa_text_buffer_trim_front
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_trim_front(NULL) == false,
        "trim_front_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: trim leading only
    buffer = d_text_buffer_new_from_string("   hello   ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_front(buffer) == true,
            "trim_front_success",
            "Trim front should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "hello   ") == 0,
            "trim_front_content",
            "Content should be 'hello   '",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 8,
            "trim_front_len",
            "Length should be 8",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no leading whitespace
    buffer = d_text_buffer_new_from_string("hello   ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_front(buffer) == true,
            "trim_front_noop",
            "Trim front with no leading ws should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 8,
            "trim_front_noop_len",
            "Length should remain 8",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_trim_back
  Tests the d_text_buffer_trim_back function.
  Tests the following:
  - NULL buffer returns false
  - trims only trailing whitespace
  - leading whitespace preserved
*/
bool
d_tests_sa_text_buffer_trim_back
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_trim_back(NULL) == false,
        "trim_back_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: trim trailing only
    buffer = d_text_buffer_new_from_string("   hello   ");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_back(buffer) == true,
            "trim_back_success",
            "Trim back should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "   hello") == 0,
            "trim_back_content",
            "Content should be '   hello'",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 8,
            "trim_back_len",
            "Length should be 8",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no trailing whitespace
    buffer = d_text_buffer_new_from_string("   hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_back(buffer) == true,
            "trim_back_noop",
            "Trim back with no trailing ws should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 8,
            "trim_back_noop_len",
            "Length should remain 8",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_trim_chars
  Tests the d_text_buffer_trim_chars function.
  Tests the following:
  - NULL buffer returns false
  - NULL chars returns false
  - trims specified characters from both ends
  - characters in the middle preserved
*/
bool
d_tests_sa_text_buffer_trim_chars
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_trim_chars(NULL, "xy") == false,
        "trim_chars_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: trim custom characters
    buffer = d_text_buffer_new_from_string("xxyhelloxyyx");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_chars(buffer, "xy") == true,
            "trim_chars_success",
            "Trim chars should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "hello") == 0,
            "trim_chars_content",
            "Content should be 'hello'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: no matching chars to trim
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_chars(buffer, "xy") == true,
            "trim_chars_noop",
            "Trim with no matching chars should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "trim_chars_noop_len",
            "Length should remain 5",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: trim all characters
    buffer = d_text_buffer_new_from_string("aaabbb");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_trim_chars(buffer, "ab") == true,
            "trim_chars_all",
            "Trimming all chars should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "trim_chars_all_empty",
            "Length should be 0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_to_upper
  Tests the d_text_buffer_to_upper function.
  Tests the following:
  - NULL buffer returns false
  - converts lowercase to uppercase
  - non-alpha characters unchanged
  - already uppercase unchanged
*/
bool
d_tests_sa_text_buffer_to_upper
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_to_upper(NULL) == false,
        "to_upper_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: convert to uppercase
    buffer = d_text_buffer_new_from_string("hello world 123");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_to_upper(buffer) == true,
            "to_upper_success",
            "to_upper should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "HELLO WORLD 123") == 0,
            "to_upper_content",
            "Content should be 'HELLO WORLD 123'",
            _counter) && result;

        // verify case-sensitive: data should literally be uppercase
        result = d_assert_standalone(
            buffer->data[0] == 'H' && buffer->data[4] == 'O',
            "to_upper_exact",
            "Characters should be uppercase",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: already uppercase
    buffer = d_text_buffer_new_from_string("ABC");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_to_upper(buffer) == true,
            "to_upper_noop",
            "to_upper on uppercase should succeed",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data[0] == 'A' && buffer->data[1] == 'B'
            && buffer->data[2] == 'C',
            "to_upper_noop_content",
            "Content should remain 'ABC'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_to_lower
  Tests the d_text_buffer_to_lower function.
  Tests the following:
  - NULL buffer returns false
  - converts uppercase to lowercase
  - non-alpha characters unchanged
  - already lowercase unchanged
*/
bool
d_tests_sa_text_buffer_to_lower
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_to_lower(NULL) == false,
        "to_lower_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: convert to lowercase
    buffer = d_text_buffer_new_from_string("HELLO WORLD 123");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_to_lower(buffer) == true,
            "to_lower_success",
            "to_lower should succeed",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data[0] == 'h' && buffer->data[6] == 'w',
            "to_lower_content",
            "Characters should be lowercase",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 15,
            "to_lower_len",
            "Length should remain 15",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: mixed case
    buffer = d_text_buffer_new_from_string("HeLLo");

    if (buffer)
    {
        d_text_buffer_to_lower(buffer);

        result = d_assert_standalone(
            buffer->data[0] == 'h' && buffer->data[1] == 'e'
            && buffer->data[2] == 'l' && buffer->data[3] == 'l'
            && buffer->data[4] == 'o',
            "to_lower_mixed",
            "Mixed case should all become lowercase",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_reverse
  Tests the d_text_buffer_reverse function.
  Tests the following:
  - NULL buffer returns false
  - reverses string content
  - single character unchanged
  - empty buffer handled
  - palindrome unchanged
*/
bool
d_tests_sa_text_buffer_reverse
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_reverse(NULL) == false,
        "reverse_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: reverse a string
    buffer = d_text_buffer_new_from_string("abcde");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_reverse(buffer) == true,
            "reverse_success",
            "Reverse should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "edcba") == 0,
            "reverse_content",
            "Content should be 'edcba'",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "reverse_len",
            "Length should remain 5",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: single character
    buffer = d_text_buffer_new_from_string("X");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_reverse(buffer) == true,
            "reverse_single",
            "Reverse single char should succeed",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data[0] == 'X',
            "reverse_single_content",
            "Single char should remain 'X'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: palindrome
    buffer = d_text_buffer_new_from_string("racecar");

    if (buffer)
    {
        d_text_buffer_reverse(buffer);

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "racecar") == 0,
            "reverse_palindrome",
            "Palindrome should remain unchanged",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 5: double reverse restores original
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        d_text_buffer_reverse(buffer);
        d_text_buffer_reverse(buffer);

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "Hello") == 0,
            "reverse_double",
            "Double reverse should restore original",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_pad_left
  Tests the d_text_buffer_pad_left function.
  Tests the following:
  - NULL buffer returns false
  - pads to specified width with given character
  - no padding when already at or beyond width
  - correct resulting length
*/
bool
d_tests_sa_text_buffer_pad_left
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_pad_left(NULL, 10, ' ') == false,
        "pad_left_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: pad left
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_pad_left(buffer, 10, '*') == true,
            "pad_left_success",
            "Pad left should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 10,
            "pad_left_len",
            "Length should be 10",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "*****hello") == 0,
            "pad_left_content",
            "Content should be '*****hello'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: width <= current length (no-op)
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_pad_left(buffer, 3, '*') == true,
            "pad_left_noop",
            "Pad left with small width should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "pad_left_noop_len",
            "Length should remain 5",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "hello") == 0,
            "pad_left_noop_content",
            "Content should remain 'hello'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: pad with spaces
    buffer = d_text_buffer_new_from_string("42");

    if (buffer)
    {
        d_text_buffer_pad_left(buffer, 5, '0');

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "00042") == 0,
            "pad_left_zero",
            "Content should be '00042'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_pad_right
  Tests the d_text_buffer_pad_right function.
  Tests the following:
  - NULL buffer returns false
  - pads to specified width with given character
  - no padding when already at or beyond width
  - correct resulting length
*/
bool
d_tests_sa_text_buffer_pad_right
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_pad_right(NULL, 10, ' ') == false,
        "pad_right_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: pad right
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_pad_right(buffer, 10, '.') == true,
            "pad_right_success",
            "Pad right should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 10,
            "pad_right_len",
            "Length should be 10",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "hello.....") == 0,
            "pad_right_content",
            "Content should be 'hello.....'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: width <= current length (no-op)
    buffer = d_text_buffer_new_from_string("hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_pad_right(buffer, 5, '.') == true,
            "pad_right_noop",
            "Pad right at exact width should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "pad_right_noop_len",
            "Length should remain 5",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: pad empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_pad_right(buffer, 4, '-') == true,
            "pad_right_empty",
            "Pad right on empty buffer should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 4,
            "pad_right_empty_len",
            "Length should be 4",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(buffer->data, "----") == 0,
            "pad_right_empty_content",
            "Content should be '----'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_text_processing_all
  Aggregation function that runs all text processing tests.
*/
bool
d_tests_sa_text_buffer_text_processing_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Text Processing Operations\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_text_buffer_trim_whitespace(_counter) && result;
    result = d_tests_sa_text_buffer_trim_front(_counter) && result;
    result = d_tests_sa_text_buffer_trim_back(_counter) && result;
    result = d_tests_sa_text_buffer_trim_chars(_counter) && result;
    result = d_tests_sa_text_buffer_to_upper(_counter) && result;
    result = d_tests_sa_text_buffer_to_lower(_counter) && result;
    result = d_tests_sa_text_buffer_reverse(_counter) && result;
    result = d_tests_sa_text_buffer_pad_left(_counter) && result;
    result = d_tests_sa_text_buffer_pad_right(_counter) && result;

    return result;
}
