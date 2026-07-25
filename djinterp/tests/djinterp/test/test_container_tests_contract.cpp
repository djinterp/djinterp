/******************************************************************************
* djinterp [test]                             test_container_tests_contract.cpp
*
*   Section III of the test_container suite: the three-rung CONTRACT LADDER.
*   - is_test_object_container   the read/run minimum (value_type whose
*                                element is evaluable, begin/end, size/empty).
*   - is_rooted_test_container   the minimum PLUS root().
*   - is_buildable_test_container the rooted rung PLUS append_child().
*
*   Each rung is pinned with a positive and the specific negatives that
* isolate it: for the minimum, four fixtures each withholding one required
* member, and a fifth whose element is not evaluable (the element gate); for
* the rooted rung, a container that has the object floor but no root(), and
* one that has root() but not the floor; for the buildable rung, a container
* that is rooted but has no append_child(), and one that has append_child()
* but no root() (buildable is layered on the rooted floor, so it must fail).
*
*   Two cross-cutting laws close the section: LADDER MONOTONICITY (buildable
* implies rooted implies object container, over a spread of fixtures) and
* CV/REF NORMALIZATION (the contract predicates run clean_t first, so a
* const / reference / cv-qualified container agrees with its bare form) - the
* latter contrasted against the raw expression probe, which queries the type
* directly and so does NOT normalize.
*
* path:      /tests/djinterp/test/test_container/test_container_tests_contract.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_contract_object_positive
//   is_test_object_container accepts the minimal object container and every
// richer container built atop it, plus a real std::vector of evaluables.
bool
tests_contract_object_positive()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::is_test_object_container<object_container_min>::value)    && ok;
    ok = D_TC_CHECK(dt::is_test_object_container<rooted_container_min>::value)    && ok;
    ok = D_TC_CHECK(dt::is_test_object_container<buildable_container_min>::value) && ok;
    ok = D_TC_CHECK(dt::is_test_object_container<tc_vector_of_tests>::value)      && ok;

    return ok;
}

// tests_contract_object_missing_members
//   withholding ANY one of the four required structural members defeats the
// minimum - each fixture removes exactly one.
bool
tests_contract_object_missing_members()
{
    bool ok = true;

    ok = D_TC_CHECK(!dt::is_test_object_container<missing_value_type>::value) && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<missing_begin_end>::value)  && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<missing_size>::value)       && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<missing_empty>::value)      && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<plain_empty>::value)        && ok;

    return ok;
}

// tests_contract_object_element_gate
//   a container with every structural member present still fails the minimum
// when its element does not satisfy the test object protocol.
bool
tests_contract_object_element_gate()
{
    bool ok = true;

    ok = D_TC_CHECK(!dt::is_test_object_container<non_evaluable_element>::value) && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<tc_vector_of_int>::value)      && ok;

    return ok;
}

// tests_contract_rooted_positive_negative
//   is_rooted_test_container is the object minimum PLUS root().  It needs
// BOTH: a container with root() but no object floor fails, and a container
// with the floor but no root() fails.
bool
tests_contract_rooted_positive_negative()
{
    bool ok = true;

    // floor + root() -> true
    ok = D_TC_CHECK(dt::is_rooted_test_container<rooted_container_min>::value)     && ok;
    ok = D_TC_CHECK(dt::is_rooted_test_container<buildable_container_min>::value)  && ok;

    // floor but no root() -> false
    ok = D_TC_CHECK(!dt::is_rooted_test_container<object_container_min>::value)    && ok;
    ok = D_TC_CHECK(!dt::is_rooted_test_container<tc_vector_of_tests>::value)      && ok;

    // root() but no floor -> false
    ok = D_TC_CHECK(!dt::is_rooted_test_container<root_only>::value)              && ok;

    return ok;
}

// tests_contract_buildable_positive_negative
//   is_buildable_test_container is the ROOTED rung PLUS append_child().  A
// rooted container without append_child() fails; a container with
// append_child() but no root() (hence not rooted) also fails.
bool
tests_contract_buildable_positive_negative()
{
    bool ok = true;

    // rooted + append_child() -> true
    ok = D_TC_CHECK(dt::is_buildable_test_container<buildable_container_min>::value) && ok;

    // rooted but no append_child() -> false
    ok = D_TC_CHECK(!dt::is_buildable_test_container<rooted_container_min>::value)   && ok;

    // append_child() but not rooted (no root()) -> false
    ok = D_TC_CHECK(!dt::is_buildable_test_container<append_no_root>::value)         && ok;

    // neither -> false
    ok = D_TC_CHECK(!dt::is_buildable_test_container<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container<tc_vector_of_tests>::value)     && ok;

    return ok;
}

// tests_contract_ladder_monotonicity
//   the ladder is a chain of containments: buildable => rooted => object
// container.  Over a spread of fixtures spanning every combination reached by
// this suite, no fixture may satisfy an outer rung without the inner one it
// builds on.
bool
tests_contract_ladder_monotonicity()
{
    bool ok = true;

    // buildable => rooted
    ok = D_TC_CHECK(!dt::is_buildable_test_container<buildable_container_min>::value ||
                     dt::is_rooted_test_container<buildable_container_min>::value)   && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container<rooted_container_min>::value ||
                     dt::is_rooted_test_container<rooted_container_min>::value)      && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container<append_no_root>::value ||
                     dt::is_rooted_test_container<append_no_root>::value)           && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container<plain_empty>::value ||
                     dt::is_rooted_test_container<plain_empty>::value)              && ok;

    // rooted => object container
    ok = D_TC_CHECK(!dt::is_rooted_test_container<buildable_container_min>::value ||
                     dt::is_test_object_container<buildable_container_min>::value)   && ok;
    ok = D_TC_CHECK(!dt::is_rooted_test_container<rooted_container_min>::value ||
                     dt::is_test_object_container<rooted_container_min>::value)      && ok;
    ok = D_TC_CHECK(!dt::is_rooted_test_container<root_only>::value ||
                     dt::is_test_object_container<root_only>::value)               && ok;
    ok = D_TC_CHECK(!dt::is_rooted_test_container<tc_vector_of_tests>::value ||
                     dt::is_test_object_container<tc_vector_of_tests>::value)       && ok;

    return ok;
}

// tests_contract_cv_ref_normalization
//   the contract predicates strip cv/ref before inspecting (they run clean_t),
// so a const / reference / cv-qualified container agrees with its bare form -
// both when the verdict is true and when it is false.
bool
tests_contract_cv_ref_normalization()
{
    bool ok = true;

    // object minimum
    ok = D_TC_CHECK(dt::is_test_object_container<const object_container_min&>::value ==
                    dt::is_test_object_container<object_container_min>::value)       && ok;
    ok = D_TC_CHECK(dt::is_test_object_container<object_container_min&&>::value)     && ok;
    ok = D_TC_CHECK(dt::is_test_object_container<const volatile object_container_min>::value) && ok;

    // rooted rung
    ok = D_TC_CHECK(dt::is_rooted_test_container<rooted_container_min&&>::value)     && ok;
    ok = D_TC_CHECK(dt::is_rooted_test_container<const rooted_container_min&>::value)&& ok;

    // buildable rung, including a stable negative under cv/ref
    ok = D_TC_CHECK(dt::is_buildable_test_container<const buildable_container_min&>::value) && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container<const rooted_container_min&>::value)   && ok;

    return ok;
}

// tests_contract_raw_probe_vs_contract_asymmetry
//   the documented design: the raw expression probe queries the type DIRECTLY
// (so a const lvalue defeats non-const-only begin()/end()), while the contract
// predicate that consumes it runs clean_t first and therefore normalizes.  On
// the very same const-qualified object_container_min the two disagree.
bool
tests_contract_raw_probe_vs_contract_asymmetry()
{
    bool ok = true;

    // raw probe: not found on a const lvalue
    ok = D_TC_CHECK(!dt::has_begin_end<const object_container_min&>::value)          && ok;
    // contract: clean_t rescues it, so the minimum still holds
    ok = D_TC_CHECK(dt::is_test_object_container<const object_container_min&>::value) && ok;
    // hence the two genuinely differ on this input
    ok = D_TC_CHECK(dt::has_begin_end<const object_container_min&>::value !=
                    dt::is_test_object_container<const object_container_min&>::value) && ok;

    return ok;
}

// tests_contract_v_companions
//   each contract predicate's `_v` companion (C++14+) agrees with its
// ::value.  Below C++14 the unit passes vacuously.
bool
tests_contract_v_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = D_TC_CHECK(dt::is_test_object_container_v<object_container_min> ==
                    dt::is_test_object_container<object_container_min>::value)       && ok;
    ok = D_TC_CHECK(dt::is_rooted_test_container_v<rooted_container_min> ==
                    dt::is_rooted_test_container<rooted_container_min>::value)       && ok;
    ok = D_TC_CHECK(dt::is_buildable_test_container_v<buildable_container_min> ==
                    dt::is_buildable_test_container<buildable_container_min>::value) && ok;

    // negatives through the companions, to prove they are not stuck true
    ok = D_TC_CHECK(!dt::is_rooted_test_container_v<object_container_min>)           && ok;
    ok = D_TC_CHECK(!dt::is_buildable_test_container_v<rooted_container_min>)        && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
