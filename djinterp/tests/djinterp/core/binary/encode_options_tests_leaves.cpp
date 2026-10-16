/*******************************************************************************
* djinterp [core]                                encode_options_tests_leaves.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_options_tests_leaves.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_options_tests.hpp"
#include "../../../../inc/djinterp/core/binary/encode.hpp"   // to assert the big-endian path reproduces enc_tau exactly


NS_DJINTERP
NS_TESTING

/*
tests_encode_options_leaf_bool
  the bool leaf is one byte, 0 or 1, regardless of byte order
  Tests the following:
  - encode_leaf<big>(true/false) is 0x01 / 0x00
  - encode_leaf<little> gives the same single byte (order cannot reorder one byte)
  - the leaf writes through a minimal push_back sink as well
*/
bool
tests_encode_options_leaf_bool()
{
    D_EO_CHECK((encode_leaf<serial_endian::big>(true) == byte_string{ 0x01 }));
    D_EO_CHECK((encode_leaf<serial_endian::big>(false) == byte_string{ 0x00 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(true) == byte_string{ 0x01 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(false) == byte_string{ 0x00 }));

    eo_sink _s;
    encode_leaf_into<serial_endian::little>(_s, true);
    D_EO_CHECK((_s.data == byte_string{ 0x01 }));

    return true;
}

/*
tests_encode_options_leaf_integrals
  integrals are written at their natural width in the chosen byte order
  Tests the following:
  - a multi-byte unsigned value reverses byte order between big and little
  - a signed value's two's-complement pattern reverses likewise
  - a one-byte value is identical under either order
  - INT32_MIN under little-endian is the byte-reversed big-endian field
*/
bool
tests_encode_options_leaf_integrals()
{
    // unsigned, order reverses
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::uint16_t>(0x1234))
                == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::uint16_t>(0x1234))
                == byte_string{ 0x34, 0x12 }));
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::uint32_t>(0xDEADBEEF))
                == byte_string{ 0xDE, 0xAD, 0xBE, 0xEF }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::uint32_t>(0xDEADBEEF))
                == byte_string{ 0xEF, 0xBE, 0xAD, 0xDE }));

    // signed, order reverses
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::int16_t>(-2))
                == byte_string{ 0xFF, 0xFE }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::int16_t>(-2))
                == byte_string{ 0xFE, 0xFF }));
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::int32_t>(-2147483647 - 1))
                == byte_string{ 0x80, 0x00, 0x00, 0x00 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::int32_t>(-2147483647 - 1))
                == byte_string{ 0x00, 0x00, 0x00, 0x80 }));

    // one byte is order-invariant
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::uint8_t>(0xFF))
                == byte_string{ 0xFF }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::uint8_t>(0xFF))
                == byte_string{ 0xFF }));

    // an all-ones 64-bit value is eight 0xFF bytes under either order
    D_EO_CHECK((encode_leaf<serial_endian::big>(static_cast<std::int64_t>(-1))
                == byte_string{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(static_cast<std::int64_t>(-1))
                == byte_string{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));

    return true;
}

/*
tests_encode_options_leaf_enums
  an enum is written as its underlying integer in the chosen byte order
  Tests the following:
  - a uint16_t-backed enum reverses byte order between big and little
  - an int32_t-backed enum's negative pattern reverses likewise
*/
bool
tests_encode_options_leaf_enums()
{
    D_EO_CHECK((encode_leaf<serial_endian::big>(eo_enum_u16::value)
                == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(eo_enum_u16::value)
                == byte_string{ 0x34, 0x12 }));

    D_EO_CHECK((encode_leaf<serial_endian::big>(eo_enum_i32::value)
                == byte_string{ 0xFF, 0xFE, 0x1D, 0xC0 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(eo_enum_i32::value)
                == byte_string{ 0xC0, 0x1D, 0xFE, 0xFF }));

    return true;
}

/*
tests_encode_options_leaf_floating
  a 4-/8-byte float is written through its bit pattern in the chosen byte order
  Tests the following:
  - float 1.0 / -2.0 reverse byte order between big and little
  - double 1.0 reverses likewise across all eight bytes
  - both the 4- and 8-byte writer branches are exercised under each order
*/
bool
tests_encode_options_leaf_floating()
{
    D_EO_CHECK((encode_leaf<serial_endian::big>(1.0f)
                == byte_string{ 0x3F, 0x80, 0x00, 0x00 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(1.0f)
                == byte_string{ 0x00, 0x00, 0x80, 0x3F }));
    D_EO_CHECK((encode_leaf<serial_endian::big>(-2.0f)
                == byte_string{ 0xC0, 0x00, 0x00, 0x00 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(-2.0f)
                == byte_string{ 0x00, 0x00, 0x00, 0xC0 }));

    D_EO_CHECK((encode_leaf<serial_endian::big>(1.0)
                == byte_string{ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(1.0)
                == byte_string{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F }));

    return true;
}

/*
tests_encode_options_leaf_member
  a custom leaf owns its byte layout - the byte order option does not touch it
  Tests the following:
  - the custom leaf emits its three-byte signature under big-endian
  - it emits the identical signature under little-endian (order is not applied)
  - it writes through a minimal push_back sink
*/
bool
tests_encode_options_leaf_member()
{
    D_EO_CHECK((encode_leaf<serial_endian::big>(eo_custom_leaf{})
                == byte_string{ 0xAB, 0xCD, 0xEF }));
    D_EO_CHECK((encode_leaf<serial_endian::little>(eo_custom_leaf{})
                == byte_string{ 0xAB, 0xCD, 0xEF }));

    eo_sink _s;
    encode_leaf_into<serial_endian::big>(_s, eo_custom_leaf{});
    D_EO_CHECK((_s.data == byte_string{ 0xAB, 0xCD, 0xEF }));

    return true;
}

/*
tests_encode_options_big_matches_encode
  encode_leaf<big> reproduces encode.hpp's fixed enc_tau byte for byte
  Tests the following:
  - the big-endian parameterised path equals encode(value) for a bool
  - it equals encode(value) for an unsigned and a signed integral
  - it equals encode(value) for an enum and for a float and a double
  - (the fixed encoder is exactly the big-endian instance of this one)
*/
bool
tests_encode_options_big_matches_encode()
{
    D_EO_CHECK(encode_leaf<serial_endian::big>(true) == encode(true));
    D_EO_CHECK(encode_leaf<serial_endian::big>(static_cast<std::uint32_t>(0xDEADBEEF))
               == encode(static_cast<std::uint32_t>(0xDEADBEEF)));
    D_EO_CHECK(encode_leaf<serial_endian::big>(static_cast<std::int32_t>(-2147483647 - 1))
               == encode(static_cast<std::int32_t>(-2147483647 - 1)));
    D_EO_CHECK(encode_leaf<serial_endian::big>(eo_enum_i32::value)
               == encode(eo_enum_i32::value));
    D_EO_CHECK(encode_leaf<serial_endian::big>(1.0f) == encode(1.0f));
    D_EO_CHECK(encode_leaf<serial_endian::big>(0.5) == encode(0.5));

    return true;
}

NS_END  // testing
NS_END  // djinterp
