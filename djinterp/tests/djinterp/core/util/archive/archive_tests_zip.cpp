/*******************************************************************************
* djinterp [test]                                          archive_tests_zip.cpp
*
*   Tests for the zip writer and reader: file and directory round-trips, the
* store vs deflate method choice, the empty archive, and rejection of an
* unknown method, truncation, and a corrupt deflate payload.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_zip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_zip_roundtrip_files
  Verifies a set of regular files survives a zip create/extract round-trip and
that the produced container has zip shape with the right member count.
  Tests the following:
  - text, binary (embedded-NUL), and empty files all round-trip
  - payloads come back byte-identical with the right count
  - the produced bytes look like a zip and the EOCD advertises the entry count
*/
bool
tests_zip_roundtrip_files()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("readme.txt", "hello zip"));
    items.push_back(::djinterp::test::make_file_entry("data.bin", binary_sample()));
    items.push_back(::djinterp::test::make_file_entry("empty.dat", byte_blob()));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob), status_ok);
    D_AR_CHECK(::djinterp::test::looks_like_zip(blob));
    D_AR_CHECK(::djinterp::test::zip_total_entries(blob) == 3L);

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<zip>(items, out), status_ok);
    D_AR_REQUIRE(out.size() == 3u);
    D_AR_CHECK(::djinterp::test::files_preserved(items, out));
    D_AR_CHECK(::djinterp::test::count_files(out) == 3u);

    return _ok;
}

/*
tests_zip_store_mode
  Verifies the store_only option forces uncompressed entries.
  Tests the following:
  - with store_only set, every local header records method 0 (stored) even for
    highly compressible data
  - the archive still round-trips with payloads intact
*/
bool
tests_zip_store_mode()
{
    using namespace ::djinterp::formats;

    bool            _ok = true;
    archive_options opt;

    opt.store_only = true;

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("a", repeat_bytes('A', 1000)));
    items.push_back(::djinterp::test::make_file_entry("b", repeat_bytes('B', 1000)));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);

    {
        std::vector<unsigned int> methods;
        D_AR_REQUIRE(::djinterp::test::zip_local_methods(blob, methods));
        D_AR_REQUIRE(methods.size() == 2u);
        D_AR_CHECK(methods[0] == 0u);
        D_AR_CHECK(methods[1] == 0u);
    }

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<zip>(items, out, opt), status_ok);
    D_AR_CHECK(::djinterp::test::files_preserved(items, out));

    return _ok;
}

/*
tests_zip_deflate_mode
  Verifies the default compression path chooses deflate when it helps and falls
back to store when it does not.
  Tests the following:
  - when a DEFLATE codec is present, a large compressible entry is stored with
    method 8, while a tiny entry that would not shrink falls back to method 0
  - when no DEFLATE codec is present, entries are stored (method 0)
  - in both configurations the archive round-trips with payloads intact
*/
bool
tests_zip_deflate_mode()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("big", repeat_bytes('A', 1000)));
    items.push_back(::djinterp::test::make_file_entry("tiny", byte_blob("hi")));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob), status_ok);   // default opts

    {
        std::vector<unsigned int> methods;
        D_AR_REQUIRE(::djinterp::test::zip_local_methods(blob, methods));
        D_AR_REQUIRE(methods.size() == 2u);

        if (codec_is_available<codecs::deflate>())
        {
            // compressible entry deflated; incompressible tiny entry stored
            D_AR_CHECK(methods[0] == 8u);
            D_AR_CHECK(methods[1] == 0u);
        }
        else
        {
            // no backend: store is the only method
            D_AR_CHECK(methods[0] == 0u);
            D_AR_CHECK(methods[1] == 0u);
        }
    }

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<zip>(items, out), status_ok);
    D_AR_CHECK(::djinterp::test::files_preserved(items, out));

    return _ok;
}

/*
tests_zip_directories
  Verifies directory members round-trip through zip.
  Tests the following:
  - a directory entry is recorded (counted in the EOCD) and comes back flagged
    is_directory with a trailing-slash name and no data
  - the accompanying file is preserved
*/
bool
tests_zip_directories()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_dir_entry("d"));
    items.push_back(::djinterp::test::make_text_entry("d/f.txt", "inside"));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob), status_ok);
    D_AR_CHECK(::djinterp::test::zip_total_entries(blob) == 2L);

    entry_list out;
    D_AR_CHECK_STATUS(::djinterp::test::roundtrip<zip>(items, out), status_ok);
    D_AR_REQUIRE(out.size() == 2u);

    {
        const entry* d = ::djinterp::test::find_entry(out, "d");
        D_AR_REQUIRE(d != (const entry*)0);
        D_AR_CHECK(d->is_directory == true);
        D_AR_CHECK(d->data.empty());
        // the writer appends the conventional trailing separator
        D_AR_CHECK(!d->name.empty() && d->name[d->name.size() - 1] == '/');
    }

    D_AR_CHECK(::djinterp::test::files_preserved(items, out));

    return _ok;
}

