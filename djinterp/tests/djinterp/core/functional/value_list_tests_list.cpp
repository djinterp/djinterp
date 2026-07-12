// djinterp [test]  value_list_tests_list.cpp
//   Section I -- value_list<auto...>: the NTTP sequence and its size().

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_value_list_size_member
  The list reports its own length.
  Tests the following:
  - size() on the empty list, on a singleton, and on longer lists
  - it is callable both on the type and on an instance
*/
bool
tests_value_list_size_member()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(value_list<>::size() == 0u, "empty");
    static_assert(value_list<1>::size() == 1u, "singleton");
    static_assert(value_list<1, 2, 3>::size() == 3u, "three");
    static_assert(value_list<0, 1, 2, 3, 4, 5, 6, 7>::size() == 8u, "eight");

    // on an instance, too -- the list is passed by value.
    constexpr value_list<1, 2, 3> list{};
    static_assert(list.size() == 3u, "instance");

    ok = ok && (value_list<1, 2, 3>::size() == 3u);
#endif

    return ok;
}


/*
tests_value_list_is_empty_object
  The values live in the TYPE, so the list carries no storage -- it is an empty
  struct, cheap to pass by value into the constexpr free-function ops.
  Tests the following:
  - the list is an empty class, whatever its length
  - it is trivially and constexpr default-constructible
*/
bool
tests_value_list_is_empty_object()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_empty<value_list<> >::value, "empty list");
    static_assert(std::is_empty<value_list<1, 2, 3> >::value, "populated list");
    static_assert(std::is_empty<value_list<0, 1, 2, 3, 4, 5, 6, 7> >::value,
                  "a long list is still empty");

    static_assert(std::is_trivially_default_constructible<
                      value_list<1, 2, 3> >::value, "trivially constructible");

    // an empty class still has non-zero size, but carries no members.
    static_assert(sizeof(value_list<1, 2, 3>) == sizeof(value_list<>),
                  "length does not change the object's size");

    constexpr value_list<1, 2, 3> list{};
    (void)list;

    ok = ok && (std::is_empty<value_list<1, 2, 3> >::value);
#endif

    return ok;
}


/*
tests_value_list_heterogeneous
  The pack is auto..., so the elements need not share a type -- this is the
  value-domain analog of a std::tuple's heterogeneity.
  Tests the following:
  - an int, a char, a bool, and a scoped enum coexist in one list
  - every element is counted
*/
bool
tests_value_list_heterogeneous()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using mixed = value_list<10, 'x', true, color::red>;

    static_assert(mixed::size() == 4u, "four elements of four kinds");
    static_assert(value_list_at_v<0, mixed> == 10, "int");
    static_assert(value_list_at_v<1, mixed> == 'x', "char");
    static_assert(value_list_at_v<2, mixed> == true, "bool");
    static_assert(value_list_at_v<3, mixed> == color::red, "enum");

    ok = ok && (mixed::size() == 4u);
#endif

    return ok;
}


/*
tests_value_list_size_noexcept
  size() is a static constexpr noexcept function -- usable anywhere a constant is
  needed, and never throwing.
  Tests the following:
  - the call is noexcept
  - it folds in a constant expression (used as an array bound)
*/
bool
tests_value_list_size_noexcept()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(noexcept(value_list<1, 2>::size()), "noexcept");
    static_assert(noexcept(value_list<>::size()), "noexcept, empty");

    // a constant expression: it may be an array bound.
    int arr[value_list<1, 2, 3>::size()];
    ok = ok && (sizeof(arr) / sizeof(arr[0]) == 3u);

    static_assert(std::is_same<decltype(value_list<1>::size()),
                               std::size_t>::value, "yields size_t");
#endif

    return ok;
}


/*
tests_value_list_duplicates
  A list is a sequence, not a set: repeated values are distinct elements, each
  addressable.
  Tests the following:
  - a list of identical values has that many elements
  - every index yields the value
*/
bool
tests_value_list_duplicates()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using triple = value_list<7, 7, 7>;

    static_assert(triple::size() == 3u, "three distinct elements");
    static_assert(value_list_at_v<0, triple> == 7, "first");
    static_assert(value_list_at_v<2, triple> == 7, "last");

    ok = ok && (triple::size() == 3u);
#endif

    return ok;
}


/*
tests_value_list_type_identity
  Two lists are the same type exactly when their packs match -- so ORDER matters,
  and so does the TYPE of each value, since an NTTP carries its type with it.
  Tests the following:
  - reordering the values yields a different type
  - a differing length yields a different type
  - the int 1 and the char 1 are different elements, though they compare equal
*/
bool
tests_value_list_type_identity()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<value_list<1, 2>, value_list<1, 2> >::value,
                  "same pack, same type");
    static_assert(!std::is_same<value_list<1, 2>, value_list<2, 1> >::value,
                  "order matters");
    static_assert(!std::is_same<value_list<1>, value_list<1, 2> >::value,
                  "length matters");

    // the value's TYPE is part of the pack: int 1 is not char 1.
    static_assert(!std::is_same<value_list<1>,
                                value_list<static_cast<char>(1)> >::value,
                  "the NTTP's type is part of the identity");
    static_assert(value_list_at_v<0, value_list<1> > ==
                  value_list_at_v<0, value_list<static_cast<char>(1)> >,
                  "...though the values compare equal");

    ok = ok && (!std::is_same<value_list<1, 2>, value_list<2, 1> >::value);
#endif

    return ok;
}


/*
tests_value_list_tier_gate
  The TIER claim: value_list is an auto-NTTP facility, so the module is gated to
  C++17 and below that the header declares NOTHING (it adds no trait-floor
  surface). That this translation unit includes the header and still compiles
  under -std=c++11 and -std=c++14 IS the check -- nothing below the gate may name
  value_list at all.
  Tests the following:
  - at C++17 and above, the sequence and its ops are present and work
  - below, the header contributed nothing, and the suite still builds
*/
bool
tests_value_list_tier_gate()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // at the tier: the facility exists.
    ok = ok && (value_list<1, 2, 3>::size() == 3u);
    ok = ok && (is_value_list<value_list<> >::value);
    ok = ok && (value_list_size_v<value_list<1, 2> > == 2u);
    static_assert(D_ENV_LANG_IS_CPP17_OR_HIGHER,
                  "the gate agrees with the language level");
#else
    // below the tier: the header declared nothing. Naming value_list here would
    // not compile -- which is exactly the property under test, and the build
    // itself is the assertion.
    ok = ok && true;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
