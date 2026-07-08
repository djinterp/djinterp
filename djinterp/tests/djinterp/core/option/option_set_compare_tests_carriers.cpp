/******************************************************************************
* djinterp [test]                         option_set_compare_tests_carriers.cpp
*
*   Section IV of the option_set_compare.hpp suite: the value-carrier sentinels
* and their structural equality.
*
*     value_absent          - the "no value of the requested kind" carrier
*                             (::has_value == false).
*     value_present<V>      - carries an extracted NTTP (::has_value == true,
*                             ::value == V, ::value_type == decltype(V)).
*     internal::carrier_eq  - structural equality over the carrier interface:
*                             absent==absent, present==present iff the VALUES
*                             match, and mixed absent/present is unequal.
*
*   The interface these define is exactly what an extractor must yield and what
* option_set_value_eq compares at each key (Section V).  One deliberate edge:
* carrier_eq compares the carried VALUE, not the value's type, so value_present
* over 5 (int) and 5u (unsigned) are equal.  carrier_eq is internal:: and is
* reached through a TU-local alias.
*
*
* path:      /tests/djinterp/core/option/option_set_compare_tests_carriers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_set_compare_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// carriers_value_absent
//   value_absent is the empty carrier: has_value is false.
bool
carriers_value_absent()
{
    constexpr bool ok = (value_absent::has_value == false);

    static_assert(ok, "value_absent::has_value == false");
    return ok;
}

// carriers_value_present
//   value_present<V> carries the value and its type: has_value true, value == V,
// value_type == decltype(V).
bool
carriers_value_present()
{
    constexpr bool ok =
        value_present<5>::has_value                                            &&
        (value_present<5>::value == 5)                                         &&
        std::is_same<value_present<5>::value_type, int>::value                 &&
        std::is_same<value_present<'x'>::value_type, char>::value;

    static_assert(ok, "value_present<V>: has_value, value == V, value_type == decltype(V)");
    return ok;
}

// carriers_carrier_eq
//   carrier_eq is structural over the carrier interface: absent==absent,
// present==present iff the values are equal, and any absent/present mix is
// unequal.  The equality is value-based, so 5 and 5u (different types, same
// value) are equal.
bool
carriers_carrier_eq()
{
    constexpr bool ok =
        ic::carrier_eq<value_absent, value_absent>::value                      &&
        ic::carrier_eq<value_present<5>, value_present<5>>::value              &&
        !ic::carrier_eq<value_present<5>, value_present<6>>::value             &&
        !ic::carrier_eq<value_absent, value_present<5>>::value                 &&
        !ic::carrier_eq<value_present<5>, value_absent>::value                 &&
        ic::carrier_eq<value_present<5>, value_present<5u>>::value;

    static_assert(ok, "carrier_eq: absent==absent; present==present iff values match; value-based");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_compare_carriers_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "IV. value carriers";
    b.descriptor = "value_absent / value_present and structural carrier_eq (value-based)";
    b.tests = {
        { "carriers_value_absent",
          "value_absent::has_value == false",
          &carriers_value_absent },
        { "carriers_value_present",
          "value_present<V>: has_value, value == V, value_type == decltype(V)",
          &carriers_value_present },
        { "carriers_carrier_eq",
          "carrier_eq: absent/present structural equality, value-based",
          &carriers_carrier_eq },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
