// djinterp [test]  functor_tests_protocol.cpp
//   Section I -- functor_traits, is_functor (+ _v), and the deliberate absence
//   of rebind from the core protocol.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "functor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_functor_positive
  Both roads into the protocol are recognised: a type that becomes a functor
  through the blanket monad bridge, and one carrying its own explicit
  specialization.
  Tests the following:
  - two monads (bridged) and a view (explicit) are all functors
*/
bool
tests_is_functor_positive()
{
    bool ok = true;

    // bridged: these have monad_traits and nothing else.
    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (is_functor<tbox<int> >::value);
    ok = ok && (is_functor<tmaybe<std::string> >::value);

    // explicit: a non-monad with its own functor_traits.
    ok = ok && (is_functor<lazy_view<int, dbl> >::value);

    return ok;
}


/*
tests_is_functor_negative
  Everything else is refused -- including a bare lambda, which is a mapping
  FUNCTION, not a context to map over.
  Tests the following:
  - an unrelated struct, scalars, and a raw callable are not functors
*/
bool
tests_is_functor_negative()
{
    bool ok = true;

    ok = ok && (!is_functor<not_a_functor>::value);
    ok = ok && (!is_functor<int>::value);
    ok = ok && (!is_functor<std::string>::value);

    // a mapping function is not a functor.
    ok = ok && (!is_functor<dbl>::value);
    auto lam = [](int _x){ return _x; };
    ok = ok && (!is_functor<decltype(lam)>::value);

    return ok;
}


/*
tests_is_functor_decay
  Detection applies std::decay, so cv-qualifiers and references are stripped
  first.
  Tests the following:
  - const / reference / rvalue / volatile spellings all resolve to the functor
  - a negative stays negative through the same decay
*/
bool
tests_is_functor_decay()
{
    bool ok = true;

    ok = ok && (is_functor<const tmaybe<int> >::value);
    ok = ok && (is_functor<tmaybe<int>& >::value);
    ok = ok && (is_functor<const tmaybe<int>& >::value);
    ok = ok && (is_functor<tmaybe<int>&& >::value);
    ok = ok && (is_functor<volatile tmaybe<int> >::value);

    ok = ok && (is_functor<const lazy_view<int, dbl>& >::value);

    ok = ok && (!is_functor<const not_a_functor&>::value);

    return ok;
}


/*
tests_is_functor_requires_marker
  Detection keys on the is_specialized marker, not on the mere presence of a
  functor_traits specialization or of a map.
  Tests the following:
  - a specialization carrying a map but NO marker is not detected
  - properly marked instances still are (the contrast)
*/
bool
tests_is_functor_requires_marker()
{
    bool ok = true;

    ok = ok && (!is_functor<no_marker>::value);

    // it really does have a functor_traits, and a map -- only the marker is
    // missing.
    ok = ok && (is_complete<functor_traits<no_marker> >::value);
    ok = ok && (functor_traits<no_marker>::map(no_marker(), dbl()) == 0);

    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (is_functor<lazy_view<int, dbl> >::value);

    return ok;
}


/*
tests_functor_traits_primary_is_undefined
  The primary template is declared but left UNDEFINED, so a use on a non-functor
  is a clean resolution error rather than a silently wrong answer. That makes it
  an INCOMPLETE type, which a completeness probe can read SFINAE-safely.
  Tests the following:
  - functor_traits<non-functor> is incomplete, for a struct and for a scalar
  - functor_traits<functor> is complete, by both roads into the protocol
*/
bool
tests_functor_traits_primary_is_undefined()
{
    bool ok = true;

    // the probe is sound.
    static_assert(is_complete<int>::value, "int is complete");

    // undefined primary -> incomplete for a non-functor.
    static_assert(!is_complete<functor_traits<not_a_functor> >::value,
                  "no specialization -> incomplete");
    static_assert(!is_complete<functor_traits<int> >::value,
                  "a scalar -> incomplete");

    // populated for both kinds of functor.
    static_assert(is_complete<functor_traits<tmaybe<int> > >::value,
                  "bridged");
    static_assert(is_complete<functor_traits<lazy_view<int, dbl> > >::value,
                  "explicit");

    ok = ok && (!is_complete<functor_traits<not_a_functor> >::value);

    return ok;
}


