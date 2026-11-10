/*******************************************************************************
* djinterp [test]                                     text_buffer_tests_filter.c
*
*   Standalone tests of text_buffer.h's filter functions: filter,
* filter_in_place, filter_indices, count_matching and filter_chunked, with
* predicates of fn_predicate's shape, (const void* element, void* context).
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_filter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * SECTION X: FILTER FUNCTIONS
 *
 * NOTE ON API MIGRATION:
 *   text_buffer.h currently declares filter functions with
 *   `const struct d_filter_expr*` which no longer exists in filter.h.
 *
 *   These tests assume text_buffer.h will be updated to use:
 *       const struct dj_filter_chain* _chain
 *   in place of:
 *       const struct d_filter_expr* _expression
 *
 *   The updated signatures would be:
 *     d_text_buffer_filter(buffer, chain)          -> struct dj_text_buffer*
 *     d_text_buffer_filter_in_place(buffer, chain) -> bool
 *     d_text_buffer_filter_indices(buffer, chain,
 *                                  out_idx, out_count) -> bool
 *     d_text_buffer_count_matching(buffer, chain)  -> size_t
 *     d_text_buffer_filter_chunked(buffer, chain)  -> struct dj_text_buffer*
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
 *   single char, and fn_predicate's context, which it does not use.
 *****************************************************************************/


/*
pred_is_uppercase
  Predicate: returns true if the character is uppercase A-Z.
*/
D_STATIC
bool pred_is_uppercase(
    const void* _element,
    void*       _context
)
{
    char c;

    // fn_predicate's context, which these do not use
    (void)_context;

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
D_STATIC
bool pred_is_vowel(
    const void* _element,
    void*       _context
)
{
    char c;

    // fn_predicate's context, which these do not use
    (void)_context;

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
D_STATIC
bool pred_is_digit(
    const void* _element,
    void*       _context
)
{
    char c;

    // fn_predicate's context, which these do not use
    (void)_context;

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
  Creates a dj_filter_chain containing a single WHERE operation
  using the supplied predicate.  Caller must free with
  d_filter_chain_free().
*/
D_STATIC
struct dj_filter_chain* build_where_chain(
    fn_predicate _test
)
{
    struct dj_filter_chain* chain;

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


/**
 * @brief Part 1 of d_tests_text_buffer_filter.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL buffer
 *   - test 2: NULL chain
 *   - test 3: filter uppercase from mixed string
 */
static bool
d_internal_tests_text_buffer_filter_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_text_buffer*  filtered;
    struct dj_filter_chain* chain;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_null_buf",
                                  d_text_buffer_filter(NULL, chain) == NULL,
                                  "NULL buffer should return NULL") && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_null_chain",
                                  d_text_buffer_filter(buffer, NULL) == NULL,
                                  "NULL chain should return NULL") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter uppercase from mixed string
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter(buffer, chain);

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_upper_not_null",
                                  filtered != NULL,
                                  "Filtered result should not be NULL") && result;

        if (filtered)
        {
            // "Hello World" uppercase chars: H, W
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_upper_len",
                                      d_text_buffer_length(filtered) == 2,
                                      "Filtered buffer should have 2 uppercase chars") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_upper_content",
                                      filtered->data[0] == 'H' && filtered->data[1] == 'W',
                                      "Filtered buffer should contain 'HW'") && result;

            d_text_buffer_free(filtered);
        }

        // test 4: original unchanged
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_original_unchanged",
                                  d_text_buffer_length(buffer) == 11,
                                  "Original buffer should be unchanged") && result;

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


/**
 * @brief Part 2 of d_tests_text_buffer_filter.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: no matches -> empty or NULL result
 *   - test 6: all characters match
 */
