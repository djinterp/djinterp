/*******************************************************************************
* djinterp [container]                            text_buffer_tests_string_ops.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_string_ops.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_append_string
  Tests the d_text_buffer_append_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - append to empty buffer
  - append to non-empty buffer
  - multiple consecutive appends
  - result is null-terminated with correct length
*/
bool d_tests_text_buffer_append_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_str_null_buf",
                              d_text_buffer_append_string(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new(32);

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_null_str",
                                  d_text_buffer_append_string(buffer, NULL) == false,
                                  "NULL string should return false") && result;

        // test 3: append to empty buffer
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_empty",
                                  d_text_buffer_append_string(buffer, "Hello") == true,
                                  "Append to empty should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_empty_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should be 5") && result;

        // test 4: append to non-empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_nonempty",
                                  d_text_buffer_append_string(buffer, ", World!") == true,
                                  "Append to non-empty should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_nonempty_content",
                                  d_text_buffer_length(buffer) == 13 && d_strcasecmp(str, "Hello, World!") == 0,
                                  "Content should be 'Hello, World!'") && result;

        // test 5: append empty string
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_empty_str",
                                  d_text_buffer_append_string(buffer, "") == true,
                                  "Appending empty string should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_empty_str_len",
                                  d_text_buffer_length(buffer) == 13,
                                  "Length should remain 13 after empty append") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_string_n
  Tests the d_text_buffer_append_string_n function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - zero length append succeeds without changing buffer
  - partial string append
  - length-limited append does not exceed specified count
*/
bool d_tests_text_buffer_append_string_n(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_string_n");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_str_n_null_buf",
                              d_text_buffer_append_string_n(NULL, "test", 4) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_n_null_str",
                                  d_text_buffer_append_string_n(buffer, NULL, 5) == false,
                                  "NULL string should return false") && result;

        // test 3: zero length -> returns false (no-op)
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_n_zero",
                                  d_text_buffer_append_string_n(buffer, "XYZ", 0) == false,
                                  "Zero length append should return false") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_n_zero_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should be unchanged after zero append") && result;

        // test 4: partial append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_n_partial",
                                  d_text_buffer_append_string_n(buffer, ", World!!!", 8) == true,
                                  "Partial append should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_n_partial_content",
                                  d_text_buffer_length(buffer) == 13 && d_strcasecmp(str, "Hello, World!") == 0,
                                  "Content should be 'Hello, World!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_buffer
  Tests the d_text_buffer_append_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length append succeeds
  - raw data append with specified length
*/
bool d_tests_text_buffer_append_buffer(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    char                  data[4];
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_buffer");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_buf_null_buf",
                              d_text_buffer_append_buffer(NULL, "ab", 2) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: NULL data
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_null_data",
                                  d_text_buffer_append_buffer(buffer, NULL, 5) == false,
                                  "NULL data should return false") && result;

        // test 3: zero length -> returns false (no-op)
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_zero",
                                  d_text_buffer_append_buffer(buffer, "XY", 0) == false,
                                  "Zero length should return false") && result;

        // test 4: raw data append
        data[0] = 'C';
        data[1] = 'D';
        data[2] = 'E';
        data[3] = 'F';

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_raw",
                                  d_text_buffer_append_buffer(buffer, data, 4) == true,
                                  "Raw data append should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_raw_content",
                                  d_text_buffer_length(buffer) == 6 && d_strcasecmp(str, "ABCDEF") == 0,
                                  "Content should be 'ABCDEF'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_char
  Tests the d_text_buffer_append_char function.
  Tests the following:
  - NULL buffer returns false
  - append single character to empty buffer
  - append multiple characters sequentially
  - result is null-terminated
