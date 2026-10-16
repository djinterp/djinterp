/*******************************************************************************
* djinterp [core]                                decode_options_tests_leaves.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_options_tests_leaves.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"
#include "../../../../inc/djinterp/core/binary/decode.hpp"   // to assert the big-endian path reproduces decode.hpp's dec_tau


NS_DJINTERP
NS_TESTING

// dl
//   helper: decode one leaf of `_Type` under byte order `_E` from a fresh reader
// over `_bytes`, returning the partial result for inspection.
template<serial_endian _E,
         typename       _Type>
static decode_result<_Type>
dl(const byte_string& _bytes)
{
    byte_reader _r(_bytes);

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
bool
tests_decode_options_leaf_bool()
{
    D_DO_CHECK((dl<serial_endian::big, bool>(byte_string{ 0x01 }).value == true));
    D_DO_CHECK((dl<serial_endian::big, bool>(byte_string{ 0x00 }).value == false));
    D_DO_CHECK((dl<serial_endian::little, bool>(byte_string{ 0x01 }).value == true));
    D_DO_CHECK((dl<serial_endian::little, bool>(byte_string{ 0x00 }).value == false));

    const byte_string _b = { 0x01 };
    byte_reader _r(_b);
    decode_result<bool> _res = decode_leaf<serial_endian::big, bool>(_r);
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
bool
tests_decode_options_leaf_integrals()
{
    // unsigned, both orders yield the same value
    D_DO_CHECK((dl<serial_endian::big, std::uint16_t>(byte_string{ 0x12, 0x34 }).value == 0x1234u));
    D_DO_CHECK((dl<serial_endian::little, std::uint16_t>(byte_string{ 0x34, 0x12 }).value == 0x1234u));
    D_DO_CHECK((dl<serial_endian::big, std::uint32_t>(byte_string{ 0xDE, 0xAD, 0xBE, 0xEF }).value == 0xDEADBEEFu));
    D_DO_CHECK((dl<serial_endian::little, std::uint32_t>(byte_string{ 0xEF, 0xBE, 0xAD, 0xDE }).value == 0xDEADBEEFu));

    // signed
    D_DO_CHECK((dl<serial_endian::big, std::int16_t>(byte_string{ 0xFF, 0xFE }).value == static_cast<std::int16_t>(-2)));
    D_DO_CHECK((dl<serial_endian::little, std::int16_t>(byte_string{ 0xFE, 0xFF }).value == static_cast<std::int16_t>(-2)));
    D_DO_CHECK((dl<serial_endian::big, std::int32_t>(byte_string{ 0x80, 0x00, 0x00, 0x00 }).value == (-2147483647 - 1)));
    D_DO_CHECK((dl<serial_endian::little, std::int32_t>(byte_string{ 0x00, 0x00, 0x00, 0x80 }).value == (-2147483647 - 1)));

    // one byte, order-invariant
    D_DO_CHECK((dl<serial_endian::big, std::uint8_t>(byte_string{ 0xFF }).value == 0xFFu));
    D_DO_CHECK((dl<serial_endian::little, std::uint8_t>(byte_string{ 0xFF }).value == 0xFFu));

    return true;
}

/*
tests_decode_options_leaf_enums
  an enum is read through its underlying integer in the chosen byte order
  Tests the following:
  - a uint16_t-backed enum reads from its big and little fields
  - an int32_t-backed enum's negative pattern reads from either order
*/
bool
tests_decode_options_leaf_enums()
{
    D_DO_CHECK((dl<serial_endian::big, do_enum_u16>(byte_string{ 0x12, 0x34 }).value == do_enum_u16::value));
    D_DO_CHECK((dl<serial_endian::little, do_enum_u16>(byte_string{ 0x34, 0x12 }).value == do_enum_u16::value));

    D_DO_CHECK((dl<serial_endian::big, do_enum_i32>(byte_string{ 0xFF, 0xFE, 0x1D, 0xC0 }).value == do_enum_i32::value));
    D_DO_CHECK((dl<serial_endian::little, do_enum_i32>(byte_string{ 0xC0, 0x1D, 0xFE, 0xFF }).value == do_enum_i32::value));

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
bool
tests_decode_options_leaf_floating()
{
    D_DO_CHECK((dl<serial_endian::big, float>(byte_string{ 0x3F, 0x80, 0x00, 0x00 }).value == 1.0f));
    D_DO_CHECK((dl<serial_endian::little, float>(byte_string{ 0x00, 0x00, 0x80, 0x3F }).value == 1.0f));

    D_DO_CHECK((dl<serial_endian::big, double>(byte_string{ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }).value == 1.0));
    D_DO_CHECK((dl<serial_endian::little, double>(byte_string{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F }).value == 1.0));

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
bool
tests_decode_options_leaf_member()
{
    const byte_string _b = { 0x00, 0x00, 0x00, 0x2A };

    D_DO_CHECK((dl<serial_endian::big, do_leaf>(_b).value.payload == 0x2Au));
    D_DO_CHECK((dl<serial_endian::little, do_leaf>(_b).value.payload == 0x2Au));

    byte_reader _r(_b);
    decode_result<do_leaf> _res = decode_leaf<serial_endian::little, do_leaf>(_r);
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
bool
tests_decode_options_leaf_failures()
{
    // truncated integral, little-endian
    {
        const byte_string _b = { 0x01, 0x02 };
        byte_reader _r(_b);
        decode_result<std::uint32_t> _res = decode_leaf<serial_endian::little, std::uint32_t>(_r);
        D_DO_CHECK(!_res.ok);
        D_DO_CHECK(!_r.good());
    }
    // truncated floating, big-endian
    D_DO_CHECK((!dl<serial_endian::big, double>(byte_string{ 0x3F, 0x80, 0x00, 0x00 }).ok));
    // truncated enum
    D_DO_CHECK((!dl<serial_endian::little, do_enum_i32>(byte_string{ 0x12, 0x34 }).ok));
    // empty bool
    D_DO_CHECK((!dl<serial_endian::big, bool>(byte_string{}).ok));
    // truncated member
    D_DO_CHECK((!dl<serial_endian::big, do_leaf>(byte_string{ 0x00, 0x2A }).ok));

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
bool
tests_decode_options_big_matches_decode()
{
    // bool
    {
        const byte_string _b = { 0x01 };
        byte_reader _r1(_b);
        byte_reader _r2(_b);
        D_DO_CHECK(decode_leaf<serial_endian::big, bool>(_r1).value
                   == decode<bool>(_r2).value);
    }
    // unsigned integral
    {
        const byte_string _b = { 0xDE, 0xAD, 0xBE, 0xEF };
        byte_reader _r1(_b);
        byte_reader _r2(_b);
        D_DO_CHECK(decode_leaf<serial_endian::big, std::uint32_t>(_r1).value
                   == decode<std::uint32_t>(_r2).value);
    }
    // signed integral
    {
        const byte_string _b = { 0x80, 0x00, 0x00, 0x00 };
        byte_reader _r1(_b);
        byte_reader _r2(_b);
        D_DO_CHECK(decode_leaf<serial_endian::big, std::int32_t>(_r1).value
                   == decode<std::int32_t>(_r2).value);
    }
    // enum
    {
        const byte_string _b = { 0xFF, 0xFE, 0x1D, 0xC0 };
        byte_reader _r1(_b);
        byte_reader _r2(_b);
        D_DO_CHECK(decode_leaf<serial_endian::big, do_enum_i32>(_r1).value
                   == decode<do_enum_i32>(_r2).value);
    }
    // float and double
    {
        const byte_string _f = { 0x3F, 0x80, 0x00, 0x00 };
        byte_reader _r1(_f);
        byte_reader _r2(_f);
        D_DO_CHECK(decode_leaf<serial_endian::big, float>(_r1).value
                   == decode<float>(_r2).value);
    }
    {
        const byte_string _d = { 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte_reader _r1(_d);
        byte_reader _r2(_d);
        D_DO_CHECK(decode_leaf<serial_endian::big, double>(_r1).value
                   == decode<double>(_r2).value);
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
