/*******************************************************************************
* djinterp [cpp][utility]                                compress_tests_gzip.cpp
*
*   Tests for the gzip pair, compress.hpp's face of the kernel's gzip header
* functions: try_compress_gzip and try_decompress_gzip, the throwing
* compress_gzip and decompress_gzip, and gzip_header. A header's name, time
* and OS byte are checked where RFC 1952 puts them in the stream, as well as
* read back. Where the build has no gzip, each test checks the pair says so.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_tests_gzip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "./compress_tests.hpp"  // D_CP_CHECK, the facade, make_text


NS_DJINTERP
NS_TESTING


/*
gzip_byte
  A stream's byte at _at, as an unsigned value; 256 past its end.
*/
static unsigned
gzip_byte(
    const byte_blob& _stream,
    std::size_t      _at
)
{
    return (_at < _stream.size())
               ? static_cast<unsigned char>(_stream[_at])
               : 256u;
}

/*
gzip_unavailable_both_ways
  When the build has no gzip: both halves of the pair say so.
*/
static bool
gzip_unavailable_both_ways(void)
{
    byte_blob   out;
    gzip_header header;

    D_CP_CHECK(try_compress_gzip(byte_blob("x"), out, header) ==
               status_unavailable);
    D_CP_CHECK(try_decompress_gzip(byte_blob("x"), out, header) ==
               status_unavailable);

    return true;
}


/*
tests_compress_gzip_default_header
  Tests the following:
  - a default gzip_header writes codecs::gzip's bytes exactly: no name, no
    time, the options' OS byte (255 by default)
  - reading codecs::gzip's stream back gives an empty name, time 0 and OS
    255, and the input
*/
bool tests_compress_gzip_default_header()
{
    const byte_blob in = test::make_text(3000);
    byte_blob       plain;
    byte_blob       paired;
    byte_blob       back;
    gzip_header     header;

    if (!codec_is_available<codecs::gzip>())
    {
        return gzip_unavailable_both_ways();
    }

    D_CP_CHECK(try_compress<codecs::gzip>(in, plain) == status_ok);
    D_CP_CHECK(try_compress_gzip(in, paired, gzip_header()) == status_ok);
    D_CP_CHECK(paired == plain);
    D_CP_CHECK(gzip_byte(paired, 9) == 255u);

    header.name  = "stale";
    header.mtime = 7u;
    D_CP_CHECK(try_decompress_gzip(plain, back, header) == status_ok);
    D_CP_CHECK(back == in);
    D_CP_CHECK(header.name.empty() && (header.mtime == 0u) &&
               (header.os == 255) && !header.name_truncated);

    return true;
}

/*
tests_compress_gzip_header_round_trip
  Tests the following:
  - a name, a time and an OS byte are written where RFC 1952 puts them:
    FLG's FNAME bit, MTIME little-endian at 4, OS at 9, the name after the
    ten bytes, ended by a NUL
  - they read back as written, with the input; a UTF-8 name is kept as its
    bytes
*/
bool tests_compress_gzip_header_round_trip()
{
    const byte_blob   in   = test::make_text(500);
    const std::string name = "r\xC3\xA9sum\xC3\xA9.txt";
    byte_blob         stream;
    byte_blob         back;
    gzip_header       header;
    gzip_header       read;

    if (!codec_is_available<codecs::gzip>())
    {
        return gzip_unavailable_both_ways();
    }

    header.name  = name;
    header.mtime = 1763500000u;
    header.os    = 3;

    D_CP_CHECK(try_compress_gzip(in, stream, header) == status_ok);
    D_CP_CHECK((gzip_byte(stream, 0) == 0x1Fu) &&
               (gzip_byte(stream, 1) == 0x8Bu) &&
               ((gzip_byte(stream, 3) & 0x08u) != 0u));
    D_CP_CHECK((gzip_byte(stream, 4) |
                (gzip_byte(stream, 5) << 8) |
                (gzip_byte(stream, 6) << 16) |
                (gzip_byte(stream, 7) << 24)) == 1763500000u);
    D_CP_CHECK(gzip_byte(stream, 9) == 3u);
    D_CP_CHECK((stream.compare(10, name.size(), name) == 0) &&
               (gzip_byte(stream, 10 + name.size()) == 0u));

    D_CP_CHECK(try_decompress_gzip(stream, back, read) == status_ok);
    D_CP_CHECK(back == in);
    D_CP_CHECK((read.name == name) && (read.mtime == 1763500000u) &&
               (read.os == 3) && !read.name_truncated);

    return true;
}

