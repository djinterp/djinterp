/******************************************************************************
* djinterp [test]                                             dstring_tests_sa.h
*
*   Unit test declarations for the d_string safe string module. Tests are
* organized by functional category matching the sections in dstring.h.
* Each section has its own .c implementation file to keep test definitions
* manageable.
*
*
* path:      \inc\test\dstring_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
/*
TABLE OF CONTENTS
=================
  I.    CREATION & DESTRUCTION TESTS     (dstring_tests_creation.c)
  II.   CAPACITY MANAGEMENT TESTS        (dstring_tests_capacity.c)
  III.  ACCESS FUNCTION TESTS            (dstring_tests_access.c)
  IV.   SAFE COPY TESTS                  (dstring_tests_copy.c)
  V.    CONCATENATION TESTS              (dstring_tests_concat.c)
  VI.   DUPLICATION TESTS                (dstring_tests_dup.c)
  VII.  COMPARISON TESTS                 (dstring_tests_compare.c)
  VIII. SEARCH TESTS                     (dstring_tests_search.c)
  IX.   MODIFICATION TESTS               (dstring_tests_modify.c)
  X.    CASE CONVERSION TESTS            (dstring_tests_case.c)
  XI.   REVERSAL TESTS                   (dstring_tests_reverse.c)
  XII.  TRIMMING TESTS                   (dstring_tests_trim.c)
  XIII. TOKENIZATION TESTS               (dstring_tests_token.c)
  XIV.  JOIN TESTS                       (dstring_tests_join.c)
  XV.   UTILITY TESTS                    (dstring_tests_util.c)
  XVI.  ERROR STRING TESTS               (dstring_tests_error.c)
  XVII. FORMATTED STRING TESTS           (dstring_tests_format.c)
*/


#ifndef DJINTERP_DSTRING_TESTS_STANDALONE_
#define DJINTERP_DSTRING_TESTS_STANDALONE_ 1

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../../inc/c/djinterp.h"
#include "../../inc/c/dmemory.h"
#include "../../inc/c/test/test_standalone.h"
#include "../../inc/c/dstring.h"


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
struct d_test_object* d_tests_sa_dstring_new(void);
struct d_test_object* d_tests_sa_dstring_new_with_capacity(void);
struct d_test_object* d_tests_sa_dstring_new_from_cstr(void);
struct d_test_object* d_tests_sa_dstring_new_from_cstr_n(void);
struct d_test_object* d_tests_sa_dstring_new_from_buffer(void);
struct d_test_object* d_tests_sa_dstring_new_copy(void);
struct d_test_object* d_tests_sa_dstring_new_fill(void);
struct d_test_object* d_tests_sa_dstring_new_formatted(void);
struct d_test_object* d_tests_sa_dstring_free(void);
struct d_test_object* d_tests_sa_dstring_free_contents(void);
struct d_test_object* d_tests_sa_dstring_creation_all(void);

// II. CAPACITY MANAGEMENT TESTS
struct d_test_object* d_tests_sa_dstring_reserve(void);
struct d_test_object* d_tests_sa_dstring_shrink_to_fit(void);
struct d_test_object* d_tests_sa_dstring_capacity(void);
struct d_test_object* d_tests_sa_dstring_resize(void);
struct d_test_object* d_tests_sa_dstring_capacity_all(void);

// III. ACCESS FUNCTION TESTS
struct d_test_object* d_tests_sa_dstring_length(void);
struct d_test_object* d_tests_sa_dstring_size(void);
struct d_test_object* d_tests_sa_dstring_cstr(void);
struct d_test_object* d_tests_sa_dstring_data(void);
struct d_test_object* d_tests_sa_dstring_is_empty(void);
struct d_test_object* d_tests_sa_dstring_char_at(void);
struct d_test_object* d_tests_sa_dstring_set_char(void);
struct d_test_object* d_tests_sa_dstring_front(void);
struct d_test_object* d_tests_sa_dstring_back(void);
struct d_test_object* d_tests_sa_dstring_access_all(void);

// IV. SAFE COPY TESTS
struct d_test_object* d_tests_sa_dstring_copy_s(void);
struct d_test_object* d_tests_sa_dstring_copy_cstr_s(void);
struct d_test_object* d_tests_sa_dstring_ncopy_s(void);
struct d_test_object* d_tests_sa_dstring_ncopy_cstr_s(void);
struct d_test_object* d_tests_sa_dstring_to_buffer_s(void);
struct d_test_object* d_tests_sa_dstring_copy_all(void);

