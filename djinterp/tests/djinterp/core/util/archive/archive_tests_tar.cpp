/*******************************************************************************
* djinterp [test]                                          archive_tests_tar.cpp
*
*   Tests for the ustar (tar) writer and reader: file and directory round-
* trips, metadata defaults, 100-byte name truncation, the header checksum,
* truncation handling, and the empty archive.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_tar.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // ar_tar_parse_octal
    //   function: parse a space/NUL-padded octal field of _width bytes from
    // _p, mirroring the reader's own parser so the checksum test is
    // independent of it.
    //
    // Parameter(s):
    //   _p:     start of the field.
    //   _width: field width in bytes.
    // Return:
    //   the decoded value.
    unsigned long
    ar_tar_parse_octal(
        const char* _p,
        std::size_t _width
    )
    {
        unsigned long v = 0;
        std::size_t   i;

        for (i = 0; i < _width; ++i)
        {
            char c = _p[i];

            if ( (c >= '0') &&
                 (c <= '7') )
            {
                v = (v << 3) + (unsigned long)(c - '0');
            }
        }

        return v;
    }
}   // namespace


/*
tests_tar_roundtrip_files
  Verifies a set of regular files survives a tar create/extract round-trip and
that the produced stream has ustar shape.
  Tests the following:
  - text, binary (embedded-NUL), and empty files all round-trip
  - payloads come back byte-identical, in order, with the right count
  - the produced bytes carry the ustar magic and the two-block terminator
*/
bool
tests_tar_roundtrip_files()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("readme.txt", "hello tar"));
    items.push_back(::djinterp::test::make_file_entry("data.bin", binary_sample()));
    items.push_back(::djinterp::test::make_file_entry("empty.dat", byte_blob()));

    // structural shape of the produced container
    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<tar>(items, blob), status_ok);
    D_AR_CHECK(::djinterp::test::tar_has_ustar_magic(blob));
    D_AR_CHECK(::djinterp::test::tar_is_terminated(blob));

    // round-trip payload preservation
    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<tar>(items, out), status_ok);
    D_AR_REQUIRE(out.size() == 3u);
    D_AR_CHECK(::djinterp::test::files_preserved(items, out));

    // the empty file is a regular file with no data
    {
        const entry* e = ::djinterp::test::find_entry(out, "empty.dat");
        D_AR_REQUIRE(e != (const entry*)0);
        D_AR_CHECK(e->is_directory == false);
        D_AR_CHECK(e->data.empty());
    }

    return _ok;
}

/*
tests_tar_directories
  Verifies directory members round-trip through tar.
  Tests the following:
  - a directory entry is written with the directory typeflag and comes back
    flagged is_directory with no data
  - a mixed list preserves both the directory and the file
*/
bool
tests_tar_directories()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_dir_entry("docs"));
    items.push_back(::djinterp::test::make_text_entry("docs/a.txt", "inside"));

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<tar>(items, out), status_ok);
    D_AR_REQUIRE(out.size() == 2u);

    {
        const entry* d = ::djinterp::test::find_entry(out, "docs");
        D_AR_REQUIRE(d != (const entry*)0);
        D_AR_CHECK(d->is_directory == true);
        D_AR_CHECK(d->data.empty());
    }

    D_AR_CHECK(::djinterp::test::count_dirs(out) == 1u);
    D_AR_CHECK(::djinterp::test::files_preserved(items, out));

    return _ok;
}

/*
tests_tar_metadata
  Verifies permission and time fields round-trip, and that the writer's
defaults are applied when the entry leaves them at 0.
  Tests the following:
  - a custom mode and mtime survive the round-trip exactly
  - a file left at mode 0 comes back as the default 0644
  - a directory left at mode 0 comes back as the default 0755
  - a file left at mtime 0 comes back with a real (non-zero) time
*/
bool
tests_tar_metadata()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_entry("custom",
                                                 byte_blob("x"),
                                                 false,
                                                 0600u,
                                                 1000000000L));
    items.push_back(::djinterp::test::make_file_entry("defmode", byte_blob("y")));
    items.push_back(::djinterp::test::make_dir_entry("defdir"));

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<tar>(items, out), status_ok);

    {
        const entry* c = ::djinterp::test::find_entry(out, "custom");
        D_AR_REQUIRE(c != (const entry*)0);
        D_AR_CHECK(c->mode == 0600u);
        D_AR_CHECK(c->mtime == 1000000000L);
    }
    {
        const entry* f = ::djinterp::test::find_entry(out, "defmode");
        D_AR_REQUIRE(f != (const entry*)0);
        D_AR_CHECK(f->mode == 0644u);
        D_AR_CHECK(f->mtime > 0L);          // writer substituted "now"
    }
    {
        const entry* d = ::djinterp::test::find_entry(out, "defdir");
        D_AR_REQUIRE(d != (const entry*)0);
        D_AR_CHECK(d->mode == 0755u);
    }

    return _ok;
}

