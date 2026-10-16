/*******************************************************************************
* djinterp [core]                           encode_options_tests_convenience.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_options_tests_convenience.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_options_convenience
  encode_leaf<E>(value) is enc_tau<E> realised as a fresh byte_string
  Tests the following:
  - it returns exactly what encode_leaf_into<E> would append (per byte order)
  - it allocates a fresh buffer each call (no accumulation across calls)
  - it agrees with a manual encode_leaf_into for both big and little endian
  - it carries the byte order into the produced bytes (big vs little differ)
*/
bool
tests_encode_options_convenience()
{
    // fresh buffer each call
    const byte_string _a =
        encode_leaf<serial_endian::big>(static_cast<std::uint16_t>(0x1234));
    const byte_string _b =
        encode_leaf<serial_endian::big>(static_cast<std::uint16_t>(0x1234));
    D_EO_CHECK((_a == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK((_b == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK(_a.size() == 2);

    // agrees with a manual encode_leaf_into, big-endian
    {
        eo_sink _s;
        encode_leaf_into<serial_endian::big>(_s, static_cast<std::uint32_t>(0xDEADBEEF));
        D_EO_CHECK(encode_leaf<serial_endian::big>(static_cast<std::uint32_t>(0xDEADBEEF))
                   == _s.data);
    }
    // agrees with a manual encode_leaf_into, little-endian
    {
        eo_sink _s;
        encode_leaf_into<serial_endian::little>(_s, static_cast<std::uint32_t>(0xDEADBEEF));
        D_EO_CHECK(encode_leaf<serial_endian::little>(static_cast<std::uint32_t>(0xDEADBEEF))
                   == _s.data);
    }

    // the option reaches the produced bytes: the two orders differ
    D_EO_CHECK(encode_leaf<serial_endian::big>(static_cast<std::uint32_t>(0xDEADBEEF))
               != encode_leaf<serial_endian::little>(static_cast<std::uint32_t>(0xDEADBEEF)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
