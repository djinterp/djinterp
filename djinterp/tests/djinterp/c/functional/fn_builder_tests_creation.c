/*******************************************************************************
* djinterp [functional]                              fn_builder_tests_creation.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./fn_builder_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_fn_builder_new
  Tests d_fn_builder_new.
  Tests the following:
  - returns a non-NULL pointer on success
  - transform_count is 0 on creation
  - predicate_count is 0 on creation
  - transforms array is non-NULL (pre-allocated)
  - predicates array is non-NULL (pre-allocated)
  - capacity is positive (initial capacity pre-allocated)
  - two separate builders are independent allocations
*/
bool d_tests_fn_builder_new(
    struct dj_test_suite* _suite
)
{
    struct dj_fn_builder* builder;
    struct dj_fn_builder* builder2;
    bool                 all_passed;

    d_test_suite_test(_suite, "fn_builder_new");

    all_passed = true;

    // ---- successful creation ----
    builder = d_fn_builder_new();

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: returns non-NULL pointer",
                                   builder != NULL,
                                   "allocation should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: transform_count is 0",
                                   builder->transform_count == 0,
                                   "no transforms registered on creation");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: predicate_count is 0",
                                   builder->predicate_count == 0,
                                   "no predicates registered on creation");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: transforms array is pre-allocated",
                                   builder->transforms != NULL,
                                   "transforms should be non-NULL after creation");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: predicates array is pre-allocated",
                                   builder->predicates != NULL,
                                   "predicates should be non-NULL after creation");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: capacity is positive",
                                   builder->capacity > 0,
                                   "initial capacity should be pre-allocated");

    // ---- two builders are independent ----
    builder2 = d_fn_builder_new();

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: two builders are independent allocations",
                                   builder2 != NULL && builder2 != builder,
                                   "each call should return a distinct pointer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "new: transform arrays are independent",
                                   builder2->transforms != builder->transforms,
                                   "each builder should own its own array");

    d_fn_builder_free(builder);
    d_fn_builder_free(builder2);

    return all_passed;
}


/*
d_tests_fn_builder_creation_all
  Runs all builder creation tests.
  Tests the following:
  - d_fn_builder_new
*/
bool d_tests_fn_builder_creation_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "fn_builder_creation_all");

    all_passed = true;

    all_passed &= d_tests_fn_builder_new(_suite);

    return all_passed;
}