/*
tests_tar_name_truncation
  Verifies the writer's defensive 100-byte name field.
  Tests the following:
  - a name longer than 100 bytes is truncated to its first 100 bytes
  - a name of exactly 100 bytes is preserved in full
*/
bool
tests_tar_name_truncation()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    const std::string head100(100, 'a');
    const std::string long_name = head100 + std::string(20, 'b');   // 120 bytes
    const std::string name100(100, 'c');

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry(long_name, byte_blob("d")));
    items.push_back(::djinterp::test::make_file_entry(name100, byte_blob("e")));

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<tar>(items, out), status_ok);
    D_AR_REQUIRE(out.size() == 2u);

    // over-long name truncated to 100 bytes (its first 100)
    D_AR_CHECK(::djinterp::test::has_entry(out, head100));
    {
        const entry* e = ::djinterp::test::find_entry(out, head100);
        D_AR_REQUIRE(e != (const entry*)0);
        D_AR_CHECK(e->name.size() == 100u);
    }

    // exactly-100 name preserved
    D_AR_CHECK(::djinterp::test::has_entry(out, name100));

    return _ok;
}

/*
tests_tar_checksum
  Verifies the ustar header checksum the writer emits, recomputing it here.
  Tests the following:
  - the stored checksum field equals the sum of all 512 header bytes with the
    checksum field itself taken as eight spaces (the ustar convention)
*/
bool
tests_tar_checksum()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("f", "some contents"));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<tar>(items, blob), status_ok);
    D_AR_REQUIRE(blob.size() >= 512u);

    // recompute the header checksum independently
    {
        const char*   h   = blob.data();
        unsigned long sum = 0;
        std::size_t   i;

        for (i = 0; i < 512u; ++i)
        {
            // the 8-byte checksum field [148, 156) is summed as spaces
            unsigned char b = ( (i >= 148u) && (i < 156u) )
                              ? (unsigned char)' '
                              : (unsigned char)h[i];
            sum += (unsigned long)b;
        }

        // the stored field is six octal digits at offset 148
        unsigned long stored = ar_tar_parse_octal(h + 148, 8);

        D_AR_CHECK(stored == sum);
    }

    return _ok;
}

/*
tests_tar_extract_truncated
  Verifies the reader's handling of a stream cut short.
  Tests the following:
  - a stream whose declared file data runs past the buffer end is rejected
    with status_backend_error
  - a prefix shorter than a single 512-byte block is treated as an empty
    (already-ended) archive: status_ok, no entries
*/
bool
tests_tar_extract_truncated()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_file_entry("big", repeat_bytes('Z', 100)));

    byte_blob full;
    D_AR_CHECK_STATUS(try_archive<tar>(items, full), status_ok);
    D_AR_REQUIRE(full.size() >= 1024u);   // header + padded data + terminator

    // cut inside the file data: header intact, declared size overruns
    {
        byte_blob cut(full.data(), 550u);
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<tar>(cut, out), status_backend_error);
    }

    // cut below one block: the reader sees no complete header and stops clean
    {
        byte_blob tiny(full.data(), 300u);
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<tar>(tiny, out), status_ok);
        D_AR_CHECK(out.empty());
    }

    return _ok;
}

/*
tests_tar_empty_archive
  Verifies the degenerate empty-archive case.
  Tests the following:
  - creating a tar from an empty list yields just the two-block terminator
  - it carries no ustar header (no magic) but is properly terminated
  - extracting it yields no entries
*/
bool
tests_tar_empty_archive()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list empty;

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<tar>(empty, blob), status_ok);
    D_AR_CHECK(blob.size() == 1024u);
    D_AR_CHECK(::djinterp::test::tar_is_terminated(blob));
    D_AR_CHECK(!::djinterp::test::tar_has_ustar_magic(blob));

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<tar>(blob, out), status_ok);
    D_AR_CHECK(out.empty());

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
