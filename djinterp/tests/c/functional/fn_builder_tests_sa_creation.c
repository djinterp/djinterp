#include ".\fn_builder_tests_sa.h"


/*
d_tests_sa_fn_builder_new
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
bool
d_tests_sa_fn_builder_new
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    struct d_fn_builder* builder2;
    bool                 all_passed;

    all_passed = true;

    // ---- successful creation ----
    builder = d_fn_builder_new();

    all_passed &= d_assert_standalone(
        builder != NULL,
        "new: returns non-NULL pointer",
        "allocation should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transform_count == 0,
        "new: transform_count is 0",
        "no transforms registered on creation",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->predicate_count == 0,
        "new: predicate_count is 0",
        "no predicates registered on creation",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->transforms != NULL,
        "new: transforms array is pre-allocated",
        "transforms should be non-NULL after creation",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->predicates != NULL,
        "new: predicates array is pre-allocated",
        "predicates should be non-NULL after creation",
        _test_info);

    all_passed &= d_assert_standalone(
        builder->capacity > 0,
        "new: capacity is positive",
        "initial capacity should be pre-allocated",
        _test_info);

    // ---- two builders are independent ----
    builder2 = d_fn_builder_new();

    all_passed &= d_assert_standalone(
        builder2 != NULL && builder2 != builder,
        "new: two builders are independent allocations",
        "each call should return a distinct pointer",
        _test_info);

    all_passed &= d_assert_standalone(
        builder2->transforms != builder->transforms,
        "new: transform arrays are independent",
        "each builder should own its own array",
        _test_info);

    d_fn_builder_free(builder);
    d_fn_builder_free(builder2);

    return all_passed;
}


/*
d_tests_sa_fn_builder_creation_all
  Runs all builder creation tests.
  Tests the following:
  - d_fn_builder_new
*/
bool
d_tests_sa_fn_builder_creation_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_fn_builder_new(_test_info);

    return all_passed;
}
