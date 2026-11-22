/*******************************************************************************
* djinterp [c][utility]                              archive_kernel_tests_gzip.c
*
*   gzip's header, the name and time RFC 1952 lets a stream carry: written and
* read by the compress kernel's gzip pair, and by the archive kernel for a .gz
* and a .tar.gz from the gz knobs.  The defaults store neither, and stay byte
* for byte what the GZIP codec writes.  A file written by `gzip -N` is read
* back with its name and time.
*   The OS byte is 255, RFC 1952's "unknown", on every machine unless a
* header, the option sets' gzip.os and gz.os, or the build choose another, and
* "zlib's" is one of the choices.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests_gzip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's helpers
// std
#include <stdlib.h>  // free
#include <string.h>  // memcmp, memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/util/compress/compress_common.h"
                                         // dj_gzip_header, the gzip pair
#if D_INTERNAL_COMPRESS_DEFLATE
#include <zlib.h>  // deflate*: the byte zlib writes of its own choosing
#endif


// d_tests_ak_gzip_n
//   constant: "hello, gzip\n" as `gzip -N` wrote it, from a file hello.txt
// whose time was 1700000000 (2023-11-14 22:13:20 UTC), on a Unix (OS 3).
static const unsigned char d_tests_ak_gzip_n[42] =
{
    0x1f, 0x8b, 0x08, 0x08, 0x00, 0xf1, 0x53, 0x65, 0x00, 0x03, 0x68, 0x65,
    0x6c, 0x6c, 0x6f, 0x2e, 0x74, 0x78, 0x74, 0x00, 0xcb, 0x48, 0xcd, 0xc9,
    0xc9, 0xd7, 0x51, 0x48, 0xaf, 0xca, 0x2c, 0xe0, 0x02, 0x00, 0x86, 0x1f,
    0x82, 0xa4, 0x0c, 0x00, 0x00, 0x00
};

// d_tests_ak_gzip_text
//   constant: the payload every test here compresses.
static const char d_tests_ak_gzip_text[] = "hello, gzip\n";

// d_tests_ak_gzip_bytes, d_tests_ak_gzip_size
//   the bytes the collecting sink has taken.
static unsigned char d_tests_ak_gzip_bytes[512];
static size_t        d_tests_ak_gzip_size = 0u;


/*
d_tests_ak_gzip_collect
  A sink write that keeps what fits in d_tests_ak_gzip_bytes, and takes
nothing more.
*/
static size_t
d_tests_ak_gzip_collect(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;

    if (_size > (sizeof(d_tests_ak_gzip_bytes) - d_tests_ak_gzip_size))
    {
        return 0u;
    }

    memcpy(d_tests_ak_gzip_bytes + d_tests_ak_gzip_size, _data, _size);
    d_tests_ak_gzip_size += _size;

    return _size;
}

/*
d_tests_ak_gzip_sink
  A collecting sink, emptied.
*/
static struct dj_pack_sink
d_tests_ak_gzip_sink(void)
{
    struct dj_pack_sink sink;

    d_tests_ak_gzip_size = 0u;
    sink.write           = &d_tests_ak_gzip_collect;
    sink.context         = NULL;

    return sink;
}

/*
d_tests_ak_gzip_drop
  A sink write that takes everything and keeps nothing: for a tar, too big to
collect, whose header alone is checked.
*/
static size_t
d_tests_ak_gzip_drop(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;

    return _size;
}

/*
d_tests_ak_gzip_is
  Whether a span holds exactly _s.
*/
static int
d_tests_ak_gzip_is(
    struct dj_pack_text _t,
    const char*         _s
)
{
    return ( (_t.length == strlen(_s)) &&
             ( (_t.length == 0u) ||
               (memcmp(_t.data, _s, _t.length) == 0) ) );
}


