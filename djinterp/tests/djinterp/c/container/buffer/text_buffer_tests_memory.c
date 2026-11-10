/*******************************************************************************
* djinterp [container]                                text_buffer_tests_memory.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_memory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_free
  Tests the d_text_buffer_free function.
  Tests the following:
  - NULL buffer handled gracefully (no crash)
  - frees a newly created buffer without error
  - frees a populated buffer without error
  - frees a buffer with chunks without error
  - frees a buffer after modifications without error
*/
bool d_tests_text_buffer_free(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_free");

    result = true;

    // test 1: NULL should not crash
    // (we cannot assert return value as free returns void,
    //  but we verify it does not segfault)
    d_text_buffer_free(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "free_null",
                              true,
                              "Freeing NULL should not crash") && result;

    // test 2: free newly created buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        d_text_buffer_free(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_new",
                                  true,
                                  "Freeing new buffer should not crash") && result;
    }

    // test 3: free populated buffer
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        d_text_buffer_free(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_populated",
                                  true,
                                  "Freeing populated buffer should not crash") && result;
    }

    // test 4: free buffer with chunks
    buffer = d_text_buffer_new(8);

    if (buffer)
    {
        d_text_buffer_append_string_chunked(buffer,
            "overflow chunk data", 0);
        d_text_buffer_free(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_chunked",
                                  true,
                                  "Freeing buffer with chunks should not crash") && result;
    }

    // test 5: free buffer after multiple operations
    buffer = d_text_buffer_new_from_string("start");

    if (buffer)
    {
        d_text_buffer_append_string(buffer, " middle");
        d_text_buffer_to_upper(buffer);
        d_text_buffer_reverse(buffer);
        d_text_buffer_free(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_modified",
                                  true,
                                  "Freeing heavily modified buffer should not crash") && result;
    }

    return result;
}

/*
d_tests_text_buffer_memory_all
  Aggregation function that runs all memory management tests.
*/
bool d_tests_text_buffer_memory_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_memory_all");

    result = true;

    result = d_tests_text_buffer_free(_suite) && result;

    return result;
}
