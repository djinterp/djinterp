/*******************************************************************************
* djinterp [container]                          text_buffer_tests_modification.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_modification.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_set_string
  Tests the d_text_buffer_set_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - set replaces entire buffer content
  - empty string clears buffer content
  - set with longer string grows buffer
*/
bool d_tests_text_buffer_set_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_set_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "set_str_null_buf",
                              d_text_buffer_set_string(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_null_str",
                                  d_text_buffer_set_string(buffer, NULL) == false,
                                  "NULL string should return false") && result;

        // test 3: replace content
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_replace",
                                  d_text_buffer_set_string(buffer, "Replaced") == true,
                                  "Set string should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_replace_content",
                                  d_text_buffer_length(buffer) == 8 && d_strcasecmp(str, "Replaced") == 0,
                                  "Content should be 'Replaced'") && result;

        // test 4: set with empty string
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_empty",
                                  d_text_buffer_set_string(buffer, "") == true,
                                  "Set empty string should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_empty_len",
                                  d_text_buffer_length(buffer) == 0,
                                  "Length should be 0 after setting empty") && result;

        // test 5: set with longer string
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_longer",
                                  d_text_buffer_set_string(buffer, "A much longer replacement string") == true,
                                  "Set with longer string should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_str_longer_len",
                                  d_text_buffer_length(buffer) == 32,
                                  "Length should be 32") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_set_buffer
  Tests the d_text_buffer_set_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length clears content
  - raw data replaces content with specified length
*/
bool d_tests_text_buffer_set_buffer(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;
    char                  data[4];

    d_test_suite_test(_suite, "text_buffer_set_buffer");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "set_buf_null",
                              d_text_buffer_set_buffer(NULL, "ab", 2) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL data
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_buf_null_data",
                                  d_text_buffer_set_buffer(buffer, NULL, 5) == false,
                                  "NULL data should return false") && result;

        // test 3: set with raw data
        data[0] = 'X';
        data[1] = 'Y';
        data[2] = 'Z';

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_buf_success",
                                  d_text_buffer_set_buffer(buffer, data, 3) == true,
                                  "Set buffer should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_buf_content",
                                  d_text_buffer_length(buffer) == 3 && str[0] == 'X' && str[1] == 'Y' && str[2] == 'Z',
                                  "Content should be 'XYZ'") && result;

        // test 4: set with zero length -> returns false (no-op)
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_buf_zero",
                                  d_text_buffer_set_buffer(buffer, data, 0) == false,
                                  "Set with zero length should return false") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_buf_zero_len",
                                  d_text_buffer_length(buffer) == 3,
                                  "Length should remain 3 after failed zero-length set") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_set_formatted
  Tests the d_text_buffer_set_formatted function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted set replaces entire content
  - multiple argument substitution
*/
bool d_tests_text_buffer_set_formatted(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_set_formatted");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "set_fmt_null",
                              d_text_buffer_set_formatted(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Original");

    if (buffer)
    {
        // test 2: NULL format
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_fmt_null_fmt",
                                  d_text_buffer_set_formatted(buffer, NULL) == false,
                                  "NULL format should return false") && result;

        // test 3: formatted set
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_fmt_success",
                                  d_text_buffer_set_formatted(buffer, "x=%d, y=%d", 10, 20) == true,
                                  "Formatted set should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_fmt_content",
                                  d_strcasecmp(str, "x=10, y=20") == 0,
                                  "Content should be 'x=10, y=20'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_replace_char
  Tests the d_text_buffer_replace_char function.
  Tests the following:
  - NULL buffer returns false
  - no matching characters leaves buffer unchanged
  - all occurrences replaced
  - length unchanged after replacement
*/
bool d_tests_text_buffer_replace_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_replace_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "replace_char_null",
                              d_text_buffer_replace_char(NULL, 'a', 'b') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("banana");

    if (buffer)
    {
        // test 2: replace 'a' with 'o'
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_char_success",
                                  d_text_buffer_replace_char(buffer, 'a', 'o') == true,
                                  "Replace char should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_char_content",
                                  d_strcasecmp(str, "bonono") == 0,
                                  "Content should be 'bonono'") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_char_len",
                                  d_text_buffer_length(buffer) == 6,
                                  "Length should be unchanged at 6") && result;

        // test 3: no match
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_char_no_match",
                                  d_text_buffer_replace_char(buffer, 'z', 'x') == true,
                                  "Replace with no match should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_char_no_match_content",
                                  d_strcasecmp(d_text_buffer_get_string(buffer), "bonono") == 0,
                                  "Content should be unchanged") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_replace_string
  Tests the d_text_buffer_replace_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL old/new string returns false
  - single occurrence replaced
  - multiple occurrences replaced
  - replacement with different length adjusts buffer
  - replacement with empty string removes occurrences
