#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING

// avi
//   type: the module's internal little-endian readers.
namespace avi = ::djinterp::test::archive_verify_internal;


/*
tests_archive_read_u16_le_byte_order
  the byte order of the 16-bit reader.
  Tests the following:
  - the low byte comes first, so 0x34 0x12 decodes to 0x1234
  - the reversed pair decodes to a different value, which is what makes the
    ordering observable
  - a zero pair decodes to zero
*/
bool
tests_archive_read_u16_le_byte_order()
{
    dj::byte_buffer b;

    ta_put_u16(b, 0x1234u);

    D_TA_CHECK(b.size() == 2u);
    D_TA_CHECK(static_cast<unsigned char>(b[0]) == 0x34u);
    D_TA_CHECK(static_cast<unsigned char>(b[1]) == 0x12u);
    D_TA_CHECK(avi::read_u16_le(b.data(), 0) == 0x1234u);

    // the reverse ordering decodes differently
    dj::byte_buffer rev;

    rev.push_back(static_cast<char>(0x12u));
    rev.push_back(static_cast<char>(0x34u));
    D_TA_CHECK(avi::read_u16_le(rev.data(), 0) == 0x3412u);
    D_TA_CHECK(avi::read_u16_le(rev.data(), 0) !=
               avi::read_u16_le(b.data(), 0));

    // zero
    dj::byte_buffer z(2u, static_cast<char>(0));

    D_TA_CHECK(avi::read_u16_le(z.data(), 0) == 0u);

    return true;
}

/*
tests_archive_read_u16_le_range_and_offset
  the value range and the offset argument.
  Tests the following:
  - the full 16-bit range decodes, including values with the top bit set,
    so no byte is sign-extended on the way through
  - the offset selects which pair is read
  - a value written into the middle of a larger buffer reads back intact
*/
bool
tests_archive_read_u16_le_range_and_offset()
{
    // the extremes
    dj::byte_buffer hi;

    ta_put_u16(hi, 0xFFFFu);
    D_TA_CHECK(avi::read_u16_le(hi.data(), 0) == 0xFFFFu);

    dj::byte_buffer top;

    ta_put_u16(top, 0x8000u);
    D_TA_CHECK(avi::read_u16_le(top.data(), 0) == 0x8000u);

    // a high byte in each position must not sign-extend
    dj::byte_buffer a;

    a.push_back(static_cast<char>(0xFFu));
    a.push_back(static_cast<char>(0x00u));
    D_TA_CHECK(avi::read_u16_le(a.data(), 0) == 0x00FFu);

    dj::byte_buffer c;

    c.push_back(static_cast<char>(0x00u));
    c.push_back(static_cast<char>(0xFFu));
    D_TA_CHECK(avi::read_u16_le(c.data(), 0) == 0xFF00u);

    // the offset selects the pair
    dj::byte_buffer many;

    ta_put_u16(many, 0x1111u);
    ta_put_u16(many, 0x2222u);
    ta_put_u16(many, 0x3333u);

    D_TA_CHECK(avi::read_u16_le(many.data(), 0) == 0x1111u);
    D_TA_CHECK(avi::read_u16_le(many.data(), 2) == 0x2222u);
    D_TA_CHECK(avi::read_u16_le(many.data(), 4) == 0x3333u);

    // an unaligned offset is honoured too
    D_TA_CHECK(avi::read_u16_le(many.data(), 1) == 0x2211u);

    return true;
}

/*
tests_archive_read_u32_le_byte_order
  the byte order of the 32-bit reader.
  Tests the following:
  - all four bytes contribute, lowest first
  - each byte lands in its own octet of the result, so none is dropped or
    doubled
  - the ZIP signatures the module compares against decode as written
*/
bool
tests_archive_read_u32_le_byte_order()
{
    dj::byte_buffer b;

    ta_put_u32(b, 0x12345678uL);

    D_TA_CHECK(b.size() == 4u);
    D_TA_CHECK(static_cast<unsigned char>(b[0]) == 0x78u);
    D_TA_CHECK(static_cast<unsigned char>(b[1]) == 0x56u);
    D_TA_CHECK(static_cast<unsigned char>(b[2]) == 0x34u);
    D_TA_CHECK(static_cast<unsigned char>(b[3]) == 0x12u);
    D_TA_CHECK(avi::read_u32_le(b.data(), 0) == 0x12345678uL);

    // each byte occupies its own octet
    dj::byte_buffer o0;
    dj::byte_buffer o1;
    dj::byte_buffer o2;
    dj::byte_buffer o3;

    ta_put_u32(o0, 0x000000FFuL);
    ta_put_u32(o1, 0x0000FF00uL);
    ta_put_u32(o2, 0x00FF0000uL);
    ta_put_u32(o3, 0xFF000000uL);

    D_TA_CHECK(avi::read_u32_le(o0.data(), 0) == 0x000000FFuL);
    D_TA_CHECK(avi::read_u32_le(o1.data(), 0) == 0x0000FF00uL);
    D_TA_CHECK(avi::read_u32_le(o2.data(), 0) == 0x00FF0000uL);
    D_TA_CHECK(avi::read_u32_le(o3.data(), 0) == 0xFF000000uL);

    // the two signatures the sniffers actually compare
    dj::byte_buffer local = ta_local_header("n", dj::byte_buffer(), 0u);
    dj::byte_buffer eocd  = ta_eocd(0u);

    D_TA_CHECK(avi::read_u32_le(local.data(), 0) == 0x04034b50uL);
    D_TA_CHECK(avi::read_u32_le(eocd.data(), 0) == 0x06054b50uL);

    return true;
}

/*
tests_archive_read_u32_le_range_and_offset
  the value range and the offset argument.
  Tests the following:
  - a value with the top bit set decodes without sign extension, which a
    signed intermediate would corrupt
  - the maximum 32-bit value decodes intact
  - the offset selects which quad is read
*/
bool
tests_archive_read_u32_le_range_and_offset()
{
    dj::byte_buffer hi;

    ta_put_u32(hi, 0xFFFFFFFFuL);
    D_TA_CHECK(avi::read_u32_le(hi.data(), 0) == 0xFFFFFFFFuL);

    // the top bit alone
    dj::byte_buffer top;

    ta_put_u32(top, 0x80000000uL);
    D_TA_CHECK(avi::read_u32_le(top.data(), 0) == 0x80000000uL);
    D_TA_CHECK(avi::read_u32_le(top.data(), 0) > 0x7FFFFFFFuL);

    // zero
    dj::byte_buffer z(4u, static_cast<char>(0));

    D_TA_CHECK(avi::read_u32_le(z.data(), 0) == 0uL);

    // the offset selects the quad
    dj::byte_buffer many;

    ta_put_u32(many, 0x11111111uL);
    ta_put_u32(many, 0x22222222uL);
    ta_put_u32(many, 0xDEADBEEFuL);

    D_TA_CHECK(avi::read_u32_le(many.data(), 0) == 0x11111111uL);
    D_TA_CHECK(avi::read_u32_le(many.data(), 4) == 0x22222222uL);
    D_TA_CHECK(avi::read_u32_le(many.data(), 8) == 0xDEADBEEFuL);

    // the two readers agree on the low half of a quad
    D_TA_CHECK(avi::read_u16_le(many.data(), 8) ==
               static_cast<unsigned int>(
                   avi::read_u32_le(many.data(), 8) & 0xFFFFuL));

    return true;
}

NS_END  // testing
NS_END  // djinterp
