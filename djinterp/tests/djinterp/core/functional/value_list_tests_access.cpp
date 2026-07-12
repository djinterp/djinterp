// djinterp [test]  value_list_tests_access.cpp
//   Section III -- value_list_size, value_list_at, and at(): the trait faces and
//   the carrier-returning element accessor.

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_value_list_size_trait
  value_list_size is the value-domain analog of std::tuple_size.
  Tests the following:
  - the count for an empty, a singleton, and a longer list
  - it agrees with the list's own size() member
*/
bool
tests_value_list_size_trait()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(value_list_size<value_list<> >::value == 0u, "empty");
    static_assert(value_list_size<value_list<1> >::value == 1u, "singleton");
    static_assert(value_list_size<value_list<1, 2, 3> >::value == 3u, "three");
    static_assert(value_list_size<value_list<10, 'x', true> >::value == 3u,
                  "heterogeneous");

    // the trait face and the member agree.
    static_assert(value_list_size<value_list<1, 2, 3> >::value ==
                  value_list<1, 2, 3>::size(), "trait == member");

    ok = ok && (value_list_size<value_list<1, 2, 3> >::value ==
                value_list<1, 2, 3>::size());
#endif

    return ok;
}


/*
tests_value_list_size_integral_constant
  It IS an integral_constant, so it carries that whole surface -- the same as
  std::tuple_size.
  Tests the following:
  - value_type, type, and the derivation from integral_constant
  - it converts implicitly to size_t
*/
bool
tests_value_list_size_integral_constant()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using three = value_list_size<value_list<1, 2, 3> >;

    static_assert(std::is_same<three::value_type, std::size_t>::value,
                  "value_type is size_t");
    static_assert(std::is_same<three::type,
                               std::integral_constant<std::size_t, 3> >::value,
                  "type");
    static_assert(std::is_base_of<std::integral_constant<std::size_t, 3>,
                                  three>::value, "derives from it");

    // implicit conversion.
    const std::size_t n = three();
    ok = ok && (n == 3u);
#endif

    return ok;
}


/*
tests_value_list_size_v_agrees
  The _v shorthand is exactly the trait's value.
  Tests the following:
  - the two agree across lengths
  - the shorthand is a constant expression of type size_t
*/
bool
tests_value_list_size_v_agrees()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(value_list_size_v<value_list<> > ==
                  value_list_size<value_list<> >::value, "empty");
    static_assert(value_list_size_v<value_list<1, 2, 3> > ==
                  value_list_size<value_list<1, 2, 3> >::value, "three");

    static_assert(std::is_same<decltype(value_list_size_v<value_list<1> >),
                               const std::size_t>::value, "typed size_t");

    // usable as an array bound.
    int arr[value_list_size_v<value_list<1, 2, 3, 4> >];
    ok = ok && (sizeof(arr) / sizeof(arr[0]) == 4u);
#endif

    return ok;
}


/*
tests_value_list_at_positions
  value_list_at yields the I-th value, 0-based.
  Tests the following:
  - the first, a middle, and the last element
  - the boundary index (size - 1) is in range; the guard is a static_assert, so
    an out-of-range index is a hard error by design and cannot be probed
*/
bool
tests_value_list_at_positions()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = value_list<10, 20, 30, 40>;

    static_assert(value_list_at<0, list>::value == 10, "first");
    static_assert(value_list_at<1, list>::value == 20, "middle");
    static_assert(value_list_at<2, list>::value == 30, "middle");
    static_assert(value_list_at<3, list>::value == 40, "last (the boundary)");

    // a singleton: index 0 is both first and last.
    static_assert(value_list_at<0, value_list<99> >::value == 99, "singleton");

    ok = ok && (value_list_at<3, list>::value == 40);
#endif

    return ok;
}


/*
tests_value_list_at_preserves_value_type
  An NTTP carries its type with it, so the extracted value keeps that type -- an
  int stays an int, a char a char, an enum an enum.
  Tests the following:
  - each element of a heterogeneous list comes back with its own type
*/
bool
tests_value_list_at_preserves_value_type()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using mixed = value_list<10, 'x', true, color::red>;

    static_assert(std::is_same<decltype(value_list_at_v<0, mixed>),
                               const int>::value, "int");
    static_assert(std::is_same<decltype(value_list_at_v<1, mixed>),
                               const char>::value, "char");
    static_assert(std::is_same<decltype(value_list_at_v<2, mixed>),
                               const bool>::value, "bool");
    static_assert(std::is_same<decltype(value_list_at_v<3, mixed>),
                               const color>::value, "enum");

    static_assert(value_list_at_v<1, mixed> == 'x', "and the value is right");
    static_assert(value_list_at_v<3, mixed> == color::red, "enum value");

    ok = ok && (value_list_at_v<2, mixed> == true);
#endif

    return ok;
}


/*
tests_value_list_at_deep
  The accessor peels one head per index step, so a deep index exercises that
  recursion.
  Tests the following:
  - every index of an eight-element list yields its own value
  - the last index (the deepest peel) is correct
*/
bool
tests_value_list_at_deep()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using deep = value_list<0, 1, 2, 3, 4, 5, 6, 7>;

    static_assert(value_list_at_v<0, deep> == 0, "0");
    static_assert(value_list_at_v<1, deep> == 1, "1");
    static_assert(value_list_at_v<2, deep> == 2, "2");
    static_assert(value_list_at_v<3, deep> == 3, "3");
    static_assert(value_list_at_v<4, deep> == 4, "4");
    static_assert(value_list_at_v<5, deep> == 5, "5");
    static_assert(value_list_at_v<6, deep> == 6, "6");
    static_assert(value_list_at_v<7, deep> == 7, "7 -- the deepest peel");

    ok = ok && (value_list_at_v<7, deep> == 7);
