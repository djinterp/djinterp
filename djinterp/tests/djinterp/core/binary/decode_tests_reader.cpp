/*******************************************************************************
* djinterp [core]                                        decode_tests_reader.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_tests_reader.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_reader_construction
  each byte_reader constructor sets up the cursor and good flag correctly
  Tests the following:
  - a default reader is immediately exhausted, has zero remaining, and is good
  - a raw-range reader reports the full width as remaining and is not exhausted
  - a zero-width raw-range reader is exhausted with zero remaining
  - a byte_string reader reports the string's size as remaining
  - all fresh readers start good()
*/
bool
tests_decode_reader_construction()
{
    // default: empty, exhausted, good
    byte_reader _empty;
    D_DE_CHECK(_empty.good());
    D_DE_CHECK(_empty.remaining() == 0);
    D_DE_CHECK(_empty.exhausted());

    // raw range over three bytes
    const byte _raw[3] = { 0x01, 0x02, 0x03 };
    byte_reader _r(_raw, 3);
    D_DE_CHECK(_r.good());
    D_DE_CHECK(_r.remaining() == 3);
    D_DE_CHECK(!_r.exhausted());

    // zero-width raw range
    byte_reader _z(_raw, 0);
    D_DE_CHECK(_z.good());
    D_DE_CHECK(_z.remaining() == 0);
    D_DE_CHECK(_z.exhausted());

    // byte_string constructor
    const byte_string _bytes = { 0xAA, 0xBB, 0xCC, 0xDD };
    byte_reader _b(_bytes);
    D_DE_CHECK(_b.good());
    D_DE_CHECK(_b.remaining() == 4);
    D_DE_CHECK(!_b.exhausted());

    return true;
}

/*
tests_decode_reader_take
  take copies and advances, and reports underflow without over-reading
  Tests the following:
  - a partial take copies exactly the requested bytes and advances the cursor
  - remaining() falls and exhausted() flips once every byte is consumed
  - taking exactly the remaining bytes leaves the reader good and exhausted
  - take of zero bytes always succeeds and moves nothing
  - an underflowing take returns false, consumes nothing, and clears good()
*/
bool
tests_decode_reader_take()
{
    const byte_string _bytes = { 0x10, 0x20, 0x30, 0x40 };

    // partial take of two bytes
    byte_reader _r(_bytes);
    byte _out[2] = { 0, 0 };
    D_DE_CHECK(_r.take(_out, 2));
    D_DE_CHECK(_out[0] == static_cast<byte>(0x10));
    D_DE_CHECK(_out[1] == static_cast<byte>(0x20));
    D_DE_CHECK(_r.remaining() == 2);
    D_DE_CHECK(!_r.exhausted());
    D_DE_CHECK(_r.good());

    // take the remaining two - now exhausted, still good
    byte _rest[2] = { 0, 0 };
    D_DE_CHECK(_r.take(_rest, 2));
    D_DE_CHECK(_rest[0] == static_cast<byte>(0x30));
    D_DE_CHECK(_rest[1] == static_cast<byte>(0x40));
    D_DE_CHECK(_r.remaining() == 0);
    D_DE_CHECK(_r.exhausted());
    D_DE_CHECK(_r.good());

    // take of zero bytes always succeeds and changes nothing
    byte _none[1] = { 0 };
    D_DE_CHECK(_r.take(_none, 0));
    D_DE_CHECK(_r.good());
    D_DE_CHECK(_r.remaining() == 0);

    // underflow: request more than remains -> false, nothing consumed, bad
    byte_reader _u(_bytes);
    byte _big[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    D_DE_CHECK(!_u.take(_big, 8));
    D_DE_CHECK(!_u.good());
    D_DE_CHECK(_u.remaining() == 4);   // cursor did not advance

    return true;
}

/*
tests_decode_reader_sticky_bad
  a cleared good flag is sticky - no further read succeeds
  Tests the following:
  - after an underflowing take the reader is bad
  - a subsequent take that would otherwise fit still returns false
  - good() remains false thereafter and no bytes are consumed
*/
bool
tests_decode_reader_sticky_bad()
{
    const byte_string _bytes = { 0x01, 0x02, 0x03, 0x04 };
    byte_reader _r(_bytes);

    // fail once by over-reading
    byte _big[5] = { 0, 0, 0, 0, 0 };
    D_DE_CHECK(!_r.take(_big, 5));
    D_DE_CHECK(!_r.good());

    // a read that WOULD have fit is refused because the flag is sticky
    byte _one[1] = { 0 };
    D_DE_CHECK(!_r.take(_one, 1));
    D_DE_CHECK(!_r.good());
    D_DE_CHECK(_r.remaining() == 4);

    return true;
}

/*
tests_decode_get_uint_be
  the internal big-endian reader reassembles widths 1..8 and guards over-wide reads
  Tests the following:
  - widths 1 / 2 / 4 / 8 reassemble a big-endian field most-significant first
  - a width-0 read yields 0 and consumes nothing
  - a width greater than 8 returns false and leaves the reader good (no consume)
  - an underflowing width returns false and leaves the reader bad
*/
bool
tests_decode_get_uint_be()
{
    // width 1
    {
        const byte_string _b = { 0x2A };
        byte_reader _r(_b);
        decode_length_type _v = 0xFFFF;
        D_DE_CHECK(djinterp::internal::get_uint_be(_r, 1, _v));
        D_DE_CHECK(_v == 0x2Au);
        D_DE_CHECK(_r.exhausted());
    }
    // width 2
    {
        const byte_string _b = { 0x12, 0x34 };
        byte_reader _r(_b);
        decode_length_type _v = 0;
        D_DE_CHECK(djinterp::internal::get_uint_be(_r, 2, _v));
        D_DE_CHECK(_v == 0x1234u);
    }
    // width 4
    {
        const byte_string _b = { 0xDE, 0xAD, 0xBE, 0xEF };
        byte_reader _r(_b);
        decode_length_type _v = 0;
        D_DE_CHECK(djinterp::internal::get_uint_be(_r, 4, _v));
        D_DE_CHECK(_v == 0xDEADBEEFu);
    }
    // width 8
    {
        const byte_string _b =
            { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
        byte_reader _r(_b);
        decode_length_type _v = 0;
        D_DE_CHECK(djinterp::internal::get_uint_be(_r, 8, _v));
        D_DE_CHECK(_v == 0x0102030405060708ull);
    }
    // width 0: reads nothing, yields 0, reader untouched and good
    {
        const byte_string _b = { 0x77 };
        byte_reader _r(_b);
        decode_length_type _v = 123;
        D_DE_CHECK(djinterp::internal::get_uint_be(_r, 0, _v));
        D_DE_CHECK(_v == 0u);
        D_DE_CHECK(_r.remaining() == 1);
        D_DE_CHECK(_r.good());
    }
    // width > 8: refused up front, reader stays good and unconsumed
    {
        const byte_string _b =
            { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
        byte_reader _r(_b);
        decode_length_type _v = 0;
        D_DE_CHECK(!djinterp::internal::get_uint_be(_r, 9, _v));
        D_DE_CHECK(_r.good());
        D_DE_CHECK(_r.remaining() == 9);
    }
    // underflow: width exceeds the buffer -> false and bad
    {
        const byte_string _b = { 0x01, 0x02 };
        byte_reader _r(_b);
        decode_length_type _v = 0;
        D_DE_CHECK(!djinterp::internal::get_uint_be(_r, 4, _v));
        D_DE_CHECK(!_r.good());
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
