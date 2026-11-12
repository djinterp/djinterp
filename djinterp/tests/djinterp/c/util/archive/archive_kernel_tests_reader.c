/*******************************************************************************
* djinterp [test]                                  archive_kernel_tests_reader.c
*
*   The tar reader on archives its writer would not make, built here record by
* record: GNU long names, pax global headers, pax size records, typeflags it
* does not know, and damage -- a malformed record, a bad checksum, a cut.
* Every archive is read three ways, measure, extract and find, which share
* one walk.
*
*
* path:      /tests/djinterp/c/util/archive/archive_kernel_tests_reader.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "archive_kernel_tests.h"  // the suite's helpers
// std
#include <stdlib.h>  // free
#include <string.h>  // memcpy, memset, strlen


// dj_tests_ak_tar
//   struct: an archive being built by hand, record by record.
struct dj_tests_ak_tar
{
    unsigned char bytes[8192];
    size_t        size;
};


/*
d_tests_ak_put
  Append a header, then _data padded to a record.
*/
static void
d_tests_ak_put(
    struct dj_tests_ak_tar* _t,
    const char*             _name,
    char                    _type,
    const char*             _data,
    size_t                  _size
)
{
    d_tests_ak_tar_header(_t->bytes + _t->size, _name, _size, _type);
    _t->size += 512u;

    if (_size != 0u)
    {
        memset(_t->bytes + _t->size, 0, ((_size + 511u) / 512u) * 512u);
        memcpy(_t->bytes + _t->size, _data, _size);
        _t->size += ((_size + 511u) / 512u) * 512u;
    }

    return;
}

/*
d_tests_ak_end
  Append the two zero records that end an archive.
*/
static void
d_tests_ak_end(
    struct dj_tests_ak_tar* _t
)
{
    memset(_t->bytes + _t->size, 0, 1024u);
    _t->size += 1024u;

    return;
}

/*
d_tests_ak_reads
  Read a hand-built archive; 1 when it extracts and _name holds _data.
*/
static int
d_tests_ak_reads(
    const struct dj_tests_ak_tar* _t,
    const char*                   _name,
    const char*                   _data,
    size_t*                       _out_count
)
{
    struct dj_archive_entry* got = NULL;
    struct dj_archive_layout layout;
    enum dj_pack_status       status;
    int                      ok;

    ok = d_tests_ak_read(D_FORMAT_ID_TAR, _t->bytes, _t->size, &got,
                         &layout, &status) &&
         d_tests_ak_has(got, layout.entry_count, _name, strlen(_name),
                        _data, strlen(_data));
    *_out_count = ok ? layout.entry_count : 0u;
    free(got);

    return ok;
}

/*
d_tests_ak_reader_extensions
  Tests the following:
  - a GNU long name ('L') names the member after it, which find reaches
  - a pax global header ('g') is skipped
  - a pax size record overrides the header's size field
  - a typeflag the reader does not know is read as a member
*/
static void
d_tests_ak_reader_extensions(void)
{
    static struct dj_tests_ak_tar t;
    char                          longname[201];
    unsigned char                 one[8];
    size_t                        count = 0u;
    size_t                        n     = 0u;

    d_tests_ak_open("reader_extensions",
                    "GNU long names, pax global and size records, unknown "
                    "typeflags");

    memset(longname, 'L', 200);
    longname[100] = '/';
    longname[200] = '\0';

    t.size = 0u;
    d_tests_ak_put(&t, "././@LongLink", 'L', longname, 201u);
    d_tests_ak_put(&t, "LLLL", '0', "gnu", 3u);
    d_tests_ak_put(&t, "pax_global_header", 'g', "17 comment=hello\n", 17u);
    d_tests_ak_put(&t, "plain", '0', "abc", 3u);
    d_tests_ak_end(&t);
    D_TESTS_AK_CHECK(d_tests_ak_reads(&t, longname, "gnu", &count));
    D_TESTS_AK_CHECK(count == 2u);
    D_TESTS_AK_CHECK( (d_archive_extract_entry(
                           D_FORMAT_ID_TAR, t.bytes, t.size,
                           d_tests_ak_text(longname), NULL,
                           one, sizeof(one), &n) == D_PACK_STATUS_OK) &&
                      (n == 3u) &&
                      (memcmp(one, "gnu", 3) == 0) );

    // the header says 0 bytes; the pax record says 3, and it wins
    t.size = 0u;
    d_tests_ak_put(&t, "PaxHeader/sized", 'x', "10 size=3\n", 10u);
    d_tests_ak_tar_header(t.bytes + t.size, "sized", 0u, '0');
    memset(t.bytes + t.size + 512u, 0, 512u);
    memcpy(t.bytes + t.size + 512u, "pax", 3u);
    t.size += 1024u;
    d_tests_ak_put(&t, "contiguous", '7', "seven", 5u);
    d_tests_ak_end(&t);
    D_TESTS_AK_CHECK(d_tests_ak_reads(&t, "sized", "pax", &count));
    D_TESTS_AK_CHECK(d_tests_ak_reads(&t, "contiguous", "seven", &count));

    return;
}

