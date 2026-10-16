/*******************************************************************************
* djinterp [core]                                   encode_tests_convenience.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_tests_convenience.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_convenience
  encode(value) is enc_tau realised as a total map to a fresh byte_string
  Tests the following:
  - encode returns a byte_string holding exactly what encode_into would append
  - it allocates a fresh buffer each call (successive calls do not accumulate)
  - it agrees with encode_into across the leaf families (bool, integral, float,
    enum, member surface)
  - encoding a value into an empty buffer and calling encode give equal results
*/
bool
tests_encode_convenience()
{
    // a fresh buffer each call - no accumulation across calls
    const byte_string _first  = encode(static_cast<std::uint16_t>(0x1234));
    const byte_string _second = encode(static_cast<std::uint16_t>(0x1234));
    D_EN_CHECK((_first == byte_string{ 0x12, 0x34 }));
    D_EN_CHECK((_second == byte_string{ 0x12, 0x34 }));
    D_EN_CHECK(_first.size() == 2);

    // encode agrees with a manual encode_into into an empty buffer, per family
    {
        byte_string _manual;
        encode_into(_manual, true);
        D_EN_CHECK(encode(true) == _manual);
    }
    {
        byte_string _manual;
        encode_into(_manual, static_cast<std::int32_t>(-2147483647 - 1));
        D_EN_CHECK(encode(static_cast<std::int32_t>(-2147483647 - 1)) == _manual);
    }
    {
        byte_string _manual;
        encode_into(_manual, 0.5);
        D_EN_CHECK(encode(0.5) == _manual);
    }
    {
        byte_string _manual;
        encode_into(_manual, enc_enum_u16::value);
        D_EN_CHECK(encode(enc_enum_u16::value) == _manual);
    }
    {
        byte_string _manual;
        encode_into(_manual, enc_custom_leaf{});
        D_EN_CHECK(encode(enc_custom_leaf{}) == _manual);
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