/*
tests_functor_traits_surface
  A specialization exposes exactly the protocol surface: the marker, the inner
  value type, and map.
  Tests the following:
  - is_specialized is std::true_type, on both roads
  - value_type names the inner type
  - map is callable and yields the mapped context
*/
bool
tests_functor_traits_surface()
{
    bool ok = true;

    // bridged.
    static_assert(std::is_same<functor_traits<tmaybe<int> >::is_specialized,
                               std::true_type>::value, "bridged marker");
    static_assert(std::is_same<functor_traits<tmaybe<int> >::value_type,
                               int>::value, "bridged value_type");

    // explicit.
    static_assert(std::is_same<
        functor_traits<lazy_view<int, dbl> >::is_specialized,
        std::true_type>::value, "explicit marker");
    static_assert(std::is_same<
        functor_traits<lazy_view<int, dbl> >::value_type,
        int>::value, "explicit value_type (dbl : int -> int)");

    // map, on both.
    ok = ok && (functor_traits<tmaybe<int> >::map(just(21), dbl()).value == 42);
    ok = ok && (functor_traits<lazy_view<int, dbl> >::map(
                    make_view(5, dbl()), add1()).get() == 11);

    return ok;
}


/*
tests_is_functor_v_agrees
  The _v shorthand is exactly the trait's value.
  Tests the following:
  - the two agree, positively and negatively and through decay
*/
bool
tests_is_functor_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_functor_v<tmaybe<int> > == is_functor<tmaybe<int> >::value);
    ok = ok && (is_functor_v<not_a_functor> ==
                is_functor<not_a_functor>::value);

    static_assert(is_functor_v<tbox<int> >, "bridged");
    static_assert(is_functor_v<lazy_view<int, dbl> >, "explicit");
    static_assert(!is_functor_v<no_marker>, "no marker");
    static_assert(is_functor_v<const tmaybe<int>&>, "decayed");
#endif

    return ok;
}


/*
tests_rebind_is_not_core_protocol
  The design point, made checkable. rebind<U> (= F<U>) is supplied by contexts
  for which it is well-defined, but it is deliberately NOT part of the protocol,
  because a context whose mapped type depends on the mapping function has no
  single F<U> to name. So: the bridged functors expose rebind, the view does NOT,
  and yet BOTH are functors and BOTH map.
  Tests the following:
  - the monad-bridged traits supply rebind; the view's traits do not
  - both are nevertheless functors, and both are mappable
  - the view's mapped type genuinely depends on the function it was given
*/
bool
tests_rebind_is_not_core_protocol()
{
    bool ok = true;

    // the bridge names it (it has monad_rebind to hand).
    static_assert(has_rebind<functor_traits<tmaybe<int> > >::value,
                  "bridged functor has rebind");
    static_assert(has_rebind<functor_traits<tbox<int> > >::value,
                  "and so does the other one");

    // the view cannot -- and does not.
    static_assert(!has_rebind<functor_traits<lazy_view<int, dbl> > >::value,
                  "a view names no rebind");

    // yet both are functors...
    static_assert(is_functor<tmaybe<int> >::value, "still a functor");
    static_assert(is_functor<lazy_view<int, dbl> >::value, "so is the view");

    // ...and both map.
    static_assert(is_fmappable<tmaybe<int>, dbl>::value, "mappable");
    static_assert(is_fmappable<lazy_view<int, dbl>, add1>::value,
                  "the view is mappable without a rebind");

    // and the view's mapped type follows from the FUNCTION, not from a rebind.
    using mapped_1 = decltype(functor_map(make_view(5, dbl()), add1()));
    using mapped_2 = decltype(functor_map(make_view(5, dbl()), is_even()));
    static_assert(!std::is_same<mapped_1, mapped_2>::value,
                  "a different function gives a different mapped type");

    ok = ok && (functor_map(make_view(5, dbl()), add1()).get() == 11);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