/*
tests_compress_gzip_names
  Tests the following:
  - a 1,000-byte name, past the reader's first buffer, reads back whole
  - a name past gzip_name_max reads back cut to it, saying name_truncated
  - a name holding a NUL is status_invalid_argument, and writes nothing
*/
bool tests_compress_gzip_names()
{
    const byte_blob in = test::make_text(100);
    byte_blob       stream;
    byte_blob       back;
    gzip_header     header;
    gzip_header     read;

    if (!codec_is_available<codecs::gzip>())
    {
        return gzip_unavailable_both_ways();
    }

    header.name.assign(1000u, 'n');
    D_CP_CHECK(try_compress_gzip(in, stream, header) == status_ok);
    D_CP_CHECK(try_decompress_gzip(stream, back, read) == status_ok);
    D_CP_CHECK((read.name == header.name) && !read.name_truncated &&
               (back == in));

    header.name.assign(gzip_name_max + 10u, 'm');
    D_CP_CHECK(try_compress_gzip(in, stream, header) == status_ok);
    D_CP_CHECK(try_decompress_gzip(stream, back, read) == status_ok);
    D_CP_CHECK((read.name == header.name.substr(0, gzip_name_max)) &&
               read.name_truncated && (back == in));

    header.name = std::string("a\0b", 3);
    D_CP_CHECK(try_compress_gzip(in, stream, header) ==
               status_invalid_argument);
    D_CP_CHECK(stream.empty());

    return true;
}

/*
tests_compress_gzip_os_choices
  Tests the following:
  - gzip_os_default takes the options' gzip.os: 255 unless set, and the
    value set otherwise
  - gzip_os_zlib writes the byte zlib picks for this platform, the same as
    the options' gzip.os set to it
*/
bool tests_compress_gzip_os_choices()
{
    const byte_blob  in = test::make_text(64);
    byte_blob        stream;
    byte_blob        other;
    gzip_header      header;
    compress_options opt;

    if (!codec_is_available<codecs::gzip>())
    {
        return gzip_unavailable_both_ways();
    }

    D_CP_CHECK(try_compress_gzip(in, stream, header) == status_ok);
    D_CP_CHECK(gzip_byte(stream, 9) == 255u);

    opt.gzip.os = 11;
    D_CP_CHECK(try_compress_gzip(in, stream, header, opt) == status_ok);
    D_CP_CHECK(gzip_byte(stream, 9) == 11u);

    header.os   = gzip_os_zlib;
    opt.gzip.os = gzip_os_default;
    D_CP_CHECK(try_compress_gzip(in, stream, header, opt) == status_ok);
    opt.gzip.os = gzip_os_zlib;
    D_CP_CHECK(try_compress_gzip(in, other, gzip_header(), opt) ==
               status_ok);
    D_CP_CHECK((gzip_byte(stream, 9) < 256u) &&
               (gzip_byte(stream, 9) == gzip_byte(other, 9)));

    return true;
}

/*
tests_compress_gzip_corrupt
  Tests the following:
  - a stream that isn't gzip fails to read, with a default header and no
    bytes; a cut stream fails too
*/
bool tests_compress_gzip_corrupt()
{
    const byte_blob in = test::make_text(400);
    byte_blob       stream;
    byte_blob       back;
    gzip_header     read;

    if (!codec_is_available<codecs::gzip>())
    {
        return gzip_unavailable_both_ways();
    }

    read.name = "kept?";
    D_CP_CHECK(try_decompress_gzip(byte_blob("not a gzip stream"), back,
                                   read) != status_ok);
    D_CP_CHECK(read.name.empty() && (read.mtime == 0u));

    D_CP_CHECK(try_compress<codecs::gzip>(in, stream) == status_ok);
    stream.resize(stream.size() / 2u);
    D_CP_CHECK(try_decompress_gzip(stream, back, read) != status_ok);

    return true;
}

/*
tests_compress_gzip_throwing
  Tests the following:
  - compress_gzip and decompress_gzip give what the try_ forms give
  - each throws DCompressionError on failure, naming itself
  (trivial pass when exceptions are disabled, or the build has no gzip)
*/
bool tests_compress_gzip_throwing()
{
#if D_INTERNAL_COMPRESS_EXCEPTIONS
    const byte_blob in = test::make_text(256);
    gzip_header     header;
    gzip_header     read;
    std::string     what;

    if (!codec_is_available<codecs::gzip>())
    {
        return true;
    }

    header.name = "t.txt";
    D_CP_CHECK(decompress_gzip(compress_gzip(in, header), read) == in);
    D_CP_CHECK(read.name == "t.txt");

    try
    {
        (void)decompress_gzip(byte_blob("garbage"), read);
    }
    catch (const DCompressionError& _error)
    {
        what = _error.what();
    }

    D_CP_CHECK(what.find("decompress_gzip") != std::string::npos);

    what.clear();
    header.name = std::string("\0", 1);

    try
    {
        (void)compress_gzip(in, header);
    }
    catch (const DCompressionError& _error)
    {
        what = _error.what();
    }

    D_CP_CHECK(what.find("compress_gzip") != std::string::npos);

    return true;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