static bool
d_internal_tests_text_buffer_filter_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_text_buffer*  filtered;
    struct dj_filter_chain* chain;

    result = true;

    // test 5: no matches -> empty or NULL result
    buffer = d_text_buffer_new_from_string("hello");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter(buffer, chain);

        // depending on implementation, may return empty buffer or NULL
        if (filtered)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_no_match_empty",
                                      d_text_buffer_length(filtered) == 0,
                                      "No matches should produce empty buffer") && result;

            d_text_buffer_free(filtered);
        }
        else
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_no_match_null",
                                      true,
                                      "No matches may return NULL") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_all_match_not_null",
                                  filtered != NULL,
                                  "All-match filter should not return NULL") && result;

        if (filtered)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_all_match_len",
                                      d_text_buffer_length(filtered) == 5,
                                      "All-match filter should return all 5 chars") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_all_match_content",
                                      d_strcasecmp(filtered->data, "12345") == 0,
                                      "All-match filter should return '12345'") && result;

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
d_tests_text_buffer_filter
  Tests the d_text_buffer_filter function.
  Tests the following:
  - NULL buffer returns NULL
  - NULL chain returns NULL
  - returns new buffer containing only matching characters
  - original buffer is unchanged
  - empty result when no characters match
  - all characters match returns full copy
*/
bool d_tests_text_buffer_filter(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "text_buffer_filter");

    result = true;
    result &= d_internal_tests_text_buffer_filter_part1(_suite);
    result &= d_internal_tests_text_buffer_filter_part2(_suite);

    return result;
}

/*
d_tests_text_buffer_filter_in_place
  Tests the d_text_buffer_filter_in_place function.
  Tests the following:
  - NULL buffer returns false
  - NULL chain returns false
  - buffer is modified to contain only matching characters
  - length is updated correctly
  - empty result when no characters match
*/
bool d_tests_text_buffer_filter_in_place(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_text_buffer*  buffer;
    struct dj_filter_chain* chain;

    d_test_suite_test(_suite, "text_buffer_filter_in_place");

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_null_buf",
                                  d_text_buffer_filter_in_place(NULL, chain) == false,
                                  "NULL buffer should return false") && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_null_chain",
                                  d_text_buffer_filter_in_place(buffer, NULL) == false,
                                  "NULL chain should return false") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter vowels in place
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_vowel);

    if (buffer && chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_vowels",
                                  d_text_buffer_filter_in_place(buffer, chain) == true,
                                  "In-place vowel filter should succeed") && result;

        // "Hello World" vowels: e, o, o
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_vowels_len",
                                  d_text_buffer_length(buffer) == 3,
                                  "Length should be 3 after vowel filter") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_vowels_content",
                                  buffer->data[0] == 'e' && buffer->data[1] == 'o' && buffer->data[2] == 'o',
                                  "Content should be 'eoo'") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_ip_no_match",
                                  d_text_buffer_length(buffer) == 0,
                                  "No matches should leave buffer empty") && result;

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

/**
 * @brief Part 1 of d_tests_text_buffer_filter_indices.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL buffer
 *   - test 2: NULL chain
 *   - test 3: NULL out_indices
 *   - test 4: NULL out_count
 */
static bool
d_internal_tests_text_buffer_filter_indices_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_filter_chain* chain;
    dj_index*               indices;
    size_t                  count;

    result  = true;
    indices = NULL;
    count   = 0;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_null_buf",
                                  d_text_buffer_filter_indices(NULL, chain, &indices, &count) == false,
                                  "NULL buffer should return false") && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_null_chain",
                                  d_text_buffer_filter_indices(buffer, NULL, &indices, &count) == false,
                                  "NULL chain should return false") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: NULL out_indices
    buffer = d_text_buffer_new_from_string("Hello");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_null_out",
                                  d_text_buffer_filter_indices(buffer, chain, NULL, &count) == false,
                                  "NULL out_indices should return false") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_null_count",
                                  d_text_buffer_filter_indices(buffer, chain, &indices, NULL) == false,
                                  "NULL out_count should return false") && result;

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


