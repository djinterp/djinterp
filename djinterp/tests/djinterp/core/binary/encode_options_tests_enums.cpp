/*******************************************************************************
* djinterp [core]                                 encode_options_tests_enums.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_options_tests_enums.cpp
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
  - serial_endian names two distinct orders (big, little)
  - serial_length names four distinct widths (u16, u32, u64, varint)
  - serial_fidelity names four distinct levels (native, sequence, bag, set)
  - distinct enumerators compare unequal (they are separate positions)
*/
bool
tests_encode_options_enums()
{
    // serial_endian: two positions, distinct
    D_EO_CHECK(serial_endian::big != serial_endian::little);

    // serial_length: four positions, pairwise distinct
    D_EO_CHECK(serial_length::u16 != serial_length::u32);
    D_EO_CHECK(serial_length::u32 != serial_length::u64);
    D_EO_CHECK(serial_length::u64 != serial_length::varint);
    D_EO_CHECK(serial_length::u16 != serial_length::varint);

    // serial_fidelity: four positions, pairwise distinct
    D_EO_CHECK(serial_fidelity::native != serial_fidelity::sequence);
    D_EO_CHECK(serial_fidelity::sequence != serial_fidelity::bag);
    D_EO_CHECK(serial_fidelity::bag != serial_fidelity::set);
    D_EO_CHECK(serial_fidelity::native != serial_fidelity::set);

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
bool
tests_encode_options_defaults()
{
    D_EO_CHECK(default_endian == serial_endian::big);
    D_EO_CHECK(default_length == serial_length::u64);
    D_EO_CHECK(default_fidelity == serial_fidelity::native);

    // constant-expression usability
    static_assert(default_endian == serial_endian::big, "default endian is big");
    static_assert(default_length == serial_length::u64, "default length is u64");
    static_assert(default_fidelity == serial_fidelity::native, "default fidelity is native");

    return true;
}

NS_END  // testing
NS_END  // djinterp
