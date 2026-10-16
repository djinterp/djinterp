/*******************************************************************************
* djinterp [core]                                        decode_tests_leaves.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_tests_leaves.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_bool
  the bool leaf reads one byte, zero as false and any nonzero as true
  Tests the following:
  - 0x00 decodes to false, 0x01 to true
  - a nonzero byte other than 1 (0xFF) also decodes to true
  - a successful bool decode consumes exactly one byte
*/
bool
tests_decode_bool()
{
    {
        const byte_string _b = { 0x00 };
        byte_reader _r(_b);
        decode_result<bool> _res = decode<bool>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == false);
        D_DE_CHECK(_r.exhausted());
    }
    {
        const byte_string _b = { 0x01 };
        byte_reader _r(_b);
        decode_result<bool> _res = decode<bool>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == true);
    }
    {
        const byte_string _b = { 0xFF };
        byte_reader _r(_b);
        decode_result<bool> _res = decode<bool>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == true);
    }

    return true;
}

/*
tests_decode_unsigned_integrals
  unsigned integrals are read big-endian at their natural width
  Tests the following:
  - an 8-bit field reads one byte
  - 16-/32-/64-bit fields reassemble most-significant byte first
  - the decoded value equals the big-endian field exactly
*/
bool
tests_decode_unsigned_integrals()
{
    {
        const byte_string _b = { 0xFF };
        byte_reader _r(_b);
        decode_result<std::uint8_t> _res = decode<std::uint8_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 0xFFu);
    }
    {
        const byte_string _b = { 0x12, 0x34 };
        byte_reader _r(_b);
        decode_result<std::uint16_t> _res = decode<std::uint16_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 0x1234u);
    }
    {
        const byte_string _b = { 0xDE, 0xAD, 0xBE, 0xEF };
        byte_reader _r(_b);
        decode_result<std::uint32_t> _res = decode<std::uint32_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 0xDEADBEEFu);
    }
    {
        const byte_string _b =
            { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
        byte_reader _r(_b);
        decode_result<std::uint64_t> _res = decode<std::uint64_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 0x0102030405060708ull);
    }

    return true;
}

/*
tests_decode_signed_integrals
  signed integrals are read through their two's-complement pattern
  Tests the following:
  - int8_t 0x80 is -128, 0x7F is 127, 0xFF is -1
  - int16_t 0xFF 0xFE is -2
  - int32_t 0x80 00 00 00 is INT32_MIN
  - int64_t eight 0xFF bytes is -1
*/
bool
tests_decode_signed_integrals()
{
    {
        const byte_string _b = { 0x80 };
        byte_reader _r(_b);
        D_DE_CHECK(decode<std::int8_t>(_r).value == static_cast<std::int8_t>(-128));
    }
    {
        const byte_string _b = { 0x7F };
        byte_reader _r(_b);
        D_DE_CHECK(decode<std::int8_t>(_r).value == static_cast<std::int8_t>(127));
    }
    {
        const byte_string _b = { 0xFF };
        byte_reader _r(_b);
        D_DE_CHECK(decode<std::int8_t>(_r).value == static_cast<std::int8_t>(-1));
    }
    {
        const byte_string _b = { 0xFF, 0xFE };
        byte_reader _r(_b);
        decode_result<std::int16_t> _res = decode<std::int16_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == static_cast<std::int16_t>(-2));
    }
    {
        const byte_string _b = { 0x80, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        decode_result<std::int32_t> _res = decode<std::int32_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == (-2147483647 - 1));
    }
    {
        const byte_string _b =
            { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        byte_reader _r(_b);
        decode_result<std::int64_t> _res = decode<std::int64_t>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == static_cast<std::int64_t>(-1));
    }

    return true;
}

/*
tests_decode_enums
  an enumeration is read through its underlying integral type
  Tests the following:
  - a uint16_t-backed enum reads a 2-byte big-endian field to its enumerator
  - an int32_t-backed enum reads a negative two's-complement field correctly
  - the enum path consumes exactly the underlying width
*/
bool
tests_decode_enums()
{
    {
        const byte_string _b = { 0x12, 0x34 };
        byte_reader _r(_b);
        decode_result<dec_enum_u16> _res = decode<dec_enum_u16>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == dec_enum_u16::value);
        D_DE_CHECK(_r.exhausted());
    }
    {
        const byte_string _b = { 0xFF, 0xFE, 0x1D, 0xC0 };
        byte_reader _r(_b);
        decode_result<dec_enum_i32> _res = decode<dec_enum_i32>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == dec_enum_i32::value);
    }

    return true;
}

