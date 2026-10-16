/*******************************************************************************
* djinterp [container]                       text_buffer_tests_sa_modification.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_modification.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_set_string
  Tests the d_text_buffer_set_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - set replaces entire buffer content
  - empty string clears buffer content
  - set with longer string grows buffer
*/
bool
d_tests_sa_text_buffer_set_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_set_string(NULL, "test") == false,
        "set_str_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_set_string(buffer, NULL) == false,
            "set_str_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: replace content
        result = d_assert_standalone(
            d_text_buffer_set_string(buffer, "Replaced") == true,
            "set_str_replace",
            "Set string should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 8 &&
            d_strcasecmp(str, "Replaced") == 0,
            "set_str_replace_content",
            "Content should be 'Replaced'",
            _counter) && result;

        // test 4: set with empty string
        result = d_assert_standalone(
            d_text_buffer_set_string(buffer, "") == true,
            "set_str_empty",
            "Set empty string should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "set_str_empty_len",
            "Length should be 0 after setting empty",
            _counter) && result;

        // test 5: set with longer string
        result = d_assert_standalone(
            d_text_buffer_set_string(buffer,
                "A much longer replacement string") == true,
            "set_str_longer",
            "Set with longer string should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 32,
            "set_str_longer_len",
            "Length should be 32",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_set_buffer
  Tests the d_text_buffer_set_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length clears content
  - raw data replaces content with specified length
*/
bool
d_tests_sa_text_buffer_set_buffer
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;
    char                  data[4];

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_set_buffer(NULL, "ab", 2) == false,
        "set_buf_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL data
        result = d_assert_standalone(
            d_text_buffer_set_buffer(buffer, NULL, 5) == false,
            "set_buf_null_data",
            "NULL data should return false",
            _counter) && result;

        // test 3: set with raw data
        data[0] = 'X';
        data[1] = 'Y';
        data[2] = 'Z';

        result = d_assert_standalone(
            d_text_buffer_set_buffer(buffer, data, 3) == true,
            "set_buf_success",
            "Set buffer should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3 &&
            str[0] == 'X' && str[1] == 'Y' && str[2] == 'Z',
            "set_buf_content",
            "Content should be 'XYZ'",
            _counter) && result;

        // test 4: set with zero length -> returns false (no-op)
        result = d_assert_standalone(
            d_text_buffer_set_buffer(buffer, data, 0) == false,
            "set_buf_zero",
            "Set with zero length should return false",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3,
            "set_buf_zero_len",
            "Length should remain 3 after failed zero-length set",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_set_formatted
  Tests the d_text_buffer_set_formatted function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted set replaces entire content
  - multiple argument substitution
*/
bool
d_tests_sa_text_buffer_set_formatted
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_set_formatted(NULL, "test") == false,
        "set_fmt_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL format
        result = d_assert_standalone(
            d_text_buffer_set_formatted(buffer, NULL) == false,
            "set_fmt_null_fmt",
            "NULL format should return false",
            _counter) && result;

        // test 3: formatted set
        result = d_assert_standalone(
            d_text_buffer_set_formatted(buffer,
                                        "x=%d, y=%d", 10, 20) == true,
            "set_fmt_success",
            "Formatted set should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "x=10, y=20") == 0,
            "set_fmt_content",
            "Content should be 'x=10, y=20'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_replace_char
  Tests the d_text_buffer_replace_char function.
  Tests the following:
  - NULL buffer returns false
  - no matching characters leaves buffer unchanged
  - all occurrences replaced
  - length unchanged after replacement
*/
bool
d_tests_sa_text_buffer_replace_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_replace_char(NULL, 'a', 'b') == false,
        "replace_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("banana");

    if (buffer)
    {
        // test 2: replace 'a' with 'o'
        result = d_assert_standalone(
            d_text_buffer_replace_char(buffer, 'a', 'o') == true,
            "replace_char_success",
            "Replace char should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "bonono") == 0,
            "replace_char_content",
            "Content should be 'bonono'",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 6,
            "replace_char_len",
            "Length should be unchanged at 6",
            _counter) && result;

        // test 3: no match
        result = d_assert_standalone(
            d_text_buffer_replace_char(buffer, 'z', 'x') == true,
            "replace_char_no_match",
            "Replace with no match should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(d_text_buffer_get_string(buffer),
                         "bonono") == 0,
            "replace_char_no_match_content",
            "Content should be unchanged",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_replace_string
  Tests the d_text_buffer_replace_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL old/new string returns false
  - single occurrence replaced
  - multiple occurrences replaced
  - replacement with different length adjusts buffer
  - replacement with empty string removes occurrences
*/
bool
d_tests_sa_text_buffer_replace_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_replace_string(NULL, "a", "b") == false,
        "replace_str_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("foo bar foo baz foo");

    if (buffer)
    {
        // test 2: NULL old string
        result = d_assert_standalone(
            d_text_buffer_replace_string(buffer, NULL, "x") == false,
            "replace_str_null_old",
            "NULL old string should return false",
            _counter) && result;

        // test 3: NULL new string
        result = d_assert_standalone(
            d_text_buffer_replace_string(buffer, "x", NULL) == false,
            "replace_str_null_new",
            "NULL new string should return false",
            _counter) && result;

        // test 4: replace all "foo" with "qux"
        result = d_assert_standalone(
            d_text_buffer_replace_string(buffer, "foo", "qux") == true,
            "replace_str_all",
            "Replace all should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "qux bar qux baz qux") == 0,
            "replace_str_all_content",
            "Content should be 'qux bar qux baz qux'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 5: replace with shorter string
    buffer = d_text_buffer_new_from_string("aaa bbb aaa");

    if (buffer)
    {
        d_text_buffer_replace_string(buffer, "aaa", "x");

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "x bbb x") == 0,
            "replace_str_shorter",
            "Content should be 'x bbb x' after shorter replace",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_replace_range
  Tests the d_text_buffer_replace_range function.
  Tests the following:
  - NULL buffer returns false
  - NULL replacement returns false
  - out-of-bounds range returns false
  - replacement with same length
  - replacement with shorter string
  - replacement with longer string
*/
bool
d_tests_sa_text_buffer_replace_range
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_replace_range(NULL, 0, 3, "abc") == false,
        "replace_range_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: NULL replacement
        result = d_assert_standalone(
            d_text_buffer_replace_range(buffer, 0, 5, NULL) == false,
            "replace_range_null_repl",
            "NULL replacement should return false",
            _counter) && result;

        // test 3: replace "Hello" with "Greetings"
        result = d_assert_standalone(
            d_text_buffer_replace_range(buffer, 0, 5,
                                        "Greetings") == true,
            "replace_range_longer",
            "Replacing with longer string should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "Greetings, World!") == 0,
            "replace_range_longer_content",
            "Content should be 'Greetings, World!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: replace with shorter
    buffer = d_text_buffer_new_from_string("ABCDEF");

    if (buffer)
    {
        d_text_buffer_replace_range(buffer, 2, 4, "X");

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "ABXEF") == 0,
            "replace_range_shorter",
            "Content should be 'ABXEF'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_remove_char
  Tests the d_text_buffer_remove_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - removing first character
  - removing middle character
  - removing last character
  - length decremented correctly
*/
bool
d_tests_sa_text_buffer_remove_char
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_remove_char(NULL, 0) == false,
        "remove_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: remove middle character (index 2 = 'C')
        result = d_assert_standalone(
            d_text_buffer_remove_char(buffer, 2) == true,
            "remove_char_middle",
            "Remove middle char should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 4 &&
            d_strcasecmp(str, "ABDE") == 0,
            "remove_char_middle_content",
            "Content should be 'ABDE'",
            _counter) && result;

        // test 3: remove first character
        d_text_buffer_remove_char(buffer, 0);
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "BDE") == 0,
            "remove_char_first",
            "Content should be 'BDE'",
            _counter) && result;

        // test 4: remove last character
        d_text_buffer_remove_char(
            buffer, (d_index)(d_text_buffer_length(buffer) - 1));
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "BD") == 0,
            "remove_char_last",
            "Content should be 'BD'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_remove_range
  Tests the d_text_buffer_remove_range function.
  Tests the following:
  - NULL buffer returns false
  - start > end returns false
  - remove range from middle
  - remove range from beginning
  - remove entire content
*/
bool
d_tests_sa_text_buffer_remove_range
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_remove_range(NULL, 0, 5) == false,
        "remove_range_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: remove middle range [5, 12) => ", World"
        result = d_assert_standalone(
            d_text_buffer_remove_range(buffer, 5, 12) == true,
            "remove_range_middle",
            "Remove middle range should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "Hello!") == 0,
            "remove_range_middle_content",
            "Content should be 'Hello!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: remove from beginning
    buffer = d_text_buffer_new_from_string("ABCDEF");

    if (buffer)
    {
        d_text_buffer_remove_range(buffer, 0, 3);
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "DEF") == 0,
            "remove_range_begin",
            "Content should be 'DEF'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_consume_front
  Tests the d_text_buffer_consume_front function.
  Tests the following:
  - NULL buffer returns false
  - zero amount succeeds without change
  - consume removes leading characters
  - consuming all characters empties buffer
  - consuming more than length returns false or clamps
*/
bool
d_tests_sa_text_buffer_consume_front
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_consume_front(NULL, 3) == false,
        "consume_front_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: zero amount
        result = d_assert_standalone(
            d_text_buffer_consume_front(buffer, 0) == true,
            "consume_front_zero",
            "Zero consume should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "consume_front_zero_len",
            "Length should be unchanged",
            _counter) && result;

        // test 3: consume 7 from front
        result = d_assert_standalone(
            d_text_buffer_consume_front(buffer, 7) == true,
            "consume_front_partial",
            "Partial consume should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 6 &&
            d_strcasecmp(str, "World!") == 0,
            "consume_front_partial_content",
            "Content should be 'World!'",
            _counter) && result;

        // test 4: consume all remaining
        d_text_buffer_consume_front(buffer, 6);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0 &&
            d_text_buffer_is_empty(buffer) == true,
            "consume_front_all",
            "Consuming all should empty the buffer",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_consume_back
  Tests the d_text_buffer_consume_back function.
  Tests the following:
  - NULL buffer returns false
  - zero amount succeeds without change
  - consume removes trailing characters
  - consuming all characters empties buffer
*/
bool
d_tests_sa_text_buffer_consume_back
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_consume_back(NULL, 3) == false,
        "consume_back_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: zero amount
        result = d_assert_standalone(
            d_text_buffer_consume_back(buffer, 0) == true,
            "consume_back_zero",
            "Zero consume should succeed",
            _counter) && result;

        // test 3: consume 8 from back
        result = d_assert_standalone(
            d_text_buffer_consume_back(buffer, 8) == true,
            "consume_back_partial",
            "Partial consume should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5 &&
            d_strcasecmp(str, "Hello") == 0,
            "consume_back_partial_content",
            "Content should be 'Hello'",
            _counter) && result;

        // test 4: consume all
        d_text_buffer_consume_back(buffer, 5);
        result = d_assert_standalone(
            d_text_buffer_is_empty(buffer) == true,
            "consume_back_all",
            "Consuming all should empty the buffer",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_modification_all
  Aggregation function that runs all modification operation tests.
*/
bool
d_tests_sa_text_buffer_modification_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Modification Operations\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_text_buffer_set_string(_counter) && result;
    result = d_tests_sa_text_buffer_set_buffer(_counter) && result;
    result = d_tests_sa_text_buffer_set_formatted(_counter) && result;
    result = d_tests_sa_text_buffer_replace_char(_counter) && result;
    result = d_tests_sa_text_buffer_replace_string(_counter) && result;
    result = d_tests_sa_text_buffer_replace_range(_counter) && result;
    result = d_tests_sa_text_buffer_remove_char(_counter) && result;
    result = d_tests_sa_text_buffer_remove_range(_counter) && result;
    result = d_tests_sa_text_buffer_consume_front(_counter) && result;
    result = d_tests_sa_text_buffer_consume_back(_counter) && result;

    return result;
}
