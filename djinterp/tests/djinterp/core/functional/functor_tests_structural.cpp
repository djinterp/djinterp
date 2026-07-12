// djinterp [test]  functor_tests_structural.cpp
//   Section 0 -- the structural detection vocabulary: functor_value_type,
//   is_fmappable, and the C++20 concept faces.

// std
#include <string>
#include <type_traits>
// djinterp
#include "functor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_functor_value_type
  The inner value type T of a functor F -- the first of the finer-grained
  questions generic code depends on.
  Tests the following:
  - the inner type of both bridged monads, at several inners
  - the inner type of the view, which is the type its pending function YIELDS
    (not the type of its source)
  - std::decay is applied to the functor first
*/
bool
tests_functor_value_type()
{
    bool ok = true;

    // bridged.
    static_assert(std::is_same<functor_value_type<tmaybe<int> >::type,
                               int>::value, "tmaybe<int>");
    static_assert(std::is_same<functor_value_type<tmaybe<std::string> >::type,
                               std::string>::value, "tmaybe<string>");
    static_assert(std::is_same<functor_value_type<tbox<bool> >::type,
                               bool>::value, "tbox<bool>");

    // the view: its inner type is what its pending function produces.
    static_assert(std::is_same<functor_value_type<lazy_view<int, dbl> >::type,
                               int>::value, "dbl : int -> int");
    static_assert(std::is_same<
        functor_value_type<lazy_view<int, is_even> >::type,
        bool>::value, "is_even : int -> bool -- the OUTPUT type");
    static_assert(std::is_same<
        functor_value_type<lazy_view<int, to_str> >::type,
        std::string>::value, "to_str : int -> string");

    // decayed.
    static_assert(std::is_same<functor_value_type<const tmaybe<int>&>::type,
                               int>::value, "decay applied");

    ok = ok && (std::is_same<functor_value_type<tbox<int> >::type, int>::value);

    return ok;
}


/*
tests_functor_value_type_t_alias
  The _t alias is exactly the trait's type.
  Tests the following:
  - the two agree on both roads into the protocol
*/
bool
tests_functor_value_type_t_alias()
{
    bool ok = true;

    static_assert(std::is_same<functor_value_type_t<tmaybe<int> >,
                               functor_value_type<tmaybe<int> >::type>::value,
                  "bridged");
    static_assert(std::is_same<
        functor_value_type_t<lazy_view<int, is_even> >,
        functor_value_type<lazy_view<int, is_even> >::type>::value, "explicit");

    static_assert(std::is_same<functor_value_type_t<tmaybe<std::string> >,
                               std::string>::value, "and it is the inner type");

    ok = ok && (std::is_same<functor_value_type_t<tbox<int> >, int>::value);

    return ok;
}


/*
tests_functor_value_type_helper_sfinae
  The extraction is built on a SFINAE helper whose primary is a COMPLETE but
  MEMBERLESS struct -- a soft failure, yielding no ::type rather than an error.
  This is the machinery behind the trait's SFINAE-friendliness claim, and it is
  the level at which that claim can actually be read.
  Tests the following:
  - the helper has a ::type for both kinds of functor
  - it has NONE for a non-functor -- and is nevertheless a complete type, so the
    failure is soft
  NOTE: the PUBLIC functor_value_type re-declares `type` unconditionally, so
  naming functor_value_type<non-functor>::type is a hard error rather than a
  detectable absence; only the helper's soft failure is testable here.
*/
bool
tests_functor_value_type_helper_sfinae()
{
    bool ok = true;

    // present for a functor, on both roads.
    static_assert(has_type<vt_helper<tmaybe<int> > >::value, "bridged");
    static_assert(has_type<vt_helper<tbox<int> > >::value, "the other monad");
    static_assert(has_type<vt_helper<lazy_view<int, dbl> > >::value, "explicit");

    // absent for a non-functor -- softly.
    static_assert(!has_type<vt_helper<not_a_functor> >::value,
                  "no ::type for a non-functor");
    static_assert(!has_type<vt_helper<int> >::value, "nor for a scalar");

    // and the failure IS soft: the primary is a complete, memberless struct.
    static_assert(is_complete<vt_helper<not_a_functor> >::value,
                  "complete, but memberless -- a soft failure");

    // the helper and the public face agree wherever the public face is usable.
    static_assert(std::is_same<vt_helper<tmaybe<int> >::type,
                               functor_value_type_t<tmaybe<int> > >::value,
                  "the same answer");

    ok = ok && (!has_type<vt_helper<not_a_functor> >::value);

    return ok;
}