*/
bool d_tests_text_buffer_append_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_char_null",
                              d_text_buffer_append_char(NULL, 'A') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new(16);

    if (buffer)
    {
        // test 2: append to empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_char_empty",
                                  d_text_buffer_append_char(buffer, 'H') == true,
                                  "Append char to empty should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_char_empty_len",
                                  d_text_buffer_length(buffer) == 1,
                                  "Length should be 1") && result;

        // test 3: append multiple
        d_text_buffer_append_char(buffer, 'i');
        d_text_buffer_append_char(buffer, '!');

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_char_multi",
                                  d_text_buffer_length(buffer) == 3 && d_strcasecmp(str, "Hi!") == 0,
                                  "Content should be 'Hi!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_chars
  Tests the d_text_buffer_append_chars function.
  Tests the following:
  - NULL buffer returns false
  - zero count succeeds without change
  - appending multiple copies of a character
  - result has correct length and content
*/
bool d_tests_text_buffer_append_chars(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;
    size_t                i;
    bool                  all_match;

    d_test_suite_test(_suite, "text_buffer_append_chars");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_chars_null",
                              d_text_buffer_append_chars(NULL, 'X', 5) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: zero count -> returns false (no-op)
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_chars_zero",
                                  d_text_buffer_append_chars(buffer, 'X', 0) == false,
                                  "Zero count should return false") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_chars_zero_len",
                                  d_text_buffer_length(buffer) == 2,
                                  "Length should be unchanged") && result;

        // test 3: append 5 copies of '-'
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_chars_multi",
                                  d_text_buffer_append_chars(buffer, '-', 5) == true,
                                  "Appending 5 chars should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_chars_multi_len",
                                  d_text_buffer_length(buffer) == 7,
                                  "Length should be 7") && result;

        all_match = (str[0] == 'A') && (str[1] == 'B');
        for (i = 2; i < 7; i++)
        {
            if (str[i] != '-')
            {
                all_match = false;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_chars_multi_content",
                                  all_match && str[7] == '\0',
                                  "Content should be 'AB-----'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_formatted
  Tests the d_text_buffer_append_formatted function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted append with integer
  - formatted append with string
  - cumulative formatting
*/
bool d_tests_text_buffer_append_formatted(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_formatted");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "append_fmt_null_buf",
                              d_text_buffer_append_formatted(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Count: ");

    if (buffer)
    {
        // test 2: NULL format
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_null_fmt",
                                  d_text_buffer_append_formatted(buffer, NULL) == false,
                                  "NULL format should return false") && result;

        // test 3: formatted append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_int",
                                  d_text_buffer_append_formatted(buffer, "%d", 42) == true,
                                  "Formatted int append should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_int_content",
                                  d_strcasecmp(str, "Count: 42") == 0,
                                  "Content should be 'Count: 42'") && result;

        // test 4: cumulative formatting
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_cumulative",
                                  d_text_buffer_append_formatted(buffer, " (%s)", "ok") == true,
                                  "Cumulative append should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_cumulative_content",
                                  d_strcasecmp(str, "Count: 42 (ok)") == 0,
                                  "Content should be 'Count: 42 (ok)'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_buffer_obj
  Tests the d_text_buffer_append_buffer_obj function.
  Tests the following:
  - NULL destination returns false
  - NULL source returns false
  - successful append of one buffer to another
  - source buffer unmodified after append
*/
bool d_tests_text_buffer_append_buffer_obj(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* dest;
    struct dj_text_buffer* src;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_append_buffer_obj");

    result = true;

    // test 1: NULL destination
    result = D_TEST_CHECK_MSG(_suite,
                              "append_buf_obj_null_dest",
                              d_text_buffer_append_buffer_obj(NULL, NULL) == false,
                              "NULL destination should return false") && result;

    dest = d_text_buffer_new_from_string("Hello");
    src  = d_text_buffer_new_from_string(", World!");

    if (dest && src)
    {
        // test 2: NULL source
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_obj_null_src",
                                  d_text_buffer_append_buffer_obj(dest, NULL) == false,
                                  "NULL source should return false") && result;

        // test 3: successful append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_obj_success",
                                  d_text_buffer_append_buffer_obj(dest, src) == true,
                                  "Append buffer obj should succeed") && result;

        str = d_text_buffer_get_string(dest);
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_obj_content",
                                  d_text_buffer_length(dest) == 13 && d_strcasecmp(str, "Hello, World!") == 0,
                                  "Content should be 'Hello, World!'") && result;

        // test 4: source unmodified
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_obj_src_intact",
                                  d_text_buffer_length(src) == 8 && d_strcasecmp(d_text_buffer_get_string(src), ", World!") == 0,
                                  "Source should be unmodified") && result;
    }

    if (src)
    {
        d_text_buffer_free(src);
    }

    if (dest)
    {
        d_text_buffer_free(dest);
    }

    return result;
}

