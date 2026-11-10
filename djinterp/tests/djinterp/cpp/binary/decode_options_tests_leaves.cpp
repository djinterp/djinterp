/*******************************************************************************
* djinterp [cpp]                                 decode_options_tests_leaves.cpp
*
*
* path:      /tests/djinterp/cpp/binary/decode_options_tests_leaves.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"
#include "../../../../inc/djinterp/cpp/binary/decode.hpp"   // to assert the big-endian path reproduces decode.hpp's dec_tau


NS_DJINTERP
NS_TESTING

// dl
//   helper: decode one leaf of `_Type` under byte order `_E` from a fresh reader
// over `_bytes`, returning the partial result for inspection.
template<DSerialEndian _E,
         typename       _Type>
static
DDecodeResult<_Type> dl(
    const byte_string& _bytes
)
{
    DByteReader _r(_bytes);

    return decode_leaf<_E, _Type>(_r);
}

/*
tests_decode_options_leaf_bool
  the bool leaf reads one byte, order-independent
  Tests the following:
  - 0x01 decodes to true and 0x00 to false under big-endian
  - the same holds under little-endian (one byte cannot be reordered)
  - a successful bool decode consumes exactly one byte
*/
bool tests_decode_options_leaf_bool()
{
    D_DO_CHECK((dl<DSerialEndian::big, bool>(byte_string{ 0x01 }).value == true));
    D_DO_CHECK((dl<DSerialEndian::big, bool>(byte_string{ 0x00 }).value == false));
    D_DO_CHECK((dl<DSerialEndian::little, bool>(byte_string{ 0x01 }).value == true));
    D_DO_CHECK((dl<DSerialEndian::little, bool>(byte_string{ 0x00 }).value == false));

    const byte_string _b = { 0x01 };
    DByteReader _r(_b);
    DDecodeResult<bool> _res = decode_leaf<DSerialEndian::big, bool>(_r);
    D_DO_CHECK(_res.ok);
    D_DO_CHECK(_r.exhausted());

    return true;
}

/*
tests_decode_options_leaf_integrals
  integrals are read at their natural width in the chosen byte order
  Tests the following:
  - a multi-byte unsigned value reads from either order to the same number
  - a signed two's-complement value reads from either order likewise
  - INT32_MIN reads from its big field and its byte-reversed little field
  - a one-byte value is order-invariant
*/
bool tests_decode_options_leaf_integrals()
{
    // unsigned, both orders yield the same value
    D_DO_CHECK((dl<DSerialEndian::big, re_std::uint16_t>(byte_string{ 0x12, 0x34 }).value == 0x1234u));
    D_DO_CHECK((dl<DSerialEndian::little, re_std::uint16_t>(byte_string{ 0x34, 0x12 }).value == 0x1234u));
    D_DO_CHECK((dl<DSerialEndian::big, re_std::uint32_t>(byte_string{ 0xDE, 0xAD, 0xBE, 0xEF }).value == 0xDEADBEEFu));
    D_DO_CHECK((dl<DSerialEndian::little, re_std::uint32_t>(byte_string{ 0xEF, 0xBE, 0xAD, 0xDE }).value == 0xDEADBEEFu));

    // signed
    D_DO_CHECK((dl<DSerialEndian::big, re_std::int16_t>(byte_string{ 0xFF, 0xFE }).value == static_cast<re_std::int16_t>(-2)));
    D_DO_CHECK((dl<DSerialEndian::little, re_std::int16_t>(byte_string{ 0xFE, 0xFF }).value == static_cast<re_std::int16_t>(-2)));
    D_DO_CHECK((dl<DSerialEndian::big, re_std::int32_t>(byte_string{ 0x80, 0x00, 0x00, 0x00 }).value == (-2147483647 - 1)));
    D_DO_CHECK((dl<DSerialEndian::little, re_std::int32_t>(byte_string{ 0x00, 0x00, 0x00, 0x80 }).value == (-2147483647 - 1)));

    // one byte, order-invariant
    D_DO_CHECK((dl<DSerialEndian::big, re_std::uint8_t>(byte_string{ 0xFF }).value == 0xFFu));
    D_DO_CHECK((dl<DSerialEndian::little, re_std::uint8_t>(byte_string{ 0xFF }).value == 0xFFu));

    return true;
}

/*
tests_decode_options_leaf_enums
  an enum is read through its underlying integer in the chosen byte order
  Tests the following:
  - a uint16_t-backed enum reads from its big and little fields
  - an int32_t-backed enum's negative pattern reads from either order
*/
bool tests_decode_options_leaf_enums()
{
    D_DO_CHECK((dl<DSerialEndian::big, DDoEnumU16>(byte_string{ 0x12, 0x34 }).value == DDoEnumU16::value));
    D_DO_CHECK((dl<DSerialEndian::little, DDoEnumU16>(byte_string{ 0x34, 0x12 }).value == DDoEnumU16::value));

    D_DO_CHECK((dl<DSerialEndian::big, DDoEnumI32>(byte_string{ 0xFF, 0xFE, 0x1D, 0xC0 }).value == DDoEnumI32::value));
    D_DO_CHECK((dl<DSerialEndian::little, DDoEnumI32>(byte_string{ 0xC0, 0x1D, 0xFE, 0xFF }).value == DDoEnumI32::value));

    return true;
}

