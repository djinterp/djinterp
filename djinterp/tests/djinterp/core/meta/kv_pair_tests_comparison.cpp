/*******************************************************************************
* djinterp [core]                                   kv_pair_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/meta/kv_pair_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "kv_pair_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_comparison_equality_is_key_only
  operator== consults the key alone.
  Tests the following:
  - two pairs with the same key AND same value are equal
  - two pairs with the same key but DIFFERENT values are still equal
  - two pairs with different keys are not equal
  - equality is reflexive
*/
bool
tests_comparison_equality_is_key_only()
{
    D_KV_CHECK((ii(1, 100) == ii(1, 100)));   // same key, same value
    D_KV_CHECK((ii(1, 100) == ii(1, 999)));   // same key, different value -> equal
    D_KV_CHECK(!(ii(1, 100) == ii(2, 100)));  // different key -> not equal

    // reflexive
    ii p(5, 5);
    D_KV_CHECK((p == p));

    return true;
}

/*
tests_comparison_inequality_is_key_only
  operator!= is the negation of the key-only equality.
  Tests the following:
  - pairs with different keys are unequal
  - pairs with the same key but different values are NOT unequal (they are equal)
  - pairs with the same key and value are not unequal
  - != always disagrees with == on the same operands
*/
bool
tests_comparison_inequality_is_key_only()
{
    D_KV_CHECK((ii(1, 0) != ii(2, 0)));       // different key -> unequal
    D_KV_CHECK(!(ii(1, 100) != ii(1, 999)));  // same key, diff value -> NOT unequal
    D_KV_CHECK(!(ii(1, 1) != ii(1, 1)));      // identical -> not unequal

    // != is the negation of ==
    D_KV_CHECK((ii(1, 100) != ii(2, 100)) == !(ii(1, 100) == ii(2, 100)));
    D_KV_CHECK((ii(1, 100) != ii(1, 999)) == !(ii(1, 100) == ii(1, 999)));

    return true;
}

/*
tests_comparison_ordering_is_by_key
  operator< orders by key.
  Tests the following:
  - a smaller key is less than a larger key
  - the larger key is not less than the smaller
  - equal keys are not less than one another (irreflexive on equal keys)
  - the ordering ignores the values entirely
*/
bool
tests_comparison_ordering_is_by_key()
{
    D_KV_CHECK((ii(1, 0) < ii(2, 0)));        // smaller key is less
    D_KV_CHECK(!(ii(2, 0) < ii(1, 0)));       // larger key is not less

    // equal keys: neither is less than the other
    D_KV_CHECK(!(ii(3, 0) < ii(3, 9)));
    D_KV_CHECK(!(ii(3, 9) < ii(3, 0)));

    // ordering ignores values: a smaller key with a huge value is still less
    D_KV_CHECK((ii(1, 9999) < ii(2, -9999)));

    return true;
}

/*
tests_comparison_value_is_ignored_entirely
  Varying only the value changes no comparison result.
  Tests the following:
  - == is invariant when the value changes and the key is held fixed
  - != is invariant likewise
  - < is invariant likewise (against a fixed second operand)
  - sweeping several values against one key gives one constant answer per
    operator
*/
bool
tests_comparison_value_is_ignored_entirely()
{
    ii ref(5, 0);

    // hold key = 5, vary the value: equality with ref(5, *) is always true
    for (int v = -3; v <= 3; ++v)
    {
        D_KV_CHECK((ii(5, v) == ref));
        D_KV_CHECK(!(ii(5, v) != ref));
        D_KV_CHECK(!(ii(5, v) < ref));   // equal keys -> not less
        D_KV_CHECK(!(ref < ii(5, v)));
    }

    // against a different key, the value still does not matter
    ii low(4, 0);
    for (int v = -3; v <= 3; ++v)
    {
        D_KV_CHECK((low < ii(5, v)));
        D_KV_CHECK(!(ii(5, v) < low));
    }

    return true;
}

/*
tests_comparison_operators_are_constexpr
  ==, != and < are usable in constant expressions.
  Tests the following:
  - a constexpr equality result is available
  - a constexpr inequality result is available
  - a constexpr ordering result is available
  - the compile-time answers match the key-only semantics
*/
bool
tests_comparison_operators_are_constexpr()
{
    constexpr ii a(1, 100);
    constexpr ii b(1, 999);
    constexpr ii c(2, 0);

    constexpr bool eq = (a == b);   // same key -> true
    constexpr bool ne = (a != c);   // different key -> true
    constexpr bool lt = (a < c);    // 1 < 2 -> true

    static_assert(eq, "same-key equality is constexpr and true");
    static_assert(ne, "different-key inequality is constexpr and true");
    static_assert(lt, "key ordering is constexpr and true");

    D_KV_CHECK(eq);
    D_KV_CHECK(ne);
    D_KV_CHECK(lt);

    return true;
}

/*
tests_comparison_with_string_keys
  The key-only comparison works for a non-trivial key type.
  Tests the following:
  - equal string keys with different values compare equal
  - different string keys are unequal
  - ordering follows the string key
  - the value type is free to differ (int values here)
*/
bool
tests_comparison_with_string_keys()
{
    using si = kv_pair<std::string, int>;

    D_KV_CHECK((si("k", 1) == si("k", 2)));     // same key, diff value
    D_KV_CHECK((si("a", 0) != si("b", 0)));     // different key
    D_KV_CHECK((si("a", 0) < si("b", 0)));      // "a" < "b"
    D_KV_CHECK(!(si("b", 0) < si("a", 0)));

    // equal keys are not less than one another regardless of value
    D_KV_CHECK(!(si("k", 1) < si("k", 2)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
