/******************************************************************************
* djinterp [test]                      test_output_config_tests_cast_hazards.cpp
*
*   The header's loudest claim, and the reason this file exists apart from
* …_tests_mappings.cpp:
*
*       "THE MAPPINGS ARE BY VALUE, NOT BY CAST"
*
*   Section I tests WHAT the switches map.  This section tests WHY THEY EXIST -
* that a static_cast would actively corrupt the configuration.  The two claims
* fail for different reasons: a mapping test fails when an arm is wrong; these
* fail when someone decides a switch was redundant boilerplate and "simplifies"
* it into a cast.  That is a plausible, well-intentioned refactor, and it would
* silently mis-route documents.  These are the tests that stop it.
*
*   THERE ARE TWO HAZARDS.  ONLY ONE IS DOCUMENTED.
*
*   (1) to_format_id  -- the documented one.  The two enums transpose tar_gz and
*       gz at ordinals 2 and 3:
*
*           ordinal    test_archive_format      format_id
*              0       zip                      format_id_zip
*              1       tar                      format_id_tar
*              2       tar_gz         <-->      format_id_gz
*              3       gz             <-->      format_id_tar_gz
*              4       sevenzip                 format_id_sevenzip
*              5       rar                      format_id_rar
*
*       A cast is wrong for 2 of 6 - and it is subtle PRECISELY BECAUSE the other
*       4 accidentally work.  A casual test that only checked zip would pass.
*       The failure mode is a gzipped tarball silently becoming a single-entry
*       gzip stream, or vice versa.
*
*   (2) to_doc_format -- UNDOCUMENTED, and worse.  doc_format carries a `markdown`
*       member at ordinal 1 that test_doc_type has no counterpart for, so a cast
*       shifts every member past `txt` by one:
*
*           test_doc_type     cast gives          correct
*              txt   (0)      text      (0)  ok   text
*              xml   (1)      markdown  (1)  XX   xml
*              html  (2)      xml       (2)  XX   html
*              pdf   (3)      html      (3)  XX   pdf
*
*       Wrong for 3 of 4.  The header's comment on to_doc_format explains WHY the
*       shapes differ ("test_doc_type has no markdown member today") but never
*       draws the cast conclusion, so the banner's warning names only the archive
*       case.  Emitting a markdown document when XML was requested is exactly the
*       kind of quiet, plausible-looking failure that survives review.
*
*   to_pack_mode and to_codec_id happen to agree with a cast today.  That is a
* coincidence of declaration order, not a guarantee, and nothing in either header
* enforces it - so those mappings are pinned by value here as well.  If anyone
* reorders test_compressor or codec_id, the pin fails immediately instead of the
* framework quietly compressing with the wrong codec.
*
* path:      /tests/djinterp/test/test_output_config_tests_cast_hazards.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
// djinterp
#include "test_output_config_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                HAZARD 1:  to_format_id  (tar_gz <-> gz)                  ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_format_id_cast_would_mismap_tar_gz
//   the exact corruption: a cast turns "gzipped tarball" into "single-entry gzip
// stream".  Both are legal format_ids, so nothing downstream would complain - the
// archive would just be the wrong shape.
bool
tests_output_config_format_id_cast_would_mismap_tar_gz()
{
    const dt::test_archive_format _in = dt::test_archive_format::tar_gz;

    const format_id _correct = dt::to_format_id(_in);
    const format_id _cast    = static_cast<format_id>(_in);

    D_OC_CHECK(_correct == format_id_tar_gz);   // what the switch gives
    D_OC_CHECK(_cast    == format_id_gz);       // what a cast would give
    D_OC_CHECK(_correct != _cast);              // ...and they are NOT the same

    return true;
}


// tests_output_config_format_id_cast_would_mismap_gz
//   and the other half of the transposition, in the other direction.
bool
tests_output_config_format_id_cast_would_mismap_gz()
{
    const dt::test_archive_format _in = dt::test_archive_format::gz;

    const format_id _correct = dt::to_format_id(_in);
    const format_id _cast    = static_cast<format_id>(_in);

    D_OC_CHECK(_correct == format_id_gz);
    D_OC_CHECK(_cast    == format_id_tar_gz);
    D_OC_CHECK(_correct != _cast);

    return true;
}


// tests_output_config_format_id_cast_is_wrong_for_exactly_two_members
//   the shape of the hazard, not just two instances of it: a cast agrees on 4 of
// 6 and disagrees on exactly 2.  Counting matters - it is the 4 accidental
// agreements that make this bug survive a careless test suite, and if the enums
// ever drift further the count changes and this test says so.
bool
tests_output_config_format_id_cast_is_wrong_for_exactly_two_members()
{
    const dt::test_archive_format _all[6] =
    {
        dt::test_archive_format::zip,      dt::test_archive_format::tar,
        dt::test_archive_format::tar_gz,   dt::test_archive_format::gz,
        dt::test_archive_format::sevenzip, dt::test_archive_format::rar
    };

    std::size_t _wrong = 0;
    std::size_t _i     = 0;

    for (_i = 0; _i < 6; ++_i)
    {
        if (dt::to_format_id(_all[_i]) != static_cast<format_id>(_all[_i]))
        {
            ++_wrong;
        }
    }

    D_OC_CHECK(_wrong == 2u);

    // and they are the two the header names
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::tar_gz) !=
               static_cast<format_id>(dt::test_archive_format::tar_gz));
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::gz) !=
               static_cast<format_id>(dt::test_archive_format::gz));

    // the other four agree - which is the trap
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::zip) ==
               static_cast<format_id>(dt::test_archive_format::zip));
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::tar) ==
               static_cast<format_id>(dt::test_archive_format::tar));
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::sevenzip) ==
               static_cast<format_id>(dt::test_archive_format::sevenzip));
    D_OC_CHECK(dt::to_format_id(dt::test_archive_format::rar) ==
               static_cast<format_id>(dt::test_archive_format::rar));

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                HAZARD 2:  to_doc_format  (the markdown shift)            ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_doc_format_cast_would_mismap_xml
//   the worst instance: ask for XML, a cast gives you MARKDOWN.  Both render
// successfully, both produce a plausible document, and nothing anywhere reports
// an error - you simply get the wrong file.
bool
tests_output_config_doc_format_cast_would_mismap_xml()
{
    const dt::test_doc_type _in = dt::test_doc_type::xml;

    const dt::doc_format _correct = dt::to_doc_format(_in);
    const dt::doc_format _cast    = static_cast<dt::doc_format>(_in);

    D_OC_CHECK(_correct == dt::doc_format::xml);
    D_OC_CHECK(_cast    == dt::doc_format::markdown);   // silently, a .md
    D_OC_CHECK(_correct != _cast);

    return true;
}


// tests_output_config_doc_format_cast_is_wrong_for_three_of_four
//   the whole table.  Only `txt` survives a cast; every other member is off by
// one.  Three of four wrong is a worse ratio than the documented archive hazard,
// which is why this one deserves to be in the banner too.
bool
tests_output_config_doc_format_cast_is_wrong_for_three_of_four()
{
    const dt::test_doc_type _all[4] =
    {
        dt::test_doc_type::txt,  dt::test_doc_type::xml,
        dt::test_doc_type::html, dt::test_doc_type::pdf
    };

    std::size_t _wrong = 0;
    std::size_t _i     = 0;

    for (_i = 0; _i < 4; ++_i)
    {
        if (dt::to_doc_format(_all[_i]) != static_cast<dt::doc_format>(_all[_i]))
        {
            ++_wrong;
        }
    }

    D_OC_CHECK(_wrong == 3u);

    // only txt survives the cast
    D_OC_CHECK(dt::to_doc_format(dt::test_doc_type::txt) ==
               static_cast<dt::doc_format>(dt::test_doc_type::txt));

    // ...every other member is shifted
    D_OC_CHECK(static_cast<dt::doc_format>(dt::test_doc_type::xml)  == dt::doc_format::markdown);
    D_OC_CHECK(static_cast<dt::doc_format>(dt::test_doc_type::html) == dt::doc_format::xml);
    D_OC_CHECK(static_cast<dt::doc_format>(dt::test_doc_type::pdf)  == dt::doc_format::html);

    return true;
}


// tests_output_config_doc_format_hazard_is_caused_by_markdown
//   the ROOT CAUSE, pinned so the diagnosis survives even if the numbers change:
// doc_format has an extra member (markdown, at ordinal 1) that test_doc_type has
// no counterpart for, so the two vocabularies are different sizes and everything
// past the insertion point slides by exactly one.  If a test_doc_type::markdown
// is ever added and routed, THIS test is the one that should be revisited - it
// documents why the shift existed.
bool
tests_output_config_doc_format_hazard_is_caused_by_markdown()
{
    // the extra member sits at ordinal 1, immediately after text
    D_OC_CHECK(static_cast<int>(dt::doc_format::text)     == 0);
    D_OC_CHECK(static_cast<int>(dt::doc_format::markdown) == 1);

    // test_doc_type has no member there: its ordinal 1 is xml
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::txt) == 0);
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::xml) == 1);

    // hence the shift is exactly one, for every member past txt
    D_OC_CHECK(static_cast<int>(dt::to_doc_format(dt::test_doc_type::xml)) ==
               static_cast<int>(dt::test_doc_type::xml) + 1);
    D_OC_CHECK(static_cast<int>(dt::to_doc_format(dt::test_doc_type::html)) ==
               static_cast<int>(dt::test_doc_type::html) + 1);
    D_OC_CHECK(static_cast<int>(dt::to_doc_format(dt::test_doc_type::pdf)) ==
               static_cast<int>(dt::test_doc_type::pdf) + 1);

    // ...and zero for txt, the only member before the insertion point
    D_OC_CHECK(static_cast<int>(dt::to_doc_format(dt::test_doc_type::txt)) ==
               static_cast<int>(dt::test_doc_type::txt));

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                THE TWO THAT AGREE - TODAY                                ///
///////////////////////////////////////////////////////////////////////////////

// tests_output_config_pack_mode_cast_agrees_today
//   test_output_pack and pack_mode declare their members in the same order, so a
// cast would happen to work.  That is a coincidence, not a contract: nothing in
// either header ties the orders together.  Pinning the by-value mapping means a
// future reorder fails HERE, loudly, instead of silently archiving when the user
// asked to compress.
bool
tests_output_config_pack_mode_cast_agrees_today()
{
    const dt::test_output_pack _all[3] =
    {
        dt::test_output_pack::none,
        dt::test_output_pack::compress,
        dt::test_output_pack::archive
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 3; ++_i)
    {
        D_OC_CHECK(dt::to_pack_mode(_all[_i]) == static_cast<pack_mode>(_all[_i]));
    }

    // the orders that make it so - pinned, because this is the fragile part
    D_OC_CHECK(static_cast<int>(pack_mode::none)     == 0);
    D_OC_CHECK(static_cast<int>(pack_mode::compress) == 1);
    D_OC_CHECK(static_cast<int>(pack_mode::archive)  == 2);

    return true;
}


// tests_output_config_codec_id_cast_agrees_today
//   the same coincidence, across nine members.  Nine is a lot of ordinals to keep
// accidentally in step, and codec_id is the enum most likely to grow (a new codec
// lands in the middle of the list and the whole tail shifts) - so this pin is the
// one most likely to earn its keep.
bool
tests_output_config_codec_id_cast_agrees_today()
{
    const dt::test_compressor _all[9] =
    {
        dt::test_compressor::store,   dt::test_compressor::deflate,
        dt::test_compressor::zlib,    dt::test_compressor::gzip,
        dt::test_compressor::bzip2,   dt::test_compressor::xz,
        dt::test_compressor::zstd,    dt::test_compressor::lz4,
        dt::test_compressor::brotli
    };

    std::size_t _i = 0;

    for (_i = 0; _i < 9; ++_i)
    {
        D_OC_CHECK(dt::to_codec_id(_all[_i]) == static_cast<codec_id>(_all[_i]));
    }

    return true;
}


// tests_output_config_enum_ordinals_are_pinned
//   the raw ordinals on BOTH sides of the bridge.  Every claim in this file is
// downstream of these numbers, so pinning them turns "the cast hazard changed
// shape" from a mysterious failure into an obvious one: if a member is inserted,
// removed, or reordered anywhere, this test names the enum that moved.
bool
tests_output_config_enum_ordinals_are_pinned()
{
    // ---- the transposed pair, both sides ----
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::zip)      == 0);
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::tar)      == 1);
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::tar_gz)   == 2);
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::gz)       == 3);
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::sevenzip) == 4);
    D_OC_CHECK(static_cast<int>(dt::test_archive_format::rar)      == 5);

    D_OC_CHECK(static_cast<int>(format_id_zip)      == 0);
    D_OC_CHECK(static_cast<int>(format_id_tar)      == 1);
    D_OC_CHECK(static_cast<int>(format_id_gz)       == 2);   // <-- transposed
    D_OC_CHECK(static_cast<int>(format_id_tar_gz)   == 3);   // <-- transposed
    D_OC_CHECK(static_cast<int>(format_id_sevenzip) == 4);
    D_OC_CHECK(static_cast<int>(format_id_rar)      == 5);

    // ---- the doc vocabularies, different sizes ----
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::txt)  == 0);
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::xml)  == 1);
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::html) == 2);
    D_OC_CHECK(static_cast<int>(dt::test_doc_type::pdf)  == 3);

    D_OC_CHECK(static_cast<int>(dt::doc_format::text)     == 0);
    D_OC_CHECK(static_cast<int>(dt::doc_format::markdown) == 1);   // <-- the extra one
    D_OC_CHECK(static_cast<int>(dt::doc_format::xml)      == 2);
    D_OC_CHECK(static_cast<int>(dt::doc_format::html)     == 3);
    D_OC_CHECK(static_cast<int>(dt::doc_format::pdf)      == 4);

    // ---- the two that align, and the codec list most likely to drift ----
    D_OC_CHECK(static_cast<int>(codec_id_store)  == 0);
    D_OC_CHECK(static_cast<int>(codec_id_gzip)   == 3);
    D_OC_CHECK(static_cast<int>(codec_id_brotli) == 8);

    D_OC_CHECK(static_cast<int>(dt::test_compressor::store)  == 0);
    D_OC_CHECK(static_cast<int>(dt::test_compressor::gzip)   == 3);
    D_OC_CHECK(static_cast<int>(dt::test_compressor::brotli) == 8);

    return true;
}


NS_END   // testing
NS_END   // djinterp