/*
d_tests_ak_gzip_codec
  The compress kernel's gzip pair.
  Tests the following:
  - a null header, and one with no name or time, write exactly what the GZIP
    codec writes, OS byte included
  - a name and a time come back as written, with the codec's OS byte
  - an OS byte given is written as given
  - `gzip -N`'s file reads back with its name, time and OS byte
  - a name longer than the buffer is cut, NUL-terminated, and flagged; with
    no buffer the name is flagged and empty; one that just fits is whole
  - a name holding a NUL, and an OS byte outside 0 to 255, are refused
*/
static void
d_tests_ak_gzip_codec(void)
{
    struct dj_gzip_header header = D_GZIP_HEADER_NONE;
    struct dj_gzip_header read   = D_GZIP_HEADER_NONE;
    unsigned char         plain[512];
    size_t                plain_size = 0u;
    char                  name[64];
    size_t                n = strlen(d_tests_ak_gzip_text);

    d_tests_ak_open("gzip_header_codec",
                    "the gzip pair writes and reads FNAME, MTIME and OS; with "
                    "none, the GZIP codec's bytes exactly");

    D_TESTS_AK_CHECK(d_compress_to_sink(D_CODEC_ID_GZIP, d_tests_ak_gzip_text,
                                        n, NULL, d_tests_ak_gzip_sink(),
                                        NULL) == D_PACK_STATUS_OK);
    plain_size = d_tests_ak_gzip_size;
    memcpy(plain, d_tests_ak_gzip_bytes, plain_size);

    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             NULL, d_tests_ak_gzip_sink(),
                                             NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (d_tests_ak_gzip_size == plain_size) &&
                      (memcmp(d_tests_ak_gzip_bytes, plain, plain_size) == 0) );
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (d_tests_ak_gzip_size == plain_size) &&
                      (memcmp(d_tests_ak_gzip_bytes, plain, plain_size) == 0) );

    // a name and a time, round trip
    header.name  = d_pack_text_from_cstr("notes.txt");
    header.mtime = 1700000000u;
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_bytes[9] == plain[9]);
    {
        unsigned char stream[512];
        size_t        size = d_tests_ak_gzip_size;

        memcpy(stream, d_tests_ak_gzip_bytes, size);
        D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(stream, size, &read, name,
                                                   sizeof(name),
                                                   d_tests_ak_gzip_sink(),
                                                   NULL) == D_PACK_STATUS_OK);
    }
    D_TESTS_AK_CHECK(d_tests_ak_gzip_is(read.name, "notes.txt") &&
                     (read.mtime == 1700000000u) &&
                     (read.os == (int32_t)plain[9]) &&
                     (!read.name_truncated));
    D_TESTS_AK_CHECK( (d_tests_ak_gzip_size == n) &&
                      (memcmp(d_tests_ak_gzip_bytes, d_tests_ak_gzip_text,
                              n) == 0) );

    // an OS byte given
    header.os = D_GZIP_OS_UNKNOWN;
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_bytes[9] == 255u);

    // gzip -N's file
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(d_tests_ak_gzip_n,
                                               sizeof(d_tests_ak_gzip_n),
                                               &read, name, sizeof(name),
                                               d_tests_ak_gzip_sink(),
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_is(read.name, "hello.txt") &&
                     (read.mtime == 1700000000u) &&
                     (read.os == 3));
    D_TESTS_AK_CHECK( (d_tests_ak_gzip_size == n) &&
                      (memcmp(d_tests_ak_gzip_bytes, d_tests_ak_gzip_text,
                              n) == 0) );

    // the name's room: cut, none, just enough
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(d_tests_ak_gzip_n,
                                               sizeof(d_tests_ak_gzip_n),
                                               &read, name, 5u,
                                               d_tests_ak_gzip_sink(),
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_is(read.name, "hell") &&
                     read.name_truncated);
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(d_tests_ak_gzip_n,
                                               sizeof(d_tests_ak_gzip_n),
                                               &read, NULL, 0u,
                                               d_tests_ak_gzip_sink(),
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (read.name.length == 0u) &&
                      read.name_truncated &&
                      (read.mtime == 1700000000u) );
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(d_tests_ak_gzip_n,
                                               sizeof(d_tests_ak_gzip_n),
                                               &read, name, 10u,
                                               d_tests_ak_gzip_sink(),
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_is(read.name, "hello.txt") &&
                     (!read.name_truncated));

    // refusals
    header.name = d_pack_text_from_span("a\0b", 3u);
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);
    header.name = d_pack_text_from_cstr("ok");
    header.os   = 256;
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, n, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);

    return;
}

