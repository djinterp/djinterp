/*******************************************************************************
* djinterp [container]                             buffer_common_tests_chunked.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_chunked.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_chunk_new
  Tests the d_buffer_common_chunk_new function.
  Tests the following:
  - zero element_size returns NULL
  - zero capacity returns NULL
  - successful allocation sets count=0, capacity, next=NULL
*/
bool d_tests_buffer_common_chunk_new(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_buffer_chunk* chunk;

    d_test_suite_test(_suite, "buffer_common_chunk_new");

    result = true;

    // test 1: zero element_size
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_new_zero_size",
                              d_buffer_common_chunk_new(0, 10) == NULL,
                              "Zero element_size should return NULL") && result;

    // test 2: zero capacity
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_new_zero_cap",
                              d_buffer_common_chunk_new(sizeof(int), 0) == NULL,
                              "Zero capacity should return NULL") && result;

    // test 3: successful allocation
    chunk = d_buffer_common_chunk_new(sizeof(int), 8);
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_new_success",
                              chunk != NULL,
                              "Chunk allocation should succeed") && result;

    if (chunk)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "chunk_new_count",
                                  chunk->count == 0,
                                  "Chunk count should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chunk_new_capacity",
                                  chunk->capacity == 8,
                                  "Chunk capacity should be 8") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chunk_new_next_null",
                                  chunk->next == NULL,
                                  "Chunk next should be NULL") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "chunk_new_elements",
                                  chunk->elements != NULL,
                                  "Chunk elements should be non-NULL") && result;

        d_buffer_common_chunk_free(chunk);
    }

    return result;
}


/*
d_tests_buffer_common_chunk_free
  Tests the d_buffer_common_chunk_free function.
  Tests the following:
  - NULL chunk does not crash
  - valid chunk is freed without error
*/
bool d_tests_buffer_common_chunk_free(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_buffer_chunk* chunk;

    d_test_suite_test(_suite, "buffer_common_chunk_free");

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_free_null",
                              true,
                              "NULL chunk free should not crash") && result;

    // test 2: valid chunk freed
    chunk = d_buffer_common_chunk_new(sizeof(int), 4);

    if (chunk)
    {
        d_buffer_common_chunk_free(chunk);
        result = D_TEST_CHECK_MSG(_suite,
                                  "chunk_free_valid",
                                  true,
                                  "Valid chunk should be freed") && result;
    }

    return result;
}


/*
d_tests_buffer_common_chunk_list_init
  Tests the d_buffer_common_chunk_list_init function.
  Tests the following:
  - NULL list does not crash
  - initialized list has zeroed fields
*/
bool d_tests_buffer_common_chunk_list_init(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_buffer_chunk_list list;

    d_test_suite_test(_suite, "buffer_common_chunk_list_init");

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_list_init(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_init_null",
                              true,
                              "NULL list init should not crash") && result;

    // test 2: fields zeroed
    list.head        = (struct dj_buffer_chunk*)0xDEAD;
    list.tail        = (struct dj_buffer_chunk*)0xBEEF;
    list.chunk_count = 99;
    list.total_count = 99;

    d_buffer_common_chunk_list_init(&list);

    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_init_pointers",
                              list.head == NULL && list.tail == NULL,
                              "Head and tail should be NULL") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_init_counts",
                              list.chunk_count == 0 && list.total_count == 0,
                              "Chunk count and total count should be 0") && result;

    return result;
}


/*
d_tests_buffer_common_chunk_list_free
  Tests the d_buffer_common_chunk_list_free function.
  Tests the following:
  - NULL list does not crash
  - list with chunks is freed and re-initialized
*/
bool d_tests_buffer_common_chunk_list_free(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_buffer_chunk_list list;
    int                        value;

    d_test_suite_test(_suite, "buffer_common_chunk_list_free");

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_list_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_free_null",
                              true,
                              "NULL list free should not crash") && result;

    // test 2: populated list freed correctly
    d_buffer_common_chunk_list_init(&list);
    value = 42;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 43;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_free_precondition",
                              list.total_count == 2,
                              "List should have 2 elements before free") && result;

    d_buffer_common_chunk_list_free(&list);

    result = D_TEST_CHECK_MSG(_suite,
                              "chunk_list_free_result",
                              list.head == NULL && list.chunk_count == 0,
                              "List should be re-initialized after free") && result;

    return result;
}


