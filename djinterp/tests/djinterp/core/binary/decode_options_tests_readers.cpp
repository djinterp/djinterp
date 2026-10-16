/*******************************************************************************
* djinterp [core]                               decode_options_tests_readers.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_options_tests_readers.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_options_get_uint
  get_uint<E> reads a fixed-width scalar in the chosen byte order
  Tests the following:
  - big-endian reassembles most-significant byte first at widths 1/2/4/8
  - little-endian reassembles least-significant byte first
  - a width greater than 8 is refused, leaving the reader good and unconsumed
  - an underflowing width fails and leaves the reader bad
*/
bool
tests_decode_options_get_uint()
{
    // big-endian
    {
        const byte_string _b = { 0x12, 0x34 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::big>(_r, 2, _v));
        D_DO_CHECK(_v == 0x1234u);
    }
    {
        const byte_string _b = { 0xDE, 0xAD, 0xBE, 0xEF };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::big>(_r, 4, _v));
        D_DO_CHECK(_v == 0xDEADBEEFu);
    }
    // little-endian
    {
        const byte_string _b = { 0x34, 0x12 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::little>(_r, 2, _v));
        D_DO_CHECK(_v == 0x1234u);
    }
    {
        const byte_string _b = { 0xEF, 0xBE, 0xAD, 0xDE };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::little>(_r, 4, _v));
        D_DO_CHECK(_v == 0xDEADBEEFu);
    }
    // width 1 identical, width 8 both orders
    {
        const byte_string _b = { 0x2A };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::little>(_r, 1, _v));
        D_DO_CHECK(_v == 0x2Au);
    }
    {
        const byte_string _b =
            { 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_uint<serial_endian::little>(_r, 8, _v));
        D_DO_CHECK(_v == 0x0102030405060708ull);
    }
    // width > 8: refused, reader good and unconsumed
    {
        const byte_string _b =
            { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(!djinterp::internal::get_uint<serial_endian::big>(_r, 9, _v));
        D_DO_CHECK(_r.good());
        D_DO_CHECK(_r.remaining() == 9);
    }
    // underflow: bad reader
    {
        const byte_string _b = { 0x01, 0x02 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(!djinterp::internal::get_uint<serial_endian::little>(_r, 4, _v));
        D_DO_CHECK(!_r.good());
    }

    return true;
}

/*
tests_decode_options_get_varint
  get_varint reads an unsigned LEB128 and rejects malformed input
  Tests the following:
  - single- and multi-group values decode (0, 127, 128, 300, 16384)
  - the maximal ten-group value decodes to the full 64-bit maximum
  - a truncated varint (continuation bit with no successor) fails
  - an overlong varint (more groups than 64 bits allow) fails
*/
bool
tests_decode_options_get_varint()
{
    struct case_t { byte_string bytes; std::uint64_t value; };
    const case_t _cases[] =
    {
        { { 0x00 }, 0u },
        { { 0x7F }, 127u },
        { { 0x80, 0x01 }, 128u },
        { { 0xAC, 0x02 }, 300u },
        { { 0x80, 0x80, 0x01 }, 16384u },
        { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 },
          0xFFFFFFFFFFFFFFFFull },
    };

    for (std::size_t _i = 0; _i < (sizeof(_cases) / sizeof(_cases[0])); ++_i)
    {
        byte_reader _r(_cases[_i].bytes);
        std::uint64_t _v = 0;
        D_DO_CHECK(djinterp::internal::get_varint(_r, _v));
        D_DO_CHECK(_v == _cases[_i].value);
    }

    // truncated: a lone continuation byte has no successor
    {
        const byte_string _b = { 0x80 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(!djinterp::internal::get_varint(_r, _v));
    }
    // overlong: ten continuation groups overrun the 64-bit budget
    {
        const byte_string _b =
            { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK(!djinterp::internal::get_varint(_r, _v));
    }

    return true;
}

/*
tests_decode_options_get_length
  get_length<L,E> reads the leading count per the length policy
  Tests the following:
  - u16 / u32 / u64 read a fixed 2/4/8-byte count in the chosen order
  - the fixed widths honour byte order (u16 big vs little)
  - varint self-delimits and ignores byte order
  - a truncated fixed width fails
*/
bool
tests_decode_options_get_length()
{
    // fixed widths, big-endian
    {
        const byte_string _b = { 0x00, 0x05 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((djinterp::internal::get_length<serial_length::u16, serial_endian::big>(_r, _v)));
        D_DO_CHECK(_v == 5u);
    }
    {
        const byte_string _b = { 0x00, 0x00, 0x00, 0x05 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((djinterp::internal::get_length<serial_length::u32, serial_endian::big>(_r, _v)));
        D_DO_CHECK(_v == 5u);
    }
    {
        const byte_string _b =
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((djinterp::internal::get_length<serial_length::u64, serial_endian::big>(_r, _v)));
        D_DO_CHECK(_v == 5u);
    }
    // fixed width honours byte order
    {
        const byte_string _b = { 0x05, 0x00 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((djinterp::internal::get_length<serial_length::u16, serial_endian::little>(_r, _v)));
        D_DO_CHECK(_v == 5u);
    }
    // varint self-delimits, order-independent
    {
        const byte_string _b = { 0xAC, 0x02 };
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((djinterp::internal::get_length<serial_length::varint, serial_endian::big>(_r, _v)));
        D_DO_CHECK(_v == 300u);
    }
    // truncated fixed width fails
    {
        const byte_string _b = { 0x00 };  // u16 needs two bytes
        byte_reader _r(_b);
        std::uint64_t _v = 0;
        D_DO_CHECK((!djinterp::internal::get_length<serial_length::u16, serial_endian::big>(_r, _v)));
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
