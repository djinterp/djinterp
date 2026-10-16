/*******************************************************************************
* djinterp [container]                        text_buffer_tests_sa_append_mode.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_append_mode.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_append_string_chunked
  Tests the d_text_buffer_append_string_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - successful chunked append creates overflow chunks
  - data accessible after chunked append
*/
bool
d_tests_sa_text_buffer_append_string_chunked
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_string_chunked(NULL,
                                            "test", 64) == false,
        "append_str_chunked_null_buf",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: NULL string
        result = d_assert_standalone(
            d_text_buffer_append_string_chunked(buffer,
                                                NULL, 64) == false,
            "append_str_chunked_null_str",
            "NULL string should return false",
            _counter) && result;

        // test 3: successful chunked append
        result = d_assert_standalone(
            d_text_buffer_append_string_chunked(buffer,
                                                ", World!", 64) == true,
            "append_str_chunked_success",
            "Chunked append should succeed",
            _counter) && result;

        // test 4: has chunks
        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == true,
            "append_str_chunked_has_chunks",
            "Buffer should have chunks after chunked append",
            _counter) && result;

        // test 5: total length includes chunks
        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 13,
            "append_str_chunked_total_len",
            "Total length should be 13",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_buffer_chunked
  Tests the d_text_buffer_append_buffer_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - successful raw data chunked append
  - total length is correct
*/
bool
d_tests_sa_text_buffer_append_buffer_chunked
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    char                  data[4];
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_buffer_chunked(NULL,
                                            "ab", 2, 64) == false,
        "append_buf_chunked_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: NULL data
        result = d_assert_standalone(
            d_text_buffer_append_buffer_chunked(buffer,
                                                NULL, 4, 64) == false,
            "append_buf_chunked_null_data",
            "NULL data should return false",
            _counter) && result;

        // test 3: successful chunked append
        data[0] = 'C';
        data[1] = 'D';
        data[2] = 'E';
        data[3] = 'F';

        result = d_assert_standalone(
            d_text_buffer_append_buffer_chunked(buffer,
                                                data, 4, 64) == true,
            "append_buf_chunked_success",
            "Chunked buffer append should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 6,
            "append_buf_chunked_len",
            "Total length should be 6",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_char_chunked
  Tests the d_text_buffer_append_char_chunked function.
  Tests the following:
  - NULL buffer returns false
  - successful single char chunked append
  - multiple chunked char appends
*/
bool
d_tests_sa_text_buffer_append_char_chunked
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_char_chunked(NULL, 'X', 64) == false,
        "append_char_chunked_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: single char chunked append
        result = d_assert_standalone(
            d_text_buffer_append_char_chunked(buffer, 'C', 64) == true,
            "append_char_chunked_success",
            "Chunked char append should succeed",
            _counter) && result;

        // test 3: multiple chunked appends
        d_text_buffer_append_char_chunked(buffer, 'D', 64);
        d_text_buffer_append_char_chunked(buffer, 'E', 64);

        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 5,
            "append_char_chunked_multi_len",
            "Total length should be 5 after 3 chunked appends",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_formatted_chunked
  Tests the d_text_buffer_append_formatted_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted chunked append with integer substitution
  - total length correct after chunked formatted append
*/
bool
d_tests_sa_text_buffer_append_formatted_chunked
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_append_formatted_chunked(NULL, 64,
                                               "test") == false,
        "append_fmt_chunked_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("val=");

    if (buffer)
    {
        // test 2: NULL format
        result = d_assert_standalone(
            d_text_buffer_append_formatted_chunked(buffer, 64,
                                                   NULL) == false,
            "append_fmt_chunked_null_fmt",
            "NULL format should return false",
            _counter) && result;

        // test 3: formatted chunked append
        result = d_assert_standalone(
            d_text_buffer_append_formatted_chunked(buffer, 64,
                                                   "%d", 100) == true,
            "append_fmt_chunked_success",
            "Formatted chunked append should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 7,
            "append_fmt_chunked_len",
            "Total length should be 7 ('val=100')",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_consolidate
  Tests the d_text_buffer_consolidate function.
  Tests the following:
  - NULL buffer returns false
  - consolidate buffer with no chunks succeeds
  - consolidate merges chunked data into contiguous store
  - after consolidation, has_chunks returns false
  - consolidated content matches expected concatenation
*/
bool
d_tests_sa_text_buffer_consolidate
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_consolidate(NULL) == false,
        "consolidate_null",
        "NULL buffer should return false",
        _counter) && result;

    // test 2: consolidate with no chunks (no-op)
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_consolidate(buffer) == true,
            "consolidate_no_chunks",
            "Consolidate with no chunks should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_strcasecmp(d_text_buffer_get_string(buffer),
                         "Hello") == 0,
            "consolidate_no_chunks_content",
            "Content should be unchanged",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: consolidate with chunks
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        d_text_buffer_append_string_chunked(buffer, ", ", 64);
        d_text_buffer_append_string_chunked(buffer, "World", 64);
        d_text_buffer_append_string_chunked(buffer, "!", 64);

        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == true,
            "consolidate_pre_has_chunks",
            "Buffer should have chunks before consolidation",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_consolidate(buffer) == true,
            "consolidate_success",
            "Consolidation should succeed",
            _counter) && result;

        // test 4: no more chunks
        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == false,
            "consolidate_no_chunks_after",
            "Buffer should not have chunks after consolidation",
            _counter) && result;

        // test 5: content correct
        str = d_text_buffer_get_string(buffer);
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13 &&
            d_strcasecmp(str, "Hello, World!") == 0,
            "consolidate_content",
            "Consolidated content should be 'Hello, World!'",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_total_length
  Tests the d_text_buffer_total_length function.
  Tests the following:
  - NULL buffer returns 0
  - buffer with no chunks returns count
  - buffer with chunks returns primary + chunk total
