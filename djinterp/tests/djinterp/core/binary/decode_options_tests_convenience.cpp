/*******************************************************************************
* djinterp [core]                           decode_options_tests_convenience.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_options_tests_convenience.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_options_convenience
  decode_leaf<E>(byte_string) wraps a reader and reads one leaf from the front
  Tests the following:
  - a whole-buffer decode returns the leading value in the chosen order
  - only the leading leaf is consumed; trailing bytes are ignored
  - a too-short buffer fails with ok = false
  - the byte order reaches the result (the same bytes differ big vs little)
*/
bool
tests_decode_options_convenience()
{
    // whole-buffer decode, big-endian
    D_DO_CHECK((decode_leaf<serial_endian::big, std::uint16_t>(byte_string{ 0x12, 0x34 }).value == 0x1234u));
    // whole-buffer decode, little-endian
    D_DO_CHECK((decode_leaf<serial_endian::little, std::uint16_t>(byte_string{ 0x34, 0x12 }).value == 0x1234u));

    // leading leaf only - a trailing byte is left unread
    decode_result<std::uint8_t> _lead =
        decode_leaf<serial_endian::big, std::uint8_t>(byte_string{ 0xAB, 0xCD });
    D_DO_CHECK(_lead.ok);
    D_DO_CHECK(_lead.value == 0xABu);

    // too short
    D_DO_CHECK((!decode_leaf<serial_endian::little, std::uint32_t>(byte_string{ 0x01, 0x02 }).ok));

    // the same two bytes decode to different values under the two orders
    const byte_string _b = { 0x12, 0x34 };
    D_DO_CHECK(decode_leaf<serial_endian::big, std::uint16_t>(_b).value
               != decode_leaf<serial_endian::little, std::uint16_t>(_b).value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