/*
d_tests_ak_gzip_gz
  A .gz through the archive kernel.
  Tests the following:
  - with the defaults the .gz is the GZIP codec's bytes, and reads back with
    no name and no time
  - with gz.store_name and gz.store_mtime it reads back with the entry's
    name, its last component only, and its time
  - gz.original_name, when set, is the name stored
  - a time RFC 1952 can't hold is stored as none
  - `gzip -N`'s file reads as a .gz named hello.txt, with its time, and the
    arena holds the name and then the payload
*/
static void
d_tests_ak_gzip_gz(void)
{
    struct dj_archive_entry   entry;
    struct dj_archive_entry*  got = NULL;
    struct dj_archive_layout  layout;
    struct dj_archive_options opt;
    enum dj_pack_status       status;
    unsigned char*            gz;
    size_t                    gz_size = 0u;
    size_t                    n = strlen(d_tests_ak_gzip_text);

    d_tests_ak_open("gz_names_and_times",
                    "a .gz stores its entry's name and time only when asked, "
                    "and reads back what it stores");

    entry       = d_tests_ak_file("docs/notes.txt", d_tests_ak_gzip_text, n);
    entry.mtime = 1700000000;
    d_archive_options_init(&opt);

    // the defaults
    gz = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &gz_size,
                           &status);
    D_TESTS_AK_CHECK(status == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_compress_to_sink(D_CODEC_ID_GZIP, d_tests_ak_gzip_text,
                                        n, NULL, d_tests_ak_gzip_sink(),
                                        NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (gz != NULL) &&
                      (gz_size == d_tests_ak_gzip_size) &&
                      (memcmp(gz, d_tests_ak_gzip_bytes, gz_size) == 0) );
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_GZ, gz, gz_size, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      (got[0].name.length == 0u) &&
                      (got[0].mtime == 0) &&
                      (got[0].data.size == n) );
    free(got);
    free(gz);

    // the name and the time, asked for
    opt.gz.store_name  = 1;
    opt.gz.store_mtime = 1;
    gz = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &gz_size,
                           &status);
    D_TESTS_AK_CHECK(status == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_GZ, gz, gz_size, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      d_tests_ak_gzip_is(got[0].name, "notes.txt") &&
                      (got[0].mtime == 1700000000) &&
                      (got[0].data.size == n) &&
                      (memcmp(got[0].data.data, d_tests_ak_gzip_text,
                              n) == 0) );
    D_TESTS_AK_CHECK(layout.arena_size == (strlen("notes.txt") + 1u + n));
    free(got);
    free(gz);

    // gz.original_name, and a time out of RFC 1952's range
    opt.gz.original_name = d_pack_text_from_cstr("original.txt");
    entry.mtime          = -5;
    gz = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &gz_size,
                           &status);
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_GZ, gz, gz_size, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      d_tests_ak_gzip_is(got[0].name, "original.txt") &&
                      (got[0].mtime == 0) );
    free(got);
    free(gz);

    // gzip -N's file, as a .gz
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_GZ, d_tests_ak_gzip_n,
                                     sizeof(d_tests_ak_gzip_n), &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      d_tests_ak_gzip_is(got[0].name, "hello.txt") &&
                      (got[0].mtime == 1700000000) &&
                      (got[0].data.size == n) &&
                      ((const char*)got[0].data.data ==
                       got[0].name.data + got[0].name.length + 1u) );
    free(got);

    return;
}

