/*******************************************************************************
* djinterp [test]                          option_set_compare_tests_value_eq.cpp
*
*   Section V of the option_set_compare.hpp suite: option_set_value_eq, the
* parameterized value-equality trait.
*
*     option_set_value_eq<_A, _B, _Extract> / _v
*         - true iff _A and _B are key-congruent AND, at every key, the carriers
*           the extractor pulls compare equal (via carrier_eq).
*
*   _Extract has no default in the corrected header (the retired actual<> /
* default_ extractors are gone), so these tests supply the self-contained
* extract_cmp_val from the fixtures - an extractor over the local DCmpVal<>
* carrier.  That keeps the coverage honest: it exercises the trait's own logic
* (key-congruence gate + per-key carrier comparison, including the DValueAbsent
* path for keys with no carried value) rather than any retired vocabulary.
*
*   Cases: identical sets are equal; a reordering is still equal (congruence is
* order-insensitive and comparison is by key, not position); a single differing
* value breaks it; a differing key set breaks it before any value is compared;
* absent==absent at a key holds while present-vs-absent does not; and two empty
* sets are (vacuously) equal.
*
*
* path:      /tests/djinterp/cpp/option/option_set_compare_tests_value_eq.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.07
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_set_compare_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace  // section-local set aliases (built on the fixtures)
{
    using DSetAb   = option_set<option<cmp_key::a, DCmpVal<5>>,
                                option<cmp_key::b, DCmpVal<10>>>;
    using DSetAbR = option_set<option<cmp_key::b, DCmpVal<10>>,   // reordered
                                option<cmp_key::a, DCmpVal<5>>>;
    using DSetAbX = option_set<option<cmp_key::a, DCmpVal<5>>,    // b differs
                                option<cmp_key::b, DCmpVal<99>>>;
    using DSetAc   = option_set<option<cmp_key::a, DCmpVal<5>>,    // key c, not b
                                option<cmp_key::c, DCmpVal<10>>>;
    using DSetBare = option_set<option<cmp_key::a>,                // no carriers
                                option<cmp_key::b>>;
}

// value_eq_equal
//   identical sets are value-equal, and so is a reordering: congruence is
// order-insensitive and the per-key comparison matches by key, not position.
bool value_eq_equal()
{
    constexpr bool ok =
        option_set_value_eq_v<DSetAb, DSetAb,   extract_cmp_val>               &&
        option_set_value_eq_v<DSetAb, DSetAbR, extract_cmp_val>               &&
        option_set_value_eq<DSetAb, DSetAbR, extract_cmp_val>::value;   // struct form

    static_assert(ok, "value_eq: identical and reordered sets are equal (matched by key)");
    return ok;
}

// value_eq_value_differs
//   a single differing carried value at a shared key breaks equality.
bool value_eq_value_differs()
{
    constexpr bool ok = !option_set_value_eq_v<DSetAb, DSetAbX, extract_cmp_val>;

    static_assert(ok, "value_eq: one differing value at a key -> not equal");
    return ok;
}

// value_eq_keys_differ
//   a differing key set fails the congruence gate, so the sets are unequal
// regardless of the values present.
bool value_eq_keys_differ()
{
    constexpr bool ok = !option_set_value_eq_v<DSetAb, DSetAc, extract_cmp_val>;

    static_assert(ok, "value_eq: differing key set -> not equal (congruence gate)");
    return ok;
}

// value_eq_absent_carrier
//   the DValueAbsent path: two sets whose keys carry no value are equal
// (absent==absent at each key), but a set with carriers is not equal to the
// bare set even though they are key-congruent (present vs absent).
bool value_eq_absent_carrier()
{
    constexpr bool ok =
        option_set_value_eq_v<DSetBare,
                              option_set<option<cmp_key::a>, option<cmp_key::b>>,
                              extract_cmp_val>                                  &&
        !option_set_value_eq_v<DSetAb, DSetBare, extract_cmp_val>;

    static_assert(ok, "value_eq: absent==absent at a key; present vs absent -> not equal");
    return ok;
}

// value_eq_empty
//   two empty sets are key-congruent with nothing to compare, so they are
// (vacuously) value-equal.
bool value_eq_empty()
{
    constexpr bool ok =
        option_set_value_eq_v<option_set<>, option_set<>, extract_cmp_val>;

    static_assert(ok, "value_eq: two empty sets are (vacuously) equal");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec option_set_compare_value_eq_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "V. option_set_value_eq";
    b.descriptor = "parameterized value equality via a self-contained custom extractor";
    b.tests = {
        { "value_eq_equal",
          "identical and reordered sets are equal (matched by key)",
          &value_eq_equal },
        { "value_eq_value_differs",
          "one differing value at a key -> not equal",
          &value_eq_value_differs },
        { "value_eq_keys_differ",
          "differing key set -> not equal (congruence gate)",
          &value_eq_keys_differ },
        { "value_eq_absent_carrier",
          "absent==absent at a key; present vs absent -> not equal",
          &value_eq_absent_carrier },
        { "value_eq_empty",
          "two empty sets are (vacuously) equal",
          &value_eq_empty },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