*/
bool d_tests_text_buffer_replace_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_replace_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "replace_str_null",
                              d_text_buffer_replace_string(NULL, "a", "b") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("foo bar foo baz foo");

    if (buffer)
    {
        // test 2: NULL old string
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_str_null_old",
                                  d_text_buffer_replace_string(buffer, NULL, "x") == false,
                                  "NULL old string should return false") && result;

        // test 3: NULL new string
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_str_null_new",
                                  d_text_buffer_replace_string(buffer, "x", NULL) == false,
                                  "NULL new string should return false") && result;

        // test 4: replace all "foo" with "qux"
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_str_all",
                                  d_text_buffer_replace_string(buffer, "foo", "qux") == true,
                                  "Replace all should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_str_all_content",
                                  d_strcasecmp(str, "qux bar qux baz qux") == 0,
                                  "Content should be 'qux bar qux baz qux'") && result;

        d_text_buffer_free(buffer);
    }

    // test 5: replace with shorter string
    buffer = d_text_buffer_new_from_string("aaa bbb aaa");

    if (buffer)
    {
        d_text_buffer_replace_string(buffer, "aaa", "x");

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_str_shorter",
                                  d_strcasecmp(str, "x bbb x") == 0,
                                  "Content should be 'x bbb x' after shorter replace") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_replace_range
  Tests the d_text_buffer_replace_range function.
  Tests the following:
  - NULL buffer returns false
  - NULL replacement returns false
  - out-of-bounds range returns false
  - replacement with same length
  - replacement with shorter string
  - replacement with longer string
*/
bool d_tests_text_buffer_replace_range(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_replace_range");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "replace_range_null",
                              d_text_buffer_replace_range(NULL, 0, 3, "abc") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: NULL replacement
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_range_null_repl",
                                  d_text_buffer_replace_range(buffer, 0, 5, NULL) == false,
                                  "NULL replacement should return false") && result;

        // test 3: replace "Hello" with "Greetings"
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_range_longer",
                                  d_text_buffer_replace_range(buffer, 0, 5, "Greetings") == true,
                                  "Replacing with longer string should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_range_longer_content",
                                  d_strcasecmp(str, "Greetings, World!") == 0,
                                  "Content should be 'Greetings, World!'") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: replace with shorter
    buffer = d_text_buffer_new_from_string("ABCDEF");

    if (buffer)
    {
        d_text_buffer_replace_range(buffer, 2, 4, "X");

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "replace_range_shorter",
                                  d_strcasecmp(str, "ABXEF") == 0,
                                  "Content should be 'ABXEF'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_remove_char
  Tests the d_text_buffer_remove_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - removing first character
  - removing middle character
  - removing last character
  - length decremented correctly
*/
bool d_tests_text_buffer_remove_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_remove_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_char_null",
                              d_text_buffer_remove_char(NULL, 0) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: remove middle character (index 2 = 'C')
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_char_middle",
                                  d_text_buffer_remove_char(buffer, 2) == true,
                                  "Remove middle char should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_char_middle_content",
                                  d_text_buffer_length(buffer) == 4 && d_strcasecmp(str, "ABDE") == 0,
                                  "Content should be 'ABDE'") && result;

        // test 3: remove first character
        d_text_buffer_remove_char(buffer, 0);
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_char_first",
                                  d_strcasecmp(str, "BDE") == 0,
                                  "Content should be 'BDE'") && result;

        // test 4: remove last character
        d_text_buffer_remove_char(
            buffer, (dj_index)(d_text_buffer_length(buffer) - 1));
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_char_last",
                                  d_strcasecmp(str, "BD") == 0,
                                  "Content should be 'BD'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_remove_range
  Tests the d_text_buffer_remove_range function.
  Tests the following:
  - NULL buffer returns false
  - start > end returns false
  - remove range from middle
  - remove range from beginning
  - remove entire content