/*
d_tests_ak_gzip_tar_gz
  A .tar.gz's stream header.
  Tests the following:
  - with the defaults it carries no name and no time
  - with the knobs it carries gz.original_name and the newest member's time,
    and its members read back as before
*/
static void
d_tests_ak_gzip_tar_gz(void)
{
    struct dj_archive_entry   entries[2];
    struct dj_archive_entry*  got = NULL;
    struct dj_archive_layout  layout;
    struct dj_archive_options opt;
    struct dj_gzip_header     read;
    enum dj_pack_status       status;
    unsigned char*            tgz;
    size_t                    tgz_size = 0u;
    struct dj_pack_sink       drop;
    char                      name[64];
    size_t                    n = strlen(d_tests_ak_gzip_text);

    drop.write   = &d_tests_ak_gzip_drop;
    drop.context = NULL;

    d_tests_ak_open("tar_gz_header",
                    "a .tar.gz's stream carries gz.original_name and its "
                    "newest member's time, only when asked");

    entries[0]       = d_tests_ak_file("a.txt", d_tests_ak_gzip_text, n);
    entries[0].mtime = 1600000000;
    entries[1]       = d_tests_ak_file("b.txt", d_tests_ak_gzip_text, n);
    entries[1].mtime = 1700000000;
    d_archive_options_init(&opt);

    tgz = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, entries, 2u, &opt, &tgz_size,
                            &status);
    D_TESTS_AK_CHECK( (status == D_PACK_STATUS_OK) &&
                      (tgz != NULL) );
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(tgz, tgz_size, &read, name,
                                               sizeof(name), drop,
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK( (read.name.length == 0u) &&
                      (read.mtime == 0u) );
    free(tgz);

    opt.gz.store_name    = 1;
    opt.gz.store_mtime   = 1;
    opt.gz.original_name = d_pack_text_from_cstr("bundle.tar");
    tgz = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, entries, 2u, &opt, &tgz_size,
                            &status);
    D_TESTS_AK_CHECK( (status == D_PACK_STATUS_OK) &&
                      (tgz != NULL) );
    D_TESTS_AK_CHECK(d_decompress_gzip_to_sink(tgz, tgz_size, &read, name,
                                               sizeof(name), drop,
                                               NULL) == D_PACK_STATUS_OK);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_is(read.name, "bundle.tar") &&
                     (read.mtime == 1700000000u));
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_TAR_GZ, tgz, tgz_size, &got,
                                     &layout, &status));
    D_TESTS_AK_CHECK( (got != NULL) &&
                      (layout.entry_count == 2u) &&
                      d_tests_ak_has(got, layout.entry_count, "b.txt", 5u,
                                     d_tests_ak_gzip_text, n) );
    free(got);
    free(tgz);

    return;
}


/*
d_tests_ak_gzip_zlib_os
  The OS byte zlib writes when it writes a header of its own choosing, from
the header of an empty stream; D_GZIP_OS_UNKNOWN when there is no zlib.
*/
static int
d_tests_ak_gzip_zlib_os(void)
{
    int os = D_GZIP_OS_UNKNOWN;
#if D_INTERNAL_COMPRESS_DEFLATE
    unsigned char out[32];
    z_stream      zs;

    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16,
                     8, Z_DEFAULT_STRATEGY) == Z_OK)
    {
        zs.next_out  = out;
        zs.avail_out = (uInt)sizeof(out);

        if (deflate(&zs, Z_FINISH) == Z_STREAM_END)
        {
            os = (int)out[9];
        }

        (void)deflateEnd(&zs);
    }
#endif

    return os;
}

/*
d_tests_ak_gzip_os_byte
  The byte the GZIP codec writes under _options, with _header (may be null)
through the gzip pair; -1 when it fails.
*/
static int
d_tests_ak_gzip_os_byte(
    const struct dj_compress_options* _options,
    const struct dj_gzip_header*      _header
)
{
    return ( (d_compress_gzip_to_sink(d_tests_ak_gzip_text,
                                      strlen(d_tests_ak_gzip_text), _options,
                                      _header, d_tests_ak_gzip_sink(),
                                      NULL) == D_PACK_STATUS_OK) &&
             (d_tests_ak_gzip_size > 9u) )
         ? (int)d_tests_ak_gzip_bytes[9]
         : -1;
}