/*
tests_is_fmappable_positive
  The second finer-grained question: is a given (functor, function) PAIR
  mappable? It is, when functor_map(F, Fn) is a well-formed expression.
  Tests the following:
  - both kinds of functor, with functions that fit their inner type
  - functions that change the inner type still count
*/
bool
tests_is_fmappable_positive()
{
    bool ok = true;

    // bridged.
    ok = ok && (is_fmappable<tmaybe<int>, dbl>::value);
    ok = ok && (is_fmappable<tbox<int>, add1>::value);

    // changing the inner type is still mappable.
    ok = ok && (is_fmappable<tmaybe<int>, is_even>::value);
    ok = ok && (is_fmappable<tbox<int>, to_str>::value);

    // explicit.
    ok = ok && (is_fmappable<lazy_view<int, dbl>, add1>::value);
    ok = ok && (is_fmappable<lazy_view<int, dbl>, is_even>::value);

    return ok;
}


/*
tests_is_fmappable_negative
  It is strictly finer-grained than is_functor: a perfectly good functor paired
  with a function that cannot be applied to its inner type is NOT mappable.
  Tests the following:
  - a functor with an incompatible function is refused -- though it IS a functor
  - a non-functor is refused with any function
  - the pairing really is what fails (the same functor maps fine with a fitting
    function)
*/
bool
tests_is_fmappable_negative()
{
    bool ok = true;

    // a functor, but the wrong function for its inner type.
    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (!is_fmappable<tmaybe<int>, wants_string>::value);
    ok = ok && (is_fmappable<tmaybe<int>, dbl>::value);      // the contrast

    ok = ok && (!is_fmappable<lazy_view<int, dbl>, wants_string>::value);

    // ...and it does fit the functor whose inner type IS a string.
    ok = ok && (is_fmappable<tmaybe<std::string>, wants_string>::value);

    // not a functor at all -- there is no map to call, so the expression is
    // ill-formed.
    ok = ok && (!is_fmappable<not_a_functor, dbl>::value);
    ok = ok && (!is_fmappable<int, dbl>::value);

    // NB: no_marker is NOT here -- it has a map, so it IS fmappable, though it
    // is not a functor. See tests_is_fmappable_does_not_require_the_marker.

    return ok;
}


/*
tests_is_fmappable_does_not_require_the_marker
  A divergence worth pinning. is_functor keys on the is_specialized MARKER;
  is_fmappable keys on whether the EXPRESSION functor_map(F, Fn) is well-formed.
  Those are different questions, and they can disagree: a functor_traits
  specialization that carries a map but no marker is NOT a functor, yet IS
  fmappable -- because its map is there to be called.
  Tests the following:
  - no_marker is not a functor, but IS fmappable, and functor_map really does
    call its map
  - the fmappable_with concept inherits the same looseness, so code constrained
    on it alone admits a type the Functor face would refuse
  - a type with no traits at all is refused by both (the expression is ill-formed)
*/
bool
tests_is_fmappable_does_not_require_the_marker()
{
    bool ok = true;

    // the two traits disagree here.
    ok = ok && (!is_functor<no_marker>::value);
    ok = ok && (is_fmappable<no_marker, dbl>::value);

    // ...because functor_map(no_marker, dbl) is a well-formed call.
    ok = ok && (functor_map(no_marker(), dbl()) == 0);

    // with no traits at all, both refuse: there is no map to call.
    ok = ok && (!is_functor<not_a_functor>::value);
    ok = ok && (!is_fmappable<not_a_functor, dbl>::value);

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // the concept faces inherit the same split.
    static_assert(!Functor<no_marker>, "the Functor face refuses it");
    static_assert(fmappable_with<no_marker, dbl>,
                  "but fmappable_with admits it");
    ok = ok && (which_functor(no_marker()) == 0);
    ok = ok && (which_fmappable(no_marker(), dbl()) == 1);
#endif

    return ok;
}