// V. CONCATENATION TESTS
struct d_test_object* d_tests_sa_dstring_cat_s(void);
struct d_test_object* d_tests_sa_dstring_cat_cstr_s(void);
struct d_test_object* d_tests_sa_dstring_ncat_s(void);
struct d_test_object* d_tests_sa_dstring_ncat_cstr_s(void);
struct d_test_object* d_tests_sa_dstring_concat_all(void);

// VI. DUPLICATION TESTS
struct d_test_object* d_tests_sa_dstring_dup(void);
struct d_test_object* d_tests_sa_dstring_ndup(void);
struct d_test_object* d_tests_sa_dstring_substr(void);
struct d_test_object* d_tests_sa_dstring_dup_all(void);

// VII. COMPARISON TESTS
struct d_test_object* d_tests_sa_dstring_cmp(void);
struct d_test_object* d_tests_sa_dstring_cmp_cstr(void);
struct d_test_object* d_tests_sa_dstring_ncmp(void);
struct d_test_object* d_tests_sa_dstring_ncmp_cstr(void);
struct d_test_object* d_tests_sa_dstring_casecmp(void);
struct d_test_object* d_tests_sa_dstring_casecmp_cstr(void);
struct d_test_object* d_tests_sa_dstring_ncasecmp(void);
struct d_test_object* d_tests_sa_dstring_ncasecmp_cstr(void);
struct d_test_object* d_tests_sa_dstring_equals(void);
struct d_test_object* d_tests_sa_dstring_equals_cstr(void);
struct d_test_object* d_tests_sa_dstring_equals_ignore_case(void);
struct d_test_object* d_tests_sa_dstring_equals_cstr_ignore_case(void);
struct d_test_object* d_tests_sa_dstring_compare_all(void);

// VIII. SEARCH TESTS
struct d_test_object* d_tests_sa_dstring_find_char(void);
struct d_test_object* d_tests_sa_dstring_find_char_from(void);
struct d_test_object* d_tests_sa_dstring_rfind_char(void);
struct d_test_object* d_tests_sa_dstring_chr(void);
struct d_test_object* d_tests_sa_dstring_rchr(void);
struct d_test_object* d_tests_sa_dstring_chrnul(void);
struct d_test_object* d_tests_sa_dstring_find(void);
struct d_test_object* d_tests_sa_dstring_find_cstr(void);
struct d_test_object* d_tests_sa_dstring_find_from(void);
struct d_test_object* d_tests_sa_dstring_find_cstr_from(void);
struct d_test_object* d_tests_sa_dstring_rfind(void);
struct d_test_object* d_tests_sa_dstring_rfind_cstr(void);
struct d_test_object* d_tests_sa_dstring_str(void);
struct d_test_object* d_tests_sa_dstring_casefind(void);
struct d_test_object* d_tests_sa_dstring_casefind_cstr(void);
struct d_test_object* d_tests_sa_dstring_casestr(void);
struct d_test_object* d_tests_sa_dstring_contains(void);
struct d_test_object* d_tests_sa_dstring_contains_cstr(void);
struct d_test_object* d_tests_sa_dstring_contains_char(void);
struct d_test_object* d_tests_sa_dstring_starts_with(void);
struct d_test_object* d_tests_sa_dstring_starts_with_cstr(void);
struct d_test_object* d_tests_sa_dstring_ends_with(void);
struct d_test_object* d_tests_sa_dstring_ends_with_cstr(void);
struct d_test_object* d_tests_sa_dstring_spn(void);
struct d_test_object* d_tests_sa_dstring_cspn(void);
struct d_test_object* d_tests_sa_dstring_pbrk(void);
struct d_test_object* d_tests_sa_dstring_search_all(void);

