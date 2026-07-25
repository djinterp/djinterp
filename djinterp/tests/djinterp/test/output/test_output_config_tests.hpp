/******************************************************************************
* djinterp [test]                                    test_output_config_tests.hpp
*
*   Unit-test suite for test_output_config.hpp - the bridge from a
* test_option_set's KNOBS to the emit layer's VOCABULARY.  This header is
* DECLARATIONS + SHARED FIXTURES only; every test body lives in one of three
* section translation units:
*
*     …_tests_mappings.cpp      I.  the four enum mappings - every arm, and
*                                   every `default:` arm
*     …_tests_cast_hazards.cpp  I.  the header's loudest claim: the mappings are
*                                   BY VALUE, NOT BY CAST.  These tests assert a
*                                   cast WOULD mis-map, so nobody ever
*                                   "simplifies" a switch into one.
*     …_tests_lowering.cpp      II. to_output_config / requested_doc_formats
*
*   Every test is a `bool tests_*()` sitting FLAT in djinterp::testing.
*
*   WHY THE CAST HAZARDS GET THEIR OWN SECTION:
*   They are not testing WHAT the switches map - that is section I's job.  They
* are testing WHY THE SWITCHES EXIST AT ALL.  The two are different claims and
* they fail for different reasons: section I fails if an arm is wrong, the hazard
* section fails if someone decides the switch was redundant.  There are TWO such
* hazards, and only one of them is documented:
*
*     to_format_id   test_archive_format and format_id transpose tar_gz / gz
*                    (ordinals 2 and 3).  A cast is wrong for 2 of 6 members -
*                    subtle precisely BECAUSE the other 4 accidentally work.
*                    This is the one the header's banner warns about.
*
*     to_doc_format  doc_format carries a `markdown` member at ordinal 1 that
*                    test_doc_type has no counterpart for, so a cast shifts every
*                    member past `txt` by one: xml->markdown, html->xml,
*                    pdf->html.  Wrong for 3 of 4.  The header explains why the
*                    shapes differ but never draws the cast conclusion, so the
*                    banner's warning under-states the danger.
*
*   to_pack_mode and to_codec_id happen to align with a cast TODAY.  Nothing
* enforces that, so their mappings are pinned by value too: a future reordering
* of either side then fails loudly instead of silently.
*
* path:      /tests/djinterp/test/test_output_config_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_OUTPUT_CONFIG_TESTS_
#define DJINTERP_TESTS_TEST_OUTPUT_CONFIG_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
//   The core header uses the FULL-FROM-INC form, so it resolves against
// ${DJINTERP_INC_ROOT} with no extra include dirs.  test_output_config.hpp is
// bare, resolved by the leaf's INCLUDES (inc/djinterp/test); it drags in
// output_packaging (pack_mode / codec_id / format_id / output_config),
// test_output (doc_format) and test_options (the test_* enums + accessors), which
// between them are the whole vocabulary on both sides of the bridge.
#include "djinterp/core/djinterp.hpp"   // NS_*, D_* qualifiers, language gates
#ifndef DTEST_SPEC_MODE
#include "test_output_config.hpp"       // THE HEADER UNDER TEST
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING


// dt
//   alias: the namespace under test.  pack_mode / codec_id / format_id /
// output_config live one level up in ::djinterp and resolve unqualified.
namespace dt = ::djinterp::test;


// D_OC_CHECK
//   macro: assert inside a test body; on failure it reports the expression and
// its location and returns false.  Unique letters (OC = output_config) so
// co-compiled suites never collide on the macro name.
#define D_OC_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("        check failed:  %s\n"                          \
                        "        at:            %s:%d\n",                      \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures: normal (section-file) mode only


///////////////////////////////////////////////////////////////////////////////
///                FIXTURES                                                  ///
///////////////////////////////////////////////////////////////////////////////

// out_of_range
//   an enumerator value that is NOT one of the enum's members.  This is the ONLY
// way to reach a switch's `default:` arm, and every one of the four mappings has
// one - so without this the suite could not claim full arm coverage.  99 is well
// past the largest member of any enum on either side of the bridge.
template<typename _Enum>
D_NODISCARD inline _Enum
out_of_range()
{
    return static_cast<_Enum>(99);
}


// make_opts
//   a test_option_set seeded with the framework defaults - the base every
// fixture below starts from.  Scalar knobs are set imperatively on a
// default_test_options() result; that is test_options' documented idiom.
D_NODISCARD inline dt::test_option_set
make_opts()
{
    return dt::default_test_options();
}


// opts_with_pack / opts_with_compressor / opts_with_archive_format /
// opts_with_document
//   one knob turned, everything else left at the defaults.  Keeping the fixtures
// single-knob is deliberate: when a lowering test fails, the option set it failed
// on says exactly which knob was responsible.
D_NODISCARD inline dt::test_option_set
opts_with_pack(
    dt::test_output_pack _p
)
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::pack>(_p);

    return _o;
}


D_NODISCARD inline dt::test_option_set
opts_with_compressor(
    dt::test_compressor _c
)
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::compressor>(_c);

    return _o;
}


D_NODISCARD inline dt::test_option_set
opts_with_archive_format(
    dt::test_archive_format _f
)
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::archive_format>(_f);

    return _o;
}


D_NODISCARD inline dt::test_option_set
opts_with_document(
    dt::test_doc_type _t
)
{
    dt::test_option_set _o = make_opts();

    _o.set<dt::test_option::document>(_t);

    return _o;
}


// marked_compress_options
//   a compress_options carrying values NO default would produce, so that "the
// aggregate copied across unchanged" is a claim about THESE bytes and not a
// coincidence of two default-constructed structs comparing equal.
D_NODISCARD inline ::djinterp::compress_options
marked_compress_options()
{
    ::djinterp::compress_options _c;

    _c.level               = 7;
    _c.deflate.window_bits = 13;
    _c.deflate.mem_level   = 5;

    return _c;
}


// marked_archive_options
//   likewise for the container aggregate - and note it carries a nested
// compress_options of its own, so this also proves the copy is deep.
D_NODISCARD inline ::djinterp::archive_options
marked_archive_options()
{
    ::djinterp::archive_options _a;

    _a.level      = 6;
    _a.store_only = true;
    _a.comment    = "a marked archive";
    _a.codec      = marked_compress_options();

    return _a;
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


///////////////////////////////////////////////////////////////////////////////
///                I.   ENUM MAPPINGS  (…_tests_mappings.cpp)                ///
///////////////////////////////////////////////////////////////////////////////

bool tests_output_config_to_pack_mode_maps_every_member();
bool tests_output_config_to_pack_mode_default_arm();
bool tests_output_config_to_codec_id_maps_every_member();
bool tests_output_config_to_codec_id_default_arm();
bool tests_output_config_to_format_id_maps_every_member();
bool tests_output_config_to_format_id_default_arm();
bool tests_output_config_to_doc_format_maps_every_member();
bool tests_output_config_to_doc_format_default_arm();
bool tests_output_config_to_doc_format_never_yields_markdown();
bool tests_output_config_mappings_are_injective();
bool tests_output_config_mappings_are_noexcept();


///////////////////////////////////////////////////////////////////////////////
///                I.   CAST HAZARDS  (…_tests_cast_hazards.cpp)             ///
///////////////////////////////////////////////////////////////////////////////

bool tests_output_config_format_id_cast_would_mismap_tar_gz();
bool tests_output_config_format_id_cast_would_mismap_gz();
bool tests_output_config_format_id_cast_is_wrong_for_exactly_two_members();
bool tests_output_config_doc_format_cast_would_mismap_xml();
bool tests_output_config_doc_format_cast_is_wrong_for_three_of_four();
bool tests_output_config_doc_format_hazard_is_caused_by_markdown();
bool tests_output_config_pack_mode_cast_agrees_today();
bool tests_output_config_codec_id_cast_agrees_today();
bool tests_output_config_enum_ordinals_are_pinned();


///////////////////////////////////////////////////////////////////////////////
///                II.  CONFIG LOWERING  (…_tests_lowering.cpp)              ///
///////////////////////////////////////////////////////////////////////////////

bool tests_output_config_lowers_pack();
bool tests_output_config_lowers_codec();
bool tests_output_config_lowers_format();
bool tests_output_config_copies_compress_opts_unchanged();
bool tests_output_config_copies_archive_opts_unchanged();
bool tests_output_config_leaves_naming_at_its_default();
bool tests_output_config_leaves_archive_name_at_its_default();
bool tests_output_config_touches_nothing_else_however_configured();
bool tests_output_config_lowering_goes_through_the_switch_not_a_cast();
bool tests_output_config_lowers_the_framework_defaults();
bool tests_output_config_lowering_does_not_mutate_the_option_set();
bool tests_output_config_requested_doc_formats_is_single_element();
bool tests_output_config_requested_doc_formats_tracks_the_document_knob();
bool tests_output_config_requested_doc_formats_never_requests_markdown();
bool tests_output_config_requested_doc_formats_returns_a_fresh_vector();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
output_config_spec()
{
    return dt::module_spec{
        "test_output_config",
        "test_output_config.hpp - lowering a test_option_set's document / "
        "packaging knobs into the compile-time output_config: the enum->id "
        "mappings, the cast hazards that make those mappings a switch rather than "
        "a static_cast, and to_output_config / requested_doc_formats.",
        {
            dt::block_spec{
                "mappings",
                "The enum-to-id maps: to_pack_mode / to_codec_id / to_format_id / "
                "to_doc_format, their default arms, injectivity, and noexcept.",
                {
                    { "tests_output_config_to_pack_mode_maps_every_member",     "to_pack_mode maps every test_output_pack member to its output pack_mode", &tests_output_config_to_pack_mode_maps_every_member },
                    { "tests_output_config_to_pack_mode_default_arm",           "to_pack_mode's default arm handles an out-of-range value", &tests_output_config_to_pack_mode_default_arm },
                    { "tests_output_config_to_codec_id_maps_every_member",      "to_codec_id maps every test_compressor to its codec_id", &tests_output_config_to_codec_id_maps_every_member },
                    { "tests_output_config_to_codec_id_default_arm",            "to_codec_id's default arm falls back to gzip", &tests_output_config_to_codec_id_default_arm },
                    { "tests_output_config_to_format_id_maps_every_member",     "to_format_id maps every test_archive_format to its format_id", &tests_output_config_to_format_id_maps_every_member },
                    { "tests_output_config_to_format_id_default_arm",           "to_format_id's default arm handles an out-of-range value", &tests_output_config_to_format_id_default_arm },
                    { "tests_output_config_to_doc_format_maps_every_member",    "to_doc_format maps every test_doc_type to its doc_format", &tests_output_config_to_doc_format_maps_every_member },
                    { "tests_output_config_to_doc_format_default_arm",          "to_doc_format's default arm handles an out-of-range value", &tests_output_config_to_doc_format_default_arm },
                    { "tests_output_config_to_doc_format_never_yields_markdown","to_doc_format never yields markdown (which the pipeline does not emit)", &tests_output_config_to_doc_format_never_yields_markdown },
                    { "tests_output_config_mappings_are_injective",            "the enum mappings are injective - distinct members map to distinct ids", &tests_output_config_mappings_are_injective },
                    { "tests_output_config_mappings_are_noexcept",             "the enum mappings are all noexcept", &tests_output_config_mappings_are_noexcept },
                }
            },
            dt::block_spec{
                "cast_hazards",
                "Why the mappings are a switch, not a static_cast: the members a "
                "cast would mismap, and the ordinal pins that guard against drift.",
                {
                    { "tests_output_config_format_id_cast_would_mismap_tar_gz",           "a raw static_cast would mismap tar_gz - the switch is required", &tests_output_config_format_id_cast_would_mismap_tar_gz },
                    { "tests_output_config_format_id_cast_would_mismap_gz",               "a raw static_cast would mismap gz", &tests_output_config_format_id_cast_would_mismap_gz },
                    { "tests_output_config_format_id_cast_is_wrong_for_exactly_two_members","a static_cast to format_id is wrong for exactly two members", &tests_output_config_format_id_cast_is_wrong_for_exactly_two_members },
                    { "tests_output_config_doc_format_cast_would_mismap_xml",             "a raw static_cast would mismap xml to the wrong doc_format", &tests_output_config_doc_format_cast_would_mismap_xml },
                    { "tests_output_config_doc_format_cast_is_wrong_for_three_of_four",   "a static_cast to doc_format is wrong for three of four members", &tests_output_config_doc_format_cast_is_wrong_for_three_of_four },
                    { "tests_output_config_doc_format_hazard_is_caused_by_markdown",      "the doc_format cast hazard is caused by the markdown ordinal shifting the rest", &tests_output_config_doc_format_hazard_is_caused_by_markdown },
                    { "tests_output_config_pack_mode_cast_agrees_today",                 "the pack_mode mapping happens to agree with a cast today - a pin against future drift", &tests_output_config_pack_mode_cast_agrees_today },
                    { "tests_output_config_codec_id_cast_agrees_today",                  "the codec_id mapping happens to agree with a cast today - a pin", &tests_output_config_codec_id_cast_agrees_today },
                    { "tests_output_config_enum_ordinals_are_pinned",                    "the source enum ordinals are pinned to their documented values", &tests_output_config_enum_ordinals_are_pinned },
                }
            },
            dt::block_spec{
                "lowering",
                "to_output_config and requested_doc_formats: the lowered fields, "
                "the untouched defaults, and non-mutation of the source.",
                {
                    { "tests_output_config_lowers_pack",                                "to_output_config lowers the pack knob to a pack_mode", &tests_output_config_lowers_pack },
                    { "tests_output_config_lowers_codec",                               "to_output_config lowers the compressor knob to a codec_id", &tests_output_config_lowers_codec },
                    { "tests_output_config_lowers_format",                              "to_output_config lowers the archive_format knob to a format_id", &tests_output_config_lowers_format },
                    { "tests_output_config_copies_compress_opts_unchanged",             "to_output_config copies the compress options through unchanged", &tests_output_config_copies_compress_opts_unchanged },
                    { "tests_output_config_copies_archive_opts_unchanged",              "to_output_config copies the archive options through unchanged", &tests_output_config_copies_archive_opts_unchanged },
                    { "tests_output_config_leaves_naming_at_its_default",               "to_output_config leaves the naming policy at its default", &tests_output_config_leaves_naming_at_its_default },
                    { "tests_output_config_leaves_archive_name_at_its_default",         "to_output_config leaves the archive name at its default", &tests_output_config_leaves_archive_name_at_its_default },
                    { "tests_output_config_touches_nothing_else_however_configured",    "to_output_config touches only the lowered fields, however the option set is configured", &tests_output_config_touches_nothing_else_however_configured },
                    { "tests_output_config_lowering_goes_through_the_switch_not_a_cast","the lowering goes through the switch, not a cast, so the hazard members map correctly", &tests_output_config_lowering_goes_through_the_switch_not_a_cast },
                    { "tests_output_config_lowers_the_framework_defaults",              "the framework defaults lower to documents emitted verbatim (no packing)", &tests_output_config_lowers_the_framework_defaults },
                    { "tests_output_config_lowering_does_not_mutate_the_option_set",    "lowering does not mutate the source option set", &tests_output_config_lowering_does_not_mutate_the_option_set },
                    { "tests_output_config_requested_doc_formats_is_single_element",    "requested_doc_formats returns a single-element list", &tests_output_config_requested_doc_formats_is_single_element },
                    { "tests_output_config_requested_doc_formats_tracks_the_document_knob","requested_doc_formats tracks the document knob", &tests_output_config_requested_doc_formats_tracks_the_document_knob },
                    { "tests_output_config_requested_doc_formats_never_requests_markdown","requested_doc_formats never requests markdown", &tests_output_config_requested_doc_formats_never_requests_markdown },
                    { "tests_output_config_requested_doc_formats_returns_a_fresh_vector","requested_doc_formats returns a fresh vector each call", &tests_output_config_requested_doc_formats_returns_a_fresh_vector },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif   // DJINTERP_TESTS_TEST_OUTPUT_CONFIG_TESTS_
