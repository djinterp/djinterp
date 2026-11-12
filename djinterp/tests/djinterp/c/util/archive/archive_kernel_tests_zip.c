/*******************************************************************************
* djinterp [test]                                     archive_kernel_tests_zip.c
*
*   The ZIP reader's checks, its methods, and the writer's limits.  Every
* member read is checked against its record -- size and CRC -- so damage the
* codec cannot see, a stored member's bytes most of all, is reported; every
* offset is checked before it is followed; lzma, which the codec set cannot
* make, is refused both ways; and what the classic records cannot hold is
* refused rather than written cut short.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests_zip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's helpers
// std
#include <stdlib.h>  // calloc, free
#include <string.h>  // memcmp, memset
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


/*
d_tests_ak_zip_find
  The offset of the first central directory record, or 0.
*/
static size_t
d_tests_ak_zip_find(
    const unsigned char* _zip,
    size_t               _size
)
{
    size_t i;

    for (i = 0; (i + 4u) <= _size; ++i)
    {
        if (memcmp(_zip + i, "PK\001\002", 4) == 0)
        {
            return i;
        }
    }

    return 0u;
}

/*
d_tests_ak_zip_damage
  Tests the following:
  - a flipped byte in a stored member is D_PACK_STATUS_CORRUPT_INPUT, by
    extract and by extract_entry, though no codec ran
  - a central record whose local offset points past the input is
    D_PACK_STATUS_CORRUPT_INPUT, by measure and by extract_entry
*/
static void
d_tests_ak_zip_damage(void)
{
    struct dj_archive_entry   e;
    struct dj_archive_options opt;
    struct dj_archive_entry*  got = NULL;
    struct dj_archive_layout  layout;
    unsigned char             one[16];
    unsigned char*            zip;
    size_t                    size = 0u;
    size_t                    n    = 0u;
    size_t                    cd;
    enum dj_pack_status        status;

    d_tests_ak_open("zip_damage",
                    "damage a codec cannot see, and offsets out of range");

    d_archive_options_init(&opt);
    opt.store_only = 1;
    e   = d_tests_ak_file("stored.txt", "stored bytes", 12u);
    zip = d_tests_ak_create(D_FORMAT_ID_ZIP, &e, 1u, &opt, &size, &status);
    D_TESTS_AK_CHECK(zip != NULL);

    if (zip == NULL)
    {
        return;
    }

    // the payload follows the 30-byte local header and the 10-byte name
    zip[30 + 10 + 3] ^= 0x01u;
    D_TESTS_AK_CHECK( !d_tests_ak_read(D_FORMAT_ID_ZIP, zip, size, &got,
                                       &layout, &status) &&
                      (status == D_PACK_STATUS_CORRUPT_INPUT) );
    D_TESTS_AK_CHECK(d_archive_extract_entry(D_FORMAT_ID_ZIP, zip, size,
                                             e.name, NULL, one, sizeof(one),
                                             &n) ==
                     D_PACK_STATUS_CORRUPT_INPUT);
    zip[30 + 10 + 3] ^= 0x01u;
    D_TESTS_AK_CHECK(d_tests_ak_read(D_FORMAT_ID_ZIP, zip, size, &got,
                                     &layout, &status));
    free(got);

    // the local header offset, 42 bytes into the central record
    cd = d_tests_ak_zip_find(zip, size);
    D_TESTS_AK_CHECK(cd != 0u);
    zip[cd + 42] = 0xFFu;
    zip[cd + 43] = 0xFFu;
    zip[cd + 44] = 0xFFu;
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_ZIP, zip, size, NULL,
                                       &layout) ==
                     D_PACK_STATUS_CORRUPT_INPUT);
    D_TESTS_AK_CHECK(d_archive_extract_entry(D_FORMAT_ID_ZIP, zip, size,
                                             e.name, NULL, one, sizeof(one),
                                             &n) ==
                     D_PACK_STATUS_CORRUPT_INPUT);
    free(zip);

    return;
}

