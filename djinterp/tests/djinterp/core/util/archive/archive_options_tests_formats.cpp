/*******************************************************************************
* djinterp [test]                              archive_options_tests_formats.cpp
*
*   Tests for the remaining per-format option blocks (sections II.ii - II.v of
* archive_options.hpp): tar_options, gz_options, sevenzip_options, and
* rar_options.  Each block's documented defaults are pinned field by field,
* including the string members their constructors leave to std::string.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_formats.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_tar_options_defaults
  Verifies tar_options defaults to the portable ustar variant with names
resolved rather than numeric.
  Tests the following:
  - format defaults to tar_format_ustar
  - numeric_owner defaults to false
  - setting one field leaves the other alone, and the change is visible to the
    shared comparator
*/
bool
tests_tar_options_defaults()
{
    bool              _ok = true;
    const tar_options t;

    D_AO_CHECK(t.format        == tar_format_ustar);
    D_AO_CHECK(t.numeric_owner == false);

    {
        tar_options m;
        m.format = tar_format_pax;

        D_AO_CHECK(m.format        == tar_format_pax);
        D_AO_CHECK(m.numeric_owner == false);
        D_AO_CHECK(!::djinterp::test::tar_options_equal(m, t));
    }

    return _ok;
}

/*
tests_gz_options_defaults
  Verifies the gzip header block defaults: no FNAME field, but a modification
time by default.
  Tests the following:
  - store_name defaults to false
  - store_mtime defaults to true
  - original_name defaults to empty (not named in the initialiser list)
  - setting original_name does not implicitly raise store_name -- the two are
    independent, as the header documents
*/
bool
tests_gz_options_defaults()
{
    bool             _ok = true;
    const gz_options g;

    D_AO_CHECK(g.store_name  == false);
    D_AO_CHECK(g.store_mtime == true);
    D_AO_CHECK(g.original_name.empty());

    {
        gz_options m;
        m.original_name = "payload.bin";

        // naming a file does not by itself request that the name be written
        D_AO_CHECK(m.store_name == false);
        D_AO_CHECK(m.original_name == "payload.bin");
        D_AO_CHECK(!::djinterp::test::gz_options_equal(m, g));
    }

    return _ok;
}

/*
tests_sevenzip_options_defaults
  Verifies the 7z block defaults to the modern, solid, header-compressed
configuration with encryption off and thread count left to the backend.
  Tests the following:
  - method defaults to sevenzip_method_lzma2
  - solid and header_compression default to true
  - header_encryption defaults to false and password to empty, so the default
    block never requests encryption
  - threads defaults to 0 (backend chooses)
*/
bool
tests_sevenzip_options_defaults()
{
    bool                   _ok = true;
    const sevenzip_options s;

    D_AO_CHECK(s.method             == sevenzip_method_lzma2);
    D_AO_CHECK(s.solid              == true);
    D_AO_CHECK(s.header_compression == true);
    D_AO_CHECK(s.header_encryption  == false);
    D_AO_CHECK(s.password.empty());
    D_AO_CHECK(s.threads            == 0);

    {
        sevenzip_options m;
        m.threads = 4;

        D_AO_CHECK(m.threads == 4);
        D_AO_CHECK(m.solid   == true);
        D_AO_CHECK(!::djinterp::test::sevenzip_options_equal(m, s));
    }

    return _ok;
}

/*
tests_rar_options_defaults
  Verifies the RAR block defaults.  No library can write RAR, so these are inert
unless the proprietary tool drives creation; the defaults must therefore be the
neutral ones that request nothing.
  Tests the following:
  - level defaults to -1 (forward the generic effort) -- note this differs from
    the 0-is-neutral convention the categorical enums use
  - solid and recovery_record default to false
  - password defaults to empty
*/
bool
tests_rar_options_defaults()
{
    bool              _ok = true;
    const rar_options r;

    D_AO_CHECK(r.level           == -1);
    D_AO_CHECK(r.solid           == false);
    D_AO_CHECK(r.recovery_record == false);
    D_AO_CHECK(r.password.empty());

    {
        rar_options m;
        m.recovery_record = true;

        D_AO_CHECK(m.recovery_record == true);
        D_AO_CHECK(m.level           == -1);
        D_AO_CHECK(!::djinterp::test::rar_options_equal(m, r));
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
