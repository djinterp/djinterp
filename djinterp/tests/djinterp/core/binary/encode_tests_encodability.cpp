/*******************************************************************************
* djinterp [core]                                  encode_tests_encodability.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_tests_encodability.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_is_leaf_encodable
  is_leaf_encodable is true for exactly the leaves this header can write
  Tests the following:
  - the member surface makes a non-built-in type encodable
  - each built-in family is encodable: bool, an <=8-byte integral, the character
    types, an enum, and a 4-/8-byte floating type
  - a non-leaf is not encodable: void, an ordinary aggregate, a pointer
  - a wide floating type is encodable iff its width is 4 or 8 (the width gate)
  - the trait strips cv / reference qualifiers before deciding
*/
bool
tests_encode_is_leaf_encodable()
{
    // member surface
    D_EN_CHECK(is_leaf_encodable<enc_custom_leaf>::value);

    // built-in families
    D_EN_CHECK(is_leaf_encodable<bool>::value);
    D_EN_CHECK(is_leaf_encodable<std::uint8_t>::value);
    D_EN_CHECK(is_leaf_encodable<std::int16_t>::value);
    D_EN_CHECK(is_leaf_encodable<std::uint32_t>::value);
    D_EN_CHECK(is_leaf_encodable<std::int64_t>::value);
    D_EN_CHECK(is_leaf_encodable<char>::value);
    D_EN_CHECK(is_leaf_encodable<char16_t>::value);
    D_EN_CHECK(is_leaf_encodable<char32_t>::value);
    D_EN_CHECK(is_leaf_encodable<wchar_t>::value);
    D_EN_CHECK(is_leaf_encodable<enc_enum_u16>::value);
    D_EN_CHECK(is_leaf_encodable<enc_enum_unscoped>::value);
    D_EN_CHECK(is_leaf_encodable<float>::value);
    D_EN_CHECK(is_leaf_encodable<double>::value);

    // non-leaves: the "no element" void, a plain aggregate, a pointer
    D_EN_CHECK(!is_leaf_encodable<void>::value);
    D_EN_CHECK(!is_leaf_encodable<enc_plain>::value);
    D_EN_CHECK(!is_leaf_encodable<int*>::value);

    // a wide float is a leaf only at width 4 or 8 (false on platforms where
    // long double is 10/12/16 bytes; true where it is 8)
    D_EN_CHECK(is_leaf_encodable<long double>::value
               == (sizeof(long double) == 4 || sizeof(long double) == 8));

    // cv / reference stripping
    D_EN_CHECK(is_leaf_encodable<const int>::value);
    D_EN_CHECK(is_leaf_encodable<int&>::value);
    D_EN_CHECK(is_leaf_encodable<const volatile double&>::value);

    return true;
}

/*
tests_encode_is_leaf_encodable_v
  the is_leaf_encodable_v companion mirrors the trait's value
  Tests the following:
  - _v equals ::value on encodable leaves (bool, integral, float, enum, member)
  - _v equals ::value on non-encodable types (void, plain aggregate)
*/
bool
tests_encode_is_leaf_encodable_v()
{
    D_EN_CHECK(is_leaf_encodable_v<bool> == is_leaf_encodable<bool>::value);
    D_EN_CHECK(is_leaf_encodable_v<std::uint32_t> == true);
    D_EN_CHECK(is_leaf_encodable_v<double> == true);
    D_EN_CHECK(is_leaf_encodable_v<enc_enum_u16> == true);
    D_EN_CHECK(is_leaf_encodable_v<enc_custom_leaf> == true);

    D_EN_CHECK(is_leaf_encodable_v<void> == false);
    D_EN_CHECK(is_leaf_encodable_v<enc_plain> == false);

    return true;
}

NS_END  // testing
NS_END  // djinterp
