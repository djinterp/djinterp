/*******************************************************************************
* djinterp [container]                             text_buffer_tests_sa_filter.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa_filter.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * SECTION X: FILTER FUNCTIONS
 *
 * NOTE ON API MIGRATION:
 *   text_buffer.h currently declares filter functions with
 *   `const struct d_filter_expr*` which no longer exists in filter.h.
 *
 *   These tests assume text_buffer.h will be updated to use:
 *       const struct d_filter_chain* _chain
 *   in place of:
 *       const struct d_filter_expr* _expression
 *
 *   The updated signatures would be:
 *     d_text_buffer_filter(buffer, chain)          -> struct d_text_buffer*
 *     d_text_buffer_filter_in_place(buffer, chain) -> bool
 *     d_text_buffer_filter_indices(buffer, chain,
 *                                  out_idx, out_count) -> bool
 *     d_text_buffer_count_matching(buffer, chain)  -> size_t
 *     d_text_buffer_filter_chunked(buffer, chain)  -> struct d_text_buffer*
 *
 *   If a different replacement type is chosen, update these tests
 *   accordingly.  The test logic itself (NULL checks, content
 *   validation, boundary conditions) will remain the same.
 *****************************************************************************/


/******************************************************************************
 * HELPER PREDICATES
 *
 *   Filter operations on a text buffer treat each character as an
 *   element.  The predicate receives a `const void*` pointing to a
 *   single char.
 *****************************************************************************/


/*
pred_is_uppercase
  Predicate: returns true if the character is uppercase A-Z.
*/
D_STATIC bool
pred_is_uppercase
(
    const void* _element
)
{
    char c;

    if (!_element)
    {
        return false;
    }

    c = *(const char*)_element;

    return (c >= 'A' && c <= 'Z');
}

/*
pred_is_vowel
  Predicate: returns true if the character is a vowel (a/e/i/o/u,
  case-insensitive).
*/
D_STATIC bool
pred_is_vowel
(
    const void* _element
)
{
    char c;

    if (!_element)
    {
        return false;
    }

    c = *(const char*)_element;

    // convert to lowercase for comparison
    if (c >= 'A' && c <= 'Z')
    {
        c = (char)(c + ('a' - 'A'));
    }

    return (c == 'a' || c == 'e' || c == 'i'
            || c == 'o' || c == 'u');
}

/*
pred_is_digit
  Predicate: returns true if the character is a digit 0-9.
*/
D_STATIC bool
pred_is_digit
(
    const void* _element
)
{
    char c;

    if (!_element)
    {
        return false;
    }

    c = *(const char*)_element;

    return (c >= '0' && c <= '9');
}

/******************************************************************************
 * HELPER: build a simple "where" filter chain from a predicate
 *****************************************************************************/


/*
build_where_chain
  Creates a d_filter_chain containing a single WHERE operation
  using the supplied predicate.  Caller must free with
  d_filter_chain_free().
*/
D_STATIC struct d_filter_chain*
build_where_chain
(
    fn_predicate _test
)
{
    struct d_filter_chain* chain;

    chain = d_filter_chain_new();

    if (chain)
    {
        d_filter_chain_add_where(chain, _test);
    }

    return chain;
}

/******************************************************************************
 * TEST FUNCTIONS
 *****************************************************************************/


