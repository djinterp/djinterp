/*******************************************************************************
* djinterp [cpp]                                    semigroup_tests_protocol.cpp
*
*
* path:      /tests/djinterp/cpp/functional/semigroup_tests_protocol.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  semigroup_tests_protocol.cpp
//   Section I -- semigroup_traits, is_semigroup, the Semigroup concept.

// std
#include <type_traits>
// djinterp
#include "semigroup_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_semigroup_positive
  Every specialised fixture is recognised as a semigroup.
  Tests the following:
  - is_semigroup is true for each concrete instance
  - is_semigroup_v agrees with the trait (where variable templates exist)
*/
bool tests_is_semigroup_positive()
{
    bool ok = true;

    ok = ok && (is_semigroup<DSgString>::value);
    ok = ok && (is_semigroup<DSgSum>::value);
    ok = ok && (is_semigroup<DSgMax>::value);
    ok = ok && (is_semigroup<DSgFirst>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_semigroup_v<DSgSum> == is_semigroup<DSgSum>::value);
    ok = ok && (is_semigroup_v<DSgString> == true);
#endif

    return ok;
}


/*
tests_is_semigroup_negative
  Non-semigroups are rejected, and the undefined primary resolves cleanly (no
  hard error) on the detection path.
  Tests the following:
  - an unrelated struct is not a semigroup
  - scalars are not semigroups (their instances live in monoid.hpp, not here)
*/
bool tests_is_semigroup_negative()
{
    bool ok = true;

    ok = ok && (!is_semigroup<DNotSemigroup>::value);
    ok = ok && (!is_semigroup<int>::value);
    ok = ok && (!is_semigroup<double>::value);

    return ok;
}


/*
tests_is_semigroup_cvref
  Detection strips cv-qualifiers and references before testing.
  Tests the following:
  - const / reference / rvalue / volatile forms of a semigroup are recognised
*/
bool tests_is_semigroup_cvref()
{
    bool ok = true;

    ok = ok && (is_semigroup<const DSgSum>::value);
    ok = ok && (is_semigroup<DSgSum&>::value);
    ok = ok && (is_semigroup<const DSgSum&>::value);
    ok = ok && (is_semigroup<DSgSum&&>::value);
    ok = ok && (is_semigroup<volatile DSgSum>::value);

    return ok;
}


/*
tests_is_semigroup_family_sfinae
  A family instance -- one specialisation keyed on the trait's _Enable SFINAE
  hook -- makes every tagged type a semigroup at once, without over-matching.
  Tests the following:
  - both members of the modular family are recognised
  - the family trait itself tags only its members
  - an untagged, unspecialised type is still not a semigroup
*/
bool tests_is_semigroup_family_sfinae()
{
    bool ok = true;

    ok = ok && (is_semigroup<z3>::value);
    ok = ok && (is_semigroup<DZ5>::value);

    ok = ok && (is_modular<z3>::value);
    ok = ok && (is_modular<DZ5>::value);
    ok = ok && (!is_modular<DSgSum>::value);

    // the family does not accidentally match everything.
    ok = ok && (!is_semigroup<DNotSemigroup>::value);

    return ok;
}


/*
tests_is_semigroup_requires_marker
  Detection keys on the is_specialized marker, not on the mere presence of a
  combine.
  Tests the following:
  - a semigroup_traits with combine but no marker is NOT detected
  - a properly marked instance still is (contrast)
*/
bool tests_is_semigroup_requires_marker()
{
    bool ok = true;

    ok = ok && (!is_semigroup<DSgNoMarker>::value);
    ok = ok && (is_semigroup<DSgSum>::value);

    return ok;
}


/*
tests_semigroup_traits_members
  A specialisation exposes exactly the protocol surface.
  Tests the following:
  - is_specialized is std::true_type
  - combine has signature T x T -> T, for a concrete and a family instance
*/
bool tests_semigroup_traits_members()
{
    // concrete instance.
    static_assert(
        std::is_same<semigroup_traits<DSgSum>::is_specialized,
                     std::true_type>::value, "sg_sum marker");
    static_assert(
        std::is_same<
            decltype(semigroup_traits<DSgSum>::combine(
                std::declval<DSgSum>(), std::declval<DSgSum>())),
            DSgSum>::value, "sg_sum combine type");

    // family instance (resolved through the _Enable hook).
    static_assert(
        std::is_same<semigroup_traits<z3>::is_specialized,
                     std::true_type>::value, "z3 marker");
    static_assert(
        std::is_same<
            decltype(semigroup_traits<z3>::combine(
                std::declval<z3>(), std::declval<z3>())),
            z3>::value, "z3 combine type");

    return true;
}


/*
tests_semigroup_concept
  The Semigroup concept mirrors is_semigroup (C++20 only).
  Tests the following:
  - it is satisfied by concrete and family instances
  - it is not satisfied by non-semigroups
*/
bool tests_semigroup_concept()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Semigroup<DSgSum>, "sg_sum is a Semigroup");
    static_assert(Semigroup<DSgString>, "sg_string is a Semigroup");
    static_assert(Semigroup<z3>, "z3 is a Semigroup");
    static_assert(!Semigroup<DNotSemigroup>, "not_semigroup is not");
    static_assert(!Semigroup<int>, "int is not");

    ok = ok && (Semigroup<DSgSum>);
    ok = ok && (!Semigroup<DNotSemigroup>);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
