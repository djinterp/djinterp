// djinterp [test]  functor_tests_bridge.cpp
//   Section I.2 -- the blanket monad bridge: every monad is a functor, keyed on
//   is_monad, with no per-type wiring.

// std
#include <string>
#include <type_traits>
// djinterp
#include "functor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_bridge_every_monad_is_a_functor
  The headline claim. tmaybe and tbox each carry a monad_traits specialization
  and NOTHING else -- there is no functor_traits<tmaybe> or functor_traits<tbox>
  anywhere in this suite. If they are functors, the blanket specialization is
  what made them so.
  Tests the following:
  - both monads are monads, and both are functors
  - the traits their functor face resolves to is populated (the bridge, not the
    undefined primary)
*/
bool
tests_bridge_every_monad_is_a_functor()
{
    bool ok = true;

    ok = ok && (is_monad<tmaybe<int> >::value);
    ok = ok && (is_functor<tmaybe<int> >::value);

    ok = ok && (is_monad<tbox<int> >::value);
    ok = ok && (is_functor<tbox<int> >::value);

    // the functor face really resolved to a populated specialization.
    static_assert(is_complete<functor_traits<tmaybe<int> > >::value,
                  "the bridge, not the undefined primary");
    static_assert(std::is_same<functor_traits<tbox<int> >::is_specialized,
                               std::true_type>::value, "and it is marked");

    return ok;
}


/*
tests_bridge_is_keyed_on_is_monad
  The bridge is keyed on is_monad, so monad-ness IMPLIES functor-ness -- but not
  the reverse. Functor is the strictly broader protocol.
  Tests the following:
  - every monad fixture is a functor
  - the view is a functor but NOT a monad, so the implication is one-way
  - a non-functor is neither
*/
bool
tests_bridge_is_keyed_on_is_monad()
{
    bool ok = true;

    // monad => functor.
    static_assert(!is_monad<tmaybe<int> >::value ||
                   is_functor<tmaybe<int> >::value, "monad => functor");
    static_assert(!is_monad<tbox<int> >::value ||
                   is_functor<tbox<int> >::value, "monad => functor");

    // but functor does NOT imply monad.
    ok = ok && (is_functor<lazy_view<int, dbl> >::value);
    ok = ok && (!is_monad<lazy_view<int, dbl> >::value);

    // neither, for a non-functor.
    ok = ok && (!is_monad<not_a_functor>::value);
    ok = ok && (!is_functor<not_a_functor>::value);

    return ok;
}


/*
tests_bridge_derives_map_from_monad_map
  The bridge derives the functor operation from the monad's own bind + unit, via
  monad_map. So over a monad, functor_map IS monad_map -- same value and same
  type, not merely a similar one.
  Tests the following:
  - the two agree in value, on a present and an absent context
  - the two agree in TYPE
*/
bool
tests_bridge_derives_map_from_monad_map()
{
    bool ok = true;

    // same value.
    ok = ok && (functor_map(just(21), dbl()).value ==
                monad_map(just(21), dbl()).value);
    ok = ok && (functor_map(nothing<int>(), dbl()).has ==
                monad_map(nothing<int>(), dbl()).has);
    ok = ok && (functor_map(boxed(5), add1()).value ==
                monad_map(boxed(5), add1()).value);

    // same type.
    static_assert(std::is_same<decltype(functor_map(just(21), dbl())),
                               decltype(monad_map(just(21), dbl()))>::value,
                  "functor_map IS monad_map, over a monad");
    static_assert(std::is_same<decltype(functor_map(boxed(5), to_str())),
                               decltype(monad_map(boxed(5), to_str()))>::value,
                  "and through a type change too");

    return ok;
}


/*
tests_bridge_value_type_from_monad
  The bridge takes the functor's inner type straight from monad_value_type.
  Tests the following:
  - the functor and monad value types agree, for both monads and several inners
*/
bool
tests_bridge_value_type_from_monad()
{
    bool ok = true;

    static_assert(std::is_same<functor_traits<tmaybe<int> >::value_type,
                               monad_value_type<tmaybe<int> >::type>::value,
                  "tmaybe<int>");
    static_assert(std::is_same<
        functor_traits<tmaybe<std::string> >::value_type,
        monad_value_type<tmaybe<std::string> >::type>::value,
        "tmaybe<string>");
    static_assert(std::is_same<functor_traits<tbox<bool> >::value_type,
                               monad_value_type<tbox<bool> >::type>::value,
                  "tbox<bool>");

    // and it is the inner type, as named.
    static_assert(std::is_same<functor_traits<tmaybe<int> >::value_type,
                               int>::value, "the inner type");

    ok = ok && (std::is_same<functor_value_type_t<tbox<int> >, int>::value);

    return ok;
}


