/*******************************************************************************
* djinterp [tests]                                           color_hsv_tests.hpp
*
*   Suite header for color_hsv.hpp - the C++ ergonomic layer over the HSV
* shared kernel: `hsv` derives from d_color_hsv without adding state and
* forwards construction, comparison, validation and canonicalizing clamp to
* the C kernel.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_hsv_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_HSV_TESTS_
#define DJINTERP_TESTS_COLOR_HSV_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_hsv.hpp"                        // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module
#endif


// ::djinterp::test must NAME a namespace in both faces before `dt` can alias
// it.  In spec mode test_defaults.hpp populates it; in normal mode nothing
// does - djinterp.hpp only defines the NS_TEST macro, and the color headers
// under test pull nothing from djinterp/test/.
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


// color_hsv_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_hsv_check(
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

// D_CV_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CV_CHECK(...)                                                       \
    do {                                                                     \
        if (!::djinterp::testing::color_hsv_check(                            \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                    \
            return false;                                                     \
        }                                                                    \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cv_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
cv_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// cv_inf
//   helper: positive infinity of the channel scalar type.
inline channel_t
cv_inf()
{
    return std::numeric_limits<channel_t>::infinity();
}

// cv_near
//   helper: absolute-tolerance comparison of two channel values.
inline bool
cv_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-4)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// cv_same
//   helper: exact channel-wise comparison against three raw values, used
// where operator== is itself under test and must not be the oracle.
inline bool
cv_same(
    const hsv& _col,
    channel_t  _h,
    channel_t  _s,
    channel_t  _v
)
{
    return ( (_col.h == _h) &&
             (_col.s == _s) &&
             (_col.v == _v) );
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CONSTRUCTION & TYPE IDENTITY   (color_hsv_tests_construction.cpp)
bool tests_color_hsv_default_is_zeroed();
bool tests_color_hsv_parameterized_stores_channels_verbatim();
bool tests_color_hsv_converting_constructor_wraps_pod();
bool tests_color_hsv_slices_back_to_pod();
bool tests_color_hsv_adds_no_state_to_the_pod();
bool tests_color_hsv_copy_and_assignment();
bool tests_color_hsv_construction_is_constexpr();
bool tests_color_hsv_model_type_identity();

// II.  COMPARISON                     (color_hsv_tests_comparison.cpp)
bool tests_color_hsv_equality_is_reflexive();
bool tests_color_hsv_equality_detects_each_channel();
bool tests_color_hsv_equality_is_symmetric();
bool tests_color_hsv_equality_is_exact_not_approximate();
bool tests_color_hsv_equality_signed_zero_and_nan();
bool tests_color_hsv_equality_does_not_canonicalize_hue();
bool tests_color_hsv_equality_is_constexpr();
bool tests_color_hsv_inequality_is_the_negation_of_equality();

// III. VALIDATION                     (color_hsv_tests_validation.cpp)
bool tests_color_hsv_is_valid_accepts_canonical_ranges();
bool tests_color_hsv_is_valid_hue_lower_bound_is_inclusive();
bool tests_color_hsv_is_valid_hue_upper_bound_is_exclusive();
bool tests_color_hsv_is_valid_rejects_negative_hue();
bool tests_color_hsv_is_valid_unit_bounds_are_inclusive();
bool tests_color_hsv_is_valid_rejects_out_of_unit_range();
bool tests_color_hsv_is_valid_rejects_non_finite();
bool tests_color_hsv_is_valid_is_constexpr();
bool tests_color_hsv_is_valid_agrees_with_kernel();

// IV.  CLAMPING                       (color_hsv_tests_clamping.cpp)
bool tests_color_hsv_clamp_wraps_hue_above_range();
bool tests_color_hsv_clamp_wraps_negative_hue();
bool tests_color_hsv_clamp_preserves_canonical_hue();
bool tests_color_hsv_clamp_constrains_unit_channels();
bool tests_color_hsv_clamp_is_per_channel();
bool tests_color_hsv_clamp_is_idempotent();
bool tests_color_hsv_clamp_result_is_valid();
bool tests_color_hsv_clamp_non_finite_unit_channels();
bool tests_color_hsv_clamp_is_constexpr();
bool tests_color_hsv_clamp_agrees_with_kernel();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_hsv_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_hsv_spec()
{
    return dt::module_spec{
        "color_hsv",
        "The C++ ergonomic layer for the HSV cylindrical model. `hsv` derives "
        "from the shared-kernel POD d_color_hsv without adding state, "
        "supplying constexpr construction, exact channel comparison, an "
        "is_valid predicate over the half-open hue interval [0, 360) with "
        "saturation and value in [0, 1], and a canonicalizing clamp that "
        "wraps hue rather than saturating it.",
        {
            dt::block_spec{
                "construction",
                "The three constructors, the no-added-state guarantee over "
                "the kernel POD, and the type identity that enrols hsv in "
                "the conversion graph.",
                {
                    { "tests_color_hsv_default_is_zeroed",
                      "the default constructor zeroes hue, saturation and value",
                      &tests_color_hsv_default_is_zeroed },
                    { "tests_color_hsv_parameterized_stores_channels_verbatim",
                      "the channel constructor stores its arguments unchanged, "
                      "imposing no range or wrapping policy",
                      &tests_color_hsv_parameterized_stores_channels_verbatim },
                    { "tests_color_hsv_converting_constructor_wraps_pod",
                      "a kernel-produced d_color_hsv implicitly wraps into "
                      "a hsv with identical channels",
                      &tests_color_hsv_converting_constructor_wraps_pod },
                    { "tests_color_hsv_slices_back_to_pod",
                      "a hsv slices to its d_color_hsv base, so it can be "
                      "handed straight to a kernel routine",
                      &tests_color_hsv_slices_back_to_pod },
                    { "tests_color_hsv_adds_no_state_to_the_pod",
                      "hsv derives publicly from d_color_hsv and adds no "
                      "members, so the two are the same size and layout",
                      &tests_color_hsv_adds_no_state_to_the_pod },
                    { "tests_color_hsv_copy_and_assignment",
                      "copy construction and copy assignment reproduce every "
                      "channel",
                      &tests_color_hsv_copy_and_assignment },
                    { "tests_color_hsv_construction_is_constexpr",
                      "all three constructors are usable in a constant "
                      "expression",
                      &tests_color_hsv_construction_is_constexpr },
                    { "tests_color_hsv_model_type_identity",
                      "model_tag is hsv_tag, value_type is channel_t, and "
                      "is_color_model accepts hsv",
                      &tests_color_hsv_model_type_identity },
                }
            },
            dt::block_spec{
                "comparison",
                "operator== as exact channel equality: reflexivity, symmetry, "
                "per-channel discrimination, floating-point exactness, the "
                "deliberate absence of hue canonicalisation, and the C++20 "
                "rewritten operator!=.",
                {
                    { "tests_color_hsv_equality_is_reflexive",
                      "a value equals itself and equals a separately built "
                      "copy of the same channels",
                      &tests_color_hsv_equality_is_reflexive },
                    { "tests_color_hsv_equality_detects_each_channel",
                      "changing hue, saturation or value alone breaks equality",
                      &tests_color_hsv_equality_detects_each_channel },
                    { "tests_color_hsv_equality_is_symmetric",
                      "the comparison gives the same verdict in either operand "
                      "order",
                      &tests_color_hsv_equality_is_symmetric },
                    { "tests_color_hsv_equality_is_exact_not_approximate",
                      "a one-ulp channel difference compares unequal - there "
                      "is no built-in tolerance",
                      &tests_color_hsv_equality_is_exact_not_approximate },
                    { "tests_color_hsv_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and any channel "
                      "holding NaN makes the value unequal to itself",
                      &tests_color_hsv_equality_signed_zero_and_nan },
                    { "tests_color_hsv_equality_does_not_canonicalize_hue",
                      "hues 360 degrees apart denote the same color but "
                      "compare unequal - equality is on stored channels, and "
                      "clamp is what canonicalises",
                      &tests_color_hsv_equality_does_not_canonicalize_hue },
                    { "tests_color_hsv_equality_is_constexpr",
                      "operator== is usable in a constant expression",
                      &tests_color_hsv_equality_is_constexpr },
                    { "tests_color_hsv_inequality_is_the_negation_of_equality",
                      "operator!= agrees with the negation of operator== "
                      "(C++20 rewrites it from the single declared operator)",
                      &tests_color_hsv_inequality_is_the_negation_of_equality },
                }
            },
            dt::block_spec{
                "validation",
                "is_valid over the canonical ranges: the half-open hue "
                "interval (0 admitted, 360 rejected), the closed unit "
                "intervals for saturation and value, non-finite operands, "
                "and agreement with the kernel.",
                {
                    { "tests_color_hsv_is_valid_accepts_canonical_ranges",
                      "interior values across all three channels are valid",
                      &tests_color_hsv_is_valid_accepts_canonical_ranges },
                    { "tests_color_hsv_is_valid_hue_lower_bound_is_inclusive",
                      "a hue of exactly 0 is valid",
                      &tests_color_hsv_is_valid_hue_lower_bound_is_inclusive },
                    { "tests_color_hsv_is_valid_hue_upper_bound_is_exclusive",
                      "a hue of exactly 360 is REJECTED while the value just "
                      "below it is accepted - the interval is half-open",
                      &tests_color_hsv_is_valid_hue_upper_bound_is_exclusive },
                    { "tests_color_hsv_is_valid_rejects_negative_hue",
                      "any negative hue is rejected, however small",
                      &tests_color_hsv_is_valid_rejects_negative_hue },
                    { "tests_color_hsv_is_valid_unit_bounds_are_inclusive",
                      "saturation and value of exactly 0 and exactly 1 are "
                      "valid",
                      &tests_color_hsv_is_valid_unit_bounds_are_inclusive },
                    { "tests_color_hsv_is_valid_rejects_out_of_unit_range",
                      "saturation or value outside [0, 1] invalidates the "
                      "color, each channel independently",
                      &tests_color_hsv_is_valid_rejects_out_of_unit_range },
                    { "tests_color_hsv_is_valid_rejects_non_finite",
                      "NaN and the infinities are rejected on every channel",
                      &tests_color_hsv_is_valid_rejects_non_finite },
                    { "tests_color_hsv_is_valid_is_constexpr",
                      "is_valid is usable in a constant expression",
                      &tests_color_hsv_is_valid_is_constexpr },
                    { "tests_color_hsv_is_valid_agrees_with_kernel",
                      "the wrapper's verdict matches d_color_hsv_is_valid on "
                      "the sliced POD for every probe",
                      &tests_color_hsv_is_valid_agrees_with_kernel },
                }
            },
            dt::block_spec{
                "clamping",
                "clamp as canonicalisation: hue wraps modulo 360 into "
                "[0, 360) in both directions while saturation and value "
                "saturate onto [0, 1], with idempotence, the validity "
                "postcondition, and the finite-hue precondition made "
                "explicit.",
                {
                    { "tests_color_hsv_clamp_wraps_hue_above_range",
                      "hues at or above 360 wrap into range rather than "
                      "saturating (360 becomes 0, 400 becomes 40)",
                      &tests_color_hsv_clamp_wraps_hue_above_range },
                    { "tests_color_hsv_clamp_wraps_negative_hue",
                      "negative hues wrap up into range (-10 becomes 350), "
                      "including magnitudes beyond a full turn",
                      &tests_color_hsv_clamp_wraps_negative_hue },
                    { "tests_color_hsv_clamp_preserves_canonical_hue",
                      "a hue already in [0, 360) passes through unchanged",
                      &tests_color_hsv_clamp_preserves_canonical_hue },
                    { "tests_color_hsv_clamp_constrains_unit_channels",
                      "saturation and value are clamped, not wrapped, onto "
                      "[0, 1]",
                      &tests_color_hsv_clamp_constrains_unit_channels },
                    { "tests_color_hsv_clamp_is_per_channel",
                      "each channel is canonicalised independently under its "
                      "own rule",
                      &tests_color_hsv_clamp_is_per_channel },
                    { "tests_color_hsv_clamp_is_idempotent",
                      "clamping an already-clamped value changes nothing",
                      &tests_color_hsv_clamp_is_idempotent },
                    { "tests_color_hsv_clamp_result_is_valid",
                      "for any finite input the clamped result satisfies "
                      "is_valid, including the hue == 360 case that clamp "
                      "repairs to 0",
                      &tests_color_hsv_clamp_result_is_valid },
                    { "tests_color_hsv_clamp_non_finite_unit_channels",
                      "infinite saturation or value saturates and NaN passes "
                      "through; hue must be finite, because the wrap "
                      "truncates through an integer cast",
                      &tests_color_hsv_clamp_non_finite_unit_channels },
                    { "tests_color_hsv_clamp_is_constexpr",
                      "clamp is usable in a constant expression",
                      &tests_color_hsv_clamp_is_constexpr },
                    { "tests_color_hsv_clamp_agrees_with_kernel",
                      "the wrapper's result matches d_color_hsv_clamp on the "
                      "sliced POD for every probe",
                      &tests_color_hsv_clamp_agrees_with_kernel },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_HSV_TESTS_
