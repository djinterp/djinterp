/*******************************************************************************
* djinterp [cpp][binary]                        encode_options_tests_writers.cpp
*
*
* path:      /tests/djinterp/cpp/binary/encode_options_tests_writers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "encode_options_tests.hpp"


NS_DJINTERP
NS_TESTING

// pu / pv / pl
//   helpers: run each internal writer into a fresh byte_string so a test can
// compare the produced bytes directly.  These writers are internal and (for
// put_varint / put_length) are not reached through the public leaf encoder, so
// they are exercised here by direct call.
template<DSerialEndian _E>
static
DByteString pu(
    re_std::uint64_t _v,
    std::size_t   _w
)
{
    DByteString _o;
    djinterp::internal::put_uint<_E>(_o, _v, _w);

    return _o;
}

static
DByteString pv(
    re_std::uint64_t _v
)
{
    DByteString _o;
    djinterp::internal::put_varint(_o, _v);

    return _o;
}

template<DSerialLength _L,
         DSerialEndian  _E>
static
DByteString pl(
    re_std::uint64_t _c
)
{
    DByteString _o;
    djinterp::internal::put_length<_L, _E>(_o, _c);

    return _o;
}

/*
tests_encode_options_put_uint
  put_uint<E> lays a fixed-width scalar in the chosen byte order
  Tests the following:
  - big-endian writes most-significant byte first at widths 1/2/4/8
  - little-endian writes least-significant byte first at widths 2/4/8
  - a single-byte field is identical under either order
  - an over-large value silently wraps to the low `width` bytes (both orders)
*/
bool tests_encode_options_put_uint()
{
    // big-endian widths
    D_EO_CHECK((pu<DSerialEndian::big>(0x2A, 1) == DByteString{ 0x2A }));
    D_EO_CHECK((pu<DSerialEndian::big>(0x1234, 2) == DByteString{ 0x12, 0x34 }));
    D_EO_CHECK((pu<DSerialEndian::big>(0xDEADBEEF, 4)
                == DByteString{ 0xDE, 0xAD, 0xBE, 0xEF }));
    D_EO_CHECK((pu<DSerialEndian::big>(0x0102030405060708ull, 8)
                == DByteString{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));

    // little-endian widths
    D_EO_CHECK((pu<DSerialEndian::little>(0x2A, 1) == DByteString{ 0x2A }));
    D_EO_CHECK((pu<DSerialEndian::little>(0x1234, 2) == DByteString{ 0x34, 0x12 }));
    D_EO_CHECK((pu<DSerialEndian::little>(0xDEADBEEF, 4)
                == DByteString{ 0xEF, 0xBE, 0xAD, 0xDE }));
    D_EO_CHECK((pu<DSerialEndian::little>(0x0102030405060708ull, 8)
                == DByteString{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 }));

    // overflow wrap to the low `width` bytes
    D_EO_CHECK((pu<DSerialEndian::big>(0x10000, 2) == DByteString{ 0x00, 0x00 }));
    D_EO_CHECK((pu<DSerialEndian::big>(0x12345, 2) == DByteString{ 0x23, 0x45 }));
    D_EO_CHECK((pu<DSerialEndian::little>(0x12345, 2) == DByteString{ 0x45, 0x23 }));

    return true;
}

/*
tests_encode_options_put_varint
  put_varint writes an unsigned LEB128 - seven bits per byte, low group first
  Tests the following:
  - values below 0x80 are a single byte (0 and 127)
  - a two-group boundary (128) sets the continuation bit on the first byte
  - multi-group values (300, 16384) group correctly, low group first
  - the maximal 64-bit value is ten groups (nine 0xFF then 0x01)
*/
bool tests_encode_options_put_varint()
{
    D_EO_CHECK((pv(0) == DByteString{ 0x00 }));
    D_EO_CHECK((pv(127) == DByteString{ 0x7F }));
    D_EO_CHECK((pv(128) == DByteString{ 0x80, 0x01 }));
    D_EO_CHECK((pv(300) == DByteString{ 0xAC, 0x02 }));
    D_EO_CHECK((pv(16384) == DByteString{ 0x80, 0x80, 0x01 }));
    D_EO_CHECK((pv(0xFFFFFFFFFFFFFFFFull)
                == DByteString{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF, 0x01 }));

    return true;
}

/*
tests_encode_options_put_length
  put_length<L,E> writes the leading count per the length policy
  Tests the following:
  - u16 / u32 / u64 write a fixed 2/4/8-byte count in the chosen order
  - the fixed widths honour byte order (u16 big vs little)
  - varint ignores byte order and self-delimits
  - a fixed width silently wraps a count that does not fit
*/
bool tests_encode_options_put_length()
{
    // fixed widths, big-endian
    D_EO_CHECK((pl<DSerialLength::u16, DSerialEndian::big>(5)
                == DByteString{ 0x00, 0x05 }));
    D_EO_CHECK((pl<DSerialLength::u32, DSerialEndian::big>(5)
                == DByteString{ 0x00, 0x00, 0x00, 0x05 }));
    D_EO_CHECK((pl<DSerialLength::u64, DSerialEndian::big>(5)
                == DByteString{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 }));

    // fixed width honours byte order
    D_EO_CHECK((pl<DSerialLength::u16, DSerialEndian::little>(5)
                == DByteString{ 0x05, 0x00 }));
    D_EO_CHECK((pl<DSerialLength::u32, DSerialEndian::little>(0x11223344)
                == DByteString{ 0x44, 0x33, 0x22, 0x11 }));

    // varint is order-independent and self-delimiting
    D_EO_CHECK((pl<DSerialLength::varint, DSerialEndian::big>(300)
                == DByteString{ 0xAC, 0x02 }));
    D_EO_CHECK((pl<DSerialLength::varint, DSerialEndian::little>(300)
                == DByteString{ 0xAC, 0x02 }));

    // a fixed width wraps an over-large count
    D_EO_CHECK((pl<DSerialLength::u16, DSerialEndian::big>(0x10000)
                == DByteString{ 0x00, 0x00 }));

    return true;
}

NS_END  // testing
NS_END  // djinterp