/**
 * @brief Part 2 of d_tests_text_buffer_filter_indices.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: correct indices returned
 *   - test 6: no matches -> count = 0
 */
static bool
d_internal_tests_text_buffer_filter_indices_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_filter_chain* chain;
    dj_index*               indices;
    size_t                  count;

    result  = true;
    indices = NULL;
    count   = 0;

    // test 5: correct indices returned
    // "Hello World" -> uppercase at indices 0 ('H') and 6 ('W')
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_uppercase);
    indices = NULL;
    count   = 0;

    if (buffer && chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_success",
                                  d_text_buffer_filter_indices(buffer, chain, &indices, &count) == true,
                                  "Index retrieval should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_count",
                                  count == 2,
                                  "Should find 2 uppercase indices") && result;

        if (indices && count == 2)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_idx_first",
                                      indices[0] == 0,
                                      "First uppercase index should be 0 ('H')") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_idx_second",
                                      indices[1] == 6,
                                      "Second uppercase index should be 6 ('W')") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_idx_no_match",
                                  count == 0,
                                  "No matches should set count to 0") && result;

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
d_tests_text_buffer_filter_indices
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
bool d_tests_text_buffer_filter_indices(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "text_buffer_filter_indices");

    result = true;
    result &= d_internal_tests_text_buffer_filter_indices_part1(_suite);
    result &= d_internal_tests_text_buffer_filter_indices_part2(_suite);

    return result;
}

/**
 * @brief Part 1 of d_tests_text_buffer_count_matching.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL buffer
 *   - test 2: NULL chain
 *   - test 3: count vowels in "Hello World"
 *   - test 4: no matches
 */
static bool
d_internal_tests_text_buffer_count_matching_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_filter_chain* chain;
    size_t                  count;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_uppercase);

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_null_buf",
                                  d_text_buffer_count_matching(NULL, chain) == 0,
                                  "NULL buffer should return 0") && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("Hello");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_null_chain",
                                  d_text_buffer_count_matching(buffer, NULL) == 0,
                                  "NULL chain should return 0") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: count vowels in "Hello World"
    // vowels: e, o, o -> 3
    buffer = d_text_buffer_new_from_string("Hello World");
    chain = build_where_chain(pred_is_vowel);

    if (buffer && chain)
    {
        count = d_text_buffer_count_matching(buffer, chain);

        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_vowels",
                                  count == 3,
                                  "Should count 3 vowels in 'Hello World'") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_zero",
                                  d_text_buffer_count_matching(buffer, chain) == 0,
                                  "Should count 0 digits in 'bcdfg'") && result;

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


/**
 * @brief Part 2 of d_tests_text_buffer_count_matching.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: all match
 *   - test 6: empty buffer
 */
static bool
d_internal_tests_text_buffer_count_matching_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_filter_chain* chain;

    result = true;

    // test 5: all match
    buffer = d_text_buffer_new_from_string("98765");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_all",
                                  d_text_buffer_count_matching(buffer, chain) == 5,
                                  "Should count 5 digits in '98765'") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_match_empty",
                                  d_text_buffer_count_matching(buffer, chain) == 0,
                                  "Empty buffer should count 0") && result;

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
d_tests_text_buffer_count_matching
  Tests the d_text_buffer_count_matching function.
  Tests the following:
  - NULL buffer returns 0
  - NULL chain returns 0
  - returns correct count of matching characters
  - returns 0 when no characters match
  - returns full length when all characters match
*/
bool d_tests_text_buffer_count_matching(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "text_buffer_count_matching");

    result = true;
    result &= d_internal_tests_text_buffer_count_matching_part1(_suite);
    result &= d_internal_tests_text_buffer_count_matching_part2(_suite);

    return result;
}

/**
 * @brief Part 1 of d_tests_text_buffer_filter_chunked.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL buffer
 *   - test 2: NULL chain
 *   - test 3: filter digits from mixed string
 */