/*
d_tests_ak_gzip_os_codec
  The OS byte through the compress kernel: the gzip.os knob, its default and
its choices.
  Tests the following:
  - by default the GZIP codec, and the gzip pair with no header or an empty
    one, write 255 ("unknown"), so the bytes are the same on every machine
  - resolving for GZIP pins gzip.os at the build's 255; resolving for
    another codec leaves it unset
  - gzip.os set to 3 is written as 3; a header's os of 7 wins over it; a
    header's D_GZIP_OS_DEFAULT takes it
  - D_GZIP_OS_ZLIB, in the option set or the header, writes zlib's own byte
  - gzip.os is knob 50, "gzip.os", the GZIP codec's
  - validation refuses 256 and -3 for gzip.os, and the gzip pair refuses a
    header os of -3
*/
static void
d_tests_ak_gzip_os_codec(void)
{
    struct dj_compress_options opt;
    struct dj_compress_options res;
    struct dj_gzip_header      header = D_GZIP_HEADER_NONE;
    enum dj_compress_knob      knob   = D_COMPRESS_KNOB_LEVEL;
    const int                  zlib_os = d_tests_ak_gzip_zlib_os();

    d_tests_ak_open("gzip_os_codec",
                    "the GZIP codec's OS byte is 255 by default, and gzip.os "
                    "or a header chooses another, zlib's included");

    // the default, every way the codec is reached
    D_TESTS_AK_CHECK( (d_compress_to_sink(D_CODEC_ID_GZIP,
                                          d_tests_ak_gzip_text,
                                          strlen(d_tests_ak_gzip_text), NULL,
                                          d_tests_ak_gzip_sink(),
                                          NULL) == D_PACK_STATUS_OK) &&
                      (d_tests_ak_gzip_bytes[9] == 255u) );
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(NULL, NULL) == 255);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(NULL, &header) == 255);

    // resolution
    D_TESTS_AK_CHECK( (d_compress_options_resolve(D_CODEC_ID_GZIP, NULL,
                                                  &res) == D_PACK_STATUS_OK) &&
                      (res.gzip.os == D_GZIP_OS_UNKNOWN) );
    D_TESTS_AK_CHECK( (d_compress_options_resolve(D_CODEC_ID_ZLIB, NULL,
                                                  &res) == D_PACK_STATUS_OK) &&
                      (res.gzip.os == D_COMPRESS_KNOB_UNSET) );

    // the knob, and a header over it
    d_compress_options_init(&opt);
    opt.gzip.os = 3;
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(&opt, NULL) == 3);
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(&opt, &header) == 3);
    header.os = 7;
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(&opt, &header) == 7);

    // zlib's own
    opt.gzip.os = D_GZIP_OS_ZLIB;
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(&opt, NULL) == zlib_os);
    d_compress_options_init(&opt);
    header.os = D_GZIP_OS_ZLIB;
    D_TESTS_AK_CHECK(d_tests_ak_gzip_os_byte(&opt, &header) == zlib_os);

    // the knob table
    D_TESTS_AK_CHECK( (D_COMPRESS_KNOB_GZIP_OS == 50) &&
                      (D_COMPRESS_KNOB_COUNT == 51) );
    D_TESTS_AK_CHECK(strcmp(d_compress_knob_name(D_COMPRESS_KNOB_GZIP_OS),
                            "gzip.os") == 0);
    D_TESTS_AK_CHECK(d_compress_knob_from_name("gzip.os", &knob) &&
                     (knob == D_COMPRESS_KNOB_GZIP_OS));
    D_TESTS_AK_CHECK(d_compress_knob_codec(D_COMPRESS_KNOB_GZIP_OS) ==
                     D_CODEC_ID_GZIP);

    // refusals
    opt.gzip.os = 256;
    D_TESTS_AK_CHECK( (d_compress_options_validate(D_CODEC_ID_GZIP, &opt,
                                                   &knob) !=
                       D_PACK_STATUS_OK) &&
                      (knob == D_COMPRESS_KNOB_GZIP_OS) );
    opt.gzip.os = -3;
    D_TESTS_AK_CHECK(d_compress_options_validate(D_CODEC_ID_GZIP, &opt,
                                                 NULL) != D_PACK_STATUS_OK);
    header.os = -3;
    D_TESTS_AK_CHECK(d_compress_gzip_to_sink(d_tests_ak_gzip_text, 1u, NULL,
                                             &header, d_tests_ak_gzip_sink(),
                                             NULL) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);

    return;
}