*/
bool
d_tests_sa_text_buffer_total_length
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_total_length(NULL) == 0,
        "total_len_null",
        "NULL buffer should return 0",
        _counter) && result;

    // test 2: no chunks
    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 5,
            "total_len_no_chunks",
            "Total length should equal primary count",
            _counter) && result;

        // test 3: with chunks
        d_text_buffer_append_string_chunked(buffer, "FGH", 64);

        result = d_assert_standalone(
            d_text_buffer_total_length(buffer) == 8,
            "total_len_with_chunks",
            "Total length should include chunk data",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_has_chunks
  Tests the d_text_buffer_has_chunks function.
  Tests the following:
  - NULL buffer returns false
  - new buffer has no chunks
  - buffer with chunked append has chunks
  - after consolidation, has_chunks is false
*/
bool
d_tests_sa_text_buffer_has_chunks
(
    struct d_test_counter* _counter
)
{
    struct d_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_has_chunks(NULL) == false,
        "has_chunks_null",
        "NULL buffer should return false",
        _counter) && result;

    buffer = d_text_buffer_new_from_string("Test");

    if (buffer)
    {
        // test 2: no chunks initially
        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == false,
            "has_chunks_initial",
            "New buffer should have no chunks",
            _counter) && result;

        // test 3: after chunked append
        d_text_buffer_append_string_chunked(buffer, "Data", 64);

        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == true,
            "has_chunks_after_append",
            "Buffer should have chunks after chunked append",
            _counter) && result;

        // test 4: after consolidation
        d_text_buffer_consolidate(buffer);

        result = d_assert_standalone(
            d_text_buffer_has_chunks(buffer) == false,
            "has_chunks_after_consolidate",
            "Buffer should have no chunks after consolidation",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_append_mode_all
  Aggregation function that runs all append mode tests.
*/
bool
d_tests_sa_text_buffer_append_mode_all
(
    struct d_test_counter* _counter
)
{
    printf("\n  [SECTION] String Operations (Append Mode)\n");
    printf("  ------------------------------------------\n");

    return d_tests_sa_text_buffer_append_string_chunked(_counter)    &&
           d_tests_sa_text_buffer_append_buffer_chunked(_counter)    &&
           d_tests_sa_text_buffer_append_char_chunked(_counter)      &&
           d_tests_sa_text_buffer_append_formatted_chunked(_counter) &&
           d_tests_sa_text_buffer_consolidate(_counter)              &&
           d_tests_sa_text_buffer_total_length(_counter)             &&
           d_tests_sa_text_buffer_has_chunks(_counter);
}
