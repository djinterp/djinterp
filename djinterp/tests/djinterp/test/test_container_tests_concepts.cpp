/******************************************************************************
* djinterp [test]                             test_container_tests_concepts.cpp
*
*   Section IV of the test_container suite: the C++20 CONCEPT layer.  Five
* structural concepts (value_typed / iterable / sized / rootable / growable)
* and three contract concepts (test_object_container / rooted_test_container /
* buildable_test_container), each pinned positive and negative, then the
* header's central claim - every concept forwards to the trait of the same
* shape, so the two agree BY CONSTRUCTION - checked directly by asserting
* concept == trait across a spread of fixtures.  Cv/ref normalization (the
* concepts run clean_t as the traits do) closes the section.
*
*   The whole file is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS - the very macro
* test_container.hpp gates its concept definitions on - so concept availability
* here tracks the header exactly.  Below C++20 the concepts do not exist and
* every unit passes vacuously.
*
* path:      /tests/djinterp/test/test_container/test_container_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_concepts_structural_positive
//   each structural concept holds for a type carrying the member it names.
bool
tests_concepts_structural_positive()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = D_TC_CHECK(dt::value_typed_test_container<object_container_min>)   && ok;
    ok = D_TC_CHECK(dt::iterable_test_container<object_container_min>)      && ok;
    ok = D_TC_CHECK(dt::sized_test_container<object_container_min>)         && ok;
    ok = D_TC_CHECK(dt::rootable_test_container<rooted_container_min>)      && ok;
    ok = D_TC_CHECK(dt::growable_test_container<buildable_container_min>)   && ok;
#endif

    return ok;
}

// tests_concepts_structural_negative
//   each structural concept is rejected for a type lacking the member.  sized
// requires BOTH size() and empty(), so removing either defeats it.
bool
tests_concepts_structural_negative()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = D_TC_CHECK(!dt::value_typed_test_container<plain_empty>)          && ok;
    ok = D_TC_CHECK(!dt::iterable_test_container<missing_begin_end>)       && ok;
    ok = D_TC_CHECK(!dt::sized_test_container<missing_size>)               && ok;
    ok = D_TC_CHECK(!dt::sized_test_container<missing_empty>)              && ok;
    ok = D_TC_CHECK(!dt::rootable_test_container<object_container_min>)    && ok;
    ok = D_TC_CHECK(!dt::growable_test_container<rooted_container_min>)    && ok;
#endif

    return ok;
}

// tests_concepts_contract_positive
//   the contract concepts hold across the ladder they mirror.
bool
tests_concepts_contract_positive()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = D_TC_CHECK(dt::test_object_container<object_container_min>)        && ok;
    ok = D_TC_CHECK(dt::test_object_container<tc_vector_of_tests>)         && ok;
    ok = D_TC_CHECK(dt::rooted_test_container<rooted_container_min>)       && ok;
    ok = D_TC_CHECK(dt::rooted_test_container<buildable_container_min>)    && ok;
    ok = D_TC_CHECK(dt::buildable_test_container<buildable_container_min>) && ok;
#endif

    return ok;
}

// tests_concepts_contract_negative
//   the contract concepts reject non-qualifying types, including the same
// layering negatives the trait ladder rejects.
bool
tests_concepts_contract_negative()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = D_TC_CHECK(!dt::test_object_container<tc_vector_of_int>)          && ok;
    ok = D_TC_CHECK(!dt::test_object_container<plain_empty>)               && ok;
    ok = D_TC_CHECK(!dt::rooted_test_container<object_container_min>)      && ok;
    ok = D_TC_CHECK(!dt::rooted_test_container<tc_vector_of_tests>)        && ok;
    ok = D_TC_CHECK(!dt::buildable_test_container<rooted_container_min>)   && ok;  // rooted, no append_child
    ok = D_TC_CHECK(!dt::buildable_test_container<append_no_root>)         && ok;  // append_child, not rooted
#endif

    return ok;
}

// tests_concepts_mirror_traits
//   the header's one-source-of-truth guarantee: each concept forwards to the
// trait of the same shape, so concept == trait for every type.  Checked in
// both truth-values on bare fixtures (clean_t is identity there, so the
// equality is exact).
bool
tests_concepts_mirror_traits()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // structural concepts vs their probes
    ok = D_TC_CHECK(dt::value_typed_test_container<object_container_min> ==
                    dt::has_value_type<object_container_min>::value)             && ok;
    ok = D_TC_CHECK(dt::value_typed_test_container<plain_empty> ==
                    dt::has_value_type<plain_empty>::value)                      && ok;
    ok = D_TC_CHECK(dt::iterable_test_container<object_container_min> ==
                    dt::has_begin_end<object_container_min>::value)              && ok;
    ok = D_TC_CHECK(dt::iterable_test_container<missing_begin_end> ==
                    dt::has_begin_end<missing_begin_end>::value)                 && ok;
    ok = D_TC_CHECK(dt::sized_test_container<object_container_min> ==
                    (dt::has_size_accessor<object_container_min>::value &&
                     dt::has_empty_method<object_container_min>::value))         && ok;
    ok = D_TC_CHECK(dt::rootable_test_container<rooted_container_min> ==
                    dt::has_root_method<rooted_container_min>::value)            && ok;
    ok = D_TC_CHECK(dt::rootable_test_container<object_container_min> ==
                    dt::has_root_method<object_container_min>::value)            && ok;
    ok = D_TC_CHECK(dt::growable_test_container<buildable_container_min> ==
                    dt::has_append_child_method<buildable_container_min>::value) && ok;
    ok = D_TC_CHECK(dt::growable_test_container<rooted_container_min> ==
                    dt::has_append_child_method<rooted_container_min>::value)    && ok;

    // contract concepts vs their predicates
    ok = D_TC_CHECK(dt::test_object_container<object_container_min> ==
                    dt::is_test_object_container<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(dt::test_object_container<tc_vector_of_int> ==
                    dt::is_test_object_container<tc_vector_of_int>::value)       && ok;
    ok = D_TC_CHECK(dt::rooted_test_container<rooted_container_min> ==
                    dt::is_rooted_test_container<rooted_container_min>::value)   && ok;
    ok = D_TC_CHECK(dt::rooted_test_container<object_container_min> ==
                    dt::is_rooted_test_container<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(dt::buildable_test_container<buildable_container_min> ==
                    dt::is_buildable_test_container<buildable_container_min>::value) && ok;
    ok = D_TC_CHECK(dt::buildable_test_container<append_no_root> ==
                    dt::is_buildable_test_container<append_no_root>::value)      && ok;
#endif

    return ok;
}

// tests_concepts_cv_ref_normalization
//   the concepts run clean_t just as the traits do, so a const / reference /
// cv-qualified container agrees with its bare form.
bool
tests_concepts_cv_ref_normalization()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = D_TC_CHECK(dt::test_object_container<const object_container_min&> ==
                    dt::test_object_container<object_container_min>)             && ok;
    ok = D_TC_CHECK(dt::value_typed_test_container<const object_container_min&>) && ok;
    ok = D_TC_CHECK(dt::buildable_test_container<const buildable_container_min&>)&& ok;
    ok = D_TC_CHECK(!dt::rooted_test_container<const object_container_min&>)     && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