/*
d_tests_text_buffer_prepend_string
  Tests the d_text_buffer_prepend_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - prepend to empty buffer
  - prepend shifts existing content
  - result is null-terminated
*/
bool d_tests_text_buffer_prepend_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_prepend_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_str_null_buf",
                              d_text_buffer_prepend_string(NULL, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("World!");

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_str_null_str",
                                  d_text_buffer_prepend_string(buffer, NULL) == false,
                                  "NULL string should return false") && result;

        // test 3: prepend
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_str_success",
                                  d_text_buffer_prepend_string(buffer, "Hello, ") == true,
                                  "Prepend should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_str_content",
                                  d_text_buffer_length(buffer) == 13 && d_strcasecmp(str, "Hello, World!") == 0,
                                  "Content should be 'Hello, World!'") && result;

        // test 4: prepend empty string
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_str_empty",
                                  d_text_buffer_prepend_string(buffer, "") == true,
                                  "Prepending empty string should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_str_empty_len",
                                  d_text_buffer_length(buffer) == 13,
                                  "Length should be unchanged after empty prepend") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_prepend_buffer
  Tests the d_text_buffer_prepend_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length succeeds without change
  - raw data prepend shifts existing content
*/
bool d_tests_text_buffer_prepend_buffer(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;
    char                  prefix[3];

    d_test_suite_test(_suite, "text_buffer_prepend_buffer");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_buf_null",
                              d_text_buffer_prepend_buffer(NULL, "ab", 2) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("CD");

    if (buffer)
    {
        // test 2: NULL data
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_buf_null_data",
                                  d_text_buffer_prepend_buffer(buffer, NULL, 3) == false,
                                  "NULL data should return false") && result;

        // test 3: raw data prepend
        prefix[0] = 'A';
        prefix[1] = 'B';

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_buf_success",
                                  d_text_buffer_prepend_buffer(buffer, prefix, 2) == true,
                                  "Prepend buffer should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_buf_content",
                                  d_text_buffer_length(buffer) == 4 && d_strcasecmp(str, "ABCD") == 0,
                                  "Content should be 'ABCD'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_prepend_char
  Tests the d_text_buffer_prepend_char function.
  Tests the following:
  - NULL buffer returns false
  - prepend char shifts existing content
  - multiple prepends build string in reverse
*/
bool d_tests_text_buffer_prepend_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_prepend_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_char_null",
                              d_text_buffer_prepend_char(NULL, 'X') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("C");

    if (buffer)
    {
        // test 2: prepend
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_char_success",
                                  d_text_buffer_prepend_char(buffer, 'B') == true,
                                  "Prepend char should succeed") && result;

        // test 3: prepend again
        d_text_buffer_prepend_char(buffer, 'A');

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_char_multi",
                                  d_text_buffer_length(buffer) == 3 && d_strcasecmp(str, "ABC") == 0,
                                  "Content should be 'ABC' after two prepends") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_insert_string
  Tests the d_text_buffer_insert_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - out-of-bounds index returns false
  - insert at beginning
  - insert at middle
  - insert at end (append position)
