/*******************************************************************************
* djinterp [test]                                archive_options_tests_enums.cpp
*
*   Tests for the categorical enums (section I of archive_options.hpp): each is
* an alias of the kernel's enum with the kernel's values, which are the values
* on the wire where the format has one; none of them is the pristine knob
* value, which marks a knob the caller left for the kernel to resolve; and
* they are plain enums, as the header's version-portability promise needs.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.11
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // ao_distinct
    //   function: whether no two of _n values are equal.
    bool ao_distinct(
        const int*  _v,
        std::size_t _n
    )
    {
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = i + 1; j < _n; ++j)
            {
                if (_v[i] == _v[j])
                {
                    return false;
                }
            }
        }

        return true;
    }

    // ao_none_unset
    //   function: whether none of _n values is the pristine knob value.
    bool ao_none_unset(
        const int*  _v,
        std::size_t _n
    )
    {
        for (std::size_t i = 0; i < _n; ++i)
        {
            if (_v[i] == archive_knob_unset)
            {
                return false;
            }
        }

        return true;
    }

    const int ao_zip_methods[] = { zip_method_store,  zip_method_deflate,
                                   zip_method_bzip2,  zip_method_lzma,
                                   zip_method_zstd,   zip_method_xz };
    const int ao_zip_encryptions[] = { zip_encryption_none,
                                       zip_encryption_legacy,
                                       zip_encryption_aes_128,
                                       zip_encryption_aes_192,
                                       zip_encryption_aes_256 };
    const int ao_tar_formats[] = { tar_format_ustar, tar_format_gnu,
                                   tar_format_pax,   tar_format_v7 };
    const int ao_sevenzip_methods[] = { sevenzip_method_lzma2,
                                        sevenzip_method_lzma,
                                        sevenzip_method_bzip2,
                                        sevenzip_method_deflate,
                                        sevenzip_method_zstd,
                                        sevenzip_method_copy };
}

/*
tests_enum_unset_is_no_enumerator
  Verifies the pristine value is a sentinel outside every enum, so "the caller
did not say" can never be mistaken for a choice.  A knob holding it is
resolved by the kernel; a knob holding an enumerator is the caller's.
  Tests the following:
  - archive_knob_unset is D_ARCHIVE_KNOB_UNSET
  - no zip_method, zip_encryption, tar_format or sevenzip_method enumerator
    equals it
*/
bool tests_enum_unset_is_no_enumerator()
{
    bool _ok = true;

    D_AO_CHECK(archive_knob_unset == D_ARCHIVE_KNOB_UNSET);
    D_AO_CHECK(ao_none_unset(ao_zip_methods, 6u));
    D_AO_CHECK(ao_none_unset(ao_zip_encryptions, 5u));
    D_AO_CHECK(ao_none_unset(ao_tar_formats, 4u));
    D_AO_CHECK(ao_none_unset(ao_sevenzip_methods, 6u));

    return _ok;
}

/*
tests_enum_distinct_values
  Verifies no enum aliases two of its own enumerators.  A duplicate would make
two categorically different requests indistinguishable to the writers that
act on them.
  Tests the following:
  - the 6 zip_method, 5 zip_encryption, 4 tar_format, and 6 sevenzip_method
    enumerators are pairwise distinct within their own enum
*/
bool tests_enum_distinct_values()
{
    bool _ok = true;

    D_AO_CHECK(ao_distinct(ao_zip_methods, 6u));
    D_AO_CHECK(ao_distinct(ao_zip_encryptions, 5u));
    D_AO_CHECK(ao_distinct(ao_tar_formats, 4u));
    D_AO_CHECK(ao_distinct(ao_sevenzip_methods, 6u));

    return _ok;
}

/*
tests_enum_wire_values
  Verifies each enumerator's value.  A ZIP method is the number the ZIP
format writes in the local header, so the mapping to the wire is the
identity; the others are the kernel's pinned values.  Pinning them catches a
reordered or inserted enumerator, which would silently change the meaning of
any value already stored or passed across the C/C++ boundary.
  Tests the following:
  - zip methods: store 0, deflate 8, bzip2 12, lzma 14, zstd 93, xz 95
  - zip encryption: none 0, legacy 1, aes_128 2, aes_192 3, aes_256 4
  - tar formats: ustar 0, gnu 1, pax 2, v7 3
  - 7z methods: lzma2 0, lzma 1, bzip2 2, deflate 3, zstd 4, copy 5
*/
bool tests_enum_wire_values()
{
    bool _ok = true;

    D_AO_CHECK((int)zip_method_store   == 0);
    D_AO_CHECK((int)zip_method_deflate == 8);
    D_AO_CHECK((int)zip_method_bzip2   == 12);
    D_AO_CHECK((int)zip_method_lzma    == 14);
    D_AO_CHECK((int)zip_method_zstd    == 93);
    D_AO_CHECK((int)zip_method_xz      == 95);

    for (int i = 0; i < 5; ++i)
    {
        D_AO_CHECK(ao_zip_encryptions[i] == i);
    }
    D_AO_CHECK((int)tar_format_ustar == 0);
    D_AO_CHECK((int)tar_format_gnu   == 1);
    D_AO_CHECK((int)tar_format_pax   == 2);
    D_AO_CHECK((int)tar_format_v7    == 3);

    for (int i = 0; i < 6; ++i)
    {
        D_AO_CHECK(ao_sevenzip_methods[i] == i);
    }

    return _ok;
}

/*
tests_enum_plain_not_scoped
  Verifies the enums are plain (unscoped) enums, as the C++98-C++23
portability promise requires.  A scoped enum (enum class) would not convert
implicitly to int, so the implicit conversions below would not compile --
this test is as much a compile-time assertion as a run-time one.
  Tests the following:
  - an enumerator converts implicitly to int without a cast
  - an enum variable is assignable and comparable, and round-trips through an
    explicit cast from its integer value
  - a knob, which is an int, takes an enumerator directly
*/
bool tests_enum_plain_not_scoped()
{
    bool _ok = true;

    // implicit conversion to int: only a plain enum permits this
    const int as_int = zip_method_deflate;
    D_AO_CHECK(as_int == 8);

    // assignment and comparison
    zip_method m = zip_method_store;
    D_AO_CHECK(m == zip_method_store);
    D_AO_CHECK(m != zip_method_deflate);

    m = zip_method_zstd;
    D_AO_CHECK(m == zip_method_zstd);

    // round-trip through the underlying integer value
    D_AO_CHECK((zip_method)as_int == zip_method_deflate);

    // the same property, spot-checked on the other three enums
    const int enc = zip_encryption_aes_256;
    const int tf  = tar_format_pax;
    const int szm = sevenzip_method_copy;

    D_AO_CHECK((zip_encryption)enc  == zip_encryption_aes_256);
    D_AO_CHECK((tar_format)tf       == tar_format_pax);
    D_AO_CHECK((sevenzip_method)szm == sevenzip_method_copy);

    // a knob is an int and takes an enumerator as it is
    archive_options o;
    o.tar.format = tar_format_ustar;
    D_AO_CHECK(o.tar.format == 0);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