*/
bool d_tests_text_buffer_remove_range(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_remove_range");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_range_null",
                              d_text_buffer_remove_range(NULL, 0, 5) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: remove middle range [5, 12) => ", World"
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_middle",
                                  d_text_buffer_remove_range(buffer, 5, 12) == true,
                                  "Remove middle range should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_middle_content",
                                  d_strcasecmp(str, "Hello!") == 0,
                                  "Content should be 'Hello!'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: remove from beginning
    buffer = d_text_buffer_new_from_string("ABCDEF");

    if (buffer)
    {
        d_text_buffer_remove_range(buffer, 0, 3);
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_begin",
                                  d_strcasecmp(str, "DEF") == 0,
                                  "Content should be 'DEF'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_consume_front
  Tests the d_text_buffer_consume_front function.
  Tests the following:
  - NULL buffer returns false
  - zero amount succeeds without change
  - consume removes leading characters
  - consuming all characters empties buffer
  - consuming more than length returns false or clamps
*/
bool d_tests_text_buffer_consume_front(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_consume_front");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "consume_front_null",
                              d_text_buffer_consume_front(NULL, 3) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: zero amount
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_zero",
                                  d_text_buffer_consume_front(buffer, 0) == true,
                                  "Zero consume should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_zero_len",
                                  d_text_buffer_length(buffer) == 13,
                                  "Length should be unchanged") && result;

        // test 3: consume 7 from front
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_partial",
                                  d_text_buffer_consume_front(buffer, 7) == true,
                                  "Partial consume should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_partial_content",
                                  d_text_buffer_length(buffer) == 6 && d_strcasecmp(str, "World!") == 0,
                                  "Content should be 'World!'") && result;

        // test 4: consume all remaining
        d_text_buffer_consume_front(buffer, 6);
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_all",
                                  d_text_buffer_length(buffer) == 0 && d_text_buffer_is_empty(buffer) == true,
                                  "Consuming all should empty the buffer") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_consume_back
  Tests the d_text_buffer_consume_back function.
  Tests the following:
  - NULL buffer returns false
  - zero amount succeeds without change
  - consume removes trailing characters
  - consuming all characters empties buffer
*/
bool d_tests_text_buffer_consume_back(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_consume_back");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "consume_back_null",
                              d_text_buffer_consume_back(NULL, 3) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: zero amount
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_zero",
                                  d_text_buffer_consume_back(buffer, 0) == true,
                                  "Zero consume should succeed") && result;

        // test 3: consume 8 from back
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_partial",
                                  d_text_buffer_consume_back(buffer, 8) == true,
                                  "Partial consume should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_partial_content",
                                  d_text_buffer_length(buffer) == 5 && d_strcasecmp(str, "Hello") == 0,
                                  "Content should be 'Hello'") && result;

        // test 4: consume all
        d_text_buffer_consume_back(buffer, 5);
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_all",
                                  d_text_buffer_is_empty(buffer) == true,
                                  "Consuming all should empty the buffer") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_modification_all
  Aggregation function that runs all modification operation tests.
*/
bool d_tests_text_buffer_modification_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_modification_all");

    result = true;

    result = d_tests_text_buffer_set_string(_suite) && result;
    result = d_tests_text_buffer_set_buffer(_suite) && result;
    result = d_tests_text_buffer_set_formatted(_suite) && result;
    result = d_tests_text_buffer_replace_char(_suite) && result;
    result = d_tests_text_buffer_replace_string(_suite) && result;
    result = d_tests_text_buffer_replace_range(_suite) && result;
    result = d_tests_text_buffer_remove_char(_suite) && result;
    result = d_tests_text_buffer_remove_range(_suite) && result;
    result = d_tests_text_buffer_consume_front(_suite) && result;
    result = d_tests_text_buffer_consume_back(_suite) && result;

    return result;
}
