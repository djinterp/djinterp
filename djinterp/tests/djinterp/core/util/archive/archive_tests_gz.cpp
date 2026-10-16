/*******************************************************************************
* djinterp [test]                                           archive_tests_gz.cpp
*
*   Tests for gz and tar.gz: the gz single-member rule, the gz and tar.gz
* round-trips, the empty tar.gz, and rejection of a corrupt gzip payload.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_gz.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_gz_requires_single_entry
  Verifies the gz format's single-member rule, which holds regardless of codec
availability (the count is checked before any compression).
  Tests the following:
  - an empty entry list is rejected with status_invalid_argument
  - a list of two entries is rejected with status_invalid_argument
*/
bool
tests_gz_requires_single_entry()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    {
        entry_list  empty;
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<gz>(empty, blob), status_invalid_argument);
    }
    {
        entry_list two;
        two.push_back(::djinterp::test::make_text_entry("a", "aaa"));
        two.push_back(::djinterp::test::make_text_entry("b", "bbb"));

        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<gz>(two, blob), status_invalid_argument);
    }

    return _ok;
}

/*
tests_gz_roundtrip
  Verifies a single gzip member round-trips when the gzip codec is present, and
degrades cleanly when it is not.
  Tests the following:
  - when gz is writable: one entry gzip-wraps (gzip magic present) and extracts
    back to a single entry whose data matches and whose name is empty (gzip
    carries no reliable name)
  - when gz is unavailable: creation returns status_unavailable
*/
bool
tests_gz_roundtrip()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list one;

    one.push_back(::djinterp::test::make_file_entry("payload", repeat_bytes('Q', 500)));

    if (format_is_writable<gz>())
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<gz>(one, blob), status_ok);
        D_AR_CHECK(::djinterp::test::looks_like_gzip(blob));

        entry_list out;
        D_AR_CHECK_STATUS(try_extract<gz>(blob, out), status_ok);
        D_AR_REQUIRE(out.size() == 1u);
        D_AR_CHECK(out[0].data == one[0].data);
        D_AR_CHECK(out[0].name.empty());     // gzip does not carry a name
    }
    else
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<gz>(one, blob), status_unavailable);
    }

    return _ok;
}

/*
tests_targz_roundtrip_files
  Verifies tar.gz preserves a multi-file set (unlike gz, tar.gz holds many
members) when the gzip codec is present, and degrades cleanly otherwise.
  Tests the following:
  - when tar.gz is writable: several files gzip-wrap (gzip magic present) and
    round-trip with payloads intact and the right count
  - when tar.gz is unavailable: creation returns status_unavailable
*/
bool
tests_targz_roundtrip_files()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("one.txt", "first file"));
    items.push_back(::djinterp::test::make_file_entry("two.bin", binary_sample()));
    items.push_back(::djinterp::test::make_file_entry("three", repeat_bytes('T', 300)));

    if (format_is_writable<tar_gz>())
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<tar_gz>(items, blob), status_ok);
        D_AR_CHECK(::djinterp::test::looks_like_gzip(blob));

        entry_list out;
        D_AR_CHECK_STATUS(::djinterp::test::roundtrip<tar_gz>(items, out), status_ok);
        D_AR_CHECK(::djinterp::test::count_files(out) == 3u);
        D_AR_CHECK(::djinterp::test::files_preserved(items, out));
    }
    else
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<tar_gz>(items, blob), status_unavailable);
    }

    return _ok;
}

/*
tests_targz_empty_archive
  Verifies the empty tar.gz case (an empty tar terminator, gzip-wrapped).
  Tests the following:
  - when tar.gz is writable: an empty list produces a valid gzip stream that
    extracts back to no entries
  - when tar.gz is unavailable: creation returns status_unavailable
*/
bool
tests_targz_empty_archive()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list empty;

    if (format_is_writable<tar_gz>())
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<tar_gz>(empty, blob), status_ok);
        D_AR_CHECK(::djinterp::test::looks_like_gzip(blob));

        entry_list out;
        D_AR_CHECK_STATUS(try_extract<tar_gz>(blob, out), status_ok);
        D_AR_CHECK(out.empty());
    }
    else
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<tar_gz>(empty, blob), status_unavailable);
    }

    return _ok;
}

/*
tests_gz_extract_corrupt
  Verifies the gz reader surfaces a decode failure when the gzip payload is
corrupt.  This exercises the reader's gzip-decompress failure arm.
  Tests the following:
  - when gz is writable and readable: a real gzip stream whose compressed body
    is overwritten with an undecodable run extracts to status_backend_error
  - otherwise: the case does not apply and the test is a no-op
*/
bool
tests_gz_extract_corrupt()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    if (!(format_is_writable<gz>() && format_is_readable<gz>()))
    {
        return _ok;   // no gzip codec to produce or read a stream here
    }

    entry_list one;
    one.push_back(::djinterp::test::make_file_entry("p", repeat_bytes('Q', 2000)));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<gz>(one, blob), status_ok);
    D_AR_REQUIRE(::djinterp::test::looks_like_gzip(blob));
    D_AR_REQUIRE(blob.size() > 18u);   // 10-byte header + body + 8-byte trailer

    // corrupt the compressed body just past the fixed 10-byte gzip header; a
    // leading 0xFF selects a reserved DEFLATE block type, so inflate fails
    blob[10] = (char)0xFF;
    blob[11] = (char)0xFF;
    blob[12] = (char)0xFF;

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<gz>(blob, out), status_backend_error);

    return _ok;
}

/*
tests_targz_extract_corrupt
  Verifies the tar.gz reader surfaces a decode failure when the outer gzip
wrapper is corrupt (the gunzip step fails before any tar parsing).
  Tests the following:
  - when tar.gz is writable and readable: a real gzip-wrapped tar whose
    compressed body is overwritten with an undecodable run extracts to
    status_backend_error
  - otherwise: the case does not apply and the test is a no-op
*/
bool
tests_targz_extract_corrupt()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    if (!(format_is_writable<tar_gz>() && format_is_readable<tar_gz>()))
    {
        return _ok;   // no gzip codec to produce or read a stream here
    }

    entry_list items;
    items.push_back(::djinterp::test::make_text_entry("a.txt", "first"));
    items.push_back(::djinterp::test::make_file_entry("b", repeat_bytes('T', 800)));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<tar_gz>(items, blob), status_ok);
    D_AR_REQUIRE(::djinterp::test::looks_like_gzip(blob));
    D_AR_REQUIRE(blob.size() > 18u);

    // corrupt the outer gzip body; the extractor gunzips the whole blob first,
    // so this fails in the decompress step before any tar walking
    blob[10] = (char)0xFF;
    blob[11] = (char)0xFF;
    blob[12] = (char)0xFF;

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<tar_gz>(blob, out), status_backend_error);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
