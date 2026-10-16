/*******************************************************************************
* djinterp [container]                          buffer_common_tests_sa_chunked.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_chunked.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_chunk_new
  Tests the d_buffer_common_chunk_new function.
  Tests the following:
  - zero element_size returns NULL
  - zero capacity returns NULL
  - successful allocation sets count=0, capacity, next=NULL
*/
bool
d_tests_sa_buffer_common_chunk_new
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_buffer_chunk* chunk;

    result = true;

    // test 1: zero element_size
    result = d_assert_standalone(
        d_buffer_common_chunk_new(0, 10) == NULL,
        "chunk_new_zero_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: zero capacity
    result = d_assert_standalone(
        d_buffer_common_chunk_new(sizeof(int), 0) == NULL,
        "chunk_new_zero_cap",
        "Zero capacity should return NULL",
        _counter) && result;

    // test 3: successful allocation
    chunk = d_buffer_common_chunk_new(sizeof(int), 8);
    result = d_assert_standalone(
        chunk != NULL,
        "chunk_new_success",
        "Chunk allocation should succeed",
        _counter) && result;

    if (chunk)
    {
        result = d_assert_standalone(
            chunk->count == 0,
            "chunk_new_count",
            "Chunk count should be 0",
            _counter) && result;

        result = d_assert_standalone(
            chunk->capacity == 8,
            "chunk_new_capacity",
            "Chunk capacity should be 8",
            _counter) && result;

        result = d_assert_standalone(
            chunk->next == NULL,
            "chunk_new_next_null",
            "Chunk next should be NULL",
            _counter) && result;

        result = d_assert_standalone(
            chunk->elements != NULL,
            "chunk_new_elements",
            "Chunk elements should be non-NULL",
            _counter) && result;

        d_buffer_common_chunk_free(chunk);
    }

    return result;
}


