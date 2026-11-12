/*******************************************************************************
* djinterp [test]                                  archive_options_tests_zip.cpp
*
*   Tests for DZipOptions (section II.i of archive_options.hpp): a fresh block
* is pristine, every knob unset and the password empty; the kernel resolves
* each unset knob to its ZIP default; and each field is plain data, settable
* without moving its neighbours.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_zip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.11
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_zip_options_defaults
  Verifies a fresh DZipOptions says nothing, and what the kernel makes of
that for a ZIP.
  Tests the following:
  - method, encryption, zip64 and utf8_names are archive_knob_unset, and the
    password is empty (std::string's own default, not named in the
    constructor's initialiser list)
  - two fresh blocks compare equal
  - resolved for zip: the build's method (deflate where DEFLATE can
    compress and the build names none, else store), no encryption, zip64
    off, and the UTF-8 name flag on
*/
bool tests_zip_options_defaults()
{
    bool              _ok = true;
    const DZipOptions z;

    D_AO_CHECK(z.method     == archive_knob_unset);
    D_AO_CHECK(z.encryption == archive_knob_unset);
    D_AO_CHECK(z.zip64      == archive_knob_unset);
    D_AO_CHECK(z.utf8_names == archive_knob_unset);
    D_AO_CHECK(z.password.empty());

    {
        const DZipOptions other;
        D_AO_CHECK(::djinterp::test::zip_options_equal(z, other));
    }

    {
        const dj_archive_options r = resolved_for(format_id_zip,
                                                  archive_options());

        D_AO_CHECK(r.zip.method     == expected_zip_method());
        D_AO_CHECK(r.zip.encryption == zip_encryption_none);
        D_AO_CHECK(r.zip.zip64      == 0);
        D_AO_CHECK(r.zip.utf8_names == 1);
    }

    return _ok;
}

/*
tests_zip_options_fields
  Verifies the block behaves as the plain, independent data it is: each field
is settable without disturbing its neighbours, and the password carries
arbitrary bytes.
  Tests the following:
  - setting one field leaves the other four as they were
  - a field the caller set survives resolution unchanged
  - the block round-trips a password containing non-ASCII and embedded NUL
    bytes (std::string is not treated as a C string)
  - a mutated block no longer compares equal to a fresh one
*/
bool tests_zip_options_fields()
{
    bool _ok = true;

    // one field at a time: the others must not move
    {
        DZipOptions z;
        z.method = zip_method_zstd;

        D_AO_CHECK(z.method     == zip_method_zstd);
        D_AO_CHECK(z.encryption == archive_knob_unset);
        D_AO_CHECK(z.password.empty());
        D_AO_CHECK(z.zip64      == archive_knob_unset);
        D_AO_CHECK(z.utf8_names == archive_knob_unset);
    }
    {
        archive_options o;
        o.zip.utf8_names = 0;

        D_AO_CHECK(o.zip.method == archive_knob_unset);
        D_AO_CHECK(o.zip.zip64  == archive_knob_unset);
        D_AO_CHECK(resolved_for(format_id_zip, o).zip.utf8_names == 0);
    }
    {
        DZipOptions z;
        z.zip64 = 1;

        D_AO_CHECK(z.zip64      == 1);
        D_AO_CHECK(z.encryption == archive_knob_unset);
    }

    // the passphrase is opaque bytes, not a C string
    {
        static const char raw[] = { 'p', '\0', 's', (char)0xC3, (char)0xA9,
                                    'z' };
        const std::string secret(raw, sizeof(raw));

        DZipOptions z;
        z.encryption = zip_encryption_aes_256;
        z.password   = secret;

        D_AO_CHECK(z.encryption == zip_encryption_aes_256);
        D_AO_CHECK(z.password.size() == sizeof(raw));
        D_AO_CHECK(z.password == secret);
    }

    // a moved field is visible to the shared comparator
    {
        const DZipOptions fresh;
        DZipOptions       z;

        z.encryption = zip_encryption_legacy;
        D_AO_CHECK(!::djinterp::test::zip_options_equal(z, fresh));
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