/*
tests_is_fmappable_v_agrees
  The _v shorthand is exactly the trait's value.
  Tests the following:
  - the two agree, positively and negatively, including the incompatible-function
    case
*/
bool
tests_is_fmappable_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && ((is_fmappable_v<tmaybe<int>, dbl>) ==
                (is_fmappable<tmaybe<int>, dbl>::value));
    ok = ok && ((is_fmappable_v<tmaybe<int>, wants_string>) ==
                (is_fmappable<tmaybe<int>, wants_string>::value));

    static_assert(is_fmappable_v<tbox<int>, dbl>, "bridged");
    static_assert(is_fmappable_v<lazy_view<int, dbl>, add1>, "explicit");
    static_assert(!is_fmappable_v<tmaybe<int>, wants_string>,
                  "incompatible function");
    static_assert(!is_fmappable_v<not_a_functor, dbl>, "not a functor");
#endif

    return ok;
}


/*
tests_functor_concept
  Functor is the PascalCase typeclass face of is_functor, and it really GATES
  overload resolution.
  Tests the following:
  - it mirrors the trait on both roads and on the negatives
  - a constrained overload wins for a functor and is excluded otherwise,
    including for the marker-less near-miss
*/
bool
tests_functor_concept()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Functor<tmaybe<int> > == is_functor<tmaybe<int> >::value,
                  "mirrors");
    static_assert(Functor<not_a_functor> ==
                  is_functor<not_a_functor>::value, "mirrors, negative");

    static_assert(Functor<tmaybe<int> >, "bridged");
    static_assert(Functor<tbox<int> >, "the other monad");
    static_assert(Functor<lazy_view<int, dbl> >, "explicit");
    static_assert(!Functor<not_a_functor>, "not a functor");
    static_assert(!Functor<no_marker>, "no marker");
    static_assert(!Functor<int>, "a scalar");
    static_assert(!Functor<dbl>, "a mapping function is not a functor");

    // and it gates.
    ok = ok && (which_functor(just(1)) == 1);
    ok = ok && (which_functor(boxed(1)) == 1);
    ok = ok && (which_functor(make_view(1, dbl())) == 1);
    ok = ok && (which_functor(no_marker()) == 0);
    ok = ok && (which_functor(not_a_functor()) == 0);
    ok = ok && (which_functor(42) == 0);
#endif

    return ok;
}


/*
tests_fmappable_with_concept
  fmappable_with is the concept face of is_fmappable (mirroring monad's
  mappable_with), and it gates on the PAIR, not merely on the functor.
  Tests the following:
  - it mirrors the trait
  - a constrained overload is excluded for a functor paired with a function that
    does not fit it -- the near-miss the plain Functor face cannot catch
*/
bool
tests_fmappable_with_concept()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(fmappable_with<tmaybe<int>, dbl> ==
                  is_fmappable<tmaybe<int>, dbl>::value, "mirrors");
    static_assert(fmappable_with<tmaybe<int>, wants_string> ==
                  is_fmappable<tmaybe<int>, wants_string>::value,
                  "mirrors, negative");

    static_assert(fmappable_with<tbox<int>, to_str>, "type change");
    static_assert(fmappable_with<lazy_view<int, dbl>, add1>, "the view");
    static_assert(!fmappable_with<tmaybe<int>, wants_string>,
                  "a functor, but the wrong function");

    // it gates on the pair.
    ok = ok && (which_fmappable(just(1), dbl()) == 1);
    ok = ok && (which_fmappable(make_view(1, dbl()), add1()) == 1);
    ok = ok && (which_fmappable(just(1), wants_string()) == 0);  // near-miss
    ok = ok && (which_fmappable(not_a_functor(), dbl()) == 0);
#endif

    return ok;
}


/*
tests_concepts_gating
  The concept faces are gated to C++20, leaving the traits as the always-present
  floor beneath them.
  Tests the following:
  - the gate agrees with the language level
  - the traits answer identically whether or not the gate is open
*/
bool
tests_concepts_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (Functor<tmaybe<int> > == is_functor<tmaybe<int> >::value);
    ok = ok && ((fmappable_with<tmaybe<int>, dbl>) ==
                (is_fmappable<tmaybe<int>, dbl>::value));
    #if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces present on a standard that lacks concepts
    #endif
#else
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces missing on a standard that has concepts
    #endif
#endif

    // the traits are the floor.
    ok = ok && (is_functor<tmaybe<int> >::value);
    ok = ok && (is_fmappable<lazy_view<int, dbl>, add1>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
