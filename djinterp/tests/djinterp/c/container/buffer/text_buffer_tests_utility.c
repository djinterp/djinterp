/*******************************************************************************
* djinterp [container]                               text_buffer_tests_utility.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_clear
  Tests the d_text_buffer_clear function.
  Tests the following:
  - NULL buffer handled gracefully (no crash)
  - clears buffer content to zero length
  - capacity preserved after clear
  - data pointer remains valid
  - buffer is empty after clear
*/
bool d_tests_text_buffer_clear(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    size_t                cap_before;

    d_test_suite_test(_suite, "text_buffer_clear");

    result = true;

    // test 1: clear populated buffer
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        cap_before = d_text_buffer_capacity(buffer);

        d_text_buffer_clear(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_length",
                                  d_text_buffer_length(buffer) == 0,
                                  "Length should be 0 after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_capacity",
                                  d_text_buffer_capacity(buffer) == cap_before,
                                  "Capacity should be preserved after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_is_empty",
                                  d_text_buffer_is_empty(buffer) == true,
                                  "Buffer should be empty after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_data_valid",
                                  buffer->data != NULL,
                                  "Data pointer should remain valid after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_null_term",
                                  buffer->data[0] == '\0',
                                  "First byte should be null terminator after clear") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: clear already-empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        d_text_buffer_clear(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Clearing empty buffer should leave length 0") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_is_empty
  Tests the d_text_buffer_is_empty function.
  Tests the following:
  - NULL buffer returns true
  - newly created buffer is empty
  - buffer with content is not empty
  - buffer after clear is empty
*/
bool d_tests_text_buffer_is_empty(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_is_empty");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "is_empty_null",
                              d_text_buffer_is_empty(NULL) == true,
                              "NULL buffer should be empty") && result;

    // test 2: new default buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_new",
                                  d_text_buffer_is_empty(buffer) == true,
                                  "New buffer should be empty") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: buffer with content
    buffer = d_text_buffer_new_from_string("data");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_with_content",
                                  d_text_buffer_is_empty(buffer) == false,
                                  "Buffer with content should not be empty") && result;

        // test 4: after clear
        d_text_buffer_clear(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "is_empty_after_clear",
                                  d_text_buffer_is_empty(buffer) == true,
                                  "Buffer after clear should be empty") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_length
  Tests the d_text_buffer_length function.
  Tests the following:
  - NULL buffer returns 0
  - empty buffer returns 0
  - returns correct length for populated buffer
  - length updates after append
*/
bool d_tests_text_buffer_length(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_length");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "length_null",
                              d_text_buffer_length(NULL) == 0,
                              "NULL buffer should return 0") && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "length_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Empty buffer should return 0") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: populated buffer
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "length_populated",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length of 'Hello' should be 5") && result;

        // test 4: length after append
        d_text_buffer_append_string(buffer, ", World!");

        result = D_TEST_CHECK_MSG(_suite,
                                  "length_after_append",
                                  d_text_buffer_length(buffer) == 13,
                                  "Length after append should be 13") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_capacity
  Tests the d_text_buffer_capacity function.
  Tests the following:
  - NULL buffer returns 0
  - default capacity matches D_BUFFER_DEFAULT_CAPACITY
  - custom capacity returned correctly
  - capacity >= count always holds
*/
bool d_tests_text_buffer_capacity(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    size_t                cap;

    d_test_suite_test(_suite, "text_buffer_capacity");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "capacity_null",
                              d_text_buffer_capacity(NULL) == 0,
                              "NULL buffer should return 0") && result;

    // test 2: default capacity
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        cap = d_text_buffer_capacity(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_default",
                                  cap >= D_BUFFER_DEFAULT_CAPACITY,
                                  "Default capacity should be >= D_BUFFER_DEFAULT_CAPACITY") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: custom capacity
    buffer = d_text_buffer_new(512);

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_custom",
                                  d_text_buffer_capacity(buffer) >= 512,
                                  "Custom capacity should be >= 512") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: capacity >= length
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "capacity_gte_length",
                                  d_text_buffer_capacity(buffer) >= d_text_buffer_length(buffer),
                                  "Capacity should always be >= length") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_utilization
  Tests the d_text_buffer_utilization function.
  Tests the following:
  - NULL buffer returns 0.0
  - empty buffer returns 0.0
  - utilization is in range [0.0, 1.0]
  - utilization increases after adding data
*/
bool d_tests_text_buffer_utilization(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    double                util;

    d_test_suite_test(_suite, "text_buffer_utilization");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "util_null",
                              d_text_buffer_utilization(NULL) == 0.0,
                              "NULL buffer should return 0.0") && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "util_empty",
                                  d_text_buffer_utilization(buffer) == 0.0,
                                  "Empty buffer utilization should be 0.0") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: populated buffer
    buffer = d_text_buffer_new(100);

    if (buffer)
    {
        d_text_buffer_append_string(buffer, "test data here");

        util = d_text_buffer_utilization(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "util_range",
                                  util > 0.0 && util <= 1.0,
                                  "Utilization should be between 0.0 and 1.0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "util_nonzero",
                                  util > 0.0,
                                  "Utilization of populated buffer should be > 0.0") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_hash
  Tests the d_text_buffer_hash function.
  Tests the following:
  - NULL buffer returns 0
  - equal content produces equal hashes
  - different content likely produces different hashes
  - empty buffer hash is consistent
*/
bool d_tests_text_buffer_hash(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buf1;
    struct dj_text_buffer* buf2;
    size_t                hash1;
    size_t                hash2;

    d_test_suite_test(_suite, "text_buffer_hash");

    result = true;

    // test 1: NULL buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "hash_null",
                              d_text_buffer_hash(NULL) == 0,
                              "NULL buffer hash should be 0") && result;

    // test 2: equal content produces equal hashes
    buf1 = d_text_buffer_new_from_string("Hello, World!");
    buf2 = d_text_buffer_new_from_string("Hello, World!");

    if (buf1 && buf2)
    {
        hash1 = d_text_buffer_hash(buf1);
        hash2 = d_text_buffer_hash(buf2);

        result = D_TEST_CHECK_MSG(_suite,
                                  "hash_equal",
                                  hash1 == hash2,
                                  "Equal content should produce equal hashes") && result;

        d_text_buffer_free(buf2);
    }

    // test 3: different content likely produces different hashes
    buf2 = d_text_buffer_new_from_string("Goodbye, World!");

    if (buf1 && buf2)
    {
        hash1 = d_text_buffer_hash(buf1);
        hash2 = d_text_buffer_hash(buf2);

        result = D_TEST_CHECK_MSG(_suite,
                                  "hash_differ",
                                  hash1 != hash2,
                                  "Different content should likely produce different hashes") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "hash_empty_consistent",
                                  d_text_buffer_hash(buf1) == d_text_buffer_hash(buf2),
                                  "Empty buffer hashes should be equal") && result;

        d_text_buffer_free(buf2);
    }

    if (buf1)
    {
        d_text_buffer_free(buf1);
    }

    return result;
}

/*
d_tests_text_buffer_utility_all
  Aggregation function that runs all utility tests.
*/
bool d_tests_text_buffer_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_utility_all");

    result = true;

    result = d_tests_text_buffer_clear(_suite) && result;
    result = d_tests_text_buffer_is_empty(_suite) && result;
    result = d_tests_text_buffer_length(_suite) && result;
    result = d_tests_text_buffer_capacity(_suite) && result;
    result = d_tests_text_buffer_utilization(_suite) && result;
    result = d_tests_text_buffer_hash(_suite) && result;

    return result;
}
