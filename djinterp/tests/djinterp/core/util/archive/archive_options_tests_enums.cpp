/*******************************************************************************
* djinterp [test]                                archive_options_tests_enums.cpp
*
*   Tests for the categorical enums (section I of archive_options.hpp): the
* neutral-zero rule each one follows, enumerator distinctness and ordering, and
* the plain-enum (not enum class) property the header's version-portability
* promise rests on.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_enum_neutral_defaults
  Verifies the header's stated rule that the first enumerator of every
categorical enum is the neutral default and equals zero.  Every per-format
struct leans on this: a zero-initialised or default-constructed block lands on
the "let the backend decide" choice.
  Tests the following:
  - zip_method_default, zip_encryption_none, zip64_auto, tar_format_ustar, and
    sevenzip_method_lzma2 each equal 0
*/
bool
tests_enum_neutral_defaults()
{
    bool _ok = true;

    D_AO_CHECK((int)zip_method_default    == 0);
    D_AO_CHECK((int)zip_encryption_none   == 0);
    D_AO_CHECK((int)zip64_auto            == 0);
    D_AO_CHECK((int)tar_format_ustar      == 0);
    D_AO_CHECK((int)sevenzip_method_lzma2 == 0);

    return _ok;
}

/*
tests_enum_distinct_values
  Verifies no enum aliases two of its own enumerators.  A duplicate would make
two categorically different requests indistinguishable to the dispatch leaves
that translate them to backend constants.
  Tests the following:
  - the 7 zip_method, 5 zip_encryption, 3 zip64_policy, 5 tar_format, and 6
    sevenzip_method enumerators are pairwise distinct within their own enum
*/
bool
tests_enum_distinct_values()
{
    bool _ok = true;

    // distinct
    //   compares every pair in a small value array.
    struct distinct
    {
        static bool
        all(
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
    };

    {
        const int v[] = { zip_method_default, zip_method_store,
                          zip_method_deflate, zip_method_bzip2,
                          zip_method_lzma,    zip_method_zstd,
                          zip_method_xz };
        D_AO_CHECK(distinct::all(v, sizeof(v) / sizeof(v[0])));
        D_AO_CHECK((sizeof(v) / sizeof(v[0])) == 7u);
    }
    {
        const int v[] = { zip_encryption_none,        zip_encryption_traditional,
                          zip_encryption_aes128,      zip_encryption_aes192,
                          zip_encryption_aes256 };
        D_AO_CHECK(distinct::all(v, sizeof(v) / sizeof(v[0])));
        D_AO_CHECK((sizeof(v) / sizeof(v[0])) == 5u);
    }
    {
        const int v[] = { zip64_auto, zip64_always, zip64_never };
        D_AO_CHECK(distinct::all(v, sizeof(v) / sizeof(v[0])));
        D_AO_CHECK((sizeof(v) / sizeof(v[0])) == 3u);
    }
    {
        const int v[] = { tar_format_ustar, tar_format_pax, tar_format_gnu,
                          tar_format_v7,    tar_format_oldgnu };
        D_AO_CHECK(distinct::all(v, sizeof(v) / sizeof(v[0])));
        D_AO_CHECK((sizeof(v) / sizeof(v[0])) == 5u);
    }
    {
        const int v[] = { sevenzip_method_lzma2,   sevenzip_method_lzma,
                          sevenzip_method_ppmd,    sevenzip_method_bzip2,
                          sevenzip_method_deflate, sevenzip_method_copy };
        D_AO_CHECK(distinct::all(v, sizeof(v) / sizeof(v[0])));
        D_AO_CHECK((sizeof(v) / sizeof(v[0])) == 6u);
    }

    return _ok;
}

/*
tests_enum_sequential_values
  Verifies the enumerators run consecutively from zero.  None of them carries an
explicit value after the first, so this is the layout the header actually
declares; pinning it catches an inserted or reordered enumerator, which would
silently change the meaning of any value already persisted or passed across a
boundary.
  Tests the following:
  - each enum's enumerators are 0, 1, 2, ... in declaration order
*/
bool
tests_enum_sequential_values()
{
    bool _ok = true;

    D_AO_CHECK((int)zip_method_store   == 1);
    D_AO_CHECK((int)zip_method_deflate == 2);
    D_AO_CHECK((int)zip_method_bzip2   == 3);
    D_AO_CHECK((int)zip_method_lzma    == 4);
    D_AO_CHECK((int)zip_method_zstd    == 5);
    D_AO_CHECK((int)zip_method_xz      == 6);

    D_AO_CHECK((int)zip_encryption_traditional == 1);
    D_AO_CHECK((int)zip_encryption_aes128      == 2);
    D_AO_CHECK((int)zip_encryption_aes192      == 3);
    D_AO_CHECK((int)zip_encryption_aes256      == 4);

    D_AO_CHECK((int)zip64_always == 1);
    D_AO_CHECK((int)zip64_never  == 2);

    D_AO_CHECK((int)tar_format_pax    == 1);
    D_AO_CHECK((int)tar_format_gnu    == 2);
    D_AO_CHECK((int)tar_format_v7     == 3);
    D_AO_CHECK((int)tar_format_oldgnu == 4);

    D_AO_CHECK((int)sevenzip_method_lzma    == 1);
    D_AO_CHECK((int)sevenzip_method_ppmd    == 2);
    D_AO_CHECK((int)sevenzip_method_bzip2   == 3);
    D_AO_CHECK((int)sevenzip_method_deflate == 4);
    D_AO_CHECK((int)sevenzip_method_copy    == 5);

    return _ok;
}

/*
tests_enum_plain_not_scoped
  Verifies the enums are plain (unscoped) enums, as the C++98-C++23 portability
promise requires.  A scoped enum (enum class) would not convert implicitly to
int, so the implicit conversions below would not compile -- this test is as
much a compile-time assertion as a run-time one.
  Tests the following:
  - an enumerator converts implicitly to int without a cast
  - an enum variable is assignable and comparable, and round-trips through an
    explicit cast from its integer value
*/
bool
tests_enum_plain_not_scoped()
{
    bool _ok = true;

    // implicit conversion to int: only a plain enum permits this
    const int as_int = zip_method_deflate;
    D_AO_CHECK(as_int == 2);

    // assignment and comparison
    zip_method m = zip_method_store;
    D_AO_CHECK(m == zip_method_store);
    D_AO_CHECK(m != zip_method_deflate);

    m = zip_method_zstd;
    D_AO_CHECK(m == zip_method_zstd);

    // round-trip through the underlying integer value
    D_AO_CHECK((zip_method)as_int == zip_method_deflate);

    // the same property, spot-checked on the other four enums
    const int enc = zip_encryption_aes256;
    const int z64 = zip64_never;
    const int tf  = tar_format_pax;
    const int szm = sevenzip_method_ppmd;

    D_AO_CHECK((zip_encryption)enc  == zip_encryption_aes256);
    D_AO_CHECK((zip64_policy)z64    == zip64_never);
    D_AO_CHECK((tar_format)tf       == tar_format_pax);
    D_AO_CHECK((sevenzip_method)szm == sevenzip_method_ppmd);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
