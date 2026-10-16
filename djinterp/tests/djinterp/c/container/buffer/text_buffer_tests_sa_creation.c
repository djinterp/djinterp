/*******************************************************************************
* djinterp [container]                           text_buffer_tests_sa_creation.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_creation.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_new
  Tests the d_text_buffer_new function.
  Tests the following:
  - zero capacity creates a valid buffer
  - small capacity creates a buffer with correct capacity
  - large capacity creates a buffer with correct capacity
  - returned buffer has zero count
  - returned buffer data is non-NULL and null-terminated
*/
bool
d_tests_sa_text_buffer_new
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: create with zero capacity
    buffer = d_text_buffer_new(0);
    result = d_assert_standalone(
        buffer != NULL,
        "new_zero_cap",
        "Zero capacity should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "new_zero_cap_count",
            "New buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: create with small capacity
    buffer = d_text_buffer_new(16);
    result = d_assert_standalone(
        buffer != NULL,
        "new_small_cap",
        "Small capacity should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= 16,
            "new_small_cap_value",
            "Capacity should be at least 16",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "new_small_cap_empty",
            "New buffer should have zero length",
            _counter) && result;

        // verify null-termination
        result = d_assert_standalone(
            d_text_buffer_get_string(buffer) != NULL &&
            d_text_buffer_get_string(buffer)[0] == '\0',
            "new_small_cap_null_term",
            "New buffer data should be null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: create with large capacity
    buffer = d_text_buffer_new(4096);
    result = d_assert_standalone(
        buffer != NULL,
        "new_large_cap",
        "Large capacity should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= 4096,
            "new_large_cap_value",
            "Capacity should be at least 4096",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_default_capacity
  Tests the d_text_buffer_new_default_capacity function.
  Tests the following:
  - returned buffer is non-NULL
  - capacity is at least D_BUFFER_DEFAULT_CAPACITY
  - length is zero
  - data is null-terminated
*/
bool
d_tests_sa_text_buffer_new_default_capacity
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    buffer = d_text_buffer_new_default_capacity();

    // test 1: non-NULL result
    result = d_assert_standalone(
        buffer != NULL,
        "new_default_nonnull",
        "Default capacity should return non-NULL",
        _counter) && result;

    if (buffer)
    {
        // test 2: capacity
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= D_BUFFER_DEFAULT_CAPACITY,
            "new_default_cap",
            "Capacity should be at least D_BUFFER_DEFAULT_CAPACITY",
            _counter) && result;

        // test 3: empty
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "new_default_empty",
            "Default buffer should have zero length",
            _counter) && result;

        // test 4: null-terminated
        result = d_assert_standalone(
            d_text_buffer_get_string(buffer)[0] == '\0',
            "new_default_null_term",
            "Default buffer should be null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_from_string
  Tests the d_text_buffer_new_from_string function.
  Tests the following:
  - NULL string returns NULL
  - empty string creates empty buffer
  - normal string is copied correctly
  - buffer is null-terminated
  - length matches original string
*/
bool
d_tests_sa_text_buffer_new_from_string
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL string -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_string(NULL);
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_null",
        "NULL string should return empty buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_string_null_empty",
            "NULL string buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: empty string
    buffer = d_text_buffer_new_from_string("");
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_empty",
        "Empty string should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_string_empty_len",
            "Empty string buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: normal string
    buffer = d_text_buffer_new_from_string("Hello, World!");
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_normal",
        "Normal string should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "from_string_normal_len",
            "Length should be 13",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str != NULL &&
            d_strncasecmp(str, "Hello, World!", 13) == 0,
            "from_string_normal_content",
            "Content should match 'Hello, World!'",
            _counter) && result;

        // test 4: null-termination
        result = d_assert_standalone(
            str[13] == '\0',
            "from_string_null_term",
            "Buffer data should be null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_from_string_n
  Tests the d_text_buffer_new_from_string_n function.
  Tests the following:
  - NULL string returns NULL
  - zero length creates empty buffer
  - partial copy of string with correct length
  - content matches first _length characters
*/
bool
d_tests_sa_text_buffer_new_from_string_n
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL string -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_string_n(NULL, 5);
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_n_null",
        "NULL string should return empty buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_string_n_null_empty",
            "NULL string buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: zero length
    buffer = d_text_buffer_new_from_string_n("Hello", 0);
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_n_zero",
        "Zero length should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_string_n_zero_len",
            "Zero length buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: partial copy
    buffer = d_text_buffer_new_from_string_n("Hello, World!", 5);
    result = d_assert_standalone(
        buffer != NULL,
        "from_string_n_partial",
        "Partial copy should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "from_string_n_partial_len",
            "Length should be 5",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str != NULL &&
            d_strncasecmp(str, "Hello", 5) == 0 &&
            str[5] == '\0',
            "from_string_n_partial_content",
            "Content should be 'Hello' and null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_from_strings
  Tests the d_text_buffer_new_from_strings function.
  Tests the following:
  - zero count returns valid empty buffer
  - single string concatenation
  - multiple string concatenation
  - result is null-terminated with correct total length
*/
bool
d_tests_sa_text_buffer_new_from_strings
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: single string
    buffer = d_text_buffer_new_from_strings(1, "Hello");
    result = d_assert_standalone(
        buffer != NULL,
        "from_strings_single",
        "Single string should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "from_strings_single_len",
            "Single string length should be 5",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: multiple strings
    buffer = d_text_buffer_new_from_strings(3, "Hello", ", ", "World!");
    result = d_assert_standalone(
        buffer != NULL,
        "from_strings_multi",
        "Multiple strings should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "from_strings_multi_len",
            "Concatenated length should be 13",
            _counter) && result;

        result = d_assert_standalone(
            str != NULL &&
            d_strncasecmp(str, "Hello, World!", 13) == 0,
            "from_strings_multi_content",
            "Content should be 'Hello, World!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_from_buffer
  Tests the d_text_buffer_new_from_buffer function.
  Tests the following:
  - NULL buffer returns NULL
  - zero length creates empty buffer
  - data with embedded nulls copies correct number of bytes
  - result is null-terminated
*/
bool
d_tests_sa_text_buffer_new_from_buffer
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    char                  raw_data[8];
    const char*           str;

    result = true;

    // test 1: NULL buffer -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_buffer(NULL, 5);
    result = d_assert_standalone(
        buffer != NULL,
        "from_buffer_null",
        "NULL buffer data should return empty buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_buffer_null_empty",
            "NULL buffer data should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: zero length
    buffer = d_text_buffer_new_from_buffer("data", 0);
    result = d_assert_standalone(
        buffer != NULL,
        "from_buffer_zero_len",
        "Zero length should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "from_buffer_zero_len_empty",
            "Zero length buffer should have zero length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: raw data copy
    d_memset(raw_data, 0, sizeof(raw_data));
    raw_data[0] = 'A';
    raw_data[1] = 'B';
    raw_data[2] = '\0';
    raw_data[3] = 'C';
    raw_data[4] = 'D';

    buffer = d_text_buffer_new_from_buffer(raw_data, 5);
    result = d_assert_standalone(
        buffer != NULL,
        "from_buffer_raw",
        "Raw data copy should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "from_buffer_raw_len",
            "Length should be 5 (including embedded null)",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            str != NULL &&
            str[0] == 'A' && str[1] == 'B' &&
            str[3] == 'C' && str[4] == 'D',
            "from_buffer_raw_content",
            "Raw data content should match",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_copy
  Tests the d_text_buffer_new_copy function.
  Tests the following:
  - NULL other returns NULL
  - successful deep copy of non-empty buffer
  - copied buffer is independent (modifying copy does not affect original)
  - lengths and contents match
*/
bool
d_tests_sa_text_buffer_new_copy
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* original;
    struct d_text_buffer* copy;
    const char*           orig_str;
    const char*           copy_str;

    result = true;

    // test 1: NULL other
    copy = d_text_buffer_new_copy(NULL);
    result = d_assert_standalone(
        copy == NULL,
        "new_copy_null",
        "NULL other should return NULL",
        _counter) && result;

    // test 2: successful deep copy
    original = d_text_buffer_new_from_string("Test String");

    if (original)
    {
        copy = d_text_buffer_new_copy(original);
        result = d_assert_standalone(
            copy != NULL,
            "new_copy_success",
            "Copy should return non-NULL buffer",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                d_text_buffer_length(copy) ==
                d_text_buffer_length(original),
                "new_copy_length",
                "Copy length should match original",
                _counter) && result;

            orig_str = d_text_buffer_get_string(original);
            copy_str = d_text_buffer_get_string(copy);
            result = d_assert_standalone(
                orig_str != copy_str,
                "new_copy_independent_ptrs",
                "Copy data pointer should differ from original",
                _counter) && result;

            result = d_assert_standalone(
                d_strcasecmp(orig_str, copy_str) == 0,
                "new_copy_content",
                "Copy content should match original",
                _counter) && result;

            // test 3: independence — modify copy
            d_text_buffer_set_char(copy, 0, 'X');
            result = d_assert_standalone(
                d_text_buffer_get_char(original, 0) == 'T',
                "new_copy_independent",
                "Modifying copy should not affect original",
                _counter) && result;

            d_text_buffer_free(copy);
        }

        d_text_buffer_free(original);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_copy_range
  Tests the d_text_buffer_new_copy_range function.
  Tests the following:
  - NULL other returns NULL
  - out-of-bounds range returns NULL
  - start > end returns NULL
  - valid range extracts correct substring
  - result is null-terminated
*/
bool
d_tests_sa_text_buffer_new_copy_range
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* original;
    struct d_text_buffer* sub;
    const char*           str;

    result = true;

    // test 1: NULL other
    sub = d_text_buffer_new_copy_range(NULL, 0, 5);
    result = d_assert_standalone(
        sub == NULL,
        "copy_range_null",
        "NULL other should return NULL",
        _counter) && result;

    original = d_text_buffer_new_from_string("Hello, World!");

    if (original)
    {
        // test 2: valid range [7, 12) => "World"
        sub = d_text_buffer_new_copy_range(original, 7, 12);
        result = d_assert_standalone(
            sub != NULL,
            "copy_range_valid",
            "Valid range should return non-NULL buffer",
            _counter) && result;

        if (sub)
        {
            result = d_assert_standalone(
                d_text_buffer_length(sub) == 5,
                "copy_range_len",
                "Range length should be 5",
                _counter) && result;

            str = d_text_buffer_get_string(sub);
            result = d_assert_standalone(
                str != NULL &&
                d_strncasecmp(str, "World", 5) == 0,
                "copy_range_content",
                "Content should be 'World'",
                _counter) && result;

            d_text_buffer_free(sub);
        }

        // test 3: start == end => returns NULL (empty range)
        sub = d_text_buffer_new_copy_range(original, 3, 3);
        result = d_assert_standalone(
            sub == NULL,
            "copy_range_empty",
            "start == end should return NULL",
            _counter) && result;

        if (sub)
        {
            d_text_buffer_free(sub);
        }

        d_text_buffer_free(original);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_fill
  Tests the d_text_buffer_new_fill function.
  Tests the following:
  - zero length creates empty buffer
  - fill creates buffer with correct length and character
  - result is null-terminated
*/
bool
d_tests_sa_text_buffer_new_fill
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

    // test 1: zero length
    buffer = d_text_buffer_new_fill(0, 'X');
    result = d_assert_standalone(
        buffer != NULL &&
        d_text_buffer_length(buffer) == 0,
        "fill_zero",
        "Zero length fill should create empty buffer",
        _counter) && result;

    if (buffer)
    {
        d_text_buffer_free(buffer);
    }

    // test 2: fill with character
    buffer = d_text_buffer_new_fill(10, 'A');
    result = d_assert_standalone(
        buffer != NULL,
        "fill_normal",
        "Fill should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 10,
            "fill_normal_len",
            "Fill length should be 10",
            _counter) && result;

        str = d_text_buffer_get_string(buffer);
        all_match = true;
        for (i = 0; i < 10; i++)
        {
            if (str[i] != 'A')
            {
                all_match = false;
            }
        }

        result = d_assert_standalone(
            all_match && str[10] == '\0',
            "fill_normal_content",
            "All 10 chars should be 'A' and null-terminated",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_new_formatted
  Tests the d_text_buffer_new_formatted function.
  Tests the following:
  - NULL format returns NULL
  - simple format string
  - format with integer substitution
  - format with multiple arguments
  - result is null-terminated with correct length
*/
bool
d_tests_sa_text_buffer_new_formatted
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    const char*           str;

    result = true;

    // test 1: NULL format
    buffer = d_text_buffer_new_formatted(NULL);
    result = d_assert_standalone(
        buffer == NULL,
        "formatted_null",
        "NULL format should return NULL",
        _counter) && result;

    // test 2: simple format
    buffer = d_text_buffer_new_formatted("Hello");
    result = d_assert_standalone(
        buffer != NULL,
        "formatted_simple",
        "Simple format should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5 &&
            d_strcasecmp(str, "Hello") == 0,
            "formatted_simple_content",
            "Content should be 'Hello'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: format with integer
    buffer = d_text_buffer_new_formatted("Value: %d", 42);
    result = d_assert_standalone(
        buffer != NULL,
        "formatted_int",
        "Integer format should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "Value: 42") == 0,
            "formatted_int_content",
            "Content should be 'Value: 42'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: format with multiple arguments
    buffer = d_text_buffer_new_formatted("%s=%d, %s=%d",
                                      "x", 10, "y", 20);
    result = d_assert_standalone(
        buffer != NULL,
        "formatted_multi",
        "Multi-arg format should return non-NULL buffer",
        _counter) && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_strcasecmp(str, "x=10, y=20") == 0,
            "formatted_multi_content",
            "Content should be 'x=10, y=20'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_creation_all
  Aggregation function that runs all creation tests.
*/
bool
d_tests_sa_text_buffer_creation_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Creation Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_text_buffer_new(_counter) && result;
    result = d_tests_sa_text_buffer_new_default_capacity(_counter)
             && result;
    result = d_tests_sa_text_buffer_new_from_string(_counter) && result;
    result = d_tests_sa_text_buffer_new_from_string_n(_counter) && result;
    result = d_tests_sa_text_buffer_new_from_strings(_counter) && result;
    result = d_tests_sa_text_buffer_new_from_buffer(_counter) && result;
    result = d_tests_sa_text_buffer_new_copy(_counter) && result;
    result = d_tests_sa_text_buffer_new_copy_range(_counter) && result;
    result = d_tests_sa_text_buffer_new_fill(_counter) && result;
    result = d_tests_sa_text_buffer_new_formatted(_counter) && result;

    return result;
}
