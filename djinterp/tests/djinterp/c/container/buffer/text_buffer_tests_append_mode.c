/*******************************************************************************
* djinterp [container]                           text_buffer_tests_append_mode.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_append_mode.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_append_string_chunked
  Tests the d_text_buffer_append_string_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL string returns false
  - successful chunked append creates overflow chunks
  - data accessible after chunked append
*/
bool d_tests_text_buffer_append_string_chunked(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_append_string_chunked");

    result = D_TEST_CHECK_MSG(_suite,
                              "append_str_chunked_null_buf",
                              d_text_buffer_append_string_chunked(NULL, "test", 64) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        // test 2: NULL string
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_chunked_null_str",
                                  d_text_buffer_append_string_chunked(buffer, NULL, 64) == false,
                                  "NULL string should return false") && result;

        // test 3: successful chunked append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_chunked_success",
                                  d_text_buffer_append_string_chunked(buffer, ", World!", 64) == true,
                                  "Chunked append should succeed") && result;

        // test 4: has chunks
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_chunked_has_chunks",
                                  d_text_buffer_has_chunks(buffer) == true,
                                  "Buffer should have chunks after chunked append") && result;

        // test 5: total length includes chunks
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_str_chunked_total_len",
                                  d_text_buffer_total_length(buffer) == 13,
                                  "Total length should be 13") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_buffer_chunked
  Tests the d_text_buffer_append_buffer_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL data returns false
  - successful raw data chunked append
  - total length is correct
*/
bool d_tests_text_buffer_append_buffer_chunked(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    char                  data[4];
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_append_buffer_chunked");

    result = D_TEST_CHECK_MSG(_suite,
                              "append_buf_chunked_null",
                              d_text_buffer_append_buffer_chunked(NULL, "ab", 2, 64) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: NULL data
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_chunked_null_data",
                                  d_text_buffer_append_buffer_chunked(buffer, NULL, 4, 64) == false,
                                  "NULL data should return false") && result;

        // test 3: successful chunked append
        data[0] = 'C';
        data[1] = 'D';
        data[2] = 'E';
        data[3] = 'F';

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_chunked_success",
                                  d_text_buffer_append_buffer_chunked(buffer, data, 4, 64) == true,
                                  "Chunked buffer append should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_buf_chunked_len",
                                  d_text_buffer_total_length(buffer) == 6,
                                  "Total length should be 6") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_char_chunked
  Tests the d_text_buffer_append_char_chunked function.
  Tests the following:
  - NULL buffer returns false
  - successful single char chunked append
  - multiple chunked char appends
*/
bool d_tests_text_buffer_append_char_chunked(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_append_char_chunked");

    result = D_TEST_CHECK_MSG(_suite,
                              "append_char_chunked_null",
                              d_text_buffer_append_char_chunked(NULL, 'X', 64) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("AB");

    if (buffer)
    {
        // test 2: single char chunked append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_char_chunked_success",
                                  d_text_buffer_append_char_chunked(buffer, 'C', 64) == true,
                                  "Chunked char append should succeed") && result;

        // test 3: multiple chunked appends
        d_text_buffer_append_char_chunked(buffer, 'D', 64);
        d_text_buffer_append_char_chunked(buffer, 'E', 64);

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_char_chunked_multi_len",
                                  d_text_buffer_total_length(buffer) == 5,
                                  "Total length should be 5 after 3 chunked appends") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_formatted_chunked
  Tests the d_text_buffer_append_formatted_chunked function.
  Tests the following:
  - NULL buffer returns false
  - NULL format returns false
  - formatted chunked append with integer substitution
  - total length correct after chunked formatted append
*/
bool d_tests_text_buffer_append_formatted_chunked(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_append_formatted_chunked");

    result = D_TEST_CHECK_MSG(_suite,
                              "append_fmt_chunked_null",
                              d_text_buffer_append_formatted_chunked(NULL, 64, "test") == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("val=");

    if (buffer)
    {
        // test 2: NULL format
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_chunked_null_fmt",
                                  d_text_buffer_append_formatted_chunked(buffer, 64, NULL) == false,
                                  "NULL format should return false") && result;

        // test 3: formatted chunked append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_chunked_success",
                                  d_text_buffer_append_formatted_chunked(buffer, 64, "%d", 100) == true,
                                  "Formatted chunked append should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_fmt_chunked_len",
                                  d_text_buffer_total_length(buffer) == 7,
                                  "Total length should be 7 ('val=100')") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_consolidate
  Tests the d_text_buffer_consolidate function.
  Tests the following:
  - NULL buffer returns false
  - consolidate buffer with no chunks succeeds
  - consolidate merges chunked data into contiguous store
  - after consolidation, has_chunks returns false
  - consolidated content matches expected concatenation
*/
bool d_tests_text_buffer_consolidate(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_consolidate");

    result = D_TEST_CHECK_MSG(_suite,
                              "consolidate_null",
                              d_text_buffer_consolidate(NULL) == false,
                              "NULL buffer should return false") && result;

    // test 2: consolidate with no chunks (no-op)
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_no_chunks",
                                  d_text_buffer_consolidate(buffer) == true,
                                  "Consolidate with no chunks should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_no_chunks_content",
                                  d_strcasecmp(d_text_buffer_get_string(buffer), "Hello") == 0,
                                  "Content should be unchanged") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: consolidate with chunks
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        d_text_buffer_append_string_chunked(buffer, ", ", 64);
        d_text_buffer_append_string_chunked(buffer, "World", 64);
        d_text_buffer_append_string_chunked(buffer, "!", 64);

        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_pre_has_chunks",
                                  d_text_buffer_has_chunks(buffer) == true,
                                  "Buffer should have chunks before consolidation") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_success",
                                  d_text_buffer_consolidate(buffer) == true,
                                  "Consolidation should succeed") && result;

        // test 4: no more chunks
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_no_chunks_after",
                                  d_text_buffer_has_chunks(buffer) == false,
                                  "Buffer should not have chunks after consolidation") && result;

        // test 5: content correct
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_content",
                                  d_text_buffer_length(buffer) == 13 && d_strcasecmp(str, "Hello, World!") == 0,
                                  "Consolidated content should be 'Hello, World!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_total_length
  Tests the d_text_buffer_total_length function.
  Tests the following:
  - NULL buffer returns 0
  - buffer with no chunks returns count
  - buffer with chunks returns primary + chunk total