/*
d_tests_buffer_common_append_element_chunked
  Tests the d_buffer_common_append_element_chunked function.
  Tests the following:
  - NULL list rejection
  - NULL value rejection
  - zero element_size rejection
  - successful single element append
  - element fills tail chunk before creating new
  - total_count tracks correctly
*/
bool d_tests_buffer_common_append_element_chunked(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_buffer_chunk_list list;
    int                        value;

    d_test_suite_test(_suite, "buffer_common_append_element_chunked");

    result = true;

    // test 1: NULL list should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_null_list",
                              d_buffer_common_append_element_chunked(NULL, sizeof(int), &value, 4) == false,
                              "NULL list should return false") && result;

    // test 2: NULL value should fail
    d_buffer_common_chunk_list_init(&list);
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_null_value",
                              d_buffer_common_append_element_chunked(&list, sizeof(int), NULL, 4) == false,
                              "NULL value should return false") && result;

    // test 3: zero element_size should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_zero_size",
                              d_buffer_common_append_element_chunked(&list, 0, &value, 4) == false,
                              "Zero element_size should return false") && result;

    // test 4: successful append
    d_buffer_common_chunk_list_init(&list);
    value  = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_success",
                              d_buffer_common_append_element_chunked(&list, sizeof(int), &value, 4) == true,
                              "Append should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_counts",
                              list.total_count == 1 && list.chunk_count == 1,
                              "total_count=1, chunk_count=1") && result;

    // test 5: second append fills existing chunk
    value  = 200;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_fill_tail",
                              d_buffer_common_append_element_chunked(&list, sizeof(int), &value, 4) == true,
                              "Second append should fill existing chunk") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_chunked_no_new_chunk",
                              list.total_count == 2 && list.chunk_count == 1,
                              "Should still have 1 chunk") && result;

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_buffer_common_append_data_chunked
  Tests the d_buffer_common_append_data_chunked function.
  Tests the following:
  - NULL list rejection
  - NULL data rejection
  - zero data_count rejection
  - successful multi-element chunked append
  - data spans multiple chunks when needed
*/
bool d_tests_buffer_common_append_data_chunked(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_buffer_chunk_list list;
    int                        data[5] = {10, 20, 30, 40, 50};

    d_test_suite_test(_suite, "buffer_common_append_data_chunked");

    result = true;

    // test 1: NULL list should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_chunked_null_list",
                              d_buffer_common_append_data_chunked(NULL, sizeof(int), data, 5, 8) == false,
                              "NULL list should return false") && result;

    // test 2: NULL data should fail
    d_buffer_common_chunk_list_init(&list);
    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_chunked_null_data",
                              d_buffer_common_append_data_chunked(&list, sizeof(int), NULL, 5, 8) == false,
                              "NULL data should return false") && result;

    // test 3: zero data_count should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_chunked_zero_count",
                              d_buffer_common_append_data_chunked(&list, sizeof(int), data, 0, 8) == false,
                              "Zero data_count should return false") && result;

    // test 4: successful append
    d_buffer_common_chunk_list_init(&list);
    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_chunked_success",
                              d_buffer_common_append_data_chunked(&list, sizeof(int), data, 5, 8) == true,
                              "Chunked data append should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_chunked_total",
                              list.total_count == 5,
                              "Total count should be 5") && result;

    // test 5: verify data via get_element_chunked
    {
        bool all_correct = true;
        size_t i;

        for (i = 0; i < 5; ++i)
        {
            void* elem = d_buffer_common_get_element_chunked(
                             NULL, 0, sizeof(int), &list, (dj_index)i);

            if ( (!elem) ||
                 (*(int*)elem != data[i]) )
            {
                all_correct = false;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_data_chunked_values",
                                  all_correct,
                                  "All chunked elements should match source") && result;
    }

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_buffer_common_consolidate
  Tests the d_buffer_common_consolidate function.
  Tests the following:
  - NULL parameter rejection
  - empty chunk list is a no-op
  - successful consolidation merges chunks into primary
  - chunk list is freed after consolidation
  - data integrity after consolidation
*/
bool d_tests_buffer_common_consolidate(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    void*                      elements;
    size_t                     count;
    size_t                     capacity;
    struct dj_buffer_chunk_list list;
    int                        value;

    d_test_suite_test(_suite, "buffer_common_consolidate");

    result = true;

    // test 1: NULL elements should fail
    count    = 0;
    capacity = 0;
    d_buffer_common_chunk_list_init(&list);
    result = D_TEST_CHECK_MSG(_suite,
                              "consolidate_null_elements",
                              d_buffer_common_consolidate(NULL, &count, &capacity, sizeof(int), &list) == false,
                              "NULL elements should return false") && result;

    // test 2: empty chunk list no-op
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 16);
    d_buffer_common_chunk_list_init(&list);

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_empty_list",
                                  d_buffer_common_consolidate(&elements, &count, &capacity, sizeof(int), &list) == true,
                                  "Empty chunk list should return true") && result;

        d_buffer_common_free_data(elements);
    }

    // test 3: full consolidation
    {
        int source[3] = {10, 20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 3, 0);
    }

    d_buffer_common_chunk_list_init(&list);

    // add chunked elements
    value = 40;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 50;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_success",
                                  d_buffer_common_consolidate(&elements, &count, &capacity, sizeof(int), &list) == true,
                                  "Consolidation should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_count",
                                  count == 5,
                                  "Count should be 5 after consolidation") && result;

        // verify data integrity
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40) &&
                           (arr[4] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "consolidate_values",
                                      correct,
                                      "Elements should be [10, 20, 30, 40, 50]") && result;
        }

        // chunk list should be freed
        result = D_TEST_CHECK_MSG(_suite,
                                  "consolidate_list_freed",
                                  list.head == NULL && list.chunk_count == 0,
                                  "Chunk list should be cleared after consolidation") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_total_count
  Tests the d_buffer_common_total_count function.
  Tests the following:
  - NULL list returns just primary count
  - correct sum of primary + chunks
