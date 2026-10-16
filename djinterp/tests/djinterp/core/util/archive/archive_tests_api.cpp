/*******************************************************************************
* djinterp [test]                                          archive_tests_api.cpp
*
*   Tests for the non-throwing API edges (empty-list round-trips) and the
* throwing convenience wrappers (round-trip, and archive_error on create
* and extract failure), the latter gated on exception support.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_api.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_try_api_empty_list
  Verifies the non-throwing template API's handling of an empty entry list,
where it must pass a null base with a zero count rather than index an empty
vector.
  Tests the following:
  - try_archive<zip> on an empty list produces a valid (empty) zip that extracts
    to no entries
  - try_archive<tar> on an empty list produces a valid (empty) tar that extracts
    to no entries
*/
bool
tests_try_api_empty_list()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list empty;

    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<zip>(empty, blob), status_ok);
        D_AR_CHECK(::djinterp::test::looks_like_zip(blob));

        entry_list out;
        D_AR_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
        D_AR_CHECK(out.empty());
    }
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<tar>(empty, blob), status_ok);

        entry_list out;
        D_AR_CHECK_STATUS(try_extract<tar>(blob, out), status_ok);
        D_AR_CHECK(out.empty());
    }

    return _ok;
}

/*
tests_throwing_roundtrip
  Verifies the throwing convenience API agrees with the non-throwing API on the
success path.
  Tests the following:
  - archive<zip> returns the same bytes as try_archive<zip>
  - extract<zip> returns the members, with file payloads preserved
  (Compiled only where exceptions are enabled.)
*/
bool
tests_throwing_roundtrip()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("a.txt", "alpha"));
    items.push_back(::djinterp::test::make_file_entry("b.bin", binary_sample()));

    try
    {
        byte_blob viatry;
        D_AR_CHECK_STATUS(try_archive<zip>(items, viatry), status_ok);

        byte_blob viathrow = archive<zip>(items);
        D_AR_CHECK(viathrow == viatry);

        entry_list out = extract<zip>(viathrow);
        D_AR_CHECK(::djinterp::test::files_preserved(items, out));
    }
    catch (const std::exception& _e)
    {
        std::printf("      unexpected exception: %s\n", _e.what());
        _ok = false;
    }

    return _ok;
#else
    return true;
#endif
}

/*
tests_throwing_error_on_create
  Verifies archive<> raises archive_error on a creation failure and that the
message names the format and the underlying status.
  Tests the following:
  - archive<gz> on a two-entry list (invalid for gz) throws archive_error
  - the exception message mentions the format ("gz") and the status text
  (Compiled only where exceptions are enabled.)
*/
bool
tests_throwing_error_on_create()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list two;

    two.push_back(::djinterp::test::make_text_entry("a", "aaa"));
    two.push_back(::djinterp::test::make_text_entry("b", "bbb"));

    bool caught = false;

    try
    {
        byte_blob blob = archive<gz>(two);
        (void)blob;
    }
    catch (const ::djinterp::archive_error& _e)
    {
        caught = true;

        const std::string w = _e.what();
        D_AR_CHECK(w.find("gz") != std::string::npos);
        D_AR_CHECK(w.find("invalid argument") != std::string::npos);
    }

    D_AR_CHECK(caught);

    return _ok;
#else
    return true;
#endif
}

/*
tests_throwing_error_on_extract
  Verifies extract<> raises archive_error on an extraction failure.
  Tests the following:
  - extract<zip> on a truncated container throws archive_error
  - the exception message mentions the format ("zip") and the status text
  (Compiled only where exceptions are enabled.)
*/
bool
tests_throwing_error_on_extract()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    using namespace ::djinterp::formats;

    bool            _ok = true;
    archive_options opt;

    opt.store_only = true;

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("f", repeat_bytes('D', 50)));

    byte_blob full;
    D_AR_CHECK_STATUS(try_archive<zip>(items, full, opt), status_ok);

    // truncate inside the first local header so extraction fails
    byte_blob truncated(full.data(), 20u);

    bool caught = false;

    try
    {
        entry_list out = extract<zip>(truncated);
        (void)out;
    }
    catch (const ::djinterp::archive_error& _e)
    {
        caught = true;

        const std::string w = _e.what();
        D_AR_CHECK(w.find("zip") != std::string::npos);
        D_AR_CHECK(w.find("backend") != std::string::npos);
    }

    D_AR_CHECK(caught);

    return _ok;
#else
    return true;
#endif
}

NS_END  // testing
NS_END  // djinterp
