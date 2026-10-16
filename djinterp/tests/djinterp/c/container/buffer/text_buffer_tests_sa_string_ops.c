/*******************************************************************************
* djinterp [container]                         text_buffer_tests_sa_string_ops.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_string_ops.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_append_string
  Tests the d_text_buffer_append_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - append to empty buffer
  - append to non-empty buffer
  - multiple consecutive appends
  - result is null-terminated with correct length
*/
bool
d_tests_sa_text_buffer_append_string
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
        d_text_buffer_append_string(NULL, "test") == false,
        "append_str_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new(32);

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_append_string(buffer, NULL) == false,
            "append_str_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: append to empty buffer
        result = d_assert_standalone(
            d_text_buffer_append_string(buffer, "Hello") == true,
            "append_str_empty",
            "Append to empty should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "append_str_empty_len",
            "Length should be 5",
            _counter) && result;

        // test 4: append to non-empty
        result = d_assert_standalone(
            d_text_buffer_append_string(buffer, ", World!") == true,
            "append_str_nonempty",
            "Append to non-empty should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13 &&
            d_strcasecmp(str, "Hello, World!") == 0,
            "append_str_nonempty_content",
            "Content should be 'Hello, World!'",
            _counter) && result;

        // test 5: append empty string
        result = d_assert_standalone(
            d_text_buffer_append_string(buffer, "") == true,
            "append_str_empty_str",
            "Appending empty string should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "append_str_empty_str_len",
            "Length should remain 13 after empty append",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_string_n
  Tests the d_text_buffer_append_string_n function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - zero length append succeeds without changing buffer
  - partial string append
  - length-limited append does not exceed specified count
*/
bool
d_tests_sa_text_buffer_append_string_n
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
        d_text_buffer_append_string_n(NULL, "test", 4) == false,
        "append_str_n_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_append_string_n(buffer, NULL, 5) == false,
            "append_str_n_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: zero length -> returns false (no-op)
        result = d_assert_standalone(
            d_text_buffer_append_string_n(buffer, "XYZ", 0) == false,
            "append_str_n_zero",
            "Zero length append should return false",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "append_str_n_zero_len",
            "Length should be unchanged after zero append",
            _counter) && result;

        // test 4: partial append
        result = d_assert_standalone(
            d_text_buffer_append_string_n(buffer,
                                          ", World!!!", 8) == true,
            "append_str_n_partial",
            "Partial append should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13 &&
            d_strcasecmp(str, "Hello, World!") == 0,
            "append_str_n_partial_content",
            "Content should be 'Hello, World!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_buffer
  Tests the d_text_buffer_append_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length append succeeds
  - raw data append with specified length
*/
bool
d_tests_sa_text_buffer_append_buffer
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    char                  data[4];
    const char*           str;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_buffer(NULL, "ab", 2) == false,
        "append_buf_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: NULL data
        result = d_assert_standalone(
            d_text_buffer_append_buffer(buffer, NULL, 5) == false,
            "append_buf_null_data",
            "NULL data should return false",
            _counter) && result;

        // test 3: zero length -> returns false (no-op)
        result = d_assert_standalone(
            d_text_buffer_append_buffer(buffer, "XY", 0) == false,
            "append_buf_zero",
            "Zero length should return false",
            _counter) && result;

        // test 4: raw data append
        data[0] = 'C';
        data[1] = 'D';
        data[2] = 'E';
        data[3] = 'F';

        result = d_assert_standalone(
            d_text_buffer_append_buffer(buffer, data, 4) == true,
            "append_buf_raw",
            "Raw data append should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 6 &&
            d_strcasecmp(str, "ABCDEF") == 0,
            "append_buf_raw_content",
            "Content should be 'ABCDEF'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_char
  Tests the d_text_buffer_append_char function.
  Tests the following:
  - NULL buffer returns false
  - append single character to empty buffer
  - append multiple characters sequentially
  - result is null-terminated
*/
bool
d_tests_sa_text_buffer_append_char
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
        d_text_buffer_append_char(NULL, 'A') == false,
        "append_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new(16);

    if (buffer)
    {
        // test 2: append to empty
        result = d_assert_standalone(
            d_text_buffer_append_char(buffer, 'H') == true,
            "append_char_empty",
            "Append char to empty should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 1,
            "append_char_empty_len",
            "Length should be 1",
            _counter) && result;

        // test 3: append multiple
        d_text_buffer_append_char(buffer, 'i');
        d_text_buffer_append_char(buffer, '!');

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3 &&
            d_strcasecmp(str, "Hi!") == 0,
            "append_char_multi",
            "Content should be 'Hi!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_chars
  Tests the d_text_buffer_append_chars function.
  Tests the following:
  - NULL buffer returns false
  - zero count succeeds without change
  - appending multiple copies of a character
  - result has correct length and content
*/
bool
d_tests_sa_text_buffer_append_chars
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;
    size_t                i;
    bool                  all_match;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_chars(NULL, 'X', 5) == false,
        "append_chars_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: zero count -> returns false (no-op)
        result = d_assert_standalone(
            d_text_buffer_append_chars(buffer, 'X', 0) == false,
            "append_chars_zero",
            "Zero count should return false",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 2,
            "append_chars_zero_len",
            "Length should be unchanged",
            _counter) && result;

        // test 3: append 5 copies of '-'
        result = d_assert_standalone(
            d_text_buffer_append_chars(buffer, '-', 5) == true,
            "append_chars_multi",
            "Appending 5 chars should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 7,
            "append_chars_multi_len",
            "Length should be 7",
            _counter) && result;

        all_match = (str[0] == 'A') && (str[1] == 'B');
        for (i = 2; i < 7; i++)
        {
            if (str[i] != '-')
            {
                all_match = false;
            }
        }

        result = d_assert_standalone(
            all_match && str[7] == '\0',
            "append_chars_multi_content",
            "Content should be 'AB-----'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_formatted
  Tests the d_text_buffer_append_formatted function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted append with integer
  - formatted append with string
  - cumulative formatting
*/
bool
d_tests_sa_text_buffer_append_formatted
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
        d_text_buffer_append_formatted(NULL, "test") == false,
        "append_fmt_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Count: ");

    if (buffer)
    {
        // test 2: NULL format
        result = d_assert_standalone(
            d_text_buffer_append_formatted(buffer, NULL) == false,
            "append_fmt_null_fmt",
            "NULL format should return false",
            _counter) && result;

        // test 3: formatted append
        result = d_assert_standalone(
            d_text_buffer_append_formatted(buffer, "%d", 42) == true,
            "append_fmt_int",
            "Formatted int append should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "Count: 42") == 0,
            "append_fmt_int_content",
            "Content should be 'Count: 42'",
            _counter) && result;

        // test 4: cumulative formatting
        result = d_assert_standalone(
            d_text_buffer_append_formatted(buffer,
                                           " (%s)", "ok") == true,
            "append_fmt_cumulative",
            "Cumulative append should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "Count: 42 (ok)") == 0,
            "append_fmt_cumulative_content",
            "Content should be 'Count: 42 (ok)'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_buffer_obj
  Tests the d_text_buffer_append_buffer_obj function.
  Tests the following:
  - NULL destination returns false
  - NULL source returns false
  - successful append of one buffer to another
  - source buffer unmodified after append
*/
bool
d_tests_sa_text_buffer_append_buffer_obj
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* dest;
    struct d_text_buffer* src;
    const char*           str;

    result = true;

    // test 1: NULL destination
    result = d_assert_standalone(
        d_text_buffer_append_buffer_obj(NULL, NULL) == false,
        "append_buf_obj_null_dest",
        "NULL destination should return false",
        _counter) && result;

    dest = d_text_buffer_new_from_string("Hello");
    src  = d_text_buffer_new_from_string(", World!");

    if (dest && src)
    {
        // test 2: NULL source
        result = d_assert_standalone(
            d_text_buffer_append_buffer_obj(dest, NULL) == false,
            "append_buf_obj_null_src",
            "NULL source should return false",
            _counter) && result;

        // test 3: successful append
        result = d_assert_standalone(
            d_text_buffer_append_buffer_obj(dest, src) == true,
            "append_buf_obj_success",
            "Append buffer obj should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(dest);
        result = d_assert_standalone(
            d_text_buffer_length(dest) == 13 &&
            d_strcasecmp(str, "Hello, World!") == 0,
            "append_buf_obj_content",
            "Content should be 'Hello, World!'",
            _counter) && result;

        // test 4: source unmodified
        result = d_assert_standalone(
            d_text_buffer_length(src) == 8 &&
            d_strcasecmp(d_text_buffer_get_string(src),
                         ", World!") == 0,
            "append_buf_obj_src_intact",
            "Source should be unmodified",
            _counter) && result;
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
d_tests_sa_text_buffer_prepend_string
  Tests the d_text_buffer_prepend_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - prepend to empty buffer
  - prepend shifts existing content
  - result is null-terminated
*/
bool
d_tests_sa_text_buffer_prepend_string
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
        d_text_buffer_prepend_string(NULL, "test") == false,
        "prepend_str_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("World!");

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_prepend_string(buffer, NULL) == false,
            "prepend_str_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: prepend
        result = d_assert_standalone(
            d_text_buffer_prepend_string(buffer, "Hello, ") == true,
            "prepend_str_success",
            "Prepend should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13 &&
            d_strcasecmp(str, "Hello, World!") == 0,
            "prepend_str_content",
            "Content should be 'Hello, World!'",
            _counter) && result;

        // test 4: prepend empty string
        result = d_assert_standalone(
            d_text_buffer_prepend_string(buffer, "") == true,
            "prepend_str_empty",
            "Prepending empty string should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "prepend_str_empty_len",
            "Length should be unchanged after empty prepend",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_prepend_buffer
  Tests the d_text_buffer_prepend_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length succeeds without change
  - raw data prepend shifts existing content
*/
bool
d_tests_sa_text_buffer_prepend_buffer
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;
    char                  prefix[3];

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_prepend_buffer(NULL, "ab", 2) == false,
        "prepend_buf_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("CD");

    if (buffer)
    {
        // test 2: NULL data
        result = d_assert_standalone(
            d_text_buffer_prepend_buffer(buffer, NULL, 3) == false,
            "prepend_buf_null_data",
            "NULL data should return false",
            _counter) && result;

        // test 3: raw data prepend
        prefix[0] = 'A';
        prefix[1] = 'B';

        result = d_assert_standalone(
            d_text_buffer_prepend_buffer(buffer, prefix, 2) == true,
            "prepend_buf_success",
            "Prepend buffer should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 4 &&
            d_strcasecmp(str, "ABCD") == 0,
            "prepend_buf_content",
            "Content should be 'ABCD'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_prepend_char
  Tests the d_text_buffer_prepend_char function.
  Tests the following:
  - NULL buffer returns false
  - prepend char shifts existing content
  - multiple prepends build string in reverse
*/
bool
d_tests_sa_text_buffer_prepend_char
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
        d_text_buffer_prepend_char(NULL, 'X') == false,
        "prepend_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("C");

    if (buffer)
    {
        // test 2: prepend
        result = d_assert_standalone(
            d_text_buffer_prepend_char(buffer, 'B') == true,
            "prepend_char_success",
            "Prepend char should succeed",
            _counter) && result;

        // test 3: prepend again
        d_text_buffer_prepend_char(buffer, 'A');

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3 &&
            d_strcasecmp(str, "ABC") == 0,
            "prepend_char_multi",
            "Content should be 'ABC' after two prepends",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_insert_string
  Tests the d_text_buffer_insert_string function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - out-of-bounds index returns false
  - insert at beginning
  - insert at middle
  - insert at end (append position)
*/
bool
d_tests_sa_text_buffer_insert_string
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
        d_text_buffer_insert_string(NULL, 0, "test") == false,
        "insert_str_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AE");

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_insert_string(buffer, 0, NULL) == false,
            "insert_str_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: insert at middle
        result = d_assert_standalone(
            d_text_buffer_insert_string(buffer, 1, "BCD") == true,
            "insert_str_middle",
            "Insert at middle should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5 &&
            d_strcasecmp(str, "ABCDE") == 0,
            "insert_str_middle_content",
            "Content should be 'ABCDE'",
            _counter) && result;

        // test 4: insert at beginning
        result = d_assert_standalone(
            d_text_buffer_insert_string(buffer, 0, ">>") == true,
            "insert_str_begin",
            "Insert at beginning should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, ">>ABCDE") == 0,
            "insert_str_begin_content",
            "Content should be '>>ABCDE'",
            _counter) && result;

        // test 5: insert at end
        result = d_assert_standalone(
            d_text_buffer_insert_string(
                buffer, (d_index)d_text_buffer_length(buffer),
                "<<") == true,
            "insert_str_end",
            "Insert at end should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, ">>ABCDE<<") == 0,
            "insert_str_end_content",
            "Content should be '>>ABCDE<<'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_insert_buffer
  Tests the d_text_buffer_insert_buffer function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - zero length insert succeeds without change
  - raw data inserted at correct position
*/
bool
d_tests_sa_text_buffer_insert_buffer
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;
    char                  data[3];

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_insert_buffer(NULL, 0, "ab", 2) == false,
        "insert_buf_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AD");

    if (buffer)
    {
        // test 2: NULL data
        result = d_assert_standalone(
            d_text_buffer_insert_buffer(buffer, 1, NULL, 3) == false,
            "insert_buf_null_data",
            "NULL data should return false",
            _counter) && result;

        // test 3: insert raw data
        data[0] = 'B';
        data[1] = 'C';

        result = d_assert_standalone(
            d_text_buffer_insert_buffer(buffer, 1, data, 2) == true,
            "insert_buf_success",
            "Insert buffer should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 4 &&
            d_strcasecmp(str, "ABCD") == 0,
            "insert_buf_content",
            "Content should be 'ABCD'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_insert_char
  Tests the d_text_buffer_insert_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - insert at beginning
  - insert at middle
  - insert at end
*/
bool
d_tests_sa_text_buffer_insert_char
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
        d_text_buffer_insert_char(NULL, 0, 'X') == false,
        "insert_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AC");

    if (buffer)
    {
        // test 2: insert at middle
        result = d_assert_standalone(
            d_text_buffer_insert_char(buffer, 1, 'B') == true,
            "insert_char_middle",
            "Insert char at middle should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3 &&
            d_strcasecmp(str, "ABC") == 0,
            "insert_char_middle_content",
            "Content should be 'ABC'",
            _counter) && result;

        // test 3: insert at beginning
        result = d_assert_standalone(
            d_text_buffer_insert_char(buffer, 0, '_') == true,
            "insert_char_begin",
            "Insert at beginning should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, 0) == '_',
            "insert_char_begin_content",
            "First char should be '_'",
            _counter) && result;

        // test 4: insert at end
        result = d_assert_standalone(
            d_text_buffer_insert_char(
                buffer, (d_index)d_text_buffer_length(buffer),
                '!') == true,
            "insert_char_end",
            "Insert at end should succeed",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "_ABC!") == 0,
            "insert_char_end_content",
            "Content should be '_ABC!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_string_ops_all
  Aggregation function that runs all string operation (resize mode) tests.
*/
bool
d_tests_sa_text_buffer_string_ops_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] String Operations (Resize Mode)\n");
    printf("  ------------------------------------------\n");

    result = d_tests_sa_text_buffer_append_string(_counter) && result;
    result = d_tests_sa_text_buffer_append_string_n(_counter) && result;
    result = d_tests_sa_text_buffer_append_buffer(_counter) && result;
    result = d_tests_sa_text_buffer_append_char(_counter) && result;
    result = d_tests_sa_text_buffer_append_chars(_counter) && result;
    result = d_tests_sa_text_buffer_append_formatted(_counter) && result;
    result = d_tests_sa_text_buffer_append_buffer_obj(_counter)
             && result;
    result = d_tests_sa_text_buffer_prepend_string(_counter) && result;
    result = d_tests_sa_text_buffer_prepend_buffer(_counter) && result;
    result = d_tests_sa_text_buffer_prepend_char(_counter) && result;
    result = d_tests_sa_text_buffer_insert_string(_counter) && result;
    result = d_tests_sa_text_buffer_insert_buffer(_counter) && result;
    result = d_tests_sa_text_buffer_insert_char(_counter) && result;

    return result;
}