#endif

    return ok;
}


/*
tests_value_list_at_v_agrees
  The _v shorthand is exactly the trait's value, at every index.
  Tests the following:
  - the two agree, and the shorthand keeps the value's type
*/
bool
tests_value_list_at_v_agrees()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = value_list<10, 'x', 30>;

    static_assert(value_list_at_v<0, list> == value_list_at<0, list>::value,
                  "index 0");
    static_assert(value_list_at_v<1, list> == value_list_at<1, list>::value,
                  "index 1");
    static_assert(value_list_at_v<2, list> == value_list_at<2, list>::value,
                  "index 2");

    static_assert(std::is_same<decltype(value_list_at_v<1, list>),
                               decltype(value_list_at<1, list>::value)>::value,
                  "and the same type");

    ok = ok && (value_list_at_v<2, list> == 30);
#endif

    return ok;
}


/*
tests_at_returns_carrier
  at<I>(list) is the object-domain accessor: it hands back the element as a val_t
  carrier, so it slots straight into the carrier pipeline.
  Tests the following:
  - the result type is val_t<V> for the element V
  - the carrier's ::value is the element, and the call is constexpr
  - it works on an rvalue list and on a named one
*/
bool
tests_at_returns_carrier()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using mixed = value_list<10, 'x', true>;

    static_assert(std::is_same<decltype(at<0>(mixed{})), val_t<10> >::value,
                  "val_t<10>");
    static_assert(std::is_same<decltype(at<1>(mixed{})), val_t<'x'> >::value,
                  "val_t<'x'> -- the char survives");
    static_assert(std::is_same<decltype(at<2>(mixed{})), val_t<true> >::value,
                  "val_t<true>");

    static_assert(at<0>(mixed{}).value == 10, "constexpr, on an rvalue");

    constexpr mixed list{};
    static_assert(at<1>(list).value == 'x', "constexpr, on a named list");
    static_assert(decltype(at<2>(list))::value == true, "via the carrier's type");

    ok = ok && (at<0>(list).value == 10);
#endif

    return ok;
}


/*
tests_at_feeds_the_carrier_pipeline
  The point of returning a carrier: at()'s result is exactly what append / prepend
  consume, so element access composes with growth without an unwrap step.
  Tests the following:
  - append(list, at<0>(list)) duplicates the head at the tail
  - prepend(at<N-1>(list), list) duplicates the tail at the head
  - a carrier fetched from one list can be grown onto another
*/
bool
tests_at_feeds_the_carrier_pipeline()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = value_list<7, 8>;

    // the head, appended at the end.
    static_assert(std::is_same<decltype(append(list{}, at<0>(list{}))),
                               value_list<7, 8, 7> >::value, "at -> append");

    // the last element, prepended at the front.
    static_assert(std::is_same<decltype(prepend(at<1>(list{}), list{})),
                               value_list<8, 7, 8> >::value, "at -> prepend");

    // across lists: a carrier taken from one grows another.
    static_assert(std::is_same<decltype(append(value_list<1>{},
                                               at<1>(list{}))),
                               value_list<1, 8> >::value, "cross-list");

    ok = ok && (at<0>(list{}).value == 7);
#endif

    return ok;
}


/*
tests_traits_require_an_unqualified_list
  A sharp edge, pinned. is_value_list applies clean_t and so accepts any cv-ref
  spelling -- but value_list_size and value_list_at do NOT: their primary template
  is declared-but-undefined, so a const-qualified or reference spelling matches the
  primary and is an INCOMPLETE type, not a false answer. That matters in practice,
  because `constexpr auto l = append(...)` gives decltype(l) == const value_list<...>,
  which cannot then index the list.
  Tests the following:
  - both traits are complete for a plain value_list
  - both are INCOMPLETE for const / reference / volatile spellings
  - is_value_list, by contrast, answers true for every one of those spellings
  - the completeness probe itself is sound (a plain type is complete)
*/
bool
tests_traits_require_an_unqualified_list()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = value_list<1, 2, 3>;

    // the probe is sound.
    static_assert(is_complete<list>::value, "a value_list is complete");
    static_assert(is_complete<int>::value, "so is int");

    // the plain spelling: both traits are populated.
    static_assert(is_complete<value_list_size<list> >::value, "size<list>");
    static_assert(is_complete<value_list_at<0, list> >::value, "at<0, list>");

    // a cv-ref spelling falls through to the undefined primary.
    static_assert(!is_complete<value_list_size<const list> >::value,
                  "size<const list> is INCOMPLETE -- no clean_t");
    static_assert(!is_complete<value_list_size<list&> >::value,
                  "size<list&> is INCOMPLETE");
    static_assert(!is_complete<value_list_at<0, const list> >::value,
                  "at<0, const list> is INCOMPLETE -- no clean_t");
    static_assert(!is_complete<value_list_at<0, list&> >::value,
                  "at<0, list&> is INCOMPLETE");

    // ...and a non-list is incomplete too, rather than answering false.
    static_assert(!is_complete<value_list_size<int> >::value, "size<int>");

    // the asymmetry: is_value_list DOES strip, and answers for all of them.
    static_assert(is_value_list<const list>::value, "is_value_list strips const");
    static_assert(is_value_list<list&>::value, "...and references");

    ok = ok && (is_complete<value_list_size<list> >::value);
    ok = ok && (!is_complete<value_list_size<const list> >::value);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
