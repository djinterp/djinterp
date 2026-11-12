/*******************************************************************************
* djinterp [test]                              archive_options_tests_formats.cpp
*
*   Tests for the tar, gz, 7z and rar blocks (sections II.ii-v of
* archive_options.hpp): each fresh block is pristine, every numeric knob unset
* and every text knob empty, and the kernel resolves each unset knob to its
* format's default.  A block resolves only for its own format: the others
* pass through untouched.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_formats.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.11
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_tar_options_defaults
  Verifies DTarOptions says nothing until set, and resolves to pax with
numeric owners for a tar or a tar.gz.
  Tests the following:
  - format and numeric_owner are archive_knob_unset
  - resolved for tar and for tar.gz: format pax (ustar for every member that
    fits, a pax record for what does not), numeric_owner on
  - resolved for zip, the tar block is left unset
  - setting one field leaves the other alone, is visible to the shared
    comparator, and survives resolution
*/
bool tests_tar_options_defaults()
{
    bool              _ok = true;
    const DTarOptions t;

    D_AO_CHECK(t.format        == archive_knob_unset);
    D_AO_CHECK(t.numeric_owner == archive_knob_unset);

    {
        const archive_options o;

        D_AO_CHECK(resolved_for(format_id_tar, o).tar.format ==
                   tar_format_pax);
        D_AO_CHECK(resolved_for(format_id_tar_gz, o).tar.format ==
                   tar_format_pax);
        D_AO_CHECK(resolved_for(format_id_tar, o).tar.numeric_owner == 1);
        D_AO_CHECK(resolved_for(format_id_zip, o).tar.format ==
                   archive_knob_unset);
    }
    {
        archive_options o;
        o.tar.format = tar_format_ustar;

        D_AO_CHECK(o.tar.numeric_owner == archive_knob_unset);
        D_AO_CHECK(!::djinterp::test::tar_options_equal(o.tar, t));
        D_AO_CHECK(resolved_for(format_id_tar, o).tar.format ==
                   tar_format_ustar);
    }

    return _ok;
}

/*
tests_gz_options_defaults
  Verifies the gzip header block says nothing until set, and resolves to a
reproducible header: no name, no time.
  Tests the following:
  - store_name and store_mtime are archive_knob_unset, original_name empty
  - resolved for gz: store_name and store_mtime off
  - setting original_name does not implicitly raise store_name -- the two
    are independent, as the header documents
*/
bool tests_gz_options_defaults()
{
    bool             _ok = true;
    const DGzOptions g;

    D_AO_CHECK(g.store_name  == archive_knob_unset);
    D_AO_CHECK(g.store_mtime == archive_knob_unset);
    D_AO_CHECK(g.original_name.empty());

    {
        const dj_archive_options r = resolved_for(format_id_gz,
                                                  archive_options());

        D_AO_CHECK(r.gz.store_name  == 0);
        D_AO_CHECK(r.gz.store_mtime == 0);
    }
    {
        archive_options o;
        o.gz.original_name = "payload.bin";

        // naming a file does not by itself request that the name be written
        D_AO_CHECK(o.gz.store_name == archive_knob_unset);
        D_AO_CHECK(resolved_for(format_id_gz, o).gz.store_name == 0);
        D_AO_CHECK(!::djinterp::test::gz_options_equal(o.gz, g));
    }

    return _ok;
}

/*
tests_sevenzip_options_defaults
  Verifies the 7z block says nothing until set, and what it resolves to.
  Tests the following:
  - method, solid, header_compression, header_encryption and threads are
    archive_knob_unset, and the password empty
  - resolved for 7z: the build's method, solid, compressed headers,
    unencrypted headers, one thread
  - store_only resolves the method to copy, 7z's store
  - a field the caller set survives resolution
*/
bool tests_sevenzip_options_defaults()
{
    bool                   _ok = true;
    const DSevenzipOptions s;

    D_AO_CHECK(s.method             == archive_knob_unset);
    D_AO_CHECK(s.solid              == archive_knob_unset);
    D_AO_CHECK(s.header_compression == archive_knob_unset);
    D_AO_CHECK(s.header_encryption  == archive_knob_unset);
    D_AO_CHECK(s.threads            == archive_knob_unset);
    D_AO_CHECK(s.password.empty());

    {
        const dj_archive_options r = resolved_for(format_id_sevenzip,
                                                  archive_options());

        D_AO_CHECK(r.sevenzip.method             == expected_sevenzip_method());
        D_AO_CHECK(r.sevenzip.solid              == 1);
        D_AO_CHECK(r.sevenzip.header_compression == 1);
        D_AO_CHECK(r.sevenzip.header_encryption  == 0);
        D_AO_CHECK(r.sevenzip.threads            == 1);
    }
    {
        archive_options o;
        o.store_only       = 1;
        o.sevenzip.threads = 4;

        const dj_archive_options r = resolved_for(format_id_sevenzip, o);

        D_AO_CHECK(r.sevenzip.method  == sevenzip_method_copy);
        D_AO_CHECK(r.sevenzip.threads == 4);
        D_AO_CHECK(!::djinterp::test::sevenzip_options_equal(o.sevenzip, s));
    }

    return _ok;
}

/*
tests_rar_options_defaults
  Verifies the RAR block says nothing until set, and resolves its level from
the archive's.
  Tests the following:
  - level, solid and recovery_record are archive_knob_unset, password empty
  - resolved for rar: the level is the archive-level one, solid and the
    recovery record off
  - an archive level the caller set is the RAR level too, unless the RAR
    block sets its own
*/
bool tests_rar_options_defaults()
{
    bool              _ok = true;
    const DRarOptions r;

    D_AO_CHECK(r.level           == archive_knob_unset);
    D_AO_CHECK(r.solid           == archive_knob_unset);
    D_AO_CHECK(r.recovery_record == archive_knob_unset);
    D_AO_CHECK(r.password.empty());

    {
        const dj_archive_options got = resolved_for(format_id_rar,
                                                    archive_options());

        D_AO_CHECK(got.rar.level           == got.level);
        D_AO_CHECK(got.rar.solid           == 0);
        D_AO_CHECK(got.rar.recovery_record == 0);
    }
    {
        archive_options o;
        o.level = 3;
        D_AO_CHECK(resolved_for(format_id_rar, o).rar.level == 3);

        o.rar.level = 5;
        D_AO_CHECK(resolved_for(format_id_rar, o).rar.level == 5);
        D_AO_CHECK(!::djinterp::test::rar_options_equal(o.rar, r));
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