/*
d_tests_ak_zip_methods
  Tests the following:
  - each method the build can compress with round-trips: deflate, bzip2,
    zstd, xz
  - lzma is D_PACK_STATUS_UNSUPPORTED to write, naming the method knob, and
    to read: ZIP's LZMA is raw LZMA1, which the codec set does not make
  - a number that is no ZIP method is D_PACK_STATUS_INVALID_ARGUMENT
*/
static void
d_tests_ak_zip_methods(void)
{
    static const int32_t      methods[4] = { D_ZIP_METHOD_DEFLATE,
                                             D_ZIP_METHOD_BZIP2,
                                             D_ZIP_METHOD_ZSTD,
                                             D_ZIP_METHOD_XZ };
    static const int32_t      codecs[4]  = { D_CODEC_ID_DEFLATE,
                                             D_CODEC_ID_BZIP2,
                                             D_CODEC_ID_ZSTD,
                                             D_CODEC_ID_XZ };
    char                      body[2000];
    struct dj_archive_entry   e;
    struct dj_archive_options opt;
    struct dj_archive_entry*  got = NULL;
    struct dj_archive_layout  layout;
    enum dj_archive_knob      offender = D_ARCHIVE_KNOB_LEVEL;
    unsigned char*            zip;
    size_t                    size = 0u;
    size_t                    cd;
    size_t                    i;
    enum dj_pack_status        status;

    d_tests_ak_open("zip_methods",
                    "the methods written and read, and lzma refused");

    for (i = 0; i < sizeof(body); ++i)
    {
        body[i] = "abcabcabd"[i % 9u];
    }

    e = d_tests_ak_file("m.txt", body, sizeof(body));
    d_archive_options_init(&opt);

    for (i = 0; i < 4u; ++i)
    {
        if (!d_codec_can_compress((enum dj_codec_id)codecs[i]))
        {
            continue;
        }

        opt.zip.method = methods[i];
        zip = d_tests_ak_create(D_FORMAT_ID_ZIP, &e, 1u, &opt, &size,
                                &status);
        D_TESTS_AK_CHECK( (zip != NULL) &&
                          (zip[8] == (unsigned char)methods[i]) &&
                          d_tests_ak_read(D_FORMAT_ID_ZIP, zip, size, &got,
                                          &layout, &status) &&
                          d_tests_ak_has(got, 1u, "m.txt", 5u, body,
                                         sizeof(body)) );
        free(got);
        got = NULL;

        // relabel the member lzma: the reader refuses it
        if ( (zip != NULL) &&
             ((cd = d_tests_ak_zip_find(zip, size)) != 0u) )
        {
            zip[8]       = (unsigned char)D_ZIP_METHOD_LZMA;
            zip[cd + 10] = (unsigned char)D_ZIP_METHOD_LZMA;
            D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_ZIP, zip, size,
                                               NULL, &layout) ==
                             D_PACK_STATUS_UNSUPPORTED);
        }

        free(zip);
    }

    opt.zip.method = D_ZIP_METHOD_LZMA;
    D_TESTS_AK_CHECK( (d_archive_options_validate(D_FORMAT_ID_ZIP, &opt,
                                                  &offender) ==
                       D_PACK_STATUS_UNSUPPORTED) &&
                      (offender == D_ARCHIVE_KNOB_ZIP_METHOD) );
    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, &e, 1u, &opt, NULL,
                                      0u, &size) ==
                     D_PACK_STATUS_UNSUPPORTED);
    opt.zip.method = 7;
    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, &e, 1u, &opt, NULL,
                                      0u, &size) ==
                     D_PACK_STATUS_INVALID_ARGUMENT);

    return;
}

/*
d_tests_ak_zip_limits
  Tests the following:
  - 65536 members, one more than the classic record counts, is
    D_PACK_STATUS_UNSUPPORTED, from create and from stage_size
  - 65535 members, the most they count, measure
*/
static void
d_tests_ak_zip_limits(void)
{
    struct dj_archive_entry* many;
    size_t                   size = 0u;
    size_t                   i;

    d_tests_ak_open("zip_limits",
                    "what the classic records cannot hold is refused");

    many = (struct dj_archive_entry*)calloc(65536u, sizeof(*many));

    if (many == NULL)
    {
        return;
    }

    for (i = 0; i < 65536u; ++i)
    {
        many[i] = d_tests_ak_file("m", NULL, 0u);
    }

    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, many, 65536u, NULL,
                                      NULL, 0u, &size) ==
                     D_PACK_STATUS_UNSUPPORTED);
    D_TESTS_AK_CHECK(d_archive_stage_size(D_FORMAT_ID_ZIP, many, 65536u,
                                          NULL, &size) ==
                     D_PACK_STATUS_UNSUPPORTED);
    D_TESTS_AK_CHECK(d_archive_create(D_FORMAT_ID_ZIP, many, 65535u, NULL,
                                      NULL, 0u, &size) == D_PACK_STATUS_OK);
    free(many);

    return;
}


/*
d_tests_ak_zip
  The section: the ZIP reader's checks and the writer's limits.
*/
void
d_tests_ak_zip(void)
{
    d_tests_ak_zip_damage();
    d_tests_ak_zip_methods();
    d_tests_ak_zip_limits();

    return;
}