// IX. MODIFICATION TESTS
struct d_test_object* d_tests_sa_dstring_assign(void);
struct d_test_object* d_tests_sa_dstring_assign_cstr(void);
struct d_test_object* d_tests_sa_dstring_assign_buffer(void);
struct d_test_object* d_tests_sa_dstring_assign_char(void);
struct d_test_object* d_tests_sa_dstring_append(void);
struct d_test_object* d_tests_sa_dstring_append_cstr(void);
struct d_test_object* d_tests_sa_dstring_append_buffer(void);
struct d_test_object* d_tests_sa_dstring_append_char(void);
struct d_test_object* d_tests_sa_dstring_append_formatted(void);
struct d_test_object* d_tests_sa_dstring_prepend(void);
struct d_test_object* d_tests_sa_dstring_prepend_cstr(void);
struct d_test_object* d_tests_sa_dstring_prepend_char(void);
struct d_test_object* d_tests_sa_dstring_insert(void);
struct d_test_object* d_tests_sa_dstring_insert_cstr(void);
struct d_test_object* d_tests_sa_dstring_insert_char(void);
struct d_test_object* d_tests_sa_dstring_erase(void);
struct d_test_object* d_tests_sa_dstring_erase_char(void);
struct d_test_object* d_tests_sa_dstring_clear(void);
struct d_test_object* d_tests_sa_dstring_replace(void);
struct d_test_object* d_tests_sa_dstring_replace_cstr(void);
struct d_test_object* d_tests_sa_dstring_replace_all(void);
struct d_test_object* d_tests_sa_dstring_replace_all_cstr(void);
struct d_test_object* d_tests_sa_dstring_replace_char(void);
struct d_test_object* d_tests_sa_dstring_modify_all(void);

// X. CASE CONVERSION TESTS
struct d_test_object* d_tests_sa_dstring_to_lower(void);
struct d_test_object* d_tests_sa_dstring_to_upper(void);
struct d_test_object* d_tests_sa_dstring_lower(void);
struct d_test_object* d_tests_sa_dstring_upper(void);
struct d_test_object* d_tests_sa_dstring_case_all(void);

// XI. REVERSAL TESTS
struct d_test_object* d_tests_sa_dstring_reverse(void);
struct d_test_object* d_tests_sa_dstring_reversed(void);
struct d_test_object* d_tests_sa_dstring_reversal_all(void);

// XII. TRIMMING TESTS
struct d_test_object* d_tests_sa_dstring_trim(void);
struct d_test_object* d_tests_sa_dstring_trim_left(void);
struct d_test_object* d_tests_sa_dstring_trim_right(void);
struct d_test_object* d_tests_sa_dstring_trim_chars(void);
struct d_test_object* d_tests_sa_dstring_trimmed(void);
struct d_test_object* d_tests_sa_dstring_trimmed_left(void);
struct d_test_object* d_tests_sa_dstring_trimmed_right(void);
struct d_test_object* d_tests_sa_dstring_trim_all(void);

// XIII. TOKENIZATION TESTS
struct d_test_object* d_tests_sa_dstring_tokenize(void);
struct d_test_object* d_tests_sa_dstring_split(void);
struct d_test_object* d_tests_sa_dstring_split_free(void);
struct d_test_object* d_tests_sa_dstring_token_all(void);

// XIV. JOIN TESTS
struct d_test_object* d_tests_sa_dstring_join(void);
struct d_test_object* d_tests_sa_dstring_join_cstr(void);
struct d_test_object* d_tests_sa_dstring_concat(void);
struct d_test_object* d_tests_sa_dstring_join_all(void);

// XV. UTILITY TESTS
struct d_test_object* d_tests_sa_dstring_is_valid(void);
struct d_test_object* d_tests_sa_dstring_is_ascii(void);
struct d_test_object* d_tests_sa_dstring_is_numeric(void);
struct d_test_object* d_tests_sa_dstring_is_alpha(void);
struct d_test_object* d_tests_sa_dstring_is_alnum(void);
struct d_test_object* d_tests_sa_dstring_is_whitespace(void);
struct d_test_object* d_tests_sa_dstring_count_char(void);
struct d_test_object* d_tests_sa_dstring_count_substr(void);
struct d_test_object* d_tests_sa_dstring_hash(void);
struct d_test_object* d_tests_sa_dstring_util_all(void);

// XVI. ERROR STRING TESTS
struct d_test_object* d_tests_sa_dstring_error(void);
struct d_test_object* d_tests_sa_dstring_error_r(void);
struct d_test_object* d_tests_sa_dstring_error_all(void);

// XVII. FORMATTED STRING TESTS
struct d_test_object* d_tests_sa_dstring_printf(void);
struct d_test_object* d_tests_sa_dstring_vprintf(void);
struct d_test_object* d_tests_sa_dstring_sprintf(void);
struct d_test_object* d_tests_sa_dstring_format_all(void);

// master test runner
struct d_test_object* d_tests_sa_dstring_all(void);


#endif  // DJINTERP_DSTRING_TESTS_STANDALONE_