/*
tests_decode_floating
  a 4- or 8-byte float is read through its same-width unsigned bit pattern
  Tests the following:
  - float 1.0 / -2.0 / 0.5 reassemble from their IEEE-754 single patterns
  - double 1.0 / -2.0 / 0.5 reassemble from their IEEE-754 double patterns
  - both the 4- and 8-byte branches of the floating reader are exercised
*/
bool
tests_decode_floating()
{
    {
        const byte_string _b = { 0x3F, 0x80, 0x00, 0x00 };
        byte_reader _r(_b);
        decode_result<float> _res = decode<float>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 1.0f);
    }
    {
        const byte_string _b = { 0xC0, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        D_DE_CHECK(decode<float>(_r).value == -2.0f);
    }
    {
        const byte_string _b = { 0x3F, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        D_DE_CHECK(decode<float>(_r).value == 0.5f);
    }
    {
        const byte_string _b =
            { 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        decode_result<double> _res = decode<double>(_r);
        D_DE_CHECK(_res.ok);
        D_DE_CHECK(_res.value == 1.0);
    }
    {
        const byte_string _b =
            { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        D_DE_CHECK(decode<double>(_r).value == -2.0);
    }
    {
        const byte_string _b =
            { 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte_reader _r(_b);
        D_DE_CHECK(decode<double>(_r).value == 0.5);
    }

    return true;
}

/*
tests_decode_member_surface
  a leaf carrying a static decode reads itself, taking precedence
  Tests the following:
  - the custom leaf reads its 4-byte big-endian payload
  - the decoded payload equals the field that was laid down
  - a successful member decode consumes exactly its four bytes
*/
bool
tests_decode_member_surface()
{
    const byte_string _b = { 0x00, 0x00, 0x00, 0x2A };
    byte_reader _r(_b);
    decode_result<rt_leaf> _res = decode<rt_leaf>(_r);
    D_DE_CHECK(_res.ok);
    D_DE_CHECK(_res.value.payload == 0x2Au);
    D_DE_CHECK(_r.exhausted());

    return true;
}

/*
tests_decode_failure_paths
  every family fails cleanly on underflow, and failure is sticky
  Tests the following:
  - a truncated integral / floating / enum / bool field fails with ok = false
  - a truncated member decode fails with ok = false
  - an underflowing decode leaves the reader bad
  - a second decode after a failure also fails (sticky bad propagates)
*/
bool
tests_decode_failure_paths()
{
    // truncated unsigned integral
    {
        const byte_string _b = { 0x01, 0x02 };
        byte_reader _r(_b);
        decode_result<std::uint32_t> _res = decode<std::uint32_t>(_r);
        D_DE_CHECK(!_res.ok);
        D_DE_CHECK(!_r.good());
    }
    // truncated floating
    {
        const byte_string _b = { 0x3F, 0x80, 0x00, 0x00 };  // 4 bytes, need 8
        byte_reader _r(_b);
        D_DE_CHECK(!decode<double>(_r).ok);
    }
    // truncated enum (underlying is 4 bytes)
    {
        const byte_string _b = { 0x12, 0x34 };
        byte_reader _r(_b);
        D_DE_CHECK(!decode<dec_enum_i32>(_r).ok);
    }
    // empty bool
    {
        const byte_string _b = {};
        byte_reader _r(_b);
        D_DE_CHECK(!decode<bool>(_r).ok);
    }
    // truncated member decode
    {
        const byte_string _b = { 0x00, 0x2A };  // need 4
        byte_reader _r(_b);
        D_DE_CHECK(!decode<rt_leaf>(_r).ok);
    }
    // sticky: a second decode after a failure also fails
    {
        const byte_string _b = { 0x01 };
        byte_reader _r(_b);
        D_DE_CHECK(!decode<std::uint32_t>(_r).ok);   // underflow, reader now bad
        D_DE_CHECK(!decode<std::uint8_t>(_r).ok);    // would fit, but sticky bad
    }

    return true;
}

/*
tests_decode_byte_string_convenience
  decode(byte_string) wraps a reader and reads one leaf from the front
  Tests the following:
  - a whole-buffer decode returns the leading value
  - only the leading leaf is consumed; trailing bytes are ignored
  - a too-short buffer fails with ok = false
*/
bool
tests_decode_byte_string_convenience()
{
    // whole-buffer decode
    D_DE_CHECK(decode<std::uint16_t>(byte_string{ 0x12, 0x34 }).value == 0x1234u);

    // leading leaf only - a trailing byte is left unread
    decode_result<std::uint8_t> _lead = decode<std::uint8_t>(byte_string{ 0xAB, 0xCD });
    D_DE_CHECK(_lead.ok);
    D_DE_CHECK(_lead.value == 0xABu);

    // too short
    D_DE_CHECK(!decode<std::uint16_t>(byte_string{ 0x12 }).ok);

    return true;
}

NS_END  // testing
NS_END  // djinterp