/*
tests_decode_options_leaf_floating
  a 4-/8-byte float is read through its bit pattern in the chosen byte order
  Tests the following:
  - float 1.0 reads from its big and little fields
  - double 1.0 reads from its big and little fields
  - both the 4- and 8-byte reader branches are exercised under each order
*/
bool tests_decode_options_leaf_floating()
{
    D_DO_CHECK((dl<DSerialEndian::big, float>(byte_string{ 0x3F, 0x80, 0x00, 0x00 }).value == 1.0f));
    D_DO_CHECK((dl<DSerialEndian::little, float>(byte_string{ 0x00, 0x00, 0x80, 0x3F }).value == 1.0f));

    D_DO_CHECK((dl<DSerialEndian::big, double>(byte_string{ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }).value == 1.0));
    D_DO_CHECK((dl<DSerialEndian::little, double>(byte_string{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F }).value == 1.0));

    return true;
}

/*
tests_decode_options_leaf_member
  a custom leaf reads itself, and the byte order option does not reach it
  Tests the following:
  - the custom leaf reads its 4-byte payload under big-endian
  - the identical bytes decode identically under little-endian (order not applied)
  - a successful member decode consumes exactly its four bytes
*/
bool tests_decode_options_leaf_member()
{
    const byte_string _b = { 0x00, 0x00, 0x00, 0x2A };

    D_DO_CHECK((dl<DSerialEndian::big, DDoLeaf>(_b).value.payload == 0x2Au));
    D_DO_CHECK((dl<DSerialEndian::little, DDoLeaf>(_b).value.payload == 0x2Au));

    DByteReader _r(_b);
    DDecodeResult<DDoLeaf> _res = decode_leaf<DSerialEndian::little, DDoLeaf>(_r);
    D_DO_CHECK(_res.ok);
    D_DO_CHECK(_r.exhausted());

    return true;
}

/*
tests_decode_options_leaf_failures
  each family fails cleanly on underflow under either order
  Tests the following:
  - a truncated integral / floating / enum / bool fails with ok = false
  - a truncated member decode fails with ok = false
  - an underflowing decode leaves the reader bad
*/
bool tests_decode_options_leaf_failures()
{
    // truncated integral, little-endian
    {
        const byte_string _b = { 0x01, 0x02 };
        DByteReader _r(_b);
        DDecodeResult<re_std::uint32_t> _res = decode_leaf<DSerialEndian::little, re_std::uint32_t>(_r);
        D_DO_CHECK(!_res.ok);
        D_DO_CHECK(!_r.good());
    }
    // truncated floating, big-endian
    D_DO_CHECK((!dl<DSerialEndian::big, double>(byte_string{ 0x3F, 0x80, 0x00, 0x00 }).ok));
    // truncated enum
    D_DO_CHECK((!dl<DSerialEndian::little, DDoEnumI32>(byte_string{ 0x12, 0x34 }).ok));
    // empty bool
    D_DO_CHECK((!dl<DSerialEndian::big, bool>(byte_string{}).ok));
    // truncated member
    D_DO_CHECK((!dl<DSerialEndian::big, DDoLeaf>(byte_string{ 0x00, 0x2A }).ok));

    return true;
}

/*
tests_decode_options_big_matches_decode
  decode_leaf<big> reproduces decode.hpp's fixed dec_tau, ok and value
  Tests the following:
  - the big-endian parameterised reader agrees with decode<T> for a bool
  - it agrees for an unsigned and a signed integral
  - it agrees for an enum and for a float and a double
  - (the fixed decoder is exactly the big-endian instance of this one)
*/
bool tests_decode_options_big_matches_decode()
{
    // bool
    {
        const byte_string _b = { 0x01 };
        DByteReader _r1(_b);
        DByteReader _r2(_b);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, bool>(_r1).value
                   == decode<bool>(_r2).value);
    }
    // unsigned integral
    {
        const byte_string _b = { 0xDE, 0xAD, 0xBE, 0xEF };
        DByteReader _r1(_b);
        DByteReader _r2(_b);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, re_std::uint32_t>(_r1).value
                   == decode<re_std::uint32_t>(_r2).value);
    }
    // signed integral
    {
        const byte_string _b = { 0x80, 0x00, 0x00, 0x00 };
        DByteReader _r1(_b);
        DByteReader _r2(_b);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, re_std::int32_t>(_r1).value
                   == decode<re_std::int32_t>(_r2).value);
    }
    // enum
    {
        const byte_string _b = { 0xFF, 0xFE, 0x1D, 0xC0 };
        DByteReader _r1(_b);
        DByteReader _r2(_b);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, DDoEnumI32>(_r1).value
                   == decode<DDoEnumI32>(_r2).value);
    }
    // float and double
    {
        const byte_string _f = { 0x3F, 0x80, 0x00, 0x00 };
        DByteReader _r1(_f);
        DByteReader _r2(_f);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, float>(_r1).value
                   == decode<float>(_r2).value);
    }
    {
        const byte_string _d = { 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        DByteReader _r1(_d);
        DByteReader _r2(_d);
        D_DO_CHECK(decode_leaf<DSerialEndian::big, double>(_r1).value
                   == decode<double>(_r2).value);
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