/*
tests_zip_empty_archive
  Verifies the degenerate empty-archive case.
  Tests the following:
  - creating a zip from an empty list yields just an end-of-central-directory
    record advertising zero entries
  - it still looks like a zip and extracts to no entries
*/
bool
tests_zip_empty_archive()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list empty;

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(empty, blob), status_ok);
    D_AR_CHECK(::djinterp::test::looks_like_zip(blob));
    D_AR_CHECK(::djinterp::test::zip_total_entries(blob) == 0L);

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
    D_AR_CHECK(out.empty());

    return _ok;
}

/*
tests_zip_unsupported_method
  Verifies the reader rejects an entry whose compression method it cannot
handle.
  Tests the following:
  - a local header advertising an unknown method (patched to 99) extracts to
    status_unsupported
*/
bool
tests_zip_unsupported_method()
{
    using namespace ::djinterp::formats;

    bool            _ok = true;
    archive_options opt;

    opt.store_only = true;   // simplest, deterministic layout

    entry_list items;
    items.push_back(::djinterp::test::make_text_entry("x.txt", "data"));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);
    D_AR_REQUIRE(blob.size() > 10u);

    // the first local header's 2-byte method field is at offset 8
    blob[8] = (char)99;
    blob[9] = (char)0;

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<zip>(blob, out), status_unsupported);

    return _ok;
}

/*
tests_zip_extract_truncated
  Verifies the reader's handling of a container cut short.
  Tests the following:
  - a stream whose entry data runs past the buffer end is rejected with
    status_backend_error
  - a stream cut inside the first local header (fewer than 30 bytes) is
    rejected with status_backend_error
*/
bool
tests_zip_extract_truncated()
{
    using namespace ::djinterp::formats;

    bool            _ok = true;
    archive_options opt;

    opt.store_only = true;

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("f", repeat_bytes('D', 50)));

    byte_blob full;
    D_AR_CHECK_STATUS(try_archive<zip>(items, full, opt), status_ok);

    // name "f" (1 byte): data begins at 30 + 1 = 31; cut inside the 50 data bytes
    {
        byte_blob cut(full.data(), 41u);
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<zip>(cut, out), status_backend_error);
    }

    // cut inside the fixed 30-byte local header
    {
        byte_blob stub(full.data(), 20u);
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<zip>(stub, out), status_backend_error);
    }

    return _ok;
}

/*
tests_zip_extract_corrupt_deflate
  Verifies the reader surfaces a decode failure when a deflated member's payload
is corrupt (the compression method is valid, but the bytes are not a decodable
DEFLATE stream).  This is the reader's inflate-failure arm, distinct from the
unknown-method and truncation arms above.
  Tests the following:
  - when a DEFLATE codec is present: a member written with method 8 whose
    payload bytes have been overwritten with an undecodable run extracts to
    status_backend_error
  - when no DEFLATE codec is present: there is no deflated member to corrupt, so
    the case does not apply and the test is a no-op
*/
bool
tests_zip_extract_corrupt_deflate()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    if (!codec_is_available<codecs::deflate>())
    {
        return _ok;   // nothing deflated to corrupt in this configuration
    }

    // a long, highly compressible run so the writer chooses deflate (method 8)
    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("c", repeat_bytes('Z', 4000)));

    byte_blob blob;
    D_AR_CHECK_STATUS(try_archive<zip>(items, blob), status_ok);

    // confirm the single member really is deflated before we corrupt it
    {
        std::vector<unsigned int> methods;
        D_AR_REQUIRE(::djinterp::test::zip_local_methods(blob, methods));
        D_AR_REQUIRE(methods.size() == 1u);
        D_AR_REQUIRE(methods[0] == 8u);
    }

    // locate the first local header's payload: 30 fixed bytes + name + extra,
    // with the field widths the writer used (name_len@26, extra_len@28,
    // comp_size@18), then overwrite the payload with an undecodable run
    {
        std::size_t name_len  = (std::size_t)(unsigned char)blob[26]
                              | ((std::size_t)(unsigned char)blob[27] << 8);
        std::size_t extra_len = (std::size_t)(unsigned char)blob[28]
                              | ((std::size_t)(unsigned char)blob[29] << 8);
        std::size_t comp_size = (std::size_t)(unsigned char)blob[18]
                              | ((std::size_t)(unsigned char)blob[19] << 8)
                              | ((std::size_t)(unsigned char)blob[20] << 16)
                              | ((std::size_t)(unsigned char)blob[21] << 24);

        std::size_t at = 30u + name_len + extra_len;
        D_AR_REQUIRE(comp_size > 0u);
        D_AR_REQUIRE(at + comp_size <= blob.size());

        for (std::size_t i = 0; i < comp_size; ++i)
        {
            blob[at + i] = (char)0xFF;   // 0xFF leads with a reserved block type
        }
    }

    entry_list out;
    D_AR_CHECK_STATUS(try_extract<zip>(blob, out), status_backend_error);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
