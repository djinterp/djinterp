/*******************************************************************************
* djinterp [test]                                  archive_options_tests_zip.cpp
*
*   Tests for zip_options (section II.i of archive_options.hpp): the documented
* default of every field -- including password, which the constructor leaves to
* std::string's own default rather than naming in its initialiser list -- and
* the field-level behaviour a call site depends on.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_zip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_zip_options_defaults
  Verifies a default-constructed zip_options lands on the neutral, portable
choice for every knob.
  Tests the following:
  - method defaults to zip_method_default (deflate where available, else store)
  - encryption defaults to zip_encryption_none
  - password defaults to empty -- it is NOT named in the constructor's
    initialiser list, so this pins that std::string's own default is relied
    upon
  - zip64 defaults to zip64_auto
  - utf8_names defaults to true (the language-encoding bit is set)
  - two default-constructed blocks compare equal
*/
bool
tests_zip_options_defaults()
{
    bool              _ok = true;
    const zip_options z;

    D_AO_CHECK(z.method     == zip_method_default);
    D_AO_CHECK(z.encryption == zip_encryption_none);
    D_AO_CHECK(z.password.empty());
    D_AO_CHECK(z.zip64      == zip64_auto);
    D_AO_CHECK(z.utf8_names == true);

    {
        const zip_options other;
        D_AO_CHECK(::djinterp::test::zip_options_equal(z, other));
    }

    return _ok;
}

/*
tests_zip_options_fields
  Verifies the block behaves as the plain, independent data it is: each field is
settable without disturbing its neighbours, and the password carries arbitrary
bytes.
  Tests the following:
  - setting one field leaves the other four at their defaults
  - the block round-trips a password containing non-ASCII and embedded NUL
    bytes (std::string is not treated as a C string)
  - a mutated block no longer compares equal to a default one
*/
bool
tests_zip_options_fields()
{
    bool _ok = true;

    // one field at a time: the others must not move
    {
        zip_options z;
        z.method = zip_method_zstd;

        D_AO_CHECK(z.method     == zip_method_zstd);
        D_AO_CHECK(z.encryption == zip_encryption_none);
        D_AO_CHECK(z.password.empty());
        D_AO_CHECK(z.zip64      == zip64_auto);
        D_AO_CHECK(z.utf8_names == true);
    }
    {
        zip_options z;
        z.utf8_names = false;

        D_AO_CHECK(z.utf8_names == false);
        D_AO_CHECK(z.method     == zip_method_default);
        D_AO_CHECK(z.zip64      == zip64_auto);
    }
    {
        zip_options z;
        z.zip64 = zip64_always;

        D_AO_CHECK(z.zip64      == zip64_always);
        D_AO_CHECK(z.encryption == zip_encryption_none);
    }

    // the passphrase is opaque bytes, not a C string
    {
        static const char raw[] = { 'p', '\0', 's', (char)0xC3, (char)0xA9, 'z' };
        const std::string secret(raw, sizeof(raw));

        zip_options z;
        z.encryption = zip_encryption_aes256;
        z.password   = secret;

        D_AO_CHECK(z.encryption == zip_encryption_aes256);
        D_AO_CHECK(z.password.size() == sizeof(raw));
        D_AO_CHECK(z.password == secret);
    }

    // a moved field is visible to the shared comparator
    {
        const zip_options fresh;
        zip_options       z;

        z.encryption = zip_encryption_traditional;
        D_AO_CHECK(!::djinterp::test::zip_options_equal(z, fresh));
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
