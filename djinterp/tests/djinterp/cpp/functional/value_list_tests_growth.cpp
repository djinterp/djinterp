/*******************************************************************************
* djinterp [cpp]                                     value_list_tests_growth.cpp
*
*
* path:      /tests/djinterp/cpp/functional/value_list_tests_growth.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  value_list_tests_growth.cpp
//   Section IV -- growth: append, prepend, and the four concat overloads.

// std
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_append
  append puts the new element at the END. The element arrives as a value carrier,
  the same shape at() hands back.
  Tests the following:
  - the result type carries the element last
  - the call is constexpr, and the source list is untouched (a new type is
    returned; the values live in the type)
*/
bool tests_append()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(append(DValueList<1, 2>{}, val<3>)),
                               DValueList<1, 2, 3> >::value, "at the end");
    static_assert(std::is_same<decltype(append(DValueList<1>{}, val<2>)),
                               DValueList<1, 2> >::value, "singleton");

    // appending twice.
    static_assert(std::is_same<decltype(append(append(DValueList<1>{}, val<2>),
                                               val<3>)),
                               DValueList<1, 2, 3> >::value, "twice");

    // the length grows by exactly one.
    static_assert(decltype(append(DValueList<1, 2>{}, val<3>))::size() == 3u,
                  "length + 1");

    // NB: `constexpr auto g = append(...)` would make decltype(g) CONST, which
    // value_list_at does not accept -- see tests_traits_require_an_unqualified_list.
    using grown = decltype(append(DValueList<1, 2>{}, val<3>));
    static_assert(value_list_at_v<2, grown> == 3, "the new tail");

    ok = ok && (grown::size() == 3u);
#endif

    return ok;
}


/*
tests_append_to_empty
  Appending to the empty list yields a singleton -- the base of the collect step.
  Tests the following:
  - the empty list grows to exactly one element
*/
bool tests_append_to_empty()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(append(DValueList<>{}, val<3>)),
                               DValueList<3> >::value, "empty -> singleton");
    static_assert(decltype(append(DValueList<>{}, val<3>))::size() == 1u,
                  "one element");

    ok = ok && (decltype(append(DValueList<>{}, val<3>))::size() == 1u);
#endif

    return ok;
}


/*
tests_prepend
  prepend puts the new element at the FRONT -- the mirror of append, and the
  reason fold with it reverses a list.
  Tests the following:
  - the result type carries the element first
  - prepend and append put the element at opposite ends
*/
bool tests_prepend()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(prepend(val<0>, DValueList<1, 2>{})),
                               DValueList<0, 1, 2> >::value, "at the front");

    // the two ends are genuinely different.
    static_assert(!std::is_same<decltype(prepend(val<0>, DValueList<1, 2>{})),
                                decltype(append(DValueList<1, 2>{}, val<0>))
                               >::value, "front is not back");
    static_assert(std::is_same<decltype(append(DValueList<1, 2>{}, val<0>)),
                               DValueList<1, 2, 0> >::value, "...and here it is");

    static_assert(value_list_at_v<0, decltype(prepend(val<0>,
                      DValueList<1, 2>{}))> == 0, "the new head");

    ok = ok && (decltype(prepend(val<0>, DValueList<1, 2>{}))::size() == 3u);
#endif

    return ok;
}


/*
tests_prepend_to_empty
  Prepending to the empty list yields a singleton.
  Tests the following:
  - the empty list grows to exactly one element, matching append's result there
*/
bool tests_prepend_to_empty()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(prepend(val<3>, DValueList<>{})),
                               DValueList<3> >::value, "empty -> singleton");

    // on the empty list the two ends coincide.
    static_assert(std::is_same<decltype(prepend(val<3>, DValueList<>{})),
                               decltype(append(DValueList<>{}, val<3>))
                              >::value, "same result on the empty list");

    ok = ok && (decltype(prepend(val<3>, DValueList<>{}))::size() == 1u);
#endif

    return ok;
}


/*
tests_growth_heterogeneous
  Growth does not homogenise: an element keeps its own type as it joins the list.
  Tests the following:
  - a char appended to an int list stays a char
  - an enum prepended to a mixed list stays an enum
*/
bool tests_growth_heterogeneous()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(append(DValueList<1, 2>{}, val<'x'>)),
                               DValueList<1, 2, 'x'> >::value, "char appended");
    static_assert(std::is_same<
        decltype(value_list_at_v<2, decltype(append(DValueList<1, 2>{},
                                                    val<'x'>))>),
        const char>::value, "and it is still a char");

    static_assert(std::is_same<
        decltype(prepend(val<color::green>, DValueList<1, 'x'>{})),
        DValueList<color::green, 1, 'x'> >::value, "enum prepended");

    ok = ok && (value_list_at_v<2, decltype(append(DValueList<1, 2>{},
                                                   val<'x'>))> == 'x');
#endif

    return ok;
}


/*
tests_concat_nullary_and_unary
  The two degenerate arities: concat of nothing is the empty list, and concat of
  one list is that list (the identity overload, which returns its argument).
  Tests the following:
  - concat() yields DValueList<>
  - concat(a) yields a, for a populated and for an empty a
*/
bool tests_concat_nullary_and_unary()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(concat()), DValueList<> >::value,
                  "concat() -> empty");
    static_assert(decltype(concat())::size() == 0u, "no elements");

    static_assert(std::is_same<decltype(concat(DValueList<1, 2>{})),
                               DValueList<1, 2> >::value, "identity");
    static_assert(std::is_same<decltype(concat(DValueList<>{})),
                               DValueList<> >::value, "identity, empty");

    ok = ok && (decltype(concat())::size() == 0u);
    ok = ok && (decltype(concat(DValueList<1, 2>{}))::size() == 2u);
