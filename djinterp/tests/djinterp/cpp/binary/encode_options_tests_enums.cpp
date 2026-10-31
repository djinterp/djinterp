/*******************************************************************************
* djinterp [cpp]                                  encode_options_tests_enums.cpp
*
*
* path:      /tests/djinterp/cpp/binary/encode_options_tests_enums.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_options_enums
  the three serial parameter enums enumerate the documented, distinct positions
  Tests the following:
  - DSerialEndian names two distinct orders (big, little)
  - DSerialLength names four distinct widths (u16, u32, u64, varint)
  - DSerialFidelity names four distinct levels (native, sequence, bag, set)
  - distinct enumerators compare unequal (they are separate positions)
*/
bool tests_encode_options_enums()
{
    // DSerialEndian: two positions, distinct
    D_EO_CHECK(DSerialEndian::big != DSerialEndian::little);

    // DSerialLength: four positions, pairwise distinct
    D_EO_CHECK(DSerialLength::u16 != DSerialLength::u32);
    D_EO_CHECK(DSerialLength::u32 != DSerialLength::u64);
    D_EO_CHECK(DSerialLength::u64 != DSerialLength::varint);
    D_EO_CHECK(DSerialLength::u16 != DSerialLength::varint);

    // DSerialFidelity: four positions, pairwise distinct
    D_EO_CHECK(DSerialFidelity::native != DSerialFidelity::sequence);
    D_EO_CHECK(DSerialFidelity::sequence != DSerialFidelity::bag);
    D_EO_CHECK(DSerialFidelity::bag != DSerialFidelity::set);
    D_EO_CHECK(DSerialFidelity::native != DSerialFidelity::set);

    return true;
}

/*
tests_encode_options_defaults
  the default_* constants are the positions encode.hpp realises
  Tests the following:
  - default_endian is big (encode.hpp writes most-significant first)
  - default_length is u64 (encode.hpp writes an 8-byte leading count)
  - default_fidelity is native (lossless)
  - the constants are usable in a constant expression (compile-time values)
*/
bool tests_encode_options_defaults()
{
    D_EO_CHECK(default_endian == DSerialEndian::big);
    D_EO_CHECK(default_length == DSerialLength::u64);
    D_EO_CHECK(default_fidelity == DSerialFidelity::native);

    // constant-expression usability
    static_assert(default_endian == DSerialEndian::big, "default endian is big");
    static_assert(default_length == DSerialLength::u64, "default length is u64");
    static_assert(default_fidelity == DSerialFidelity::native, "default fidelity is native");

    return true;
}

NS_END  // testing
NS_END  // djinterp
