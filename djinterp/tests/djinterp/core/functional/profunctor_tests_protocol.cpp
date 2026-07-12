// djinterp [test]  profunctor_tests_protocol.cpp
//   Section II -- profunctor_traits, is_profunctor, the internal helpers, the
//   Profunctor concept.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "profunctor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_profunctor_positive
  Both the canonical arrow and the second fixture instance are recognised.
  Tests the following:
  - is_profunctor is true for profn<F> and pf_arrow<F>
  - is_profunctor_v agrees (where variable templates exist)
*/
bool
tests_is_profunctor_positive()
{
    bool ok = true;

    ok = ok && (is_profunctor<profn<doubler> >::value);
    ok = ok && (is_profunctor<pf_arrow<doubler> >::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_profunctor_v<profn<doubler> > ==
                is_profunctor<profn<doubler> >::value);
    ok = ok && (is_profunctor_v<pf_arrow<doubler> > == true);
#endif

    return ok;
}


/*
tests_is_profunctor_negative
  Non-profunctors are rejected -- including a bare callable, which is precisely
  why the arrow wrapper exists.
  Tests the following:
  - an unrelated struct, scalars, and a raw lambda are not profunctors
*/
bool
tests_is_profunctor_negative()
{
    bool ok = true;

    ok = ok && (!is_profunctor<not_profunctor>::value);
    ok = ok && (!is_profunctor<int>::value);
    ok = ok && (!is_profunctor<double>::value);

    auto bare = [](int _x){ return _x; };
    ok = ok && (!is_profunctor<decltype(bare)>::value);

    return ok;
}


/*
tests_is_profunctor_cvref
  Detection strips cv-qualifiers and references before testing.
  Tests the following:
  - const / reference / rvalue / volatile forms of an arrow are recognised
*/
bool
tests_is_profunctor_cvref()
{
    bool ok = true;

    ok = ok && (is_profunctor<const profn<doubler> >::value);
    ok = ok && (is_profunctor<profn<doubler>& >::value);
    ok = ok && (is_profunctor<const profn<doubler>& >::value);
    ok = ok && (is_profunctor<profn<doubler>&& >::value);
    ok = ok && (is_profunctor<volatile profn<doubler> >::value);

    return ok;
}


/*
tests_is_profunctor_requires_marker
  Detection keys on the is_specialized marker, not on the mere presence of a
  profunctor_traits specialisation.
  Tests the following:
  - a specialisation without the marker is NOT detected
  - a properly marked instance still is (contrast)
*/
bool
tests_is_profunctor_requires_marker()
{
    bool ok = true;

    ok = ok && (!is_profunctor<pf_no_marker>::value);
    ok = ok && (is_profunctor<profn<doubler> >::value);

    return ok;
}


/*
tests_profunctor_traits_members
  A specialisation exposes exactly the protocol surface.
  Tests the following:
  - is_specialized is std::true_type
  - dimap is present and returns a profn (a fresh arrow, not an unwrapped value)
*/
bool
tests_profunctor_traits_members()
{
    static_assert(
        std::is_same<profunctor_traits<profn<doubler> >::is_specialized,
                     std::true_type>::value, "profn marker");

    using dimap_ret = decltype(
        profunctor_traits<profn<doubler> >::dimap(
            std::declval<profn<doubler> >(), add_one{}, add_ten{}));
    static_assert(is_profn_type<dimap_ret>::value,
                  "dimap yields a profn");

    return true;
}


/*
tests_profunctor_concept
  The Profunctor concept mirrors is_profunctor (C++20 only).
  Tests the following:
  - it holds for both instances and fails for non-profunctors
*/
bool
tests_profunctor_concept()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Profunctor<profn<doubler> >, "profn is a Profunctor");
    static_assert(Profunctor<pf_arrow<doubler> >, "pf_arrow is a Profunctor");
    static_assert(!Profunctor<not_profunctor>, "not_profunctor is not");
    static_assert(!Profunctor<int>, "int is not");

    ok = ok && (Profunctor<profn<doubler> >);
    ok = ok && (!Profunctor<not_profunctor>);
#endif

    return ok;
}


/*
tests_internal_helpers
  The two named internal functors behave as specified, on several floors and in
  a constant expression.
  Tests the following:
  - profunctor_identity_helper returns its argument unchanged, for several types
  - profunctor_dimap_helper computes post . fn . pre, in that order
  - both fold at compile time
*/
bool
tests_internal_helpers()
{
    bool ok = true;

    // identity, across types.
    internal::profunctor_identity_helper id;
    ok = ok && (id(42) == 42);
    ok = ok && (id(std::string("hi")) == std::string("hi"));
    ok = ok && (id(true) == true);
    static_assert(internal::profunctor_identity_helper{}(7) == 7,
                  "identity constexpr");

    // composition post . fn . pre, order-sensitive.
    internal::profunctor_dimap_helper<add_one, doubler, add_ten>
        dh{ add_one{}, doubler{}, add_ten{} };
    ok = ok && (dh(5) == 22);    // ((5 + 1) * 2) + 10
    ok = ok && (dh(0) == 12);    // ((0 + 1) * 2) + 10
    static_assert(
        internal::profunctor_dimap_helper<add_one, doubler, add_ten>{
            add_one{}, doubler{}, add_ten{} }(5) == 22,
        "dimap helper constexpr");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
