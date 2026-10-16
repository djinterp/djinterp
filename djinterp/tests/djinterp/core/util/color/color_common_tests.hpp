/*******************************************************************************
* djinterp [tests]                                        color_common_tests.hpp
*
*   Suite header for color_common.hpp - the color foundation: the channel
* scalar, the two generic scalar helpers (clamp_channel / approx_equal), the
* model-tag hierarchy, and the is_color_model detection trait.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
* normal mode pulls the header under test plus the fixtures and the check
* macro; spec mode pulls test_defaults.hpp and exposes color_common_spec().
*
*
* path:      /tests/djinterp/core/util/color/color_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_COMMON_TESTS_
#define DJINTERP_TESTS_COLOR_COMMON_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_common.hpp"                     // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module
#endif


// ::djinterp::test must NAME a namespace in both faces before `dt` can alias
// it.  In spec mode test_defaults.hpp populates it; in normal mode nothing
// does - djinterp.hpp only defines the NS_TEST macro, and the color headers
// under test pull nothing from djinterp/test/.  This empty declaration makes
// the unconditional alias below well formed either way, and is a harmless
// re-open when the framework has already declared it.
NS_DJINTERP
NS_TEST
NS_END   // test
NS_END   // djinterp


NS_DJINTERP
NS_TESTING

// dt
//   namespace alias: the entities under test live in djinterp::test; declared
// unconditionally because the spec provider needs dt::module_spec too.
namespace dt = ::djinterp::test;


// color_common_check
//   helper: reports a failed check with its expression and source location.
// Self-contained (printf only), so it stays outside the fixture guard.
inline bool
color_common_check(
    bool        _cond,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

// D_CC_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CC_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_common_check(                         \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cc_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
cc_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// cc_inf
//   helper: positive infinity of the channel scalar type.
inline channel_t
cc_inf()
{
    return std::numeric_limits<channel_t>::infinity();
}

// cc_tagged
//   type: a minimal conversion-graph participant - exposes a nested model_tag
// and nothing else, so is_color_model detects it structurally.
struct cc_tagged
{
    using model_tag = rgb_tag;
};

// cc_foreign_tag
//   type: exposes a model_tag that is NOT part of the color tag hierarchy.
// Detection is structural, not semantic, so this still reads as a model.
struct cc_foreign_tag
{
    using model_tag = int;
};

// cc_private_tag
//   type: hides model_tag behind private access.  Access checking is part of
// SFINAE, so detection must fail rather than hard-error.
class cc_private_tag
{
private:
    using model_tag = rgb_tag;
};

// cc_untagged
//   type: a transport-shaped aggregate with channels but no model_tag, the
// shape rgba deliberately has.
struct cc_untagged
{
    channel_t r;
    channel_t g;
    channel_t b;
    channel_t a;
};

// cc_tag_member
//   type: carries a model_tag DATA MEMBER rather than a nested type.  The
// detector probes for a type, so this must not be mistaken for a model.
struct cc_tag_member
{
    int model_tag;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CHANNEL SCALAR          (color_common_tests_channel.cpp)
bool tests_color_common_channel_is_floating_point();
bool tests_color_common_channel_default_is_float();
bool tests_color_common_channel_arithmetic_round_trips();
bool tests_color_common_channel_holds_out_of_gamut_values();

// II.  SCALAR HELPERS          (color_common_tests_scalar.cpp)
bool tests_color_common_clamp_channel_below_range();
bool tests_color_common_clamp_channel_above_range();
bool tests_color_common_clamp_channel_within_range();
bool tests_color_common_clamp_channel_boundaries_inclusive();
bool tests_color_common_clamp_channel_degenerate_range();
bool tests_color_common_clamp_channel_inverted_range();
bool tests_color_common_clamp_channel_non_finite();
bool tests_color_common_clamp_channel_generic_over_int();
bool tests_color_common_clamp_channel_is_constexpr();
bool tests_color_common_approx_equal_identical();
bool tests_color_common_approx_equal_within_tolerance();
bool tests_color_common_approx_equal_tolerance_is_inclusive();
bool tests_color_common_approx_equal_beyond_tolerance();
bool tests_color_common_approx_equal_is_symmetric();
bool tests_color_common_approx_equal_zero_tolerance();
bool tests_color_common_approx_equal_negative_operands();
bool tests_color_common_approx_equal_nan_is_never_equal();
bool tests_color_common_approx_equal_is_constexpr();

// III. MODEL-TAG HIERARCHY     (color_common_tests_tags.cpp)
bool tests_color_common_tags_derive_from_base();
bool tests_color_common_tags_are_distinct_types();
bool tests_color_common_tags_are_empty_and_default_constructible();
bool tests_color_common_tag_base_is_not_a_model_tag_itself();

// IV.  DETECTION TRAIT         (color_common_tests_detection.cpp)
bool tests_color_common_is_color_model_accepts_tagged();
bool tests_color_common_is_color_model_rejects_untagged();
bool tests_color_common_is_color_model_rejects_scalars_and_void();
bool tests_color_common_is_color_model_strips_cv_and_reference();
bool tests_color_common_is_color_model_is_structural_not_semantic();
bool tests_color_common_is_color_model_respects_access_control();
bool tests_color_common_is_color_model_ignores_data_members();
bool tests_color_common_is_color_model_trait_shape();
bool tests_color_common_is_color_model_v_matches_trait();
bool tests_color_common_has_model_tag_helper_direct();
bool tests_color_common_detail_void_maps_to_void();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_common_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_common_spec()
{
    return dt::module_spec{
        "color_common",
        "The color foundation shared by every model wrapper: the channel_t "
        "scalar, the constant-expression-safe scalar helpers clamp_channel "
        "and approx_equal, the empty model-tag hierarchy rooted at "
        "color_model_tag, and the is_color_model detection trait that decides "
        "which types participate in the conversion graph.",
        {
            dt::block_spec{
                "channel",
                "The channel_t scalar: its identity, its numeric character, "
                "and its willingness to carry out-of-gamut values verbatim.",
                {
                    { "tests_color_common_channel_is_floating_point",
                      "channel_t is a floating-point arithmetic type",
                      &tests_color_common_channel_is_floating_point },
                    { "tests_color_common_channel_default_is_float",
                      "channel_t defaults to float when D_COLOR_CHANNEL_TYPE "
                      "is not pre-defined",
                      &tests_color_common_channel_default_is_float },
                    { "tests_color_common_channel_arithmetic_round_trips",
                      "exactly representable channel arithmetic round-trips "
                      "without drift",
                      &tests_color_common_channel_arithmetic_round_trips },
                    { "tests_color_common_channel_holds_out_of_gamut_values",
                      "channel_t stores negative and above-unity values "
                      "unchanged (the scalar imposes no range policy)",
                      &tests_color_common_channel_holds_out_of_gamut_values },
                }
            },
            dt::block_spec{
                "scalar",
                "The two generic scalar helpers: every branch of "
                "clamp_channel's three-way select and approx_equal's "
                "absolute-difference test, including degenerate ranges, "
                "non-finite operands, and constant-expression use.",
                {
                    { "tests_color_common_clamp_channel_below_range",
                      "a value below the low bound clamps to the low bound",
                      &tests_color_common_clamp_channel_below_range },
                    { "tests_color_common_clamp_channel_above_range",
                      "a value above the high bound clamps to the high bound",
                      &tests_color_common_clamp_channel_above_range },
                    { "tests_color_common_clamp_channel_within_range",
                      "a value inside the range is returned unchanged",
                      &tests_color_common_clamp_channel_within_range },
                    { "tests_color_common_clamp_channel_boundaries_inclusive",
                      "values exactly on either bound are returned unchanged "
                      "(the interval is closed)",
                      &tests_color_common_clamp_channel_boundaries_inclusive },
                    { "tests_color_common_clamp_channel_degenerate_range",
                      "a zero-width range collapses every input to the single "
                      "admissible value",
                      &tests_color_common_clamp_channel_degenerate_range },
                    { "tests_color_common_clamp_channel_inverted_range",
                      "an inverted range (lo > hi) resolves by the documented "
                      "select order rather than hard-erroring",
                      &tests_color_common_clamp_channel_inverted_range },
                    { "tests_color_common_clamp_channel_non_finite",
                      "infinities clamp to the bounds and NaN passes through "
                      "unchanged (both comparisons are false)",
                      &tests_color_common_clamp_channel_non_finite },
                    { "tests_color_common_clamp_channel_generic_over_int",
                      "clamp_channel is a template and clamps integral types "
                      "with the same semantics",
                      &tests_color_common_clamp_channel_generic_over_int },
                    { "tests_color_common_clamp_channel_is_constexpr",
                      "clamp_channel is usable in a constant expression",
                      &tests_color_common_clamp_channel_is_constexpr },
                    { "tests_color_common_approx_equal_identical",
                      "identical operands compare approximately equal at any "
                      "non-negative tolerance",
                      &tests_color_common_approx_equal_identical },
                    { "tests_color_common_approx_equal_within_tolerance",
                      "operands differing by less than the tolerance compare "
                      "equal",
                      &tests_color_common_approx_equal_within_tolerance },
                    { "tests_color_common_approx_equal_tolerance_is_inclusive",
                      "a difference exactly equal to the tolerance still "
                      "compares equal (the test is <=)",
                      &tests_color_common_approx_equal_tolerance_is_inclusive },
                    { "tests_color_common_approx_equal_beyond_tolerance",
                      "operands differing by more than the tolerance compare "
                      "unequal",
                      &tests_color_common_approx_equal_beyond_tolerance },
                    { "tests_color_common_approx_equal_is_symmetric",
                      "the comparison is symmetric in its operands, so both "
                      "branches of the absolute-difference select agree",
                      &tests_color_common_approx_equal_is_symmetric },
                    { "tests_color_common_approx_equal_zero_tolerance",
                      "a zero tolerance degrades to exact equality",
                      &tests_color_common_approx_equal_zero_tolerance },
                    { "tests_color_common_approx_equal_negative_operands",
                      "negative and mixed-sign operands compare on true "
                      "absolute difference",
                      &tests_color_common_approx_equal_negative_operands },
                    { "tests_color_common_approx_equal_nan_is_never_equal",
                      "NaN is not approximately equal to anything, including "
                      "itself",
                      &tests_color_common_approx_equal_nan_is_never_equal },
                    { "tests_color_common_approx_equal_is_constexpr",
                      "approx_equal is usable in a constant expression",
                      &tests_color_common_approx_equal_is_constexpr },
                }
            },
            dt::block_spec{
                "tags",
                "The model-tag hierarchy: seven empty per-model tags rooted "
                "at color_model_tag, checked for derivation, distinctness, "
                "and emptiness.",
                {
                    { "tests_color_common_tags_derive_from_base",
                      "all seven model tags derive from color_model_tag",
                      &tests_color_common_tags_derive_from_base },
                    { "tests_color_common_tags_are_distinct_types",
                      "no two model tags name the same type",
                      &tests_color_common_tags_are_distinct_types },
                    { "tests_color_common_tags_are_empty_and_default_"
                      "constructible",
                      "every tag is an empty, default-constructible class",
                      &tests_color_common_tags_are_empty_and_default_constructible },
                    { "tests_color_common_tag_base_is_not_a_model_tag_itself",
                      "color_model_tag is the base only - it exposes no "
                      "nested model_tag, so it is not itself a model",
                      &tests_color_common_tag_base_is_not_a_model_tag_itself },
                }
            },
            dt::block_spec{
                "detection",
                "The is_color_model trait and the has_model_tag_helper / "
                "color_detail_void machinery beneath it: acceptance, "
                "rejection, cv-ref stripping, access control, and trait shape.",
                {
                    { "tests_color_common_is_color_model_accepts_tagged",
                      "a type exposing a nested model_tag is detected as a "
                      "color model",
                      &tests_color_common_is_color_model_accepts_tagged },
                    { "tests_color_common_is_color_model_rejects_untagged",
                      "a transport-shaped type with channels but no model_tag "
                      "is not a color model",
                      &tests_color_common_is_color_model_rejects_untagged },
                    { "tests_color_common_is_color_model_rejects_scalars_and_"
                      "void",
                      "scalars, pointers, arrays, function types and void are "
                      "rejected without hard error",
                      &tests_color_common_is_color_model_rejects_scalars_and_void },
                    { "tests_color_common_is_color_model_strips_cv_and_"
                      "reference",
                      "const, volatile and reference qualifiers are stripped "
                      "before detection",
                      &tests_color_common_is_color_model_strips_cv_and_reference },
                    { "tests_color_common_is_color_model_is_structural_not_"
                      "semantic",
                      "detection probes only for the nested name, so a "
                      "model_tag of unrelated type still qualifies",
                      &tests_color_common_is_color_model_is_structural_not_semantic },
                    { "tests_color_common_is_color_model_respects_access_"
                      "control",
                      "a private model_tag fails detection by SFINAE rather "
                      "than producing an access error",
                      &tests_color_common_is_color_model_respects_access_control },
                    { "tests_color_common_is_color_model_ignores_data_members",
                      "a model_tag DATA member does not satisfy the trait - "
                      "the probe requires a nested type",
                      &tests_color_common_is_color_model_ignores_data_members },
                    { "tests_color_common_is_color_model_trait_shape",
                      "is_color_model presents the standard bool-trait shape "
                      "(value, value_type, type, conversion, call)",
                      &tests_color_common_is_color_model_trait_shape },
                    { "tests_color_common_is_color_model_v_matches_trait",
                      "the C++14 variable-template shorthand agrees with the "
                      "class trait for every probe",
                      &tests_color_common_is_color_model_v_matches_trait },
                    { "tests_color_common_has_model_tag_helper_direct",
                      "the internal has_model_tag_helper selects its "
                      "specialization exactly when a nested model_tag exists",
                      &tests_color_common_has_model_tag_helper_direct },
                    { "tests_color_common_detail_void_maps_to_void",
                      "internal::color_detail_void maps any well-formed type "
                      "to void, the C++11-clean void_t the probe needs",
                      &tests_color_common_detail_void_maps_to_void },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_COMMON_TESTS_