static bool
d_internal_tests_text_buffer_filter_chunked_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_text_buffer*  filtered;
    struct dj_filter_chain* chain;

    result = true;

    // test 1: NULL buffer
    chain = build_where_chain(pred_is_digit);

    if (chain)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_chunk_null_buf",
                                  d_text_buffer_filter_chunked(NULL, chain) == NULL,
                                  "NULL buffer should return NULL") && result;

        d_filter_chain_free(chain);
    }

    // test 2: NULL chain
    buffer = d_text_buffer_new_from_string("abc123");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_chunk_null_chain",
                                  d_text_buffer_filter_chunked(buffer, NULL) == NULL,
                                  "NULL chain should return NULL") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: filter digits from mixed string
    buffer = d_text_buffer_new_from_string("abc123def456");
    chain = build_where_chain(pred_is_digit);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter_chunked(buffer, chain);

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_chunk_not_null",
                                  filtered != NULL,
                                  "Chunked filter result should not be NULL") && result;

        if (filtered)
        {
            // "abc123def456" digits: 1,2,3,4,5,6  -> 6 chars
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_chunk_total_len",
                                      d_text_buffer_total_length(filtered) == 6,
                                      "Total length should be 6 digits") && result;

            d_text_buffer_free(filtered);
        }

        // test 4: original unchanged
        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_chunk_orig_unchanged",
                                  d_text_buffer_length(buffer) == 12,
                                  "Original buffer should remain length 12") && result;

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


/**
 * @brief Part 2 of d_tests_text_buffer_filter_chunked.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: all characters match
 *   - test 6: no matches
 */
static bool
d_internal_tests_text_buffer_filter_chunked_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_text_buffer*  buffer;
    struct dj_text_buffer*  filtered;
    struct dj_filter_chain* chain;

    result = true;

    // test 5: all characters match
    buffer = d_text_buffer_new_from_string("HELLO");
    chain = build_where_chain(pred_is_uppercase);

    if (buffer && chain)
    {
        filtered = d_text_buffer_filter_chunked(buffer, chain);

        result = D_TEST_CHECK_MSG(_suite,
                                  "filter_chunk_all_not_null",
                                  filtered != NULL,
                                  "All-match chunked filter should not return NULL") && result;

        if (filtered)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_chunk_all_len",
                                      d_text_buffer_total_length(filtered) == 5,
                                      "All-match total length should be 5") && result;

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
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_chunk_no_match",
                                      d_text_buffer_total_length(filtered) == 0,
                                      "No matches should produce total length 0") && result;

            d_text_buffer_free(filtered);
        }
        else
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "filter_chunk_no_match_null",
                                      true,
                                      "No matches may return NULL") && result;
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
d_tests_text_buffer_filter_chunked
  Tests the d_text_buffer_filter_chunked function.
  Tests the following:
  - NULL buffer returns NULL
  - NULL chain returns NULL
  - returns a new chunked buffer with matching characters
  - original buffer unchanged
  - total length of returned buffer (primary + chunks) is correct
*/
bool d_tests_text_buffer_filter_chunked(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "text_buffer_filter_chunked");

    result = true;
    result &= d_internal_tests_text_buffer_filter_chunked_part1(_suite);
    result &= d_internal_tests_text_buffer_filter_chunked_part2(_suite);

    return result;
}

/*
d_tests_text_buffer_filter_all
  Aggregation function that runs all filter tests.
*/
bool d_tests_text_buffer_filter_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_filter_all");

    result = true;

    result = d_tests_text_buffer_filter(_suite) && result;
    result = d_tests_text_buffer_filter_in_place(_suite) && result;
    result = d_tests_text_buffer_filter_indices(_suite) && result;
    result = d_tests_text_buffer_count_matching(_suite) && result;
    result = d_tests_text_buffer_filter_chunked(_suite) && result;

    return result;
}
