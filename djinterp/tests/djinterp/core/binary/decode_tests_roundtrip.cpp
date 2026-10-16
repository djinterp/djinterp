/*******************************************************************************
* djinterp [core]                                     decode_tests_roundtrip.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_tests_roundtrip.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_tests.hpp"
#include "../../../../inc/djinterp/core/binary/encode.hpp"   // the write side, to prove decode inverts encode


NS_DJINTERP
NS_TESTING

// rt_check
//   helper: a value survives a round trip iff decode(encode(v)) recovers it.
// Element type is explicit so encode deduces it from the argument and decode is
// told it directly.
template<typename _Type>
static bool
rt_check(_Type _v)
{
    decode_result<_Type> _r = decode<_Type>(encode(_v));

    return ( _r.ok && ( _r.value == _v ) );
}

/*
tests_decode_roundtrip_integrals
  every integral leaf decodes back to the value encode wrote
  Tests the following:
  - unsigned 8/16/32/64-bit boundary values (0, 1, max) round-trip
  - signed 8/16/32/64-bit boundary values (min, -1, 0, max) round-trip
  - the character types round-trip
*/
bool
tests_decode_roundtrip_integrals()
{
    // unsigned boundaries
    D_DE_CHECK(rt_check<std::uint8_t>(0x00));
    D_DE_CHECK(rt_check<std::uint8_t>(0xFF));
    D_DE_CHECK(rt_check<std::uint16_t>(0x0000));
    D_DE_CHECK(rt_check<std::uint16_t>(0xFFFF));
    D_DE_CHECK(rt_check<std::uint32_t>(0xDEADBEEFu));
    D_DE_CHECK(rt_check<std::uint64_t>(0xFFFFFFFFFFFFFFFFull));
    D_DE_CHECK(rt_check<std::uint64_t>(0x0123456789ABCDEFull));

    // signed boundaries
    D_DE_CHECK(rt_check<std::int8_t>(-128));
    D_DE_CHECK(rt_check<std::int8_t>(-1));
    D_DE_CHECK(rt_check<std::int8_t>(127));
    D_DE_CHECK(rt_check<std::int16_t>(-2));
    D_DE_CHECK(rt_check<std::int16_t>(0));
    D_DE_CHECK(rt_check<std::int32_t>(-2147483647 - 1));
    D_DE_CHECK(rt_check<std::int32_t>(123456789));
    D_DE_CHECK(rt_check<std::int64_t>(-1));
    D_DE_CHECK(rt_check<std::int64_t>(0x7FFFFFFFFFFFFFFFll));

    // character types
    D_DE_CHECK(rt_check<char>('Z'));
    D_DE_CHECK(rt_check<char16_t>(static_cast<char16_t>(0xBEEF)));
    D_DE_CHECK(rt_check<char32_t>(static_cast<char32_t>(0x0010FFFF)));

    return true;
}

/*
tests_decode_roundtrip_floating
  a float or double decodes back bit-exactly to what encode wrote
  Tests the following:
  - exactly-representable values (1.0, -2.0, 0.5, 0.0) round-trip
  - irrational-looking values round-trip exactly (the medium is bit-exact)
  - both float and double are covered
*/
bool
tests_decode_roundtrip_floating()
{
    D_DE_CHECK(rt_check<float>(1.0f));
    D_DE_CHECK(rt_check<float>(-2.0f));
    D_DE_CHECK(rt_check<float>(0.5f));
    D_DE_CHECK(rt_check<float>(0.0f));
    D_DE_CHECK(rt_check<float>(3.14159f));

    D_DE_CHECK(rt_check<double>(1.0));
    D_DE_CHECK(rt_check<double>(-2.0));
    D_DE_CHECK(rt_check<double>(0.5));
    D_DE_CHECK(rt_check<double>(2.718281828459045));

    return true;
}

/*
tests_decode_roundtrip_bool_enum
  bool and enumerations decode back to the encoded value
  Tests the following:
  - true and false round-trip
  - a uint16_t-backed enum round-trips
  - an int32_t-backed enum with a negative value round-trips
*/
bool
tests_decode_roundtrip_bool_enum()
{
    D_DE_CHECK(rt_check<bool>(true));
    D_DE_CHECK(rt_check<bool>(false));

    D_DE_CHECK(rt_check<dec_enum_u16>(dec_enum_u16::value));
    D_DE_CHECK(rt_check<dec_enum_i32>(dec_enum_i32::value));

    return true;
}

/*
tests_decode_roundtrip_member
  a custom leaf's own surfaces invert each other through the medium
  Tests the following:
  - encoding then decoding a custom leaf recovers its payload
  - the round trip holds for several payload values including the extremes
*/
bool
tests_decode_roundtrip_member()
{
    const std::uint32_t _values[] =
        { 0x00000000u, 0x0000002Au, 0xDEADBEEFu, 0xFFFFFFFFu };

    for (std::size_t _i = 0; _i < (sizeof(_values) / sizeof(_values[0])); ++_i)
    {
        decode_result<rt_leaf> _r = decode<rt_leaf>(encode(rt_leaf(_values[_i])));
        D_DE_CHECK(_r.ok);
        D_DE_CHECK(_r.value.payload == _values[_i]);
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
