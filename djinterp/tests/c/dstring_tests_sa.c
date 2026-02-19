#include "..\tests\dstring_tests_sa.h"


/******************************************************************************
 * d_tests_sa_dstring_all
 *****************************************************************************/

 /*
 d_tests_sa_dstring_all
   Master test runner for all d_string unit tests. Runs all test categories
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
   Test object containing all d_string test results.
 */
struct d_test_object*
    d_tests_sa_dstring_all
    (
        void
    )
{
    struct d_test_object* group;
    size_t                child_idx;

    // create master group with all implemented test categories
    group = d_test_object_new_interior("d_string Module Tests", 16);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // I. CREATION & DESTRUCTION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_creation_all();

    // II. CAPACITY MANAGEMENT TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_capacity_all();

    // III. ACCESS FUNCTION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_access_all();

    // IV. SAFE COPY TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_copy_all();

    // V. DUPLICATION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_dup_all();

    // VI. COMPARISON TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_compare_all();

    // VII. SEARCH TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_search_all();

    // VIII. MODIFICATION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_modify_all();

    // IX. CASE CONVERSION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_case_all();

    // X. REVERSAL TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_reversal_all();

    // XI. TRIMMING TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_trim_all();

    // XII. TOKENIZATION TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_token_all();

    // XIII. JOIN TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_join_all();

    // XIV. UTILITY TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_util_all();

    // XV. FORMATTED STRING TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_format_all();

    // XVI. ERROR STRING TESTS
    group->elements[child_idx++] = d_tests_sa_dstring_error_all();

    return group;
}