/*
d_tests_sa_text_buffer_filter
  Tests the d_text_buffer_filter function.
  Tests the following:
  - NULL buffer returns NULL
  - NULL chain returns NULL
  - returns new buffer containing only matching characters
  - original buffer is unchanged
  - empty result when no characters match
  - all characters match returns full copy
*/
bool
d_tests_sa_text_buffer_filter
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_text_buffer*  buffer;
    struct d_text_buffer*  filtered;
    struct d_filter_chain* chain;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter(NULL, chain) == NULL,
            "filter_null_buf",
            "NULL buffer should return NULL",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_filter(buffer, NULL) == NULL,
            "filter_null_chain",
            "NULL chain should return NULL",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter uppercase from mixed string
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter(buffer, chain);

        result = d_assert_standalone(
            filtered != NULL,
            "filter_upper_not_null",
            "Filtered result should not be NULL",
            _counter) && result;

        if (filtered)
        {
            // "Hello World" uppercase chars: H, W
            result = d_assert_standalone(
                d_text_buffer_length(filtered) == 2,
                "filter_upper_len",
                "Filtered buffer should have 2 uppercase chars",
                _counter) && result;

            result = d_assert_standalone(
                filtered->data[0] == 'H' && filtered->data[1] == 'W',
                "filter_upper_content",
                "Filtered buffer should contain 'HW'",
                _counter) && result;

            d_text_buffer_free(filtered);
        }

        // test 4: original unchanged
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 11,
            "filter_original_unchanged",
            "Original buffer should be unchanged",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 5: no matches -> empty or NULL result
    buffer = d_text_buffer_new_from_string("hello");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter(buffer, chain);

        // depending on implementation, may return empty buffer or NULL
        if (filtered)
        {
            result = d_assert_standalone(
                d_text_buffer_length(filtered) == 0,
                "filter_no_match_empty",
                "No matches should produce empty buffer",
                _counter) && result;

            d_text_buffer_free(filtered);
        }
        else
        {
            result = d_assert_standalone(
                true,
                "filter_no_match_null",
                "No matches may return NULL",
                _counter) && result;
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 6: all characters match
    buffer = d_text_buffer_new_from_string("12345");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter(buffer, chain);

        result = d_assert_standalone(
            filtered != NULL,
            "filter_all_match_not_null",
            "All-match filter should not return NULL",
            _counter) && result;

        if (filtered)
        {
            result = d_assert_standalone(
                d_text_buffer_length(filtered) == 5,
                "filter_all_match_len",
                "All-match filter should return all 5 chars",
                _counter) && result;

            result = d_assert_standalone(
                d_strcasecmp(filtered->data, "12345") == 0,
                "filter_all_match_content",
                "All-match filter should return '12345'",
                _counter) && result;

            d_text_buffer_free(filtered);
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_filter_in_place
  Tests the d_text_buffer_filter_in_place function.
  Tests the following:
  - NULL buffer returns false
  - NULL chain returns false
  - buffer is modified to contain only matching characters
  - length is updated correctly
  - empty result when no characters match
*/
bool
d_tests_sa_text_buffer_filter_in_place
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_text_buffer*  buffer;
    struct d_filter_chain* chain;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_in_place(NULL, chain) == false,
            "filter_ip_null_buf",
            "NULL buffer should return false",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_in_place(buffer, NULL) == false,
            "filter_ip_null_chain",
            "NULL chain should return false",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter vowels in place
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_vowel);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_in_place(buffer, chain) == true,
            "filter_ip_vowels",
            "In-place vowel filter should succeed",
            _counter) && result;

        // "Hello World" vowels: e, o, o
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 3,
            "filter_ip_vowels_len",
            "Length should be 3 after vowel filter",
            _counter) && result;

        result = d_assert_standalone(
            buffer->data[0] == 'e' && buffer->data[1] == 'o'
            && buffer->data[2] == 'o',
            "filter_ip_vowels_content",
            "Content should be 'eoo'",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 4: no matches -> buffer becomes empty
    buffer = d_text_buffer_new_from_string("bcdfg");
    chain = build_where_chain(pred_is_vowel);

    if (buffer && chain)
    {
        d_text_buffer_filter_in_place(buffer, chain);

        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 0,
            "filter_ip_no_match",
            "No matches should leave buffer empty",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_filter_indices
  Tests the d_text_buffer_filter_indices function.
  Tests the following:
  - NULL buffer returns false
  - NULL chain returns false
  - NULL out_indices returns false
  - NULL out_count returns false
  - returns correct indices of matching characters
  - out_count is set correctly
  - caller must free returned index array
*/
bool
d_tests_sa_text_buffer_filter_indices
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_text_buffer*  buffer;
    struct d_filter_chain* chain;
    d_index*               indices;
    size_t                 count;

    result  = true;
    indices = NULL;
    count   = 0;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_indices(NULL, chain,
                                         &indices, &count) == false,
            "filter_idx_null_buf",
            "NULL buffer should return false",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_indices(buffer, NULL,
                                         &indices, &count) == false,
            "filter_idx_null_chain",
            "NULL chain should return false",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: NULL out_indices
    buffer = d_text_buffer_new_from_string("Hello");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_indices(buffer, chain,
                                         NULL, &count) == false,
            "filter_idx_null_out",
            "NULL out_indices should return false",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 4: NULL out_count
    buffer = d_text_buffer_new_from_string("Hello");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_indices(buffer, chain,
                                         &indices, NULL) == false,
            "filter_idx_null_count",
            "NULL out_count should return false",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 5: correct indices returned
    // "Hello World" -> uppercase at indices 0 ('H') and 6 ('W')
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_uppercase);
    indices = NULL;
    count   = 0;

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_indices(buffer, chain,
                                         &indices, &count) == true,
            "filter_idx_success",
            "Index retrieval should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 2,
            "filter_idx_count",
            "Should find 2 uppercase indices",
            _counter) && result;

        if (indices && count == 2)
        {
            result = d_assert_standalone(
                indices[0] == 0,
                "filter_idx_first",
                "First uppercase index should be 0 ('H')",
                _counter) && result;

            result = d_assert_standalone(
                indices[1] == 6,
                "filter_idx_second",
                "Second uppercase index should be 6 ('W')",
                _counter) && result;

            free(indices);
            indices = NULL;
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 6: no matches -> count = 0
    buffer = d_text_buffer_new_from_string("hello");
    chain = build_where_chain(pred_is_digit);
    indices = NULL;
    count   = 0;

    if (buffer && chain)
    {
        d_text_buffer_filter_indices(buffer, chain, &indices, &count);

        result = d_assert_standalone(
            count == 0,
            "filter_idx_no_match",
            "No matches should set count to 0",
            _counter) && result;

        if (indices)
        {
            free(indices);
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_count_matching
  Tests the d_text_buffer_count_matching function.
  Tests the following:
  - NULL buffer returns 0
  - NULL chain returns 0
  - returns correct count of matching characters
  - returns 0 when no characters match
  - returns full length when all characters match
*/
bool
d_tests_sa_text_buffer_count_matching
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_text_buffer*  buffer;
    struct d_filter_chain* chain;
    size_t                 count;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = d_assert_standalone(
            d_text_buffer_count_matching(NULL, chain) == 0,
            "count_match_null_buf",
            "NULL buffer should return 0",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_count_matching(buffer, NULL) == 0,
            "count_match_null_chain",
            "NULL chain should return 0",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: count vowels in "Hello World"
    // vowels: e, o, o -> 3
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_vowel);

    if (buffer && chain)
    {
        count = d_text_buffer_count_matching(buffer, chain);

        result = d_assert_standalone(
            count == 3,
            "count_match_vowels",
            "Should count 3 vowels in 'Hello World'",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 4: no matches
    buffer = d_text_buffer_new_from_string("bcdfg");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_count_matching(buffer, chain) == 0,
            "count_match_zero",
            "Should count 0 digits in 'bcdfg'",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 5: all match
    buffer = d_text_buffer_new_from_string("98765");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_count_matching(buffer, chain) == 5,
            "count_match_all",
            "Should count 5 digits in '98765'",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 6: empty buffer
    buffer = d_text_buffer_new_default_capacity();
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        result = d_assert_standalone(
            d_text_buffer_count_matching(buffer, chain) == 0,
            "count_match_empty",
            "Empty buffer should count 0",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_filter_chunked
  Tests the d_text_buffer_filter_chunked function.
  Tests the following:
  - NULL buffer returns NULL
  - NULL chain returns NULL
  - returns a new chunked buffer with matching characters
  - original buffer unchanged
  - total length of returned buffer (primary + chunks) is correct
*/
bool
d_tests_sa_text_buffer_filter_chunked
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_text_buffer*  buffer;
    struct d_text_buffer*  filtered;
    struct d_filter_chain* chain;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_digit);

    if (chain)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_chunked(NULL, chain) == NULL,
            "filter_chunk_null_buf",
            "NULL buffer should return NULL",
            _counter) && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("abc123");

    if (buffer)
    {
        result = d_assert_standalone(
            d_text_buffer_filter_chunked(buffer, NULL) == NULL,
            "filter_chunk_null_chain",
            "NULL chain should return NULL",
            _counter) && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter digits from mixed string
    buffer = d_text_buffer_new_from_string("abc123def456");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter_chunked(buffer, chain);

        result = d_assert_standalone(
            filtered != NULL,
            "filter_chunk_not_null",
            "Chunked filter result should not be NULL",
            _counter) && result;

        if (filtered)
        {
            // "abc123def456" digits: 1,2,3,4,5,6  -> 6 chars
            result = d_assert_standalone(
                d_text_buffer_total_length(filtered) == 6,
                "filter_chunk_total_len",
                "Total length should be 6 digits",
                _counter) && result;

            d_text_buffer_free(filtered);
        }

        // test 4: original unchanged
        result = d_assert_standalone(
            d_text_buffer_length(buffer) == 12,
            "filter_chunk_orig_unchanged",
            "Original buffer should remain length 12",
            _counter) && result;

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 5: all characters match
    buffer = d_text_buffer_new_from_string("HELLO");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter_chunked(buffer, chain);

        result = d_assert_standalone(
            filtered != NULL,
            "filter_chunk_all_not_null",
            "All-match chunked filter should not return NULL",
            _counter) && result;

        if (filtered)
        {
            result = d_assert_standalone(
                d_text_buffer_total_length(filtered) == 5,
                "filter_chunk_all_len",
                "All-match total length should be 5",
                _counter) && result;

            d_text_buffer_free(filtered);
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    // test 6: no matches
    buffer = d_text_buffer_new_from_string("hello");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter_chunked(buffer, chain);

        if (filtered)
        {
            result = d_assert_standalone(
                d_text_buffer_total_length(filtered) == 0,
                "filter_chunk_no_match",
                "No matches should produce total length 0",
                _counter) && result;

            d_text_buffer_free(filtered);
        }
        else
        {
            result = d_assert_standalone(
                true,
                "filter_chunk_no_match_null",
                "No matches may return NULL",
                _counter) && result;
        }

        d_filter_chain_free(chain);
        d_text_buffer_free(buffer);
    }
    else
    {
        if (chain) d_filter_chain_free(chain);
        if (buffer)   d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_sa_text_buffer_filter_all
  Aggregation function that runs all filter tests.
*/
bool
d_tests_sa_text_buffer_filter_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Filter Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_text_buffer_filter(_counter) && result;
    result = d_tests_sa_text_buffer_filter_in_place(_counter) && result;
    result = d_tests_sa_text_buffer_filter_indices(_counter) && result;
    result = d_tests_sa_text_buffer_count_matching(_counter) && result;
    result = d_tests_sa_text_buffer_filter_chunked(_counter) && result;

    return result;
}