/*
d_tests_sa_buffer_common_chunk_free
  Tests the d_buffer_common_chunk_free function.
  Tests the following:
  - NULL chunk does not crash
  - valid chunk is freed without error
*/
bool
d_tests_sa_buffer_common_chunk_free
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_buffer_chunk* chunk;

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_free(NULL);
    result = d_assert_standalone(
        true,
        "chunk_free_null",
        "NULL chunk free should not crash",
        _counter) && result;

    // test 2: valid chunk freed
    chunk = d_buffer_common_chunk_new(sizeof(int), 4);

    if (chunk)
    {
        d_buffer_common_chunk_free(chunk);
        result = d_assert_standalone(
            true,
            "chunk_free_valid",
            "Valid chunk should be freed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_buffer_common_chunk_list_init
  Tests the d_buffer_common_chunk_list_init function.
  Tests the following:
  - NULL list does not crash
  - initialized list has zeroed fields
*/
bool
d_tests_sa_buffer_common_chunk_list_init
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_buffer_chunk_list list;

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_list_init(NULL);
    result = d_assert_standalone(
        true,
        "chunk_list_init_null",
        "NULL list init should not crash",
        _counter) && result;

    // test 2: fields zeroed
    list.head        = (struct d_buffer_chunk*)0xDEAD;
    list.tail        = (struct d_buffer_chunk*)0xBEEF;
    list.chunk_count = 99;
    list.total_count = 99;

    d_buffer_common_chunk_list_init(&list);

    result = d_assert_standalone(
        list.head == NULL && list.tail == NULL,
        "chunk_list_init_pointers",
        "Head and tail should be NULL",
        _counter) && result;

    result = d_assert_standalone(
        list.chunk_count == 0 && list.total_count == 0,
        "chunk_list_init_counts",
        "Chunk count and total count should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_chunk_list_free
  Tests the d_buffer_common_chunk_list_free function.
  Tests the following:
  - NULL list does not crash
  - list with chunks is freed and re-initialized
*/
bool
d_tests_sa_buffer_common_chunk_list_free
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_buffer_chunk_list list;
    int                        value;

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_chunk_list_free(NULL);
    result = d_assert_standalone(
        true,
        "chunk_list_free_null",
        "NULL list free should not crash",
        _counter) && result;

    // test 2: populated list freed correctly
    d_buffer_common_chunk_list_init(&list);
    value = 42;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 43;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    result = d_assert_standalone(
        list.total_count == 2,
        "chunk_list_free_precondition",
        "List should have 2 elements before free",
        _counter) && result;

    d_buffer_common_chunk_list_free(&list);

    result = d_assert_standalone(
        list.head == NULL && list.chunk_count == 0,
        "chunk_list_free_result",
        "List should be re-initialized after free",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_append_element_chunked
  Tests the d_buffer_common_append_element_chunked function.
  Tests the following:
  - NULL list rejection
  - NULL value rejection
  - zero element_size rejection
  - successful single element append
  - element fills tail chunk before creating new
  - total_count tracks correctly
*/
bool
d_tests_sa_buffer_common_append_element_chunked
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_buffer_chunk_list list;
    int                        value;

    result = true;

    // test 1: NULL list should fail
    value  = 42;
    result = d_assert_standalone(
        d_buffer_common_append_element_chunked(NULL, sizeof(int),
                                               &value, 4) == false,
        "append_elem_chunked_null_list",
        "NULL list should return false",
        _counter) && result;

    // test 2: NULL value should fail
    d_buffer_common_chunk_list_init(&list);
    result = d_assert_standalone(
        d_buffer_common_append_element_chunked(&list, sizeof(int),
                                               NULL, 4) == false,
        "append_elem_chunked_null_value",
        "NULL value should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    result = d_assert_standalone(
        d_buffer_common_append_element_chunked(&list, 0,
                                               &value, 4) == false,
        "append_elem_chunked_zero_size",
        "Zero element_size should return false",
        _counter) && result;

    // test 4: successful append
    d_buffer_common_chunk_list_init(&list);
    value  = 100;
    result = d_assert_standalone(
        d_buffer_common_append_element_chunked(&list, sizeof(int),
                                               &value, 4) == true,
        "append_elem_chunked_success",
        "Append should succeed",
        _counter) && result;

    result = d_assert_standalone(
        list.total_count == 1 && list.chunk_count == 1,
        "append_elem_chunked_counts",
        "total_count=1, chunk_count=1",
        _counter) && result;

    // test 5: second append fills existing chunk
    value  = 200;
    result = d_assert_standalone(
        d_buffer_common_append_element_chunked(&list, sizeof(int),
                                               &value, 4) == true,
        "append_elem_chunked_fill_tail",
        "Second append should fill existing chunk",
        _counter) && result;

    result = d_assert_standalone(
        list.total_count == 2 && list.chunk_count == 1,
        "append_elem_chunked_no_new_chunk",
        "Should still have 1 chunk",
        _counter) && result;

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_sa_buffer_common_append_data_chunked
  Tests the d_buffer_common_append_data_chunked function.
  Tests the following:
  - NULL list rejection
  - NULL data rejection
  - zero data_count rejection
  - successful multi-element chunked append
  - data spans multiple chunks when needed
*/
bool
d_tests_sa_buffer_common_append_data_chunked
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_buffer_chunk_list list;
    int                        data[5] = {10, 20, 30, 40, 50};

    result = true;

    // test 1: NULL list should fail
    result = d_assert_standalone(
        d_buffer_common_append_data_chunked(NULL, sizeof(int),
                                            data, 5, 8) == false,
        "append_data_chunked_null_list",
        "NULL list should return false",
        _counter) && result;

    // test 2: NULL data should fail
    d_buffer_common_chunk_list_init(&list);
    result = d_assert_standalone(
        d_buffer_common_append_data_chunked(&list, sizeof(int),
                                            NULL, 5, 8) == false,
        "append_data_chunked_null_data",
        "NULL data should return false",
        _counter) && result;

    // test 3: zero data_count should fail
    result = d_assert_standalone(
        d_buffer_common_append_data_chunked(&list, sizeof(int),
                                            data, 0, 8) == false,
        "append_data_chunked_zero_count",
        "Zero data_count should return false",
        _counter) && result;

    // test 4: successful append
    d_buffer_common_chunk_list_init(&list);
    result = d_assert_standalone(
        d_buffer_common_append_data_chunked(&list, sizeof(int),
                                            data, 5, 8) == true,
        "append_data_chunked_success",
        "Chunked data append should succeed",
        _counter) && result;

    result = d_assert_standalone(
        list.total_count == 5,
        "append_data_chunked_total",
        "Total count should be 5",
        _counter) && result;

    // test 5: verify data via get_element_chunked
    {
        bool all_correct = true;
        size_t i;

        for (i = 0; i < 5; ++i)
        {
            void* elem = d_buffer_common_get_element_chunked(
                             NULL, 0, sizeof(int), &list, (d_index)i);

            if ( (!elem) ||
                 (*(int*)elem != data[i]) )
            {
                all_correct = false;
            }
        }

        result = d_assert_standalone(
            all_correct,
            "append_data_chunked_values",
            "All chunked elements should match source",
            _counter) && result;
    }

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_sa_buffer_common_consolidate
  Tests the d_buffer_common_consolidate function.
  Tests the following:
  - NULL parameter rejection
  - empty chunk list is a no-op
  - successful consolidation merges chunks into primary
  - chunk list is freed after consolidation
  - data integrity after consolidation
*/
bool
d_tests_sa_buffer_common_consolidate
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    void*                      elements;
    size_t                     count;
    size_t                     capacity;
    struct d_buffer_chunk_list list;
    int                        value;

    result = true;

    // test 1: NULL elements should fail
    count    = 0;
    capacity = 0;
    d_buffer_common_chunk_list_init(&list);
    result = d_assert_standalone(
        d_buffer_common_consolidate(NULL, &count, &capacity,
                                    sizeof(int), &list) == false,
        "consolidate_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: empty chunk list no-op
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 16);
    d_buffer_common_chunk_list_init(&list);

    if (elements)
    {
        result = d_assert_standalone(
            d_buffer_common_consolidate(&elements, &count, &capacity,
                                        sizeof(int), &list) == true,
            "consolidate_empty_list",
            "Empty chunk list should return true",
            _counter) && result;

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
        result = d_assert_standalone(
            d_buffer_common_consolidate(&elements, &count, &capacity,
                                        sizeof(int), &list) == true,
            "consolidate_success",
            "Consolidation should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 5,
            "consolidate_count",
            "Count should be 5 after consolidation",
            _counter) && result;

        // verify data integrity
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40) &&
                           (arr[4] == 50);

            result = d_assert_standalone(
                correct,
                "consolidate_values",
                "Elements should be [10, 20, 30, 40, 50]",
                _counter) && result;
        }

        // chunk list should be freed
        result = d_assert_standalone(
            list.head == NULL && list.chunk_count == 0,
            "consolidate_list_freed",
            "Chunk list should be cleared after consolidation",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_total_count
  Tests the d_buffer_common_total_count function.
  Tests the following:
  - NULL list returns just primary count
  - correct sum of primary + chunks
*/
bool
d_tests_sa_buffer_common_total_count
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_buffer_chunk_list list;
    int                        value;

    result = true;

    // test 1: NULL list returns primary count only
    result = d_assert_standalone(
        d_buffer_common_total_count(10, NULL) == 10,
        "total_count_null_list",
        "NULL list should return primary count",
        _counter) && result;

    // test 2: sum of primary + chunks
    d_buffer_common_chunk_list_init(&list);
    value = 42;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);
    value = 43;
    d_buffer_common_append_element_chunked(&list, sizeof(int),
                                           &value, 4);

    result = d_assert_standalone(
        d_buffer_common_total_count(5, &list) == 7,
        "total_count_combined",
        "Total should be primary(5) + chunks(2) = 7",
        _counter) && result;

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_sa_buffer_common_get_element_chunked
  Tests the d_buffer_common_get_element_chunked function.
  Tests the following:
  - index in primary range returns correct element
  - index in chunk range returns correct element
  - out-of-bounds index returns NULL
  - NULL list with index beyond primary returns NULL
*/
bool
d_tests_sa_buffer_common_get_element_chunked
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    void*                      elements;
    size_t                     count;
    size_t                     capacity;
    struct d_buffer_chunk_list list;
    int                        value;
    void*                      got;

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
        result = d_assert_standalone(
            got != NULL && *(int*)got == 20,
            "get_chunked_primary",
            "Primary index 1 should be 20",
            _counter) && result;

        // test 2: index in chunk range
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 3);
        result = d_assert_standalone(
            got != NULL && *(int*)got == 40,
            "get_chunked_chunk",
            "Chunk index 3 should be 40",
            _counter) && result;

        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 4);
        result = d_assert_standalone(
            got != NULL && *(int*)got == 50,
            "get_chunked_chunk_last",
            "Chunk index 4 should be 50",
            _counter) && result;

        // test 3: out-of-bounds returns NULL
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   &list, 5);
        result = d_assert_standalone(
            got == NULL,
            "get_chunked_oob",
            "Out-of-bounds should return NULL",
            _counter) && result;

        // test 4: NULL list beyond primary returns NULL
        got = d_buffer_common_get_element_chunked(elements, count,
                                                   sizeof(int),
                                                   NULL, 3);
        result = d_assert_standalone(
            got == NULL,
            "get_chunked_null_list",
            "NULL list beyond primary should return NULL",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    d_buffer_common_chunk_list_free(&list);

    return result;
}


/*
d_tests_sa_buffer_common_chunked_all
  Aggregation function that runs all chunked (append mode) tests.
*/
bool
d_tests_sa_buffer_common_chunked_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Insertion (Append Mode) Functions\n");
    printf("  ---------------------------------------------\n");

    result = d_tests_sa_buffer_common_chunk_new(_counter) && result;
    result = d_tests_sa_buffer_common_chunk_free(_counter) && result;
    result = d_tests_sa_buffer_common_chunk_list_init(_counter) && result;
    result = d_tests_sa_buffer_common_chunk_list_free(_counter) && result;
    result = d_tests_sa_buffer_common_append_element_chunked(_counter) && result;
    result = d_tests_sa_buffer_common_append_data_chunked(_counter) && result;
    result = d_tests_sa_buffer_common_consolidate(_counter) && result;
    result = d_tests_sa_buffer_common_total_count(_counter) && result;
    result = d_tests_sa_buffer_common_get_element_chunked(_counter) && result;

    return result;
}
