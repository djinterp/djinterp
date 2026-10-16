/*******************************************************************************
* djinterp [core]                                    carrier_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/meta/carrier_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "carrier_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_carrier_is_type_carrier
  Verifies is_type_carrier accepts type_t specializations (the raw success
  specialization) and cleans cv-ref on its argument.
  Tests the following:
  - a type_t over a builtin, a user type, and void is detected
  - const / reference / const-reference carriers are detected (clean in face)
*/
bool
tests_carrier_is_type_carrier()
{
    static_assert(is_type_carrier<type_t<int>>::value, "type_t<int>");
    static_assert(is_type_carrier<type_t<sample>>::value, "type_t<sample>");
    static_assert(is_type_carrier<type_t<void>>::value, "type_t<void>");
    static_assert(is_type_carrier<const type_t<int>>::value, "const carrier");
    static_assert(is_type_carrier<type_t<int>&>::value, "reference carrier");
    static_assert(is_type_carrier<const type_t<int>&>::value, "const-ref carrier");

    D_CR_CHECK(is_type_carrier_v<type_t<int>>        == true);
    D_CR_CHECK(is_type_carrier_v<const type_t<int>&> == true);

    return true;
}


/*
tests_carrier_is_type_carrier_rejects
  Verifies is_type_carrier rejects non-carriers, including a decoy that has a
  nested `type` (so detection is specialization-based, not duck-typed).
  Tests the following:
  - plain builtins and a user type are rejected
  - the decoy (nested `type`, but not a type_t) is rejected
  - a value carrier is not a type carrier (C++17)
*/
bool
tests_carrier_is_type_carrier_rejects()
{
    static_assert(!is_type_carrier<int>::value, "int");
    static_assert(!is_type_carrier<double>::value, "double");
    static_assert(!is_type_carrier<sample>::value, "sample");
    static_assert(!is_type_carrier<decoy>::value,
                  "decoy has ::type but is not type_t");

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(!is_type_carrier<val_t<5>>::value,
                  "a value carrier is not a type carrier");
#endif

    D_CR_CHECK(is_type_carrier_v<int>   == false);
    D_CR_CHECK(is_type_carrier_v<decoy> == false);

    return true;
}


/*
tests_carrier_is_value_carrier
  Verifies is_value_carrier accepts val_t specializations (the C++17 raw
  success specialization) and cleans cv-ref.
  Tests the following:
  - a val_t over int, bool, and an enumerator is detected
  - const / reference / const-reference carriers are detected
*/
bool
tests_carrier_is_value_carrier()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(is_value_carrier<val_t<5>>::value, "val_t<5>");
    static_assert(is_value_carrier<val_t<true>>::value, "val_t<true>");
    static_assert(is_value_carrier<val_t<color::green>>::value, "val_t<enum>");
    static_assert(is_value_carrier<const val_t<5>>::value, "const carrier");
    static_assert(is_value_carrier<val_t<5>&>::value, "reference carrier");
    static_assert(is_value_carrier<const val_t<5>&>::value, "const-ref carrier");

    D_CR_CHECK(is_value_carrier_v<val_t<5>>        == true);
    D_CR_CHECK(is_value_carrier_v<const val_t<5>&> == true);

#endif

    return true;
}


/*
tests_carrier_is_value_carrier_rejects
  Verifies is_value_carrier rejects non-carriers, including a decoy that has a
  static `value` (so detection is specialization-based, not duck-typed). These
  rejections hold at every floor: at the C++14 floor is_value_carrier is
  always false, which this also confirms.
  Tests the following:
  - a builtin, a type carrier, and the decoy are all rejected
*/
bool
tests_carrier_is_value_carrier_rejects()
{
    static_assert(!is_value_carrier<int>::value, "int");
    static_assert(!is_value_carrier<type_t<int>>::value,
                  "a type carrier is not a value carrier");
    static_assert(!is_value_carrier<decoy>::value,
                  "decoy has ::value but is not val_t");

    D_CR_CHECK(is_value_carrier_v<int>         == false);
    D_CR_CHECK(is_value_carrier_v<type_t<int>> == false);

    return true;
}


/*
tests_carrier_is_carrier_accepts_type
  Verifies is_carrier is satisfied via its FIRST disjunct (the type-carrier
  branch) and cleans cv-ref.
  Tests the following:
  - a type carrier satisfies is_carrier
  - const / reference carriers satisfy it too
*/
bool
tests_carrier_is_carrier_accepts_type()
{
    static_assert(is_carrier<type_t<int>>::value, "type carrier");
    static_assert(is_carrier<const type_t<int>>::value, "const type carrier");
    static_assert(is_carrier<type_t<int>&>::value, "reference type carrier");

    D_CR_CHECK(is_carrier_v<type_t<int>> == true);

    return true;
}


/*
tests_carrier_is_carrier_accepts_value
  Verifies is_carrier is satisfied via its SECOND disjunct (the value-carrier
  branch), which requires the first disjunct to be false.
  Tests the following:
  - a value carrier satisfies is_carrier
  - the same value carrier is NOT a type carrier (confirming the second
    disjunct is what carried the result)
*/
bool
tests_carrier_is_carrier_accepts_value()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(is_carrier<val_t<5>>::value, "value carrier");
    static_assert(!is_type_carrier<val_t<5>>::value,
                  "first disjunct is false for a value carrier");
    static_assert(is_carrier<const val_t<5>&>::value, "const-ref value carrier");

    D_CR_CHECK(is_carrier_v<val_t<5>> == true);

#endif

    return true;
}


/*
tests_carrier_is_carrier_rejects
  Verifies is_carrier is false when BOTH disjuncts are false.
  Tests the following:
  - a builtin, the decoy, and a user type are all rejected
*/
bool
tests_carrier_is_carrier_rejects()
{
    static_assert(!is_carrier<int>::value, "int");
    static_assert(!is_carrier<decoy>::value, "decoy");
    static_assert(!is_carrier<sample>::value, "sample");

    D_CR_CHECK(is_carrier_v<int>   == false);
    D_CR_CHECK(is_carrier_v<decoy> == false);

    return true;
}


/*
tests_carrier_v_companions
  Verifies each `_v` companion agrees with its trait's ::value member, across
  positive and negative inputs.
  Tests the following:
  - is_type_carrier_v / is_value_carrier_v / is_carrier_v all mirror ::value
*/
bool
tests_carrier_v_companions()
{
    static_assert(is_type_carrier_v<type_t<int>> ==
                  is_type_carrier<type_t<int>>::value, "type_v positive");
    static_assert(is_type_carrier_v<int> ==
                  is_type_carrier<int>::value, "type_v negative");
    static_assert(is_carrier_v<type_t<int>> ==
                  is_carrier<type_t<int>>::value, "carrier_v positive");
    static_assert(is_carrier_v<int> ==
                  is_carrier<int>::value, "carrier_v negative");
    static_assert(is_value_carrier_v<int> ==
                  is_value_carrier<int>::value, "value_v negative");

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(is_value_carrier_v<val_t<5>> ==
                  is_value_carrier<val_t<5>>::value, "value_v positive");
#endif

    D_CR_CHECK(is_type_carrier_v<type_t<int>> ==
               is_type_carrier<type_t<int>>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
