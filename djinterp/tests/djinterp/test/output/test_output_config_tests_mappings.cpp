/******************************************************************************
* djinterp [test]                          test_output_config_tests_mappings.cpp
*
*   Section I of the test_output_config suite: the four enum mappings.
*
*   These are pure, total functions over small enums, so "100% coverage" here is
* literal and countable: 22 member arms plus 4 `default:` arms.
*
*       to_pack_mode     3 members + default
*       to_codec_id      9 members + default
*       to_format_id     6 members + default
*       to_doc_format    4 members + default
*
*   Every one is exercised below.  The `default:` arms are reachable ONLY through
* an out-of-range enumerator - a legal thing to construct, and exactly what an
* option set deserialized from a stale config file could hand you - so the
* out_of_range<> fixture is what makes the arm coverage complete rather than
* merely near-complete.
*
*   WHETHER the switches are needed at all is a separate claim, tested next door
* in …_tests_cast_hazards.cpp.
*
* path:      /tests/djinterp/test/test_output_config_tests_mappings.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
// djinterp
#include "test_output_config_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_output_config_to_pack_mode_maps_every_member
//   all three members of test_output_pack.
bool
tests_output_config_to_pack_mode_maps_every_member()
{
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::none)     == pack_mode::none);
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::compress) == pack_mode::compress);
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::archive)  == pack_mode::archive);

    return true;
}


// tests_output_config_to_pack_mode_default_arm
//   an out-of-range pack degrades to `none` - the safe choice: emit the documents
// verbatim rather than run them through a packaging path nobody asked for.
bool
tests_output_config_to_pack_mode_default_arm()
{
    D_OC_CHECK(dt::to_pack_mode(out_of_range<dt::test_output_pack>()) ==
               pack_mode::none);

    return true;
}


// tests_output_config_to_codec_id_maps_every_member
//   all nine members of test_compressor.
bool
tests_output_config_to_codec_id_maps_every_member()
{
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::store)   == codec_id_store);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::deflate) == codec_id_deflate);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::zlib)    == codec_id_zlib);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::gzip)    == codec_id_gzip);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::bzip2)   == codec_id_bzip2);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::xz)      == codec_id_xz);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::zstd)    == codec_id_zstd);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::lz4)     == codec_id_lz4);
    D_OC_CHECK(dt::to_codec_id(dt::test_compressor::brotli)  == codec_id_brotli);

    return true;
}


// tests_output_config_to_codec_id_default_arm
//   an out-of-range compressor degrades to gzip - NOT to store.  That is a real
// choice and worth pinning: the fallback keeps compressing (gzip is the framework
// default codec) rather than silently turning compression off.
bool
tests_output_config_to_codec_id_default_arm()
{
    D_OC_CHECK(dt::to_codec_id(out_of_range<dt::test_compressor>()) == codec_id_gzip);
    D_OC_CHECK(dt::to_codec_id(out_of_range<dt::test_compressor>()) != codec_id_store);

    return true;
}


// tests_output_config_to_format_id_maps_every_member
//   all six members of test_archive_format.  Two of these six - tar_gz and gz -
// are the ones a cast would corrupt; see …_tests_cast_hazards.cpp.
bool
tests_output_config_to_format_id_maps_every_member()
{
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::zip)      == format_id_zip);
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::tar)      == format_id_tar);
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::tar_gz)   == format_id_tar_gz);
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::gz)       == format_id_gz);
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::sevenzip) == format_id_sevenzip);
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::rar)      == format_id_rar);

    return true;
}


// tests_output_config_to_format_id_default_arm
//   an out-of-range container degrades to zip - the one format that is writable
// everywhere.
bool
tests_output_config_to_format_id_default_arm()
{
    D_OC_CHECK(dt::to_format_id(out_of_range<dt::test_archive_format>()) ==
               format_id_zip);

    return true;
}


// tests_output_config_to_doc_format_maps_every_member
//   all four members of test_doc_type.  Note test_doc_type::txt maps to
// doc_format::TEXT - the two vocabularies do not even spell the member the same
// way, which is a small hint that a cast was never going to be safe here.
bool
tests_output_config_to_doc_format_maps_every_member()
{
    D_OC_CHECK(dt::to_doc_format(dt::test_doc_type::txt)  == dt::doc_format::text);
    D_OC_CHECK(dt::to_doc_format(dt::test_doc_type::xml)  == dt::doc_format::xml);
    D_OC_CHECK(dt::to_doc_format(dt::test_doc_type::html) == dt::doc_format::html);
    D_OC_CHECK(dt::to_doc_format(dt::test_doc_type::pdf)  == dt::doc_format::pdf);

    return true;
}


// tests_output_config_to_doc_format_default_arm
//   an out-of-range document type degrades to text - the format that always
// renders, needs no backend, and cannot fail.
bool
tests_output_config_to_doc_format_default_arm()
{
    D_OC_CHECK(dt::to_doc_format(out_of_range<dt::test_doc_type>()) ==
               dt::doc_format::text);

    return true;
}


// tests_output_config_to_doc_format_never_yields_markdown
//   the header's own claim: "test_doc_type has no markdown member today;
// doc_format::markdown is reachable only by binding a new test_doc_type member
// (and routing it here)".  So markdown is UNREACHABLE through this bridge - no
// input, valid or not, produces it.  Pinned over every member AND the default arm,
// because if someone adds a test_doc_type::markdown and forgets to route it, the
// default arm would quietly send it to `text` and this test would still pass -
// but the member test above would then be incomplete, which is the failure that
// catches it.
bool
tests_output_config_to_doc_format_never_yields_markdown()
{
    const dt::test_doc_type _all[4] =
    {
        dt::test_doc_type::txt,
        dt::test_doc_type::xml,
        dt::test_doc_type::html,
        dt::test_doc_type::pdf
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 4; ++_i)
    {
        D_OC_CHECK(dt::to_doc_format(_all[_i]) != dt::doc_format::markdown);
    }

    D_OC_CHECK(dt::to_doc_format(out_of_range<dt::test_doc_type>()) !=
               dt::doc_format::markdown);

    return true;
}


// tests_output_config_mappings_are_injective
//   distinct members must land on distinct targets: a mapping that collapsed two
// selections onto one would silently ignore a user's choice.  (Every one of these
// four enums is smaller than or equal to its target vocabulary, so injectivity is
// achievable and is the right invariant.)
bool
tests_output_config_mappings_are_injective()
{
    // pack: 3 members -> 3 distinct modes
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::none) !=
               dt::to_pack_mode(dt::test_output_pack::compress));
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::compress) !=
               dt::to_pack_mode(dt::test_output_pack::archive));
    D_OC_CHECK(dt::to_pack_mode(dt::test_output_pack::none) !=
               dt::to_pack_mode(dt::test_output_pack::archive));

    // codec: 9 members -> 9 distinct ids (pairwise)
    const dt::test_compressor _codecs[9] =
    {
        dt::test_compressor::store,   dt::test_compressor::deflate,
        dt::test_compressor::zlib,    dt::test_compressor::gzip,
        dt::test_compressor::bzip2,   dt::test_compressor::xz,
        dt::test_compressor::zstd,    dt::test_compressor::lz4,
        dt::test_compressor::brotli
    };

    std::size_t _i = 0;
    std::size_t _j = 0;

    for (_i = 0; _i < 9; ++_i)
    {
        for (_j = _i + 1; _j < 9; ++_j)
        {
            D_OC_CHECK(dt::to_codec_id(_codecs[_i]) != dt::to_codec_id(_codecs[_j]));
        }
    }

    // format: 6 members -> 6 distinct ids (pairwise).  This is the pair that a
    // cast would swap rather than collapse - so injectivity alone does NOT catch
    // the cast bug, which is exactly why the hazard section exists.
    const dt::test_archive_format _formats[6] =
    {
        dt::test_archive_format::zip,      dt::test_archive_format::tar,
        dt::test_archive_format::tar_gz,   dt::test_archive_format::gz,
        dt::test_archive_format::sevenzip, dt::test_archive_format::rar
    };

    for (_i = 0; _i < 6; ++_i)
    {
        for (_j = _i + 1; _j < 6; ++_j)
        {
            D_OC_CHECK(dt::to_format_id(_formats[_i]) != dt::to_format_id(_formats[_j]));
        }
    }

    // doc: 4 members -> 4 distinct formats (pairwise)
    const dt::test_doc_type _docs[4] =
    {
        dt::test_doc_type::txt,  dt::test_doc_type::xml,
        dt::test_doc_type::html, dt::test_doc_type::pdf
    };

    for (_i = 0; _i < 4; ++_i)
    {
        for (_j = _i + 1; _j < 4; ++_j)
        {
            D_OC_CHECK(dt::to_doc_format(_docs[_i]) != dt::to_doc_format(_docs[_j]));
        }
    }

    return true;
}


// tests_output_config_mappings_are_noexcept
//   all four mappings are declared D_NOEXCEPT - they are switches over an enum
// with a total default, so there is nothing to throw.  Pinning it matters because
// the lowering functions that call them are NOT noexcept (they build an
// output_config, which owns a std::string and a std::function), and that asymmetry
// is deliberate rather than an oversight.
bool
tests_output_config_mappings_are_noexcept()
{
    static_assert(noexcept(dt::to_pack_mode(dt::test_output_pack::none)),
                  "to_pack_mode is declared D_NOEXCEPT");
    static_assert(noexcept(dt::to_codec_id(dt::test_compressor::gzip)),
                  "to_codec_id is declared D_NOEXCEPT");
    static_assert(noexcept(dt::to_format_id(dt::test_archive_format::zip)),
                  "to_format_id is declared D_NOEXCEPT");
    static_assert(noexcept(dt::to_doc_format(dt::test_doc_type::txt)),
                  "to_doc_format is declared D_NOEXCEPT");

    D_OC_CHECK(true);

    return true;
}


NS_END   // testing
NS_END   // djinterp
