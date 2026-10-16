/*******************************************************************************
* djinterp [tests]                                         color_ycbcr_tests.hpp
*
*   Suite header for color_ycbcr.hpp - the C++ ergonomic layer over the YCbCr
* shared kernel (full-range BT.601): `ycbcr` derives from d_color_ycbcr without
* adding state and forwards construction, comparison, validation and clamping
* to the C kernel.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_ycbcr_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_YCBCR_TESTS_
#define DJINTERP_TESTS_COLOR_YCBCR_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_ycbcr.hpp"
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


// color_ycbcr_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_ycbcr_check(
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

// D_CY_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CY_CHECK(...)                                                       \
    do {                                                                     \
        if (!::djinterp::testing::color_ycbcr_check(                              \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                    \
            return false;                                                     \
        }                                                                    \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cy_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
cy_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// cy_inf
//   helper: positive infinity of the channel scalar type.
inline channel_t
cy_inf()
{
    return std::numeric_limits<channel_t>::infinity();
}

// cy_near
//   helper: absolute-tolerance comparison of two channel values.
inline bool
cy_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-4)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// cy_same
//   helper: exact channel-wise comparison against three raw values, used
// where operator== is itself under test and must not be the oracle.
inline bool
cy_same(
    const ycbcr& _col,
    channel_t    _y,
    channel_t    _cb,
    channel_t    _cr
)
{
    return ( (_col.y  == _y)  &&
             (_col.cb == _cb) &&
             (_col.cr == _cr) );
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CONSTRUCTION & TYPE IDENTITY   (color_ycbcr_tests_construction.cpp)
bool tests_color_ycbcr_default_is_zeroed();
bool tests_color_ycbcr_parameterized_stores_channels_verbatim();
bool tests_color_ycbcr_converting_constructor_wraps_pod();
bool tests_color_ycbcr_slices_back_to_pod();
bool tests_color_ycbcr_adds_no_state_to_the_pod();
bool tests_color_ycbcr_copy_and_assignment();
bool tests_color_ycbcr_construction_is_constexpr();
bool tests_color_ycbcr_model_type_identity();

// II.  COMPARISON                     (color_ycbcr_tests_comparison.cpp)
bool tests_color_ycbcr_equality_is_reflexive();
bool tests_color_ycbcr_equality_detects_each_channel();
bool tests_color_ycbcr_equality_is_symmetric();
bool tests_color_ycbcr_equality_is_exact_not_approximate();
bool tests_color_ycbcr_equality_signed_zero_and_nan();
bool tests_color_ycbcr_equality_is_constexpr();
bool tests_color_ycbcr_inequality_is_the_negation_of_equality();

// III. VALIDATION                     (color_ycbcr_tests_validation.cpp)
bool tests_color_ycbcr_is_valid_accepts_canonical_ranges();
bool tests_color_ycbcr_is_valid_luma_bounds_are_inclusive();
bool tests_color_ycbcr_is_valid_chroma_bounds_are_inclusive();
bool tests_color_ycbcr_is_valid_rejects_out_of_range_luma();
bool tests_color_ycbcr_is_valid_rejects_out_of_range_chroma();
bool tests_color_ycbcr_is_valid_chroma_admits_negatives_luma_does_not();
bool tests_color_ycbcr_is_valid_rejects_non_finite();
bool tests_color_ycbcr_is_valid_is_constexpr();
bool tests_color_ycbcr_is_valid_agrees_with_kernel();

// IV.  CLAMPING                       (color_ycbcr_tests_clamping.cpp)
bool tests_color_ycbcr_clamp_constrains_luma();
bool tests_color_ycbcr_clamp_constrains_chroma_symmetrically();
bool tests_color_ycbcr_clamp_preserves_in_range();
bool tests_color_ycbcr_clamp_is_per_channel();
bool tests_color_ycbcr_clamp_is_idempotent();
bool tests_color_ycbcr_clamp_result_is_valid();
bool tests_color_ycbcr_clamp_non_finite();
bool tests_color_ycbcr_clamp_is_constexpr();
bool tests_color_ycbcr_clamp_agrees_with_kernel();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_ycbcr_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_ycbcr_spec()
{
    return dt::module_spec{
        "color_ycbcr",
        "The C++ ergonomic layer for the YCbCr luma/chroma model "
        "(full-range ITU-R BT.601). `ycbcr` derives from the shared-kernel "
        "POD d_color_ycbcr without adding state, supplying constexpr "
        "construction, exact channel comparison, and is_valid / clamp over "
        "an ASYMMETRIC domain: luma on [0, 1] but both chroma channels on "
        "the signed interval [-0.5, 0.5].",
        {
            dt::block_spec{
                "construction",
                "The three constructors, the no-added-state guarantee over "
                "the kernel POD, and the type identity that enrols ycbcr in "
                "the conversion graph.",
                {
                    { "tests_color_ycbcr_default_is_zeroed",
                      "the default constructor zeroes luma and both chroma "
                      "channels",
                      &tests_color_ycbcr_default_is_zeroed },
                    { "tests_color_ycbcr_parameterized_stores_channels_verbatim",
                      "the channel constructor stores its arguments unchanged, "
                      "imposing no range policy",
                      &tests_color_ycbcr_parameterized_stores_channels_verbatim },
                    { "tests_color_ycbcr_converting_constructor_wraps_pod",
                      "a kernel-produced d_color_ycbcr implicitly wraps into a "
                      "ycbcr with identical channels",
                      &tests_color_ycbcr_converting_constructor_wraps_pod },
                    { "tests_color_ycbcr_slices_back_to_pod",
                      "a ycbcr slices to its d_color_ycbcr base, so it can be "
                      "handed straight to a kernel routine",
                      &tests_color_ycbcr_slices_back_to_pod },
                    { "tests_color_ycbcr_adds_no_state_to_the_pod",
                      "ycbcr derives publicly from d_color_ycbcr and adds no "
                      "members, so the two are the same size and layout",
                      &tests_color_ycbcr_adds_no_state_to_the_pod },
                    { "tests_color_ycbcr_copy_and_assignment",
                      "copy construction and copy assignment reproduce every "
                      "channel",
                      &tests_color_ycbcr_copy_and_assignment },
                    { "tests_color_ycbcr_construction_is_constexpr",
                      "all three constructors are usable in a constant "
                      "expression",
                      &tests_color_ycbcr_construction_is_constexpr },
                    { "tests_color_ycbcr_model_type_identity",
                      "model_tag is ycbcr_tag, value_type is channel_t, and "
                      "is_color_model accepts ycbcr",
                      &tests_color_ycbcr_model_type_identity },
                }
            },
            dt::block_spec{
                "comparison",
                "operator== as exact channel equality: reflexivity, symmetry, "
                "per-channel discrimination, floating-point exactness, and "
                "the C++20 rewritten operator!=.",
                {
                    { "tests_color_ycbcr_equality_is_reflexive",
                      "a value equals itself and equals a separately built "
                      "copy of the same channels",
                      &tests_color_ycbcr_equality_is_reflexive },
                    { "tests_color_ycbcr_equality_detects_each_channel",
                      "changing luma, Cb or Cr alone breaks equality",
                      &tests_color_ycbcr_equality_detects_each_channel },
                    { "tests_color_ycbcr_equality_is_symmetric",
                      "the comparison gives the same verdict in either operand "
                      "order",
                      &tests_color_ycbcr_equality_is_symmetric },
                    { "tests_color_ycbcr_equality_is_exact_not_approximate",
                      "a one-ulp channel difference compares unequal - there "
                      "is no built-in tolerance",
                      &tests_color_ycbcr_equality_is_exact_not_approximate },
                    { "tests_color_ycbcr_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and any channel "
                      "holding NaN makes the value unequal to itself",
                      &tests_color_ycbcr_equality_signed_zero_and_nan },
                    { "tests_color_ycbcr_equality_is_constexpr",
                      "operator== is usable in a constant expression",
                      &tests_color_ycbcr_equality_is_constexpr },
                    { "tests_color_ycbcr_inequality_is_the_negation_of_equality",
                      "operator!= agrees with the negation of operator== "
                      "(C++20 rewrites it from the single declared operator)",
                      &tests_color_ycbcr_inequality_is_the_negation_of_equality },
                }
            },
            dt::block_spec{
                "validation",
                "is_valid over the asymmetric domain: luma on the closed unit "
                "interval, chroma on the signed [-0.5, 0.5], with the "
                "difference between the two rules made explicit.",
                {
                    { "tests_color_ycbcr_is_valid_accepts_canonical_ranges",
                      "interior values on all three channels are valid",
                      &tests_color_ycbcr_is_valid_accepts_canonical_ranges },
                    { "tests_color_ycbcr_is_valid_luma_bounds_are_inclusive",
                      "luma of exactly 0 and exactly 1 is valid",
                      &tests_color_ycbcr_is_valid_luma_bounds_are_inclusive },
                    { "tests_color_ycbcr_is_valid_chroma_bounds_are_inclusive",
                      "chroma of exactly -0.5 and exactly +0.5 is valid on "
                      "both Cb and Cr",
                      &tests_color_ycbcr_is_valid_chroma_bounds_are_inclusive },
                    { "tests_color_ycbcr_is_valid_rejects_out_of_range_luma",
                      "luma below zero or above one invalidates the color",
                      &tests_color_ycbcr_is_valid_rejects_out_of_range_luma },
                    { "tests_color_ycbcr_is_valid_rejects_out_of_range_chroma",
                      "either chroma channel outside [-0.5, 0.5] invalidates "
                      "the color, independently",
                      &tests_color_ycbcr_is_valid_rejects_out_of_range_chroma },
                    { "tests_color_ycbcr_is_valid_chroma_admits_negatives_luma_does_not",
                      "a negative chroma is valid while the same negative luma "
                      "is not - the two channel families use different rules",
                      &tests_color_ycbcr_is_valid_chroma_admits_negatives_luma_does_not },
                    { "tests_color_ycbcr_is_valid_rejects_non_finite",
                      "NaN and the infinities are rejected on every channel",
                      &tests_color_ycbcr_is_valid_rejects_non_finite },
                    { "tests_color_ycbcr_is_valid_is_constexpr",
                      "is_valid is usable in a constant expression",
                      &tests_color_ycbcr_is_valid_is_constexpr },
                    { "tests_color_ycbcr_is_valid_agrees_with_kernel",
                      "the wrapper's verdict matches d_color_ycbcr_is_valid on "
                      "the sliced POD for every probe",
                      &tests_color_ycbcr_is_valid_agrees_with_kernel },
                }
            },
            dt::block_spec{
                "clamping",
                "clamp as the per-channel projection onto the asymmetric "
                "domain: luma onto [0, 1] and chroma onto [-0.5, 0.5], with "
                "independence, idempotence, the validity postcondition, and "
                "non-finite behaviour.",
                {
                    { "tests_color_ycbcr_clamp_constrains_luma",
                      "luma saturates onto [0, 1] in both directions",
                      &tests_color_ycbcr_clamp_constrains_luma },
                    { "tests_color_ycbcr_clamp_constrains_chroma_symmetrically",
                      "both chroma channels saturate onto [-0.5, 0.5], so a "
                      "negative excursion clamps to -0.5 and not to zero",
                      &tests_color_ycbcr_clamp_constrains_chroma_symmetrically },
                    { "tests_color_ycbcr_clamp_preserves_in_range",
                      "values already inside the domain, including the bounds, "
                      "pass through bit-exact",
                      &tests_color_ycbcr_clamp_preserves_in_range },
                    { "tests_color_ycbcr_clamp_is_per_channel",
                      "each channel is clamped independently under its own "
                      "bounds",
                      &tests_color_ycbcr_clamp_is_per_channel },
                    { "tests_color_ycbcr_clamp_is_idempotent",
                      "clamping an already-clamped value changes nothing",
                      &tests_color_ycbcr_clamp_is_idempotent },
                    { "tests_color_ycbcr_clamp_result_is_valid",
                      "for any finite input the clamped result satisfies "
                      "is_valid",
                      &tests_color_ycbcr_clamp_result_is_valid },
                    { "tests_color_ycbcr_clamp_non_finite",
                      "infinities saturate to the bounds while NaN passes "
                      "through unchanged and stays invalid",
                      &tests_color_ycbcr_clamp_non_finite },
                    { "tests_color_ycbcr_clamp_is_constexpr",
                      "clamp is usable in a constant expression",
                      &tests_color_ycbcr_clamp_is_constexpr },
                    { "tests_color_ycbcr_clamp_agrees_with_kernel",
                      "the wrapper's result matches d_color_ycbcr_clamp on the "
                      "sliced POD for every probe",
                      &tests_color_ycbcr_clamp_agrees_with_kernel },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_YCBCR_TESTS_
