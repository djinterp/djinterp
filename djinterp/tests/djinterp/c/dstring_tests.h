/*******************************************************************************
* djinterp [test]                                                dstring_tests.h
*
* dstring.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   Two faces, chosen by DTEST_SPEC_MODE as in every C suite: the section
* translation units see the header under test and the test declarations; the
* runner, which defines DTEST_SPEC_MODE first, sees the declarations and the
* spec provider, d_tests_dstring_spec().
*
*   The first block holds the register's decisions on dstring (#13, #19, #22,
* #23 and #24 of docs/djinterp_decisions.md, Part B): each fix that the
* settlement of 2026.10.04 made, with a test that fails on the code before
* it. The standalone suite (dstring_tests_sa*.c) moves in as further blocks
* as it is converted.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
*     1.  The register's decisions
* 2.  SPEC
*     ----
*     1.  The provider
*     2.  The sections' entry points
* 3.  SECTIONS
*     --------
*
*
* path:      /tests/djinterp/c/dstring_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DSTRING_TESTS_H
#define DJINTERP_TESTS_C_DSTRING_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <errno.h>
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>
    // djinterp
    #include "../../../inc/djinterp/c/dstring.h"  // the header under test
    #include "../../../inc/djinterp/c/djinterp.h"
    #include "../../../inc/djinterp/c/memory/dmemory.h"
    #include "../../../inc/djinterp/c/string_fn.h"  // d_str*
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_dstring_tokenize_updates_size(void);
bool d_tests_dstring_split_tokens(void);
bool d_tests_dstring_split_growth(void);
bool d_tests_dstring_split_only_delimiters(void);
bool d_tests_dstring_npos_counts_clamp(void);
bool d_tests_dstring_self_operands(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dstring_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_dstring_register_rows[] =
{
    { "d_tests_dstring_tokenize_updates_size",
      "#13: after the first tokenize the size is the text's length",
      d_tests_dstring_tokenize_updates_size },
    { "d_tests_dstring_split_tokens",
      "#19: the split helpers keep split's results, empty input included",
      d_tests_dstring_split_tokens },
    { "d_tests_dstring_split_growth",
      "#19: a thousand tokens grow the array through every doubling",
      d_tests_dstring_split_growth },
    { "d_tests_dstring_split_only_delimiters",
      "#24: delimiters alone give no token and a NULL array",
      d_tests_dstring_split_only_delimiters },
    { "d_tests_dstring_npos_counts_clamp",
      "#22: a SIZE_MAX count clamps in substr, erase and both replaces",
      d_tests_dstring_npos_counts_clamp },
    { "d_tests_dstring_self_operands",
      "#23: append, insert, assign and replace take the string itself",
      d_tests_dstring_self_operands }
};

// d_tests_dstring_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dstring_blocks[] =
{
    { "register",
      "the register's decisions on dstring, each with its regression test",
      d_tests_dstring_register_rows,
      sizeof(d_tests_dstring_register_rows) /
          sizeof(d_tests_dstring_register_rows[0]) }
};

// d_tests_dstring_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dstring_module =
{
    "dstring",
    "dstring.h: the dj_string type and its operations",
    d_tests_dstring_blocks,
    sizeof(d_tests_dstring_blocks) / sizeof(d_tests_dstring_blocks[0])
};

// d_tests_dstring_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dstring_spec(void)
{
    return &d_tests_dstring_module;
}

// 2.2    The sections' entry points
//------------------------------------------------------------------------------
// The tests built call by call (section 3), which the runner calls after
// adding the spec.
bool d_tests_dstring_all(struct dj_test_suite* _suite);
bool d_tests_dstring_concat_all(struct dj_test_suite* _suite);


#endif  // DTEST_SPEC_MODE


//==============================================================================
// 3.  SECTIONS
//==============================================================================
// The module's tests built call by call (test_suite.h 3.2), outside spec
// mode only, since they read the module's own types.
#ifndef DTEST_SPEC_MODE

// Test Suite Configuration

// DSTRING_TEST_VERBOSE
//   definition: when set to 1, enables verbose output during test execution.
#ifndef DSTRING_TEST_VERBOSE
    #define DSTRING_TEST_VERBOSE 0
#endif


// Test String Constants
// D_d_tests_sa_dstring_BUFFER_SIZE
//   constant: standard buffer size for test operations.
#define D_TESTS_DSTRING_BUFFER_SIZE 256

// D_d_tests_sa_dstring_SMALL_BUFFER
//   constant: small buffer size for overflow/boundary testing.
#define D_TESTS_DSTRING_SMALL_BUFFER 10

// D_d_tests_sa_dstring_SHORT_STR
//   constant: short test string (5 characters).
#define D_TESTS_DSTRING_SHORT_STR "Hello"

// D_d_tests_sa_dstring_MEDIUM_STR
//   constant: medium test string (13 characters).
#define D_TESTS_DSTRING_MEDIUM_STR "Hello, World!"

// D_d_tests_sa_dstring_LONG_STR
//   constant: long test string for overflow/truncation tests.
#define D_TEST_DSTRING_LONG_STR \
    "This is a much longer string that exceeds typical small buffer sizes " \
    "and is useful for testing truncation and overflow conditions."

// D_d_tests_sa_dstring_EMPTY_STR
//   constant: empty test string.
#define D_TESTS_DSTRING_EMPTY_STR ""

// D_d_tests_sa_dstring_WHITESPACE_STR
//   constant: string with various whitespace characters.
#define D_TESTS_DSTRING_WHITESPACE_STR "  \t\n\r  "

// D_d_tests_sa_dstring_SPECIAL_STR
//   constant: string with special characters.
#define D_TESTS_DSTRING_SPECIAL_STR "!@#$%^&*()_+-=[]{}|;':\",./<>?"

// D_d_tests_sa_dstring_NUMERIC_STR
//   constant: string containing only digits.
#define D_TESTS_DSTRING_NUMERIC_STR "1234567890"

// D_d_tests_sa_dstring_ALPHA_STR
//   constant: string containing only letters.
#define D_TESTS_DSTRING_ALPHA_STR "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

// D_d_tests_sa_dstring_ALNUM_STR
//   constant: string containing letters and digits.
#define D_TESTS_DSTRING_ALNUM_STR "ABC123def456"


// I. CREATION & DESTRUCTION TESTS
bool d_tests_dstring_new(struct dj_test_suite* _suite);
bool d_tests_dstring_new_with_capacity(struct dj_test_suite* _suite);
bool d_tests_dstring_new_from_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_new_from_cstr_n(struct dj_test_suite* _suite);
bool d_tests_dstring_new_from_buffer(struct dj_test_suite* _suite);
bool d_tests_dstring_new_copy(struct dj_test_suite* _suite);
bool d_tests_dstring_new_fill(struct dj_test_suite* _suite);
bool d_tests_dstring_new_formatted(struct dj_test_suite* _suite);
bool d_tests_dstring_free(struct dj_test_suite* _suite);
bool d_tests_dstring_free_contents(struct dj_test_suite* _suite);
bool d_tests_dstring_creation_all(struct dj_test_suite* _suite);

// II. CAPACITY MANAGEMENT TESTS
bool d_tests_dstring_reserve(struct dj_test_suite* _suite);
bool d_tests_dstring_shrink_to_fit(struct dj_test_suite* _suite);
bool d_tests_dstring_capacity(struct dj_test_suite* _suite);
bool d_tests_dstring_resize(struct dj_test_suite* _suite);
bool d_tests_dstring_capacity_all(struct dj_test_suite* _suite);

// III. ACCESS FUNCTION TESTS
bool d_tests_dstring_length(struct dj_test_suite* _suite);
bool d_tests_dstring_size(struct dj_test_suite* _suite);
bool d_tests_dstring_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_data(struct dj_test_suite* _suite);
bool d_tests_dstring_is_empty(struct dj_test_suite* _suite);
bool d_tests_dstring_char_at(struct dj_test_suite* _suite);
bool d_tests_dstring_set_char(struct dj_test_suite* _suite);
bool d_tests_dstring_front(struct dj_test_suite* _suite);
bool d_tests_dstring_back(struct dj_test_suite* _suite);
bool d_tests_dstring_access_all(struct dj_test_suite* _suite);

// IV. SAFE COPY TESTS
bool d_tests_dstring_copy_s(struct dj_test_suite* _suite);
bool d_tests_dstring_copy_cstr_s(struct dj_test_suite* _suite);
bool d_tests_dstring_ncopy_s(struct dj_test_suite* _suite);
bool d_tests_dstring_ncopy_cstr_s(struct dj_test_suite* _suite);
bool d_tests_dstring_to_buffer_s(struct dj_test_suite* _suite);
bool d_tests_dstring_copy_all(struct dj_test_suite* _suite);

// V. CONCATENATION TESTS
bool d_tests_dstring_cat_s(struct dj_test_suite* _suite);
bool d_tests_dstring_cat_cstr_s(struct dj_test_suite* _suite);
bool d_tests_dstring_ncat_s(struct dj_test_suite* _suite);
bool d_tests_dstring_ncat_cstr_s(struct dj_test_suite* _suite);
bool d_tests_dstring_concat_all(struct dj_test_suite* _suite);

// VI. DUPLICATION TESTS
bool d_tests_dstring_dup(struct dj_test_suite* _suite);
bool d_tests_dstring_ndup(struct dj_test_suite* _suite);
bool d_tests_dstring_substr(struct dj_test_suite* _suite);
bool d_tests_dstring_dup_all(struct dj_test_suite* _suite);

// VII. COMPARISON TESTS
bool d_tests_dstring_cmp(struct dj_test_suite* _suite);
bool d_tests_dstring_cmp_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_ncmp(struct dj_test_suite* _suite);
bool d_tests_dstring_ncmp_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_casecmp(struct dj_test_suite* _suite);
bool d_tests_dstring_casecmp_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_ncasecmp(struct dj_test_suite* _suite);
bool d_tests_dstring_ncasecmp_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_equals(struct dj_test_suite* _suite);
bool d_tests_dstring_equals_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_equals_ignore_case(struct dj_test_suite* _suite);
bool d_tests_dstring_equals_cstr_ignore_case(struct dj_test_suite* _suite);
bool d_tests_dstring_compare_all(struct dj_test_suite* _suite);

// VIII. SEARCH TESTS
bool d_tests_dstring_find_char(struct dj_test_suite* _suite);
bool d_tests_dstring_find_char_from(struct dj_test_suite* _suite);
bool d_tests_dstring_rfind_char(struct dj_test_suite* _suite);
bool d_tests_dstring_chr(struct dj_test_suite* _suite);
bool d_tests_dstring_rchr(struct dj_test_suite* _suite);
bool d_tests_dstring_chrnul(struct dj_test_suite* _suite);
bool d_tests_dstring_find(struct dj_test_suite* _suite);
bool d_tests_dstring_find_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_find_from(struct dj_test_suite* _suite);
bool d_tests_dstring_find_cstr_from(struct dj_test_suite* _suite);
bool d_tests_dstring_rfind(struct dj_test_suite* _suite);
bool d_tests_dstring_rfind_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_str(struct dj_test_suite* _suite);
bool d_tests_dstring_casefind(struct dj_test_suite* _suite);
bool d_tests_dstring_casefind_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_casestr(struct dj_test_suite* _suite);
bool d_tests_dstring_contains(struct dj_test_suite* _suite);
bool d_tests_dstring_contains_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_contains_char(struct dj_test_suite* _suite);
bool d_tests_dstring_starts_with(struct dj_test_suite* _suite);
bool d_tests_dstring_starts_with_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_ends_with(struct dj_test_suite* _suite);
bool d_tests_dstring_ends_with_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_spn(struct dj_test_suite* _suite);
bool d_tests_dstring_cspn(struct dj_test_suite* _suite);
bool d_tests_dstring_pbrk(struct dj_test_suite* _suite);
bool d_tests_dstring_search_all(struct dj_test_suite* _suite);

// IX. MODIFICATION TESTS
bool d_tests_dstring_assign(struct dj_test_suite* _suite);
bool d_tests_dstring_assign_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_assign_buffer(struct dj_test_suite* _suite);
bool d_tests_dstring_assign_char(struct dj_test_suite* _suite);
bool d_tests_dstring_append(struct dj_test_suite* _suite);
bool d_tests_dstring_append_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_append_buffer(struct dj_test_suite* _suite);
bool d_tests_dstring_append_char(struct dj_test_suite* _suite);
bool d_tests_dstring_append_formatted(struct dj_test_suite* _suite);
bool d_tests_dstring_prepend(struct dj_test_suite* _suite);
bool d_tests_dstring_prepend_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_prepend_char(struct dj_test_suite* _suite);
bool d_tests_dstring_insert(struct dj_test_suite* _suite);
bool d_tests_dstring_insert_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_insert_char(struct dj_test_suite* _suite);
bool d_tests_dstring_erase(struct dj_test_suite* _suite);
bool d_tests_dstring_erase_char(struct dj_test_suite* _suite);
bool d_tests_dstring_clear(struct dj_test_suite* _suite);
bool d_tests_dstring_replace(struct dj_test_suite* _suite);
bool d_tests_dstring_replace_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_replace_all(struct dj_test_suite* _suite);
bool d_tests_dstring_replace_all_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_replace_char(struct dj_test_suite* _suite);
bool d_tests_dstring_modify_all(struct dj_test_suite* _suite);

// X. CASE CONVERSION TESTS
bool d_tests_dstring_to_lower(struct dj_test_suite* _suite);
bool d_tests_dstring_to_upper(struct dj_test_suite* _suite);
bool d_tests_dstring_lower(struct dj_test_suite* _suite);
bool d_tests_dstring_upper(struct dj_test_suite* _suite);
bool d_tests_dstring_case_all(struct dj_test_suite* _suite);

// XI. REVERSAL TESTS
bool d_tests_dstring_reverse(struct dj_test_suite* _suite);
bool d_tests_dstring_reversed(struct dj_test_suite* _suite);
bool d_tests_dstring_reversal_all(struct dj_test_suite* _suite);

// XII. TRIMMING TESTS
bool d_tests_dstring_trim(struct dj_test_suite* _suite);
bool d_tests_dstring_trim_left(struct dj_test_suite* _suite);
bool d_tests_dstring_trim_right(struct dj_test_suite* _suite);
bool d_tests_dstring_trim_chars(struct dj_test_suite* _suite);
bool d_tests_dstring_trimmed(struct dj_test_suite* _suite);
bool d_tests_dstring_trimmed_left(struct dj_test_suite* _suite);
bool d_tests_dstring_trimmed_right(struct dj_test_suite* _suite);
bool d_tests_dstring_trim_all(struct dj_test_suite* _suite);

// XIII. TOKENIZATION TESTS
bool d_tests_dstring_tokenize(struct dj_test_suite* _suite);
bool d_tests_dstring_split(struct dj_test_suite* _suite);
bool d_tests_dstring_split_free(struct dj_test_suite* _suite);
bool d_tests_dstring_token_all(struct dj_test_suite* _suite);

// XIV. JOIN TESTS
bool d_tests_dstring_join(struct dj_test_suite* _suite);
bool d_tests_dstring_join_cstr(struct dj_test_suite* _suite);
bool d_tests_dstring_concat(struct dj_test_suite* _suite);
bool d_tests_dstring_join_all(struct dj_test_suite* _suite);

// XV. UTILITY TESTS
bool d_tests_dstring_is_valid(struct dj_test_suite* _suite);
bool d_tests_dstring_is_ascii(struct dj_test_suite* _suite);
bool d_tests_dstring_is_numeric(struct dj_test_suite* _suite);
bool d_tests_dstring_is_alpha(struct dj_test_suite* _suite);
bool d_tests_dstring_is_alnum(struct dj_test_suite* _suite);
bool d_tests_dstring_is_whitespace(struct dj_test_suite* _suite);
bool d_tests_dstring_count_char(struct dj_test_suite* _suite);
bool d_tests_dstring_count_substr(struct dj_test_suite* _suite);
bool d_tests_dstring_hash(struct dj_test_suite* _suite);
bool d_tests_dstring_util_all(struct dj_test_suite* _suite);

// XVI. ERROR STRING TESTS
bool d_tests_dstring_error(struct dj_test_suite* _suite);
bool d_tests_dstring_error_r(struct dj_test_suite* _suite);
bool d_tests_dstring_error_all(struct dj_test_suite* _suite);

// XVII. FORMATTED STRING TESTS
bool d_tests_dstring_printf(struct dj_test_suite* _suite);
bool d_tests_dstring_vprintf(struct dj_test_suite* _suite);
bool d_tests_dstring_sprintf(struct dj_test_suite* _suite);
bool d_tests_dstring_format_all(struct dj_test_suite* _suite);

// master test runner
bool d_tests_dstring_all(struct dj_test_suite* _suite);

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DSTRING_TESTS_H
