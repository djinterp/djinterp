/*******************************************************************************
* djinterp [core]                                        encode_tests_medium.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_tests_medium.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_medium_types
  the three medium type aliases are exactly what the model's B* requires
  Tests the following:
  - byte is an unsigned octet (unsigned char, one byte wide)
  - byte_string is the flat run of bytes std::vector<byte>
  - encode_length_type is an 8-byte unsigned integer (the leading-count width)
  - a byte_string composes from byte values and reads them back in order
*/
bool
tests_encode_medium_types()
{
    // byte: one unsigned octet
    D_EN_CHECK((std::is_same<byte, unsigned char>::value));
    D_EN_CHECK(sizeof(byte) == 1);
    D_EN_CHECK(!std::is_signed<byte>::value);

    // byte_string: the concrete B* - a growable run of bytes
    D_EN_CHECK((std::is_same<byte_string, std::vector<byte>>::value));

    // encode_length_type: the width-fixed unsigned leading count
    D_EN_CHECK((std::is_same<encode_length_type, std::uint64_t>::value));
    D_EN_CHECK(sizeof(encode_length_type) == 8);
    D_EN_CHECK(!std::is_signed<encode_length_type>::value);

    // a byte_string carries byte values in order
    byte_string _b;
    _b.push_back(static_cast<byte>(0x01));
    _b.push_back(static_cast<byte>(0xFE));
    D_EN_CHECK(_b.size() == 2);
    D_EN_CHECK(_b[0] == static_cast<byte>(0x01));
    D_EN_CHECK(_b[1] == static_cast<byte>(0xFE));

    return true;
}

NS_END  // testing
NS_END  // djinterp