#endif

    return ok;
}


/*
tests_concat_binary
  The two-list base case, which every longer concat folds down to.
  Tests the following:
  - the packs are joined, left before right
  - order matters: concat(a, b) is not concat(b, a)
*/
bool tests_concat_binary()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(concat(DValueList<1>{},
                                               DValueList<2, 3>{})),
                               DValueList<1, 2, 3> >::value, "left then right");

    // order is preserved, so concat is not commutative.
    static_assert(std::is_same<decltype(concat(DValueList<2, 3>{},
                                               DValueList<1>{})),
                               DValueList<2, 3, 1> >::value, "the other way");
    static_assert(!std::is_same<decltype(concat(DValueList<1>{},
                                               DValueList<2, 3>{})),
                                decltype(concat(DValueList<2, 3>{},
                                               DValueList<1>{}))
                               >::value, "not commutative");

    // lengths add.
    static_assert(decltype(concat(DValueList<1, 2>{},
                                  DValueList<3, 4, 5>{}))::size() == 5u,
                  "2 + 3 = 5");

    ok = ok && (decltype(concat(DValueList<1>{}, DValueList<2, 3>{}))::size()
                == 3u);
#endif

    return ok;
}


/*
tests_concat_variadic
  Three or more lists take the >= 3 overload, which folds pairwise from the left.
  Tests the following:
  - three, four, and five lists concatenate in order
  - the result matches the same pairwise folding written out by hand
*/
bool tests_concat_variadic()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(concat(DValueList<1>{},
                                               DValueList<2>{},
                                               DValueList<3>{})),
                               DValueList<1, 2, 3> >::value, "three");

    static_assert(std::is_same<decltype(concat(DValueList<1>{},
                                               DValueList<2>{},
                                               DValueList<3>{},
                                               DValueList<4>{})),
                               DValueList<1, 2, 3, 4> >::value, "four");

    static_assert(std::is_same<decltype(concat(DValueList<1, 2>{},
                                               DValueList<3>{},
                                               DValueList<4, 5>{},
                                               DValueList<6>{},
                                               DValueList<7>{})),
                               DValueList<1, 2, 3, 4, 5, 6, 7> >::value, "five");

    // and it really is the left-pairwise fold it claims to be.
    static_assert(std::is_same<
        decltype(concat(DValueList<1>{}, DValueList<2>{}, DValueList<3>{})),
        decltype(concat(concat(DValueList<1>{}, DValueList<2>{}),
                        DValueList<3>{}))
        >::value, "folds pairwise from the left");

    ok = ok && (decltype(concat(DValueList<1>{}, DValueList<2>{},
                                DValueList<3>{}))::size() == 3u);
#endif

    return ok;
}


/*
tests_concat_empty_lists
  Empty lists contribute nothing, wherever they sit.
  Tests the following:
  - an empty list in the middle, at the front, and at the end all vanish
  - concatenating only empties yields the empty list
*/
bool tests_concat_empty_lists()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(concat(DValueList<1>{},
                                               DValueList<>{},
                                               DValueList<2>{})),
                               DValueList<1, 2> >::value, "empty in the middle");

    static_assert(std::is_same<decltype(concat(DValueList<>{},
                                               DValueList<1, 2>{})),
                               DValueList<1, 2> >::value, "empty at the front");

    static_assert(std::is_same<decltype(concat(DValueList<1, 2>{},
                                               DValueList<>{})),
                               DValueList<1, 2> >::value, "empty at the end");

    static_assert(std::is_same<decltype(concat(DValueList<>{},
                                               DValueList<>{},
                                               DValueList<>{})),
                               DValueList<> >::value, "all empty");

    ok = ok && (decltype(concat(DValueList<1>{}, DValueList<>{},
                                DValueList<2>{}))::size() == 2u);
#endif

    return ok;
}


/*
tests_concat_monoid_laws
  concat is a monoid over value_lists: associative, with the empty list as its
  identity element on both sides.
  Tests the following:
  - left identity: concat(<>, a) == a
  - right identity: concat(a, <>) == a
  - associativity: concat(concat(a, b), c) == concat(a, concat(b, c))
*/
bool tests_concat_monoid_laws()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using a = DValueList<1, 2>;
    using b = DValueList<3>;
    using c = DValueList<4, 5>;

    // identity, both sides.
    static_assert(std::is_same<decltype(concat(DValueList<>{}, a{})), a>::value,
                  "left identity");
    static_assert(std::is_same<decltype(concat(a{}, DValueList<>{})), a>::value,
                  "right identity");

    // associativity.
    static_assert(std::is_same<decltype(concat(concat(a{}, b{}), c{})),
                               decltype(concat(a{}, concat(b{}, c{})))
                              >::value, "associative");
    static_assert(std::is_same<decltype(concat(concat(a{}, b{}), c{})),
                               DValueList<1, 2, 3, 4, 5> >::value,
                  "...and it is the joined pack");

    ok = ok && (decltype(concat(concat(a{}, b{}), c{}))::size() == 5u);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
