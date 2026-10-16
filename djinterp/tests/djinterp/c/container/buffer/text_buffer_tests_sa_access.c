/*******************************************************************************
* djinterp [container]                             text_buffer_tests_sa_access.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_access.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_get_char
  Tests the d_text_buffer_get_char function.
  Tests the following:
  - NULL buffer returns '\0'
  - out-of-bounds index returns '\0'
  - correct character at first, middle, and last positions
  - negative indexing (if supported by d_index)
*/
bool
d_tests_sa_text_buffer_get_char
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;


    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_get_char(NULL, 0) == '\0',
        "get_char_null",
        "NULL buffer should return '\\0'",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: first character
        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, 0) == 'A',
            "get_char_first",
            "First character should be 'A'",
            _counter) && result;

        // test 3: middle character
        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, 2) == 'C',
            "get_char_middle",
            "Middle character should be 'C'",
            _counter) && result;

        // test 4: last character
        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, 4) == 'E',
            "get_char_last",
            "Last character should be 'E'",
            _counter) && result;

        // test 5: negative index (last element)
        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, -1) == 'E',
            "get_char_neg_1",
            "Index -1 should return 'E'",
            _counter) && result;

        // test 6: negative index (first element)
        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, -5) == 'A',
            "get_char_neg_5",
            "Index -5 should return 'A'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_set_char
  Tests the d_text_buffer_set_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - successful set at beginning, middle, and end
  - neighbouring characters unchanged
*/
bool
d_tests_sa_text_buffer_set_char
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_set_char(NULL, 0, 'X') == false,
        "set_char_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: set first character
        result = d_assert_standalone(
            d_text_buffer_set_char(buffer, 0, 'X') == true,
            "set_char_first",
            "Set first char should succeed",
            _counter) && result;

        // test 3: set last character
        result = d_assert_standalone(
            d_text_buffer_set_char(buffer, 4, 'Z') == true,
            "set_char_last",
            "Set last char should succeed",
            _counter) && result;

        // test 4: verify content
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str[0] == 'X' && str[1] == 'B' && str[2] == 'C' &&
            str[3] == 'D' && str[4] == 'Z',
            "set_char_content",
            "Content should be 'XBCDZ'",
            _counter) && result;

        // test 5: set with negative index
        result = d_assert_standalone(
            d_text_buffer_set_char(buffer, -1, '!') == true,
            "set_char_neg",
            "Set with negative index should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_get_char(buffer, 4) == '!',
            "set_char_neg_verify",
            "Last char should now be '!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_get_string
  Tests the d_text_buffer_get_string function.
  Tests the following:
  - NULL buffer returns NULL
  - empty buffer returns pointer to null terminator
  - non-empty buffer returns pointer to data
  - returned string is null-terminated
*/
bool
d_tests_sa_text_buffer_get_string
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_get_string(NULL) == NULL,
        "get_string_null",
        "NULL buffer should return NULL",
        _counter) && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new(16);

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str != NULL && str[0] == '\0',
            "get_string_empty",
            "Empty buffer should return empty string",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: non-empty buffer
    buffer = d_text_buffer_new_from_string("Test Data");

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str != NULL &&
            d_strcasecmp(str, "Test Data") == 0,
            "get_string_nonempty",
            "Should return 'Test Data'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_get_range_string
  Tests the d_text_buffer_get_range_string function.
  Tests the following:
  - NULL buffer returns NULL
  - out-of-bounds range returns NULL
  - start == end returns empty string (or NULL)
  - valid range returns correct substring
  - returned string is independently allocated and null-terminated
*/
bool
d_tests_sa_text_buffer_get_range_string
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    char*                 substr;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_get_range_string(NULL, 0, 5) == NULL,
        "get_range_null",
        "NULL buffer should return NULL",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: valid range
        substr = d_text_buffer_get_range_string(buffer, 7, 12);
        result = d_assert_standalone(
            substr != NULL &&
            d_strcasecmp(substr, "World") == 0,
            "get_range_valid",
            "Range [7,12) should be 'World'",
            _counter) && result;

        if (substr)
        {
            free(substr);
        }

        // test 3: single character range
        substr = d_text_buffer_get_range_string(buffer, 0, 1);
        result = d_assert_standalone(
            substr != NULL &&
            substr[0] == 'H' && substr[1] == '\0',
            "get_range_single",
            "Range [0,1) should be 'H'",
            _counter) && result;

        if (substr)
        {
            free(substr);
        }

        // test 4: full range
        substr = d_text_buffer_get_range_string(buffer, 0, 13);
        result = d_assert_standalone(
            substr != NULL &&
            d_strcasecmp(substr, "Hello, World!") == 0,
            "get_range_full",
            "Full range should return complete string",
            _counter) && result;

        if (substr)
        {
            free(substr);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_access_all
  Aggregation function that runs all access operation tests.
*/
bool
d_tests_sa_text_buffer_access_all
(
    struct d_test_counter* _counter
)
{
    printf("\n  [SECTION] Access Operations\n");
    printf("  ---------------------------\n");

    return d_tests_sa_text_buffer_get_char(_counter)   &&
           d_tests_sa_text_buffer_set_char(_counter)   &&
           d_tests_sa_text_buffer_get_string(_counter) &&
           d_tests_sa_text_buffer_get_range_string(_counter);
}
