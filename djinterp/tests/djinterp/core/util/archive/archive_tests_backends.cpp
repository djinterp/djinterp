/*******************************************************************************
* djinterp [test]                                     archive_tests_backends.cpp
*
*   Tests for the 7z and rar backends: capability-consistent behaviour --
* create/extract succeed when the backend reports writable/readable and
* report status_unavailable otherwise.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_backends.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_sevenzip_capability_consistency
  Verifies the 7z path agrees with its advertised capability: it either works or
reports status_unavailable, never anything in between.
  Tests the following:
  - when 7z is writable, creation succeeds; when it is also readable, the set
    round-trips with payloads intact
  - when 7z is not writable, creation returns status_unavailable
  - when 7z is not readable, extraction returns status_unavailable
*/
bool
tests_sevenzip_capability_consistency()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("a.txt", "alpha"));
    items.push_back(::djinterp::test::make_file_entry("b.bin", binary_sample()));

    // creation path
    if (format_is_writable<sevenzip>())
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<sevenzip>(items, blob), status_ok);

        // if we can also read, verify the round-trip
        if (format_is_readable<sevenzip>())
        {
            entry_list out;
            D_AR_CHECK_STATUS(try_extract<sevenzip>(blob, out), status_ok);
            D_AR_CHECK(::djinterp::test::files_preserved(items, out));
        }
    }
    else
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<sevenzip>(items, blob), status_unavailable);
    }

    // extraction path when unreadable
    if (!format_is_readable<sevenzip>())
    {
        byte_blob dummy("this is not a 7z archive");
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<sevenzip>(dummy, out), status_unavailable);
    }

    return _ok;
}

/*
tests_rar_capability_consistency
  Verifies the rar path agrees with its advertised capability.  No library can
write rar, so creation is available only when the proprietary tool is present;
otherwise both directions report status_unavailable.
  Tests the following:
  - when rar is writable, creation succeeds
  - when rar is not writable, creation returns status_unavailable
  - when rar is not readable, extraction returns status_unavailable
*/
bool
tests_rar_capability_consistency()
{
    using namespace ::djinterp::formats;

    bool       _ok = true;
    entry_list items;

    items.push_back(::djinterp::test::make_text_entry("note.txt", "content"));

    // creation path
    if (format_is_writable<rar>())
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<rar>(items, blob), status_ok);
    }
    else
    {
        byte_blob blob;
        D_AR_CHECK_STATUS(try_archive<rar>(items, blob), status_unavailable);
    }

    // extraction path when unreadable
    if (!format_is_readable<rar>())
    {
        byte_blob dummy("this is not a rar archive");
        entry_list  out;
        D_AR_CHECK_STATUS(try_extract<rar>(dummy, out), status_unavailable);
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
