/*******************************************************************************
* djinterp [test]                                     archive_tests_dispatch.cpp
*
*   Tests for the internal dispatch leaves, reached only through
* djinterp::internal: the null-base argument guards, the unknown-format
* default arms, and the capability switches.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_dispatch_null_base_guard
  Verifies the argument guards on the dispatch leaves, which the templated
public API cannot exercise (it never passes a null base with a non-zero count).
  Tests the following:
  - archive_create with a null item base and a non-zero count is rejected with
    status_invalid_argument
  - archive_create with a null base and a zero count is valid and produces an
    (empty) archive
  - archive_extract with a null input and a non-zero length is rejected with
    status_invalid_argument
  - archive_extract with a null input and a zero length is valid and yields no
    entries
*/
bool
tests_dispatch_null_base_guard()
{
    bool            _ok = true;
    archive_options opt;

    // create: null base + non-zero count -> invalid_argument
    {
        byte_blob out;
        D_AR_CHECK_STATUS(::djinterp::internal::archive_create(
                              format_id_zip, (const entry*)0, 5u, opt, out),
                          status_invalid_argument);
    }

    // create: null base + zero count -> valid empty archive
    {
        byte_blob out;
        D_AR_CHECK_STATUS(::djinterp::internal::archive_create(
                              format_id_zip, (const entry*)0, 0u, opt, out),
                          status_ok);
    }

    // extract: null input + non-zero length -> invalid_argument
    {
        entry_list out;
        D_AR_CHECK_STATUS(::djinterp::internal::archive_extract(
                              format_id_zip, (const char*)0, 5u, out),
                          status_invalid_argument);
    }

    // extract: null input + zero length -> valid, no entries
    {
        entry_list out;
        D_AR_CHECK_STATUS(::djinterp::internal::archive_extract(
                              format_id_zip, (const char*)0, 0u, out),
                          status_ok);
        D_AR_CHECK(out.empty());
    }

    return _ok;
}

/*
tests_dispatch_unknown_format
  Verifies the default arm of each dispatch switch, reached only by an
out-of-range format id.
  Tests the following:
  - archive_create with an unknown format id returns status_unavailable
  - archive_extract with an unknown format id returns status_unavailable
*/
bool
tests_dispatch_unknown_format()
{
    bool             _ok = true;
    archive_options  opt;
    const format_id  bad = (format_id)999;

    {
        entry       one[1];
        byte_blob out;

        one[0].name = "x";
        one[0].data = byte_blob("y");

        D_AR_CHECK_STATUS(::djinterp::internal::archive_create(
                              bad, one, 1u, opt, out),
                          status_unavailable);
    }

    {
        byte_blob blob("some bytes");
        entry_list  out;

        D_AR_CHECK_STATUS(::djinterp::internal::archive_extract(
                              bad, blob.data(), blob.size(), out),
                          status_unavailable);
    }

    return _ok;
}

/*
tests_dispatch_capability_switches
  Verifies every case arm of the runtime capability switches, including the
default (unknown-id) arm.
  Tests the following:
  - the built-in zip and tar formats report writable and readable
  - the gz and tar.gz arms track the gzip codec's availability
  - the 7z and rar arms agree with the template capability wrappers
  - an unknown format id reports neither writable nor readable
*/
bool
tests_dispatch_capability_switches()
{
    using namespace ::djinterp::formats;

    bool            _ok = true;
    const format_id bad = (format_id)999;

    // built-in guarantees
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_zip) == true);
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_tar) == true);
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_zip)  == true);
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_tar)  == true);

    // gz / tar.gz track the gzip codec
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_gz)     == codec_is_available<codecs::gzip>());
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_tar_gz) == codec_is_available<codecs::gzip>());
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_gz)      == codec_is_available<codecs::gzip>());
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_tar_gz)  == codec_is_available<codecs::gzip>());

    // 7z / rar arms agree with the template wrappers
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_sevenzip) == format_is_writable<sevenzip>());
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_sevenzip)  == format_is_readable<sevenzip>());
    D_AR_CHECK(::djinterp::internal::format_can_write(format_id_rar)      == format_is_writable<rar>());
    D_AR_CHECK(::djinterp::internal::format_can_read(format_id_rar)       == format_is_readable<rar>());

    // unknown id -> default arm -> false
    D_AR_CHECK(::djinterp::internal::format_can_write(bad) == false);
    D_AR_CHECK(::djinterp::internal::format_can_read(bad)  == false);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