/*
d_tests_ak_reader_damage
  Tests the following:
  - a pax record whose length is wrong is D_PACK_STATUS_CORRUPT_INPUT
  - a pax size that is not a number is D_PACK_STATUS_CORRUPT_INPUT
  - a header whose checksum does not match is D_PACK_STATUS_CORRUPT_INPUT
  - a cut inside a header or a payload is D_PACK_STATUS_TRUNCATED_INPUT,
    and a cut inside the end marker reads
*/
static void
d_tests_ak_reader_damage(void)
{
    static struct dj_tests_ak_tar t;
    struct dj_archive_layout      layout;

    d_tests_ak_open("reader_damage",
                    "malformed records, checksums and cuts are reported");

    t.size = 0u;
    d_tests_ak_put(&t, "PaxHeader/bad", 'x', "9 path=abcdef\n", 14u);
    d_tests_ak_put(&t, "bad", '0', "x", 1u);
    d_tests_ak_end(&t);
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, t.size,
                                       NULL, &layout) ==
                     D_PACK_STATUS_CORRUPT_INPUT);

    t.size = 0u;
    d_tests_ak_put(&t, "PaxHeader/nan", 'x', "10 size=x\n", 10u);
    d_tests_ak_put(&t, "nan", '0', "x", 1u);
    d_tests_ak_end(&t);
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, t.size,
                                       NULL, &layout) ==
                     D_PACK_STATUS_CORRUPT_INPUT);

    t.size = 0u;
    d_tests_ak_put(&t, "member", '0', "payload", 7u);
    d_tests_ak_end(&t);
    D_TESTS_AK_CHECK( (d_archive_measure(D_FORMAT_ID_TAR, t.bytes, t.size,
                                         NULL, &layout) ==
                       D_PACK_STATUS_OK) &&
                      (layout.entry_count == 1u) );
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, 300u,
                                       NULL, &layout) ==
                     D_PACK_STATUS_TRUNCATED_INPUT);
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, 515u,
                                       NULL, &layout) ==
                     D_PACK_STATUS_TRUNCATED_INPUT);
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, 1024u + 7u,
                                       NULL, &layout) == D_PACK_STATUS_OK);

    t.bytes[0] = (unsigned char)'M';
    D_TESTS_AK_CHECK(d_archive_measure(D_FORMAT_ID_TAR, t.bytes, t.size,
                                       NULL, &layout) ==
                     D_PACK_STATUS_CORRUPT_INPUT);

    return;
}

/*
d_tests_ak_reader_regions
  Tests the following:
  - extract into an arena one byte short is D_PACK_STATUS_BUFFER_TOO_SMALL,
    with the same layout measure reports
  - an entry array one short likewise
*/
static void
d_tests_ak_reader_regions(void)
{
    static struct dj_tests_ak_tar t;
    struct dj_archive_entry       slots[3];
    unsigned char                 arena[256];
    struct dj_archive_layout      need;
    struct dj_archive_layout      used;
    char                          longname[201];

    d_tests_ak_open("reader_regions",
                    "short regions report the full requirement");

    memset(longname, 'n', 200);
    longname[200] = '\0';
    t.size = 0u;
    d_tests_ak_put(&t, "././@LongLink", 'L', longname, 201u);
    d_tests_ak_put(&t, "n", '0', "first", 5u);
    d_tests_ak_put(&t, "second", '0', "two", 3u);
    d_tests_ak_end(&t);

    D_TESTS_AK_CHECK( (d_archive_measure(D_FORMAT_ID_TAR, t.bytes, t.size,
                                         NULL, &need) ==
                       D_PACK_STATUS_OK) &&
                      (need.entry_count == 2u) &&
                      (need.arena_size == 200u + 5u + 6u + 3u) );
    D_TESTS_AK_CHECK( (d_archive_extract(D_FORMAT_ID_TAR, t.bytes, t.size,
                                         NULL, slots, 2u, arena,
                                         need.arena_size - 1u, &used) ==
                       D_PACK_STATUS_BUFFER_TOO_SMALL) &&
                      (used.entry_count == need.entry_count) &&
                      (used.arena_size == need.arena_size) );
    D_TESTS_AK_CHECK( (d_archive_extract(D_FORMAT_ID_TAR, t.bytes, t.size,
                                         NULL, slots, 1u, arena,
                                         sizeof(arena), &used) ==
                       D_PACK_STATUS_BUFFER_TOO_SMALL) &&
                      (used.arena_size == need.arena_size) );

    return;
}


/*
d_tests_ak_reader
  The section: the tar reader on hand-built archives.
*/
void
d_tests_ak_reader(void)
{
    d_tests_ak_reader_extensions();
    d_tests_ak_reader_damage();
    d_tests_ak_reader_regions();

    return;
}