/*
d_tests_ak_gzip_os_archive
  The OS byte through the archive kernel: gz.os over the codec block's
gzip.os, in a .gz and a .tar.gz.
  Tests the following:
  - by default a .gz and a .tar.gz carry 255
  - the codec block's gzip.os is used when gz.os is unset, and gz.os wins
    when set; D_GZIP_OS_ZLIB gives zlib's byte
  - d_archive_reproducible fixes gz.os at 255
  - gz.os is "gz.os", before gz.original_name in the knob index
  - validation refuses gz.os 300 for a .gz, naming gz.os
*/
static void
d_tests_ak_gzip_os_archive(void)
{
    struct dj_archive_entry   entry;
    struct dj_archive_options opt;
    enum dj_archive_knob      offender = D_ARCHIVE_KNOB_LEVEL;
    enum dj_pack_status       status;
    unsigned char*            out;
    size_t                    size = 0u;
    size_t                    n = strlen(d_tests_ak_gzip_text);

    d_tests_ak_open("gzip_os_archive",
                    "a .gz and a .tar.gz carry OS 255 by default; gz.os wins "
                    "over the codec block's gzip.os");

    entry = d_tests_ak_file("a.txt", d_tests_ak_gzip_text, n);
    d_archive_options_init(&opt);

    // the defaults
    out = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) && (out[9] == 255u) );
    free(out);
    out = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) && (out[9] == 255u) );
    free(out);

    // the codec block's, then gz.os over it
    opt.codec.gzip.os = 3;
    out = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) && (out[9] == 3u) );
    free(out);
    opt.gz.os = 11;
    out = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) && (out[9] == 11u) );
    free(out);
    opt.gz.os = 0;
    out = d_tests_ak_create(D_FORMAT_ID_TAR_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) && (out[9] == 0u) );
    free(out);
    opt.gz.os = D_GZIP_OS_ZLIB;
    out = d_tests_ak_create(D_FORMAT_ID_GZ, &entry, 1u, &opt, &size,
                            &status);
    D_TESTS_AK_CHECK( (out != NULL) && (size > 9u) &&
                      ((int)out[9] == d_tests_ak_gzip_zlib_os()) );
    free(out);

    // the reproducible preset, the index, and a refusal
    D_TESTS_AK_CHECK(d_archive_reproducible().gz.os == D_GZIP_OS_UNKNOWN);
    D_TESTS_AK_CHECK(strcmp(d_archive_knob_name(D_ARCHIVE_KNOB_GZ_OS),
                            "gz.os") == 0);
    D_TESTS_AK_CHECK( (D_ARCHIVE_KNOB_GZ_OS + 1 ==
                       D_ARCHIVE_KNOB_GZ_ORIGINAL_NAME) &&
                      (!d_archive_knob_is_text(D_ARCHIVE_KNOB_GZ_OS)) );
    opt.gz.os = 300;
    D_TESTS_AK_CHECK( (d_archive_options_validate(D_FORMAT_ID_GZ, &opt,
                                                  &offender) ==
                       D_PACK_STATUS_INVALID_ARGUMENT) &&
                      (offender == D_ARCHIVE_KNOB_GZ_OS) );

    return;
}


/*
d_tests_ak_gzip
  The section: gzip's header, in the codec and in the archive kernel.
*/
void
d_tests_ak_gzip(void)
{
    d_tests_ak_gzip_codec();
    d_tests_ak_gzip_gz();
    d_tests_ak_gzip_tar_gz();
    d_tests_ak_gzip_os_codec();
    d_tests_ak_gzip_os_archive();

    return;
}
