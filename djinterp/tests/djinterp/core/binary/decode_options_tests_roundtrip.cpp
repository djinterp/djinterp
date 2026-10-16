/*******************************************************************************
* djinterp [core]                             decode_options_tests_roundtrip.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_options_tests_roundtrip.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"
#include "../../../../inc/djinterp/core/binary/encode_options.hpp"   // the parameterised write side


NS_DJINTERP
NS_TESTING

// rt_e
//   helper: a value survives an order-`_E` round trip iff decode_leaf recovers
// what encode_leaf wrote under the same order.
template<serial_endian _E,
         typename       _Type>
static bool
rt_e(_Type _v)
{
    decode_result<_Type> _r = decode_leaf<_E, _Type>(encode_leaf<_E>(_v));

    return ( _r.ok && ( _r.value == _v ) );
}

// rt_len
//   helper: a count survives a length round trip under policy `_L` / order `_E`.
template<serial_length _L,
         serial_endian  _E>
static bool
rt_len(std::uint64_t _c)
{
    byte_string _o;
    djinterp::internal::put_length<_L, _E>(_o, _c);

    byte_reader   _r(_o);
    std::uint64_t _got = 0;

    return ( djinterp::internal::get_length<_L, _E>(_r, _got) && ( _got == _c ) );
}

/*
tests_decode_options_roundtrip_scalars
  under big-endian, every leaf family round-trips through the parameterised pair
  Tests the following:
  - bool, integrals (signed/unsigned, several widths), enum, and float/double
    all recover their value
  - the custom member leaf recovers its payload
*/
bool
tests_decode_options_roundtrip_scalars()
{
    D_DO_CHECK(rt_e<serial_endian::big, bool>(true));
    D_DO_CHECK(rt_e<serial_endian::big, bool>(false));

    D_DO_CHECK(rt_e<serial_endian::big, std::uint8_t>(0xFF));
    D_DO_CHECK(rt_e<serial_endian::big, std::uint16_t>(0x1234));
    D_DO_CHECK(rt_e<serial_endian::big, std::uint32_t>(0xDEADBEEFu));
    D_DO_CHECK(rt_e<serial_endian::big, std::uint64_t>(0xFFFFFFFFFFFFFFFFull));
    D_DO_CHECK(rt_e<serial_endian::big, std::int16_t>(-2));
    D_DO_CHECK(rt_e<serial_endian::big, std::int32_t>(-2147483647 - 1));
    D_DO_CHECK(rt_e<serial_endian::big, std::int64_t>(-1));

    D_DO_CHECK(rt_e<serial_endian::big, do_enum_u16>(do_enum_u16::value));
    D_DO_CHECK(rt_e<serial_endian::big, do_enum_i32>(do_enum_i32::value));

    D_DO_CHECK(rt_e<serial_endian::big, float>(0.5f));
    D_DO_CHECK(rt_e<serial_endian::big, double>(2.718281828459045));

    // member leaf: compare payload
    {
        decode_result<do_leaf> _r =
            decode_leaf<serial_endian::big, do_leaf>(encode_leaf<serial_endian::big>(do_leaf(0xDEADBEEFu)));
        D_DO_CHECK(_r.ok);
        D_DO_CHECK(_r.value.payload == 0xDEADBEEFu);
    }

    return true;
}

/*
tests_decode_options_roundtrip_both_orders
  the round trip holds under BOTH byte orders for each scalar family
  Tests the following:
  - the same value recovers whether written and read big- or little-endian
  - integral, enum, and floating families are each covered under both orders
  - boundary integral values (min/max) round-trip under both orders
*/
bool
tests_decode_options_roundtrip_both_orders()
{
    // unsigned
    D_DO_CHECK(rt_e<serial_endian::big, std::uint32_t>(0x89ABCDEFu));
    D_DO_CHECK(rt_e<serial_endian::little, std::uint32_t>(0x89ABCDEFu));
    D_DO_CHECK(rt_e<serial_endian::little, std::uint16_t>(0x0000));
    D_DO_CHECK(rt_e<serial_endian::little, std::uint16_t>(0xFFFF));

    // signed boundaries
    D_DO_CHECK(rt_e<serial_endian::big, std::int64_t>(0x7FFFFFFFFFFFFFFFll));
    D_DO_CHECK(rt_e<serial_endian::little, std::int64_t>(0x7FFFFFFFFFFFFFFFll));
    D_DO_CHECK(rt_e<serial_endian::little, std::int32_t>(-2147483647 - 1));

    // enum
    D_DO_CHECK(rt_e<serial_endian::little, do_enum_u16>(do_enum_u16::value));
    D_DO_CHECK(rt_e<serial_endian::little, do_enum_i32>(do_enum_i32::value));

    // floating
    D_DO_CHECK(rt_e<serial_endian::big, float>(3.14159f));
    D_DO_CHECK(rt_e<serial_endian::little, float>(3.14159f));
    D_DO_CHECK(rt_e<serial_endian::big, double>(-2.0));
    D_DO_CHECK(rt_e<serial_endian::little, double>(-2.0));

    return true;
}

/*
tests_decode_options_roundtrip_length
  a leading count survives put_length -> get_length under every policy and order
  Tests the following:
  - u16 / u32 / u64 round-trip counts under both byte orders
  - varint round-trips counts (order-independent)
  - a range of counts round-trip, including boundary widths (0, small, large)
*/
bool
tests_decode_options_roundtrip_length()
{
    // fixed widths, both orders
    D_DO_CHECK((rt_len<serial_length::u16, serial_endian::big>(5)));
    D_DO_CHECK((rt_len<serial_length::u16, serial_endian::little>(5)));
    D_DO_CHECK((rt_len<serial_length::u16, serial_endian::big>(0xFFFF)));
    D_DO_CHECK((rt_len<serial_length::u32, serial_endian::big>(0x00010000)));
    D_DO_CHECK((rt_len<serial_length::u32, serial_endian::little>(0x11223344)));
    D_DO_CHECK((rt_len<serial_length::u64, serial_endian::big>(0x0102030405060708ull)));
    D_DO_CHECK((rt_len<serial_length::u64, serial_endian::little>(0x0102030405060708ull)));

    // varint, order-independent, across magnitudes
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::big>(0)));
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::big>(127)));
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::big>(128)));
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::little>(300)));
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::big>(16384)));
    D_DO_CHECK((rt_len<serial_length::varint, serial_endian::big>(0xFFFFFFFFFFFFFFFFull)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
