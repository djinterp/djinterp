/*******************************************************************************
* djinterp [functional]                       functional_common_tests_identity.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_identity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_functional_identity_transformer
  Tests d_functional_identity_transformer, which copies one element of the
size `*_context` holds.
  Tests the following:
  - an int copied, given its size
  - a larger element copied whole, and nothing past it written
  - no size (NULL context) refused, the output untouched
  - NULL input, NULL output, or both, refused
*/
bool d_tests_functional_identity_transformer(
    struct dj_test_suite* _suite
)
{
    bool   all_passed;
    bool   ran;
    int    value;
    int    output;
    size_t size;
    double triple[3] = {1.5, -2.25, 3.0};
    double copy[4]   = {0.0, 0.0, 0.0, 9.0};
    size_t triple_size;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_identity_transformer");

    all_passed  = true;
    value       = 42;
    output      = 0;
    size        = sizeof(int);
    triple_size = sizeof(triple);

    // --- test: an int, given its size ---
    ran = d_functional_identity_transformer(&value, &output, &size);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: copies an int",
                                   ran && (output == 42),
                                   "expected true and the value copied");

    // --- test: a larger element, and nothing past it ---
    ran = d_functional_identity_transformer(triple, copy, &triple_size);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: whole element",
                                   ran                &&
                                   (copy[0] == 1.5)   &&
                                   (copy[1] == -2.25) &&
                                   (copy[2] == 3.0)   &&
                                   (copy[3] == 9.0),
                                   "expected 3 doubles, the 4th untouched");

    // --- test: no size ---
    output = 7;
    ran    = d_functional_identity_transformer(&value, &output, NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: NULL context",
                                   (!ran) && (output == 7),
                                   "expected false, the output untouched");

    // --- test: missing ends ---
    ran = d_functional_identity_transformer(NULL, &output, &size);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: NULL input",
                                   !ran,
                                   "expected false when input is NULL");
    ran = d_functional_identity_transformer(&value, NULL, &size);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: NULL output",
                                   !ran,
                                   "expected false when output is NULL");
    ran = d_functional_identity_transformer(NULL, NULL, &size);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_transformer: both NULL",
                                   !ran,
                                   "expected false when both are NULL");

    return all_passed;
}


/*
d_tests_functional_identity_predicate
  Tests d_functional_identity_predicate, which reads its element as a bool.
  Tests the following:
  - a true element is true
  - a false element is false
  - a NULL element is false
  - the context is ignored
*/
bool d_tests_functional_identity_predicate(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    bool flags[2] = {true, false};
    int  ctx;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_identity_predicate");

    all_passed = true;
    ctx        = 99;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_predicate: true element",
                                   d_functional_identity_predicate(&flags[0],
                                                                   NULL),
                                   "expected true for a true element");
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_predicate: false element",
                                   !d_functional_identity_predicate(&flags[1],
                                                                    NULL),
                                   "expected false for a false element");
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_predicate: NULL element",
                                   !d_functional_identity_predicate(NULL,
                                                                    NULL),
                                   "expected false when element is NULL");
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "identity_predicate: context ignored",
                                   (d_functional_identity_predicate(&flags[0],
                                                                    &ctx)) &&
                                   (!d_functional_identity_predicate(&flags[1],
                                                                     &ctx)) &&
                                   (!d_functional_identity_predicate(NULL,
                                                                     &ctx)),
                                   "expected the same results with one");

    return all_passed;
}
