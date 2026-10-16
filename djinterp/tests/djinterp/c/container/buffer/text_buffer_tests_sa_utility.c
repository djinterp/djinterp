/*******************************************************************************
* djinterp [container]                            text_buffer_tests_sa_utility.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_utility.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_clear
  Tests the d_text_buffer_clear function.
  Tests the following:
  - NULL buffer handled gracefully (no crash)
  - clears buffer content to zero length
  - capacity preserved after clear
  - data pointer remains valid
  - buffer is empty after clear
*/
bool
d_tests_sa_text_buffer_clear
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    size_t                cap_before;

    result = true;

    // test 1: clear populated buffer
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        cap_before = d_text_buffer_capacity(buffer);

        d_text_buffer_clear(buffer);

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "clear_length",
            "Length should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) == cap_before,
            "clear_capacity",
            "Capacity should be preserved after clear",
            _counter) && result;

        result = d_assert_standalone(
            d_text_buffer_is_empty(buffer) == true,
            "clear_is_empty",
            "Buffer should be empty after clear",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data != NULL,
            "clear_data_valid",
            "Data pointer should remain valid after clear",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data[0] == '\0',
            "clear_null_term",
            "First byte should be null terminator after clear",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 2: clear already-empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        d_text_buffer_clear(buffer);

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "clear_empty",
            "Clearing empty buffer should leave length 0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_is_empty
  Tests the d_text_buffer_is_empty function.
  Tests the following:
  - NULL buffer returns true
  - newly created buffer is empty
  - buffer with content is not empty
  - buffer after clear is empty
*/
bool
d_tests_sa_text_buffer_is_empty
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_is_empty(NULL) == true,
        "is_empty_null",
        "NULL buffer should be empty",
        _counter) && result;

    // test 2: new default buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_is_empty(buffer) == true,
            "is_empty_new",
            "New buffer should be empty",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: buffer with content
    buffer = d_text_buffer_new_from_string("data");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_is_empty(buffer) == false,
            "is_empty_with_content",
            "Buffer with content should not be empty",
            _counter) && result;

        // test 4: after clear
        d_text_buffer_clear(buffer);

        result = d_assert_standalone(
            d_text_buffer_is_empty(buffer) == true,
            "is_empty_after_clear",
            "Buffer after clear should be empty",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_length
  Tests the d_text_buffer_length function.
  Tests the following:
  - NULL buffer returns 0
  - empty buffer returns 0
  - returns correct length for populated buffer
  - length updates after append
*/
bool
d_tests_sa_text_buffer_length
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_length(NULL) == 0,
        "length_null",
        "NULL buffer should return 0",
        _counter) && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "length_empty",
            "Empty buffer should return 0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: populated buffer
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 5,
            "length_populated",
            "Length of 'Hello' should be 5",
            _counter) && result;

        // test 4: length after append
        d_text_buffer_append_string(buffer, ", World!");

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 13,
            "length_after_append",
            "Length after append should be 13",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_capacity
  Tests the d_text_buffer_capacity function.
  Tests the following:
  - NULL buffer returns 0
  - default capacity matches D_BUFFER_DEFAULT_CAPACITY
  - custom capacity returned correctly
  - capacity >= count always holds
*/
bool
d_tests_sa_text_buffer_capacity
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    size_t                cap;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_capacity(NULL) == 0,
        "capacity_null",
        "NULL buffer should return 0",
        _counter) && result;

    // test 2: default capacity
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        cap = d_text_buffer_capacity(buffer);

        result = d_assert_standalone(
            cap >= D_BUFFER_DEFAULT_CAPACITY,
            "capacity_default",
            "Default capacity should be >= D_BUFFER_DEFAULT_CAPACITY",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: custom capacity
    buffer = d_text_buffer_new(512);

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= 512,
            "capacity_custom",
            "Custom capacity should be >= 512",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 4: capacity >= length
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_capacity(buffer) >= d_text_buffer_length(buffer),
            "capacity_gte_length",
            "Capacity should always be >= length",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_utilization
  Tests the d_text_buffer_utilization function.
  Tests the following:
  - NULL buffer returns 0.0
  - empty buffer returns 0.0
  - utilization is in range [0.0, 1.0]
  - utilization increases after adding data
*/
bool
d_tests_sa_text_buffer_utilization
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buffer;
    double                util;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_utilization(NULL) == 0.0,
        "util_null",
        "NULL buffer should return 0.0",
        _counter) && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_utilization(buffer) == 0.0,
            "util_empty",
            "Empty buffer utilization should be 0.0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: populated buffer
    buffer = d_text_buffer_new(100);

    if (buffer)
    {
        d_text_buffer_append_string(buffer, "test data here");

        util = d_text_buffer_utilization(buffer);

        result = d_assert_standalone(
            util > 0.0 && util <= 1.0,
            "util_range",
            "Utilization should be between 0.0 and 1.0",
            _counter) && result;

        result = d_assert_standalone(
            util > 0.0,
            "util_nonzero",
            "Utilization of populated buffer should be > 0.0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_hash
  Tests the d_text_buffer_hash function.
  Tests the following:
  - NULL buffer returns 0
  - equal content produces equal hashes
  - different content likely produces different hashes
  - empty buffer hash is consistent
*/
bool
d_tests_sa_text_buffer_hash
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_text_buffer* buf1;
    struct d_text_buffer* buf2;
    size_t                hash1;
    size_t                hash2;

    result = true;

    // test 1: NULL buffer
    result = d_assert_standalone(
        d_text_buffer_hash(NULL) == 0,
        "hash_null",
        "NULL buffer hash should be 0",
        _counter) && result;

    // test 2: equal content produces equal hashes
    buf1 = d_text_buffer_new_from_string("Hello, World!");
    buf2 = d_text_buffer_new_from_string("Hello, World!");

    if (buf1 && buf2)
    {
        hash1 = d_text_buffer_hash(buf1);
        hash2 = d_text_buffer_hash(buf2);

        result = d_assert_standalone(
            hash1 == hash2,
            "hash_equal",
            "Equal content should produce equal hashes",
            _counter) && result;

        d_text_buffer_free(buf2);
    }

    // test 3: different content likely produces different hashes
    buf2 = d_text_buffer_new_from_string("Goodbye, World!");

    if (buf1 && buf2)
    {
        hash1 = d_text_buffer_hash(buf1);
        hash2 = d_text_buffer_hash(buf2);

        result = d_assert_standalone(
            hash1 != hash2,
            "hash_differ",
            "Different content should likely produce different hashes",
            _counter) && result;

        d_text_buffer_free(buf2);
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    // test 4: empty buffer hash consistency
    buf1 = d_text_buffer_new_default_capacity();
    buf2 = d_text_buffer_new_default_capacity();

    if (buf1 && buf2)
    {
        result = d_assert_standalone(
            d_text_buffer_hash(buf1) == d_text_buffer_hash(buf2),
            "hash_empty_consistent",
            "Empty buffer hashes should be equal",
            _counter) && result;

        d_text_buffer_free(buf2);
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_sa_text_buffer_utility_all
  Aggregation function that runs all utility tests.
*/
bool
d_tests_sa_text_buffer_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_text_buffer_clear(_counter) && result;
    result = d_tests_sa_text_buffer_is_empty(_counter) && result;
    result = d_tests_sa_text_buffer_length(_counter) && result;
    result = d_tests_sa_text_buffer_capacity(_counter) && result;
    result = d_tests_sa_text_buffer_utilization(_counter) && result;
    result = d_tests_sa_text_buffer_hash(_counter) && result;

    return result;
}