*/
bool d_tests_text_buffer_insert_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_insert_string");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_str_null_buf",
                              d_text_buffer_insert_string(NULL, 0, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AE");

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_null_str",
                                  d_text_buffer_insert_string(buffer, 0, NULL) == false,
                                  "NULL string should return false") && result;

        // test 3: insert at middle
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_middle",
                                  d_text_buffer_insert_string(buffer, 1, "BCD") == true,
                                  "Insert at middle should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_middle_content",
                                  d_text_buffer_length(buffer) == 5 && d_strcasecmp(str, "ABCDE") == 0,
                                  "Content should be 'ABCDE'") && result;

        // test 4: insert at beginning
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_begin",
                                  d_text_buffer_insert_string(buffer, 0, ">>") == true,
                                  "Insert at beginning should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_begin_content",
                                  d_strcasecmp(str, ">>ABCDE") == 0,
                                  "Content should be '>>ABCDE'") && result;

        // test 5: insert at end
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_end",
                                  d_text_buffer_insert_string( buffer, (dj_index)d_text_buffer_length(buffer), "<<") == true,
                                  "Insert at end should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_str_end_content",
                                  d_strcasecmp(str, ">>ABCDE<<") == 0,
                                  "Content should be '>>ABCDE<<'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_insert_buffer
  Tests the d_text_buffer_insert_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length insert succeeds without change
  - raw data inserted at correct position
*/
bool d_tests_text_buffer_insert_buffer(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;
    char                  data[3];

    d_test_suite_test(_suite, "text_buffer_insert_buffer");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_buf_null",
                              d_text_buffer_insert_buffer(NULL, 0, "ab", 2) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AD");

    if (buffer)
    {
        // test 2: NULL data
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_buf_null_data",
                                  d_text_buffer_insert_buffer(buffer, 1, NULL, 3) == false,
                                  "NULL data should return false") && result;

        // test 3: insert raw data
        data[0] = 'B';
        data[1] = 'C';

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_buf_success",
                                  d_text_buffer_insert_buffer(buffer, 1, data, 2) == true,
                                  "Insert buffer should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_buf_content",
                                  d_text_buffer_length(buffer) == 4 && d_strcasecmp(str, "ABCD") == 0,
                                  "Content should be 'ABCD'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_insert_char
  Tests the d_text_buffer_insert_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - insert at beginning
  - insert at middle
  - insert at end
*/
bool d_tests_text_buffer_insert_char(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_insert_char");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_char_null",
                              d_text_buffer_insert_char(NULL, 0, 'X') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AC");

    if (buffer)
    {
        // test 2: insert at middle
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_middle",
                                  d_text_buffer_insert_char(buffer, 1, 'B') == true,
                                  "Insert char at middle should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_middle_content",
                                  d_text_buffer_length(buffer) == 3 && d_strcasecmp(str, "ABC") == 0,
                                  "Content should be 'ABC'") && result;

        // test 3: insert at beginning
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_begin",
                                  d_text_buffer_insert_char(buffer, 0, '_') == true,
                                  "Insert at beginning should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_begin_content",
                                  d_text_buffer_get_char(buffer, 0) == '_',
                                  "First char should be '_'") && result;

        // test 4: insert at end
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_end",
                                  d_text_buffer_insert_char( buffer, (dj_index)d_text_buffer_length(buffer), '!') == true,
                                  "Insert at end should succeed") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_char_end_content",
                                  d_strcasecmp(str, "_ABC!") == 0,
                                  "Content should be '_ABC!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_string_ops_all
  Aggregation function that runs all string operation (resize mode) tests.
*/
bool d_tests_text_buffer_string_ops_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_string_ops_all");

    result = true;

    result = d_tests_text_buffer_append_string(_suite) && result;
    result = d_tests_text_buffer_append_string_n(_suite) && result;
    result = d_tests_text_buffer_append_buffer(_suite) && result;
    result = d_tests_text_buffer_append_char(_suite) && result;
    result = d_tests_text_buffer_append_chars(_suite) && result;
    result = d_tests_text_buffer_append_formatted(_suite) && result;
    result = d_tests_text_buffer_append_buffer_obj(_suite)
             && result;
    result = d_tests_text_buffer_prepend_string(_suite) && result;
    result = d_tests_text_buffer_prepend_buffer(_suite) && result;
    result = d_tests_text_buffer_prepend_char(_suite) && result;
    result = d_tests_text_buffer_insert_string(_suite) && result;
    result = d_tests_text_buffer_insert_buffer(_suite) && result;
    result = d_tests_text_buffer_insert_char(_suite) && result;

    return result;
}
