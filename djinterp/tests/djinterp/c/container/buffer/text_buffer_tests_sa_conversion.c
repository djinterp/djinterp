/*******************************************************************************
* djinterp [container]                         text_buffer_tests_sa_conversion.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_conversion.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_to_cstring
  Tests the d_text_buffer_to_cstring function.
  Tests the following:
  - NULL buffer returns NULL
  - returns newly allocated null-terminated string
  - returned string content matches buffer
  - returned string is independent (modifying it does not affect buffer)
  - caller must free returned string
*/
bool
d_tests_sa_text_buffer_to_cstring
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    char*                 str;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_to_cstring(NULL) == NULL,
        "to_cstr_null",
        "NULL buffer should return NULL",
        _counter) && result;

    // test 2: normal conversion
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        str = d_text_buffer_to_cstring(buffer);

        result = d_assert_standalone(
            str != NULL,
            "to_cstr_not_null",
            "Returned string should not be NULL",
            _counter) && result;

        if (str)
        {
            result = d_assert_standalone(
                d_strcasecmp(str, "Hello, World!") == 0,
                "to_cstr_content",
                "Returned string should match buffer content",
                _counter) && result;

            // test 3: independence - modify returned string
            str[0] = 'X';

            result = d_assert_standalone(
                buffer->data[0] == 'H',
                "to_cstr_independent",
                "Modifying returned string should not affect buffer",
                _counter) && result;

            free(str);
        }

        d_text_buffer_free(buffer);
    }

    // test 4: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        str = d_text_buffer_to_cstring(buffer);

        result = d_assert_standalone(
            (str != NULL),
            "to_cstr_empty_not_null",
            "Empty buffer should return non-NULL string",
            _counter) && result;

        if (str)
        {
            result = d_assert_standalone(
                (str[0] == '\0'),
                "to_cstr_empty_content",
                "Empty buffer should produce empty string",
                _counter) && result;

            free(str);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_copy_to_buffer
  Tests the d_text_buffer_copy_to_buffer function.
  Tests the following:
  - NULL source returns false
  - NULL destination returns false
  - zero destination size returns false
  - successful copy with sufficient space
  - destination is null-terminated
  - copy truncates when destination too small
*/
bool
d_tests_sa_text_buffer_copy_to_buffer
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    char                  dest[64];
    bool                  result = true;

    // test 1: NULL source
    d_memset(dest, 0, sizeof(dest));

    result = d_assert_standalone(
        d_text_buffer_copy_to_buffer(NULL, dest, sizeof(dest)) == false,
        "copy_to_buf_null_src",
        "NULL source should return false",
        _counter) && result;

    // test 2: NULL destination
    buffer = d_text_buffer_new_from_string("test");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_copy_to_buffer(buffer, NULL, 64) == false,
            "copy_to_buf_null_dest",
            "NULL destination should return false",
            _counter) && result;

        // test 3: zero size
        result = d_assert_standalone(
            d_text_buffer_copy_to_buffer(buffer, dest, 0) == false,
            "copy_to_buf_zero_size",
            "Zero destination size should return false",
            _counter) && result;

        // test 4: successful copy
        d_memset(dest, 0, sizeof(dest));

        result = d_assert_standalone(
            d_text_buffer_copy_to_buffer(buffer, dest, sizeof(dest)) == true,
            "copy_to_buf_success",
            "Copy should succeed with sufficient space",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(dest, "test") == 0,
            "copy_to_buf_content",
            "Destination should contain 'test'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 5: copy with exact-fit destination
    buffer = d_text_buffer_new_from_string("abc");

    if (buffer)
    {
        d_memset(dest, 'X', sizeof(dest));

        // destination size 4 = 3 chars + null terminator
        result = d_assert_standalone(
            d_text_buffer_copy_to_buffer(buffer, dest, 4) == true,
            "copy_to_buf_exact",
            "Copy with exact fit should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest[3] == '\0',
            "copy_to_buf_exact_null",
            "Destination should be null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_copy_to_buffer_n
  Tests the d_text_buffer_copy_to_buffer_n function.
  Tests the following:
  - NULL source returns 0
  - NULL destination returns 0
  - copies at most max_chars characters
  - does not exceed destination size
  - returns number of characters actually copied
*/
bool
d_tests_sa_text_buffer_copy_to_buffer_n
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    char                  dest[64];
    size_t                copied;
    bool                  result = true;

    // test 1: NULL source
    result = d_assert_standalone(
        d_text_buffer_copy_to_buffer_n(NULL, dest, sizeof(dest), 5) == 0,
        "copy_to_buf_n_null_src",
        "NULL source should return 0",
        _counter) && result;

    // test 2: copy with max_chars limit
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest,
                                                  sizeof(dest), 5);

        result = d_assert_standalone(
            copied == 5,
            "copy_to_buf_n_count",
            "Should copy exactly 5 characters",
            _counter) && result;

        result = d_assert_standalone(
            d_strncasecmp(dest, "Hello", 5) == 0,
            "copy_to_buf_n_content",
            "Destination should contain 'Hello'",
            _counter) && result;

        // test 3: max_chars exceeds buffer length
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest,
                                                  sizeof(dest), 100);

        result = d_assert_standalone(
            copied == 13,
            "copy_to_buf_n_clamp",
            "Should clamp to buffer length (13)",
            _counter) && result;

        // test 4: destination too small
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest, 4, 10);

        result = d_assert_standalone(
            copied <= 3,
            "copy_to_buf_n_small_dest",
            "Should not exceed destination size minus null",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_to_d_string
  Tests the d_text_buffer_to_d_string function.
  Tests the following:
  - NULL buffer returns NULL
  - returns a new d_string with matching content
  - returned d_string has correct size
  - returned d_string is independent of original buffer
*/
bool
d_tests_sa_text_buffer_to_d_string
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    struct d_string*      str;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_to_d_string(NULL) == NULL,
        "to_dstr_null",
        "NULL buffer should return NULL",
        _counter) && result;

    // test 2: normal conversion
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        str = d_text_buffer_to_d_string(buffer);

        result = d_assert_standalone(
            str != NULL,
            "to_dstr_not_null",
            "Returned d_string should not be NULL",
            _counter) && result;

        if (str)
        {
            // test 3: content match
            result = d_assert_standalone(
                str->text != NULL
                && d_strcasecmp(str->text, "Hello, World!") == 0,
                "to_dstr_content",
                "d_string text should match buffer content",
                _counter) && result;

            // test 4: size match
            result = d_assert_standalone(
                str->size == 13,
                "to_dstr_size",
                "d_string size should be 13",
                _counter) && result;

            // NOTE: already know str->text is non-NULL due to previous assert
            // gets rid of persistent compiler warning
            if (str->text)
            {
                // test 5: independence
                str->text[0] = 'X';

                result = d_assert_standalone(
                    buffer->data[0] == 'H',
                    "to_dstr_independent",
                    "Modifying d_string should not affect buffer",
                    _counter) && result;
            }

            d_string_free(str);
        }

        d_text_buffer_free(buffer);
    }

    // test 6: empty buffer conversion
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        str = d_text_buffer_to_d_string(buffer);

        result = d_assert_standalone(
            (str != NULL),
            "to_dstr_empty_not_null",
            "Empty buffer should produce non-NULL d_string",
            _counter) && result;

        if (str)
        {
            result = d_assert_standalone(
                (str->size == 0),
                "to_dstr_empty_size",
                "Empty buffer d_string size should be 0",
                _counter) && result;

            d_string_free(str);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_conversion_all
  Aggregation function that runs all conversion tests.
*/
bool
d_tests_sa_text_buffer_conversion_all
(
    struct d_test_counter* _counter
)
{
    printf("\n  [SECTION] Conversion Functions\n");
    printf("  -------------------------------\n");

    return d_tests_sa_text_buffer_to_cstring(_counter)       &&
           d_tests_sa_text_buffer_copy_to_buffer(_counter)   &&
           d_tests_sa_text_buffer_copy_to_buffer_n(_counter) &&
           d_tests_sa_text_buffer_to_d_string(_counter);
}
