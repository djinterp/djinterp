/*******************************************************************************
* djinterp [container]                             text_buffer_tests_sa_memory.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_memory.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_free
  Tests the d_text_buffer_free function.
  Tests the following:
  - NULL buffer handled gracefully (no crash)
  - frees a newly created buffer without error
  - frees a populated buffer without error
  - frees a buffer with chunks without error
  - frees a buffer after modifications without error
*/
bool
d_tests_sa_text_buffer_free
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL should not crash
    // (we cannot assert return value as free returns void,
    //  but we verify it does not segfault)
    d_text_buffer_free(NULL);

    result = d_assert_standalone(
        true,
        "free_null",
        "Freeing NULL should not crash",
        _counter) && result;

    // test 2: free newly created buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        d_text_buffer_free(buffer);

        result = d_assert_standalone(
            true,
            "free_new",
            "Freeing new buffer should not crash",
            _counter) && result;
    }

    // test 3: free populated buffer
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        d_text_buffer_free(buffer);

        result = d_assert_standalone(
            true,
            "free_populated",
            "Freeing populated buffer should not crash",
            _counter) && result;
    }

    // test 4: free buffer with chunks
    buffer = d_text_buffer_new(8);

    if (buffer)
    {
        d_text_buffer_append_string_chunked(buffer,
            "overflow chunk data", 0);
        d_text_buffer_free(buffer);

        result = d_assert_standalone(
            true,
            "free_chunked",
            "Freeing buffer with chunks should not crash",
            _counter) && result;
    }

    // test 5: free buffer after multiple operations
    buffer = d_text_buffer_new_from_string("start");

    if (buffer)
    {
        d_text_buffer_append_string(buffer, " middle");
        d_text_buffer_to_upper(buffer);
        d_text_buffer_reverse(buffer);
        d_text_buffer_free(buffer);

        result = d_assert_standalone(
            true,
            "free_modified",
            "Freeing heavily modified buffer should not crash",
            _counter) && result;
    }

    return result;
}

/*
d_tests_sa_text_buffer_memory_all
  Aggregation function that runs all memory management tests.
*/
bool
d_tests_sa_text_buffer_memory_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Memory Management Functions\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_text_buffer_free(_counter) && result;

    return result;
}
