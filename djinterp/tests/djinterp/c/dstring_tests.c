/*******************************************************************************
* djinterp [c]                                                   dstring_tests.c
*
*
* path:      /tests/djinterp/c/dstring_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dstring_tests.h"


/******************************************************************************
 * d_tests_dstring_all
 *****************************************************************************/

 /*
 d_tests_dstring_all
   Master test runner for all dj_string unit tests. Runs all test categories
   and returns an aggregate test object containing all results.

 Test categories run:
   - Creation & Destruction
   - Capacity Management
   - Access Functions
   - Safe Copy
   - Duplication
   - Comparison
   - Search
   - Modification
   - Case Conversion
   - Reversal
   - Trimming
   - Tokenization
   - Join
   - Utility
   - Formatted Strings
   - Error Functions

 Parameter(s):
   (none)

 Return:
   Test object containing all dj_string test results.
 */
bool d_tests_dstring_all(struct dj_test_suite* _suite)
{
    // I. CREATION & DESTRUCTION TESTS
    d_tests_dstring_creation_all(_suite);

    // II. CAPACITY MANAGEMENT TESTS
    d_tests_dstring_capacity_all(_suite);

    // III. ACCESS FUNCTION TESTS
    d_tests_dstring_access_all(_suite);

    // IV. SAFE COPY TESTS
    d_tests_dstring_copy_all(_suite);

    // V. DUPLICATION TESTS
    d_tests_dstring_dup_all(_suite);

    // VI. COMPARISON TESTS
    d_tests_dstring_compare_all(_suite);

    // VII. SEARCH TESTS
    d_tests_dstring_search_all(_suite);

    // VIII. MODIFICATION TESTS
    d_tests_dstring_modify_all(_suite);

    // IX. CASE CONVERSION TESTS
    d_tests_dstring_case_all(_suite);

    // X. REVERSAL TESTS
    d_tests_dstring_reversal_all(_suite);

    // XI. TRIMMING TESTS
    d_tests_dstring_trim_all(_suite);

    // XII. TOKENIZATION TESTS
    d_tests_dstring_token_all(_suite);

    // XIII. JOIN TESTS
    d_tests_dstring_join_all(_suite);

    // XIV. UTILITY TESTS
    d_tests_dstring_util_all(_suite);

    // XV. FORMATTED STRING TESTS
    d_tests_dstring_format_all(_suite);

    // XVI. ERROR STRING TESTS
    d_tests_dstring_error_all(_suite);

    return true;
}
