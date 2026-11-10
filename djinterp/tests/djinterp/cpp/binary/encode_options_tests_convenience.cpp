/*******************************************************************************
* djinterp [cpp]                            encode_options_tests_convenience.cpp
*
*
* path:      /tests/djinterp/cpp/binary/encode_options_tests_convenience.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
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
bool tests_encode_options_convenience()
{
    // fresh buffer each call
    const byte_string _a =
        encode_leaf<DSerialEndian::big>(static_cast<re_std::uint16_t>(0x1234));
    const byte_string _b =
        encode_leaf<DSerialEndian::big>(static_cast<re_std::uint16_t>(0x1234));
    D_EO_CHECK((_a == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK((_b == byte_string{ 0x12, 0x34 }));
    D_EO_CHECK(_a.size() == 2);

    // agrees with a manual encode_leaf_into, big-endian
    {
        DEoSink _s;
        encode_leaf_into<DSerialEndian::big>(_s, static_cast<re_std::uint32_t>(0xDEADBEEF));
        D_EO_CHECK(encode_leaf<DSerialEndian::big>(static_cast<re_std::uint32_t>(0xDEADBEEF))
                   == _s.data);
    }
    // agrees with a manual encode_leaf_into, little-endian
    {
        DEoSink _s;
        encode_leaf_into<DSerialEndian::little>(_s, static_cast<re_std::uint32_t>(0xDEADBEEF));
        D_EO_CHECK(encode_leaf<DSerialEndian::little>(static_cast<re_std::uint32_t>(0xDEADBEEF))
                   == _s.data);
    }

    // the option reaches the produced bytes: the two orders differ
    D_EO_CHECK(encode_leaf<DSerialEndian::big>(static_cast<re_std::uint32_t>(0xDEADBEEF))
               != encode_leaf<DSerialEndian::little>(static_cast<re_std::uint32_t>(0xDEADBEEF)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