*/
bool d_tests_buffer_common_total_count(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    struct dj_buffer_chunk_list list;
    int                        value;

    d_test_suite_test(_suite, "buffer_common_total_count");

    result = true;

    // test 1: NULL list returns primary count only
    result = D_TEST_CHECK_MSG(_suite,
                              "total_count_null_list",
                              d_buffer_common_total_count(10, NULL) == 10,
                              "NULL list should return primary count") && result;

    // test 2: sum of primary + chunks
    d_buffer_common_chunk_list_init(&list);
    value = 42;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 43;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    result = D_TEST_CHECK_MSG(_suite,
                              "total_count_combined",
                              d_buffer_common_total_count(5, &list) == 7,
                              "Total should be primary(5) + chunks(2) = 7") && result;

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_buffer_common_get_element_chunked
  Tests the d_buffer_common_get_element_chunked function.
  Tests the following:
  - index in primary range returns correct element
  - index in chunk range returns correct element
  - out-of-bounds index returns NULL
  - NULL list with index beyond primary returns NULL
*/
bool d_tests_buffer_common_get_element_chunked(
    struct dj_test_suite* _suite
)
{
    bool                       result;
    void*                      elements;
    size_t                     count;
    size_t                     capacity;
    struct dj_buffer_chunk_list list;
    int                        value;
    void*                      got;

    d_test_suite_test(_suite, "buffer_common_get_element_chunked");

    result = true;

    // set up primary buffer
    {
        int source[3] = {10, 20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 3, 0);
    }

    // set up chunk list
    d_buffer_common_chunk_list_init(&list);
    value = 40;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 50;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    if (elements)
    {
        // test 1: index in primary range
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_chunked_primary",
                                  got != NULL && *(int*)got == 20,
                                  "Primary index 1 should be 20") && result;

        // test 2: index in chunk range
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_chunked_chunk",
                                  got != NULL && *(int*)got == 40,
                                  "Chunk index 3 should be 40") && result;

        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_chunked_chunk_last",
                                  got != NULL && *(int*)got == 50,
                                  "Chunk index 4 should be 50") && result;

        // test 3: out-of-bounds returns NULL
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 5);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_chunked_oob",
                                  got == NULL,
                                  "Out-of-bounds should return NULL") && result;

        // test 4: NULL list beyond primary returns NULL
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   NULL, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_chunked_null_list",
                                  got == NULL,
                                  "NULL list beyond primary should return NULL") && result;

        d_buffer_common_free_data(elements);
    }

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_buffer_common_chunked_all
  Aggregation function that runs all chunked (append mode) tests.
*/
bool d_tests_buffer_common_chunked_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_chunked_all");

    result = true;

    result = d_tests_buffer_common_chunk_new(_suite) && result;
    result = d_tests_buffer_common_chunk_free(_suite) && result;
    result = d_tests_buffer_common_chunk_list_init(_suite) && result;
    result = d_tests_buffer_common_chunk_list_free(_suite) && result;
    result = d_tests_buffer_common_append_element_chunked(_suite) && result;
    result = d_tests_buffer_common_append_data_chunked(_suite) && result;
    result = d_tests_buffer_common_consolidate(_suite) && result;
    result = d_tests_buffer_common_total_count(_suite) && result;
    result = d_tests_buffer_common_get_element_chunked(_suite) && result;

    return result;
}
