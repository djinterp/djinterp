/*******************************************************************************
* djinterp [core]                                        encode_tests_leaves.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_tests_leaves.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_bool
  the bool leaf is exactly one byte, 0 or 1
  Tests the following:
  - encode(true) is the single byte 0x01
  - encode(false) is the single byte 0x00
  - the bool overload writes through an arbitrary push_back sink, not only
    byte_string
*/
bool
tests_encode_bool()
{
    D_EN_CHECK((encode(true)  == byte_string{ 0x01 }));
    D_EN_CHECK((encode(false) == byte_string{ 0x00 }));

    // writes through a minimal sink as well
    recording_sink _s;
    encode_into(_s, true);
    D_EN_CHECK((_s.data == byte_string{ 0x01 }));

    return true;
}

/*
tests_encode_unsigned_integrals
  unsigned integrals are written big-endian at their natural width
  Tests the following:
  - an 8-bit value occupies one byte (boundary values 0x00 and 0xFF)
  - a 16-bit value is two bytes, most-significant first
  - a 32-bit value is four bytes, most-significant first
  - a 64-bit value is eight bytes, most-significant first
  - put_uint_be is thereby exercised at widths 1 / 2 / 4 / 8
*/
bool
tests_encode_unsigned_integrals()
{
    D_EN_CHECK((encode(static_cast<std::uint8_t>(0x00)) == byte_string{ 0x00 }));
    D_EN_CHECK((encode(static_cast<std::uint8_t>(0xFF)) == byte_string{ 0xFF }));

    D_EN_CHECK((encode(static_cast<std::uint16_t>(0x1234))
                == byte_string{ 0x12, 0x34 }));

    D_EN_CHECK((encode(static_cast<std::uint32_t>(0xDEADBEEF))
                == byte_string{ 0xDE, 0xAD, 0xBE, 0xEF }));

    D_EN_CHECK((encode(static_cast<std::uint64_t>(0x0102030405060708ull))
                == byte_string{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));

    // maximal 64-bit value is eight 0xFF bytes
    D_EN_CHECK((encode(static_cast<std::uint64_t>(0xFFFFFFFFFFFFFFFFull))
                == byte_string{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));

    return true;
}

/*
tests_encode_signed_integrals
  signed integrals are written through their two's-complement pattern, big-endian
  Tests the following:
  - int8_t boundaries: -128 is 0x80, 127 is 0x7F, -1 is 0xFF
  - int16_t -2 is 0xFF 0xFE
  - int32_t INT32_MIN is 0x80 0x00 0x00 0x00
  - int64_t -1 is eight 0xFF bytes
  - a positive signed value encodes identically to its magnitude
*/
bool
tests_encode_signed_integrals()
{
    D_EN_CHECK((encode(static_cast<std::int8_t>(-128)) == byte_string{ 0x80 }));
    D_EN_CHECK((encode(static_cast<std::int8_t>(127))  == byte_string{ 0x7F }));
    D_EN_CHECK((encode(static_cast<std::int8_t>(-1))   == byte_string{ 0xFF }));

    D_EN_CHECK((encode(static_cast<std::int16_t>(-2))
                == byte_string{ 0xFF, 0xFE }));

    D_EN_CHECK((encode(static_cast<std::int32_t>(-2147483647 - 1))
                == byte_string{ 0x80, 0x00, 0x00, 0x00 }));

    D_EN_CHECK((encode(static_cast<std::int64_t>(-1))
                == byte_string{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));

    // a positive value shares the unsigned encoding
    D_EN_CHECK((encode(static_cast<std::int16_t>(0x1234))
                == byte_string{ 0x12, 0x34 }));

    return true;
}

/*
tests_encode_char_types
  the character types travel the integral path at their own fixed widths
  Tests the following:
  - char / signed char / unsigned char are one byte
  - char16_t is two bytes, big-endian
  - char32_t is four bytes, big-endian
  - wchar_t encodes big-endian at exactly sizeof(wchar_t) bytes (width-checked,
    since sizeof(wchar_t) is platform-dependent)
*/
bool
tests_encode_char_types()
{
    D_EN_CHECK((encode(static_cast<char>('A'))          == byte_string{ 0x41 }));
    D_EN_CHECK((encode(static_cast<signed char>(-1))    == byte_string{ 0xFF }));
    D_EN_CHECK((encode(static_cast<unsigned char>(0xFF)) == byte_string{ 0xFF }));

    D_EN_CHECK((encode(static_cast<char16_t>(0x1234))
                == byte_string{ 0x12, 0x34 }));

    D_EN_CHECK((encode(static_cast<char32_t>(0x00010000))
                == byte_string{ 0x00, 0x01, 0x00, 0x00 }));

    // wchar_t: width-checked and big-endian, whatever its platform width
    const byte_string _w = encode(static_cast<wchar_t>(0x41));
    D_EN_CHECK(_w.size() == sizeof(wchar_t));
    D_EN_CHECK(_w.back() == static_cast<byte>(0x41));       // low byte last (big-endian)
    for (std::size_t _i = 0; _i + 1 < _w.size(); ++_i)
    {
        D_EN_CHECK(_w[_i] == static_cast<byte>(0x00));      // leading bytes zero
    }

    return true;
}

/*
tests_encode_enums
  an enumeration encodes as the integer it names, through its underlying type
  Tests the following:
  - a uint16_t-backed scoped enum encodes as a 2-byte big-endian value
  - an int32_t-backed scoped enum with a negative value uses two's complement
  - an int8_t-backed scoped enum encodes negative and positive values in one byte
  - an unscoped enum with a fixed unsigned underlying type encodes as that integer
*/
bool
tests_encode_enums()
{
    D_EN_CHECK((encode(enc_enum_u16::value)
                == byte_string{ 0x12, 0x34 }));

    D_EN_CHECK((encode(enc_enum_i32::value)
                == byte_string{ 0xFF, 0xFE, 0x1D, 0xC0 }));

    D_EN_CHECK((encode(enc_enum_i8::neg) == byte_string{ 0xFE }));
    D_EN_CHECK((encode(enc_enum_i8::pos) == byte_string{ 0x05 }));

    D_EN_CHECK((encode(EU_A) == byte_string{ 0x2A }));

    return true;
}

/*
tests_encode_floating
  a 4- or 8-byte float encodes through its same-width unsigned bit pattern, big-endian
  Tests the following:
  - float 1.0 / -2.0 / 0.5 / 0.0 match their IEEE-754 single patterns
  - double 1.0 / -2.0 / 0.5 match their IEEE-754 double patterns
  - encode_floating_leaf is thereby exercised on both the 4- and 8-byte branches
*/
bool
tests_encode_floating()
{
    D_EN_CHECK((encode(1.0f)  == byte_string{ 0x3F, 0x80, 0x00, 0x00 }));
    D_EN_CHECK((encode(-2.0f) == byte_string{ 0xC0, 0x00, 0x00, 0x00 }));
    D_EN_CHECK((encode(0.5f)  == byte_string{ 0x3F, 0x00, 0x00, 0x00 }));
    D_EN_CHECK((encode(0.0f)  == byte_string{ 0x00, 0x00, 0x00, 0x00 }));

    D_EN_CHECK((encode(1.0)
                == byte_string{ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
    D_EN_CHECK((encode(-2.0)
                == byte_string{ 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
    D_EN_CHECK((encode(0.5)
                == byte_string{ 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

    return true;
}

/*
tests_encode_member_surface
  a leaf carrying its own encode_into writes itself, taking precedence
  Tests the following:
  - the custom leaf emits exactly its three-byte signature
  - it writes through the default byte_string convenience wrapper
  - it writes through a minimal push_back sink as well
*/
bool
tests_encode_member_surface()
{
    D_EN_CHECK((encode(enc_custom_leaf{}) == byte_string{ 0xAB, 0xCD, 0xEF }));

    recording_sink _s;
    encode_into(_s, enc_custom_leaf{});
    D_EN_CHECK((_s.data == byte_string{ 0xAB, 0xCD, 0xEF }));

    return true;
}

/*
tests_encode_writer_accumulation
  successive encode_into calls concatenate into one sink, no per-leaf temporary
  Tests the following:
  - three leaves of different families written into the same byte_string append
    in call order with no separators
  - the same holds for a minimal push_back sink
  - this is the container-encoder use of the sink (component bits into one buffer)
*/
bool
tests_encode_writer_accumulation()
{
    byte_string _buf;
    encode_into(_buf, static_cast<std::uint16_t>(0x1234));
    encode_into(_buf, true);
    encode_into(_buf, static_cast<std::int8_t>(-1));
    D_EN_CHECK((_buf == byte_string{ 0x12, 0x34, 0x01, 0xFF }));

    recording_sink _s;
    encode_into(_s, static_cast<std::uint8_t>(0xAA));
    encode_into(_s, enc_custom_leaf{});
    D_EN_CHECK((_s.data == byte_string{ 0xAA, 0xAB, 0xCD, 0xEF }));

    return true;
}

NS_END  // testing
NS_END  // djinterp
