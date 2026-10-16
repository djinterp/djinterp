/*******************************************************************************
* djinterp [core]                                  decode_tests_decodability.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_tests_decodability.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_is_leaf_decodable
  is_leaf_decodable is true for exactly the leaves this header can read
  Tests the following:
  - the member surface makes a non-built-in type decodable
  - each built-in family is decodable: bool, an <=8-byte integral, the character
    types, an enum, and a 4-/8-byte floating type
  - a non-leaf is not decodable: void, an aggregate with no decode, a pointer
  - a type whose decode has the wrong return type is not decodable
  - a wide floating type is decodable iff its width is 4 or 8 (the width gate)
  - the trait strips cv / reference qualifiers before deciding
*/
bool
tests_decode_is_leaf_decodable()
{
    // member surface
    D_DE_CHECK(is_leaf_decodable<rt_leaf>::value);

    // built-in families
    D_DE_CHECK(is_leaf_decodable<bool>::value);
    D_DE_CHECK(is_leaf_decodable<std::uint8_t>::value);
    D_DE_CHECK(is_leaf_decodable<std::int16_t>::value);
    D_DE_CHECK(is_leaf_decodable<std::uint32_t>::value);
    D_DE_CHECK(is_leaf_decodable<std::int64_t>::value);
    D_DE_CHECK(is_leaf_decodable<char>::value);
    D_DE_CHECK(is_leaf_decodable<char16_t>::value);
    D_DE_CHECK(is_leaf_decodable<char32_t>::value);
    D_DE_CHECK(is_leaf_decodable<wchar_t>::value);
    D_DE_CHECK(is_leaf_decodable<dec_enum_u16>::value);
    D_DE_CHECK(is_leaf_decodable<float>::value);
    D_DE_CHECK(is_leaf_decodable<double>::value);

    // non-leaves
    D_DE_CHECK(!is_leaf_decodable<void>::value);
    D_DE_CHECK(!is_leaf_decodable<dec_no_decode>::value);
    D_DE_CHECK(!is_leaf_decodable<dec_wrong_decode>::value);
    D_DE_CHECK(!is_leaf_decodable<int*>::value);

    // wide float: leaf only at width 4 or 8
    D_DE_CHECK(is_leaf_decodable<long double>::value
               == (sizeof(long double) == 4 || sizeof(long double) == 8));

    // cv / reference stripping
    D_DE_CHECK(is_leaf_decodable<const int>::value);
    D_DE_CHECK(is_leaf_decodable<int&>::value);
    D_DE_CHECK(is_leaf_decodable<const volatile double&>::value);

    return true;
}

/*
tests_decode_is_leaf_decodable_v
  the is_leaf_decodable_v companion mirrors the trait's value
  Tests the following:
  - _v equals ::value on decodable leaves (bool, integral, float, enum, member)
  - _v equals ::value on non-decodable types (void, aggregate with no decode)
*/
bool
tests_decode_is_leaf_decodable_v()
{
    D_DE_CHECK(is_leaf_decodable_v<bool> == is_leaf_decodable<bool>::value);
    D_DE_CHECK(is_leaf_decodable_v<std::uint32_t> == true);
    D_DE_CHECK(is_leaf_decodable_v<double> == true);
    D_DE_CHECK(is_leaf_decodable_v<dec_enum_u16> == true);
    D_DE_CHECK(is_leaf_decodable_v<rt_leaf> == true);

    D_DE_CHECK(is_leaf_decodable_v<void> == false);
    D_DE_CHECK(is_leaf_decodable_v<dec_no_decode> == false);

    return true;
}

NS_END  // testing
NS_END  // djinterp
