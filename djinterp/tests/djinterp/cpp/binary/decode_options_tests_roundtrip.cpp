/*******************************************************************************
* djinterp [cpp]                              decode_options_tests_roundtrip.cpp
*
*
* path:      /tests/djinterp/cpp/binary/decode_options_tests_roundtrip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"
#include "../../../../inc/djinterp/cpp/binary/encode_options.hpp"   // the parameterised write side


NS_DJINTERP
NS_TESTING

// rt_e
//   helper: a value survives an order-`_E` round trip iff decode_leaf recovers
// what encode_leaf wrote under the same order.
template<DSerialEndian _E,
         typename       _Type>
static
bool rt_e(
    _Type _v
)
{
    DDecodeResult<_Type> _r = decode_leaf<_E, _Type>(encode_leaf<_E>(_v));

    return ( _r.ok && ( _r.value == _v ) );
}

// rt_len
//   helper: a count survives a length round trip under policy `_L` / order `_E`.
template<DSerialLength _L,
         DSerialEndian  _E>
static
bool rt_len(
    re_std::uint64_t _c
)
{
    byte_string _o;
    djinterp::internal::put_length<_L, _E>(_o, _c);

    DByteReader   _r(_o);
    re_std::uint64_t _got = 0;

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
bool tests_decode_options_roundtrip_scalars()
{
    D_DO_CHECK(rt_e<DSerialEndian::big, bool>(true));
    D_DO_CHECK(rt_e<DSerialEndian::big, bool>(false));

    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::uint8_t>(0xFF));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::uint16_t>(0x1234));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::uint32_t>(0xDEADBEEFu));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::uint64_t>(0xFFFFFFFFFFFFFFFFull));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::int16_t>(-2));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::int32_t>(-2147483647 - 1));
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::int64_t>(-1));

    D_DO_CHECK(rt_e<DSerialEndian::big, DDoEnumU16>(DDoEnumU16::value));
    D_DO_CHECK(rt_e<DSerialEndian::big, DDoEnumI32>(DDoEnumI32::value));

    D_DO_CHECK(rt_e<DSerialEndian::big, float>(0.5f));
    D_DO_CHECK(rt_e<DSerialEndian::big, double>(2.718281828459045));

    // member leaf: compare payload
    {
        DDecodeResult<DDoLeaf> _r =
            decode_leaf<DSerialEndian::big, DDoLeaf>(encode_leaf<DSerialEndian::big>(DDoLeaf(0xDEADBEEFu)));
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
bool tests_decode_options_roundtrip_both_orders()
{
    // unsigned
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::uint32_t>(0x89ABCDEFu));
    D_DO_CHECK(rt_e<DSerialEndian::little, re_std::uint32_t>(0x89ABCDEFu));
    D_DO_CHECK(rt_e<DSerialEndian::little, re_std::uint16_t>(0x0000));
    D_DO_CHECK(rt_e<DSerialEndian::little, re_std::uint16_t>(0xFFFF));

    // signed boundaries
    D_DO_CHECK(rt_e<DSerialEndian::big, re_std::int64_t>(0x7FFFFFFFFFFFFFFFll));
    D_DO_CHECK(rt_e<DSerialEndian::little, re_std::int64_t>(0x7FFFFFFFFFFFFFFFll));
    D_DO_CHECK(rt_e<DSerialEndian::little, re_std::int32_t>(-2147483647 - 1));

    // enum
    D_DO_CHECK(rt_e<DSerialEndian::little, DDoEnumU16>(DDoEnumU16::value));
    D_DO_CHECK(rt_e<DSerialEndian::little, DDoEnumI32>(DDoEnumI32::value));

    // floating
    D_DO_CHECK(rt_e<DSerialEndian::big, float>(3.14159f));
    D_DO_CHECK(rt_e<DSerialEndian::little, float>(3.14159f));
    D_DO_CHECK(rt_e<DSerialEndian::big, double>(-2.0));
    D_DO_CHECK(rt_e<DSerialEndian::little, double>(-2.0));

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
bool tests_decode_options_roundtrip_length()
{
    // fixed widths, both orders
    D_DO_CHECK((rt_len<DSerialLength::u16, DSerialEndian::big>(5)));
    D_DO_CHECK((rt_len<DSerialLength::u16, DSerialEndian::little>(5)));
    D_DO_CHECK((rt_len<DSerialLength::u16, DSerialEndian::big>(0xFFFF)));
    D_DO_CHECK((rt_len<DSerialLength::u32, DSerialEndian::big>(0x00010000)));
    D_DO_CHECK((rt_len<DSerialLength::u32, DSerialEndian::little>(0x11223344)));
    D_DO_CHECK((rt_len<DSerialLength::u64, DSerialEndian::big>(0x0102030405060708ull)));
    D_DO_CHECK((rt_len<DSerialLength::u64, DSerialEndian::little>(0x0102030405060708ull)));

    // varint, order-independent, across magnitudes
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::big>(0)));
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::big>(127)));
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::big>(128)));
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::little>(300)));
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::big>(16384)));
    D_DO_CHECK((rt_len<DSerialLength::varint, DSerialEndian::big>(0xFFFFFFFFFFFFFFFFull)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