/*
tests_bridge_rebind_from_monad
  Where the monad names an F<U>, the bridge passes it through as the functor's
  rebind -- even though rebind is not part of the core functor protocol.
  Tests the following:
  - the functor's rebind and the monad's agree
  - it really does change the inner type
*/
bool
tests_bridge_rebind_from_monad()
{
    bool ok = true;

    static_assert(std::is_same<
        functor_traits<tmaybe<int> >::rebind<std::string>,
        monad_rebind<tmaybe<int>, std::string>::type>::value,
        "rebind comes from the monad");

    static_assert(std::is_same<functor_traits<tmaybe<int> >::rebind<bool>,
                               tmaybe<bool> >::value, "F<T> -> F<U>");
    static_assert(std::is_same<functor_traits<tbox<int> >::rebind<std::string>,
                               tbox<std::string> >::value, "the other monad");

    ok = ok && (has_rebind<functor_traits<tmaybe<int> > >::value);

    return ok;
}


/*
tests_bridge_zero_wiring_for_a_new_monad
  "Any future monad participates with no per-type specialization." tbox is that
  future monad: it is unrelated to tmaybe, was given only a monad_traits, and is
  a fully working functor.
  Tests the following:
  - tbox is a functor, has a value type, is mappable, and maps correctly
  - none of that required a functor_traits<tbox> anywhere
*/
bool
tests_bridge_zero_wiring_for_a_new_monad()
{
    bool ok = true;

    ok = ok && (is_functor<tbox<int> >::value);
    ok = ok && (is_fmappable<tbox<int>, dbl>::value);

    static_assert(std::is_same<functor_value_type_t<tbox<int> >, int>::value,
                  "value type");

    ok = ok && (functor_map(boxed(21), dbl()).value == 42);
    ok = ok && (functor_map(boxed(4), add1()).value == 5);

    // and it changes the inner type, through the bridge's rebind.
    ok = ok && (functor_map(boxed(3), to_str()).value == std::string("xxx"));
    static_assert(std::is_same<decltype(functor_map(boxed(3), is_even())),
                               tbox<bool> >::value, "int -> bool");

    return ok;
}


/*
tests_bridge_does_not_overlap_explicit
  "A view / producer is not a monad, so its explicit specialization never
  overlaps this one." The bridge's enable_if<is_monad<F>> simply does not match a
  non-monad, so there is no ambiguity to resolve.
  Tests the following:
  - the view is not a monad, so the bridge cannot claim it
  - the view's own specialization is the one in force -- its map is its own, not
    monad_map (which would not even compile for it)
  - both kinds of functor coexist and are mapped by the same generic call
*/
bool
tests_bridge_does_not_overlap_explicit()
{
    bool ok = true;

    ok = ok && (!is_monad<lazy_view<int, dbl> >::value);
    ok = ok && (is_functor<lazy_view<int, dbl> >::value);

    // the view's OWN map is in force: it returns a composed lazy_view, which no
    // monad bridge could have produced (the bridge yields rebind<U>).
    using mapped = decltype(functor_map(make_view(5, dbl()), add1()));
    static_assert(std::is_same<mapped,
                               lazy_view<int, composed<dbl, add1> > >::value,
                  "the explicit map, not the bridge's");
    ok = ok && (functor_map(make_view(5, dbl()), add1()).get() == 11);

    // the two roads coexist under one generic call.
    ok = ok && (functor_map(just(21), dbl()).value == 42);
    ok = ok && (functor_map(make_view(21, ident_fn()), dbl()).get() == 42);

    return ok;
}


/*
tests_bridge_preserves_monad_context
  The bridge routes through bind + unit, so the surrounding context is untouched:
  mapping an absent value leaves it absent, and the function is never applied.
  Tests the following:
  - nothing maps to nothing, at the original and at a changed inner type
  - just maps to just, with the function applied
*/
bool
tests_bridge_preserves_monad_context()
{
    bool ok = true;

    // present stays present.
    const tmaybe<int> present = functor_map(just(21), dbl());
    ok = ok && (present.has);
    ok = ok && (present.value == 42);

    // absent stays absent.
    const tmaybe<int> absent = functor_map(nothing<int>(), dbl());
    ok = ok && (!absent.has);

    // absent stays absent across a type change, too.
    const tmaybe<bool> absent_b = functor_map(nothing<int>(), is_even());
    ok = ok && (!absent_b.has);
    static_assert(std::is_same<decltype(functor_map(nothing<int>(), is_even())),
                               tmaybe<bool> >::value, "context rebound");

    // the identity monad has no empty case: it always carries a value.
    ok = ok && (functor_map(boxed(7), dbl()).value == 14);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