*/
bool d_tests_text_buffer_total_length(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_total_length");

    result = D_TEST_CHECK_MSG(_suite,
                              "total_len_null",
                              d_text_buffer_total_length(NULL) == 0,
                              "NULL buffer should return 0") && result;

    // test 2: no chunks
    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "total_len_no_chunks",
                                  d_text_buffer_total_length(buffer) == 5,
                                  "Total length should equal primary count") && result;

        // test 3: with chunks
        d_text_buffer_append_string_chunked(buffer, "FGH", 64);

        result = D_TEST_CHECK_MSG(_suite,
                                  "total_len_with_chunks",
                                  d_text_buffer_total_length(buffer) == 8,
                                  "Total length should include chunk data") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_has_chunks
  Tests the d_text_buffer_has_chunks function.
  Tests the following:
  - NULL buffer returns false
  - new buffer has no chunks
  - buffer with chunked append has chunks
  - after consolidation, has_chunks is false
*/
bool d_tests_text_buffer_has_chunks(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_has_chunks");

    result = D_TEST_CHECK_MSG(_suite,
                              "has_chunks_null",
                              d_text_buffer_has_chunks(NULL) == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("Test");

    if (buffer)
    {
        // test 2: no chunks initially
        result = D_TEST_CHECK_MSG(_suite,
                                  "has_chunks_initial",
                                  d_text_buffer_has_chunks(buffer) == false,
                                  "New buffer should have no chunks") && result;

        // test 3: after chunked append
        d_text_buffer_append_string_chunked(buffer, "Data", 64);

        result = D_TEST_CHECK_MSG(_suite,
                                  "has_chunks_after_append",
                                  d_text_buffer_has_chunks(buffer) == true,
                                  "Buffer should have chunks after chunked append") && result;

        // test 4: after consolidation
        d_text_buffer_consolidate(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "has_chunks_after_consolidate",
                                  d_text_buffer_has_chunks(buffer) == false,
                                  "Buffer should have no chunks after consolidation") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_append_mode_all
  Aggregation function that runs all append mode tests.
*/
bool d_tests_text_buffer_append_mode_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "text_buffer_append_mode_all");

    return d_tests_text_buffer_append_string_chunked(_suite)    &&
           d_tests_text_buffer_append_buffer_chunked(_suite)    &&
           d_tests_text_buffer_append_char_chunked(_suite)      &&
           d_tests_text_buffer_append_formatted_chunked(_suite) &&
           d_tests_text_buffer_consolidate(_suite)              &&
           d_tests_text_buffer_total_length(_suite)             &&
           d_tests_text_buffer_has_chunks(_suite);
}
