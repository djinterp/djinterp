/*******************************************************************************
* djinterp [tests]                                       color_cie_lab_tests.hpp
*
*   Suite header for color_cie_lab.hpp - the C++ ergonomic layer over the CIE
* shared kernel (color_lab.h).  The header wraps TWO models: cie_xyz, the
* device-independent tristimulus hub, and cie_lab, the perceptual L*a*b*
* space that converts through it.  Both derive from their kernel PODs without
* adding state.  The four blocks below each cover both types, because the two
* wrappers present the identical a/b/c/d surface.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_cie_lab_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_CIE_LAB_TESTS_
#define DJINTERP_TESTS_COLOR_CIE_LAB_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_cie_lab.hpp"                    // the header under test
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


// color_cie_lab_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_cie_lab_check(
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

// D_CE_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CE_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_cie_lab_check(                        \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// ce_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
ce_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// ce_inf
//   helper: positive infinity of the channel scalar type.
inline channel_t
ce_inf()
{
    return std::numeric_limits<channel_t>::infinity();
}

// ce_near
//   helper: absolute-tolerance comparison of two channel values.
inline bool
ce_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-4)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// ce_xyz_same
//   helper: exact channel-wise comparison of a cie_xyz against three raw
// values, used where operator== is itself under test.
inline bool
ce_xyz_same(
    const cie_xyz& _col,
    channel_t      _x,
    channel_t      _y,
    channel_t      _z
)
{
    return ( (_col.x == _x) &&
             (_col.y == _y) &&
             (_col.z == _z) );
}

// ce_lab_same
//   helper: exact channel-wise comparison of a cie_lab against three raw
// values, used where operator== is itself under test.
inline bool
ce_lab_same(
    const cie_lab& _col,
    channel_t      _l,
    channel_t      _a,
    channel_t      _b
)
{
    return ( (_col.l == _l) &&
             (_col.a == _a) &&
             (_col.b == _b) );
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CONSTRUCTION & TYPE IDENTITY   (color_cie_lab_tests_construction.cpp)
bool tests_color_cie_xyz_default_is_zeroed();
bool tests_color_cie_xyz_parameterized_stores_channels_verbatim();
bool tests_color_cie_xyz_converting_constructor_wraps_pod();
bool tests_color_cie_xyz_adds_no_state_to_the_pod();
bool tests_color_cie_xyz_construction_is_constexpr();
bool tests_color_cie_xyz_model_type_identity();
bool tests_color_cie_lab_default_is_zeroed();
bool tests_color_cie_lab_parameterized_stores_channels_verbatim();
bool tests_color_cie_lab_converting_constructor_wraps_pod();
bool tests_color_cie_lab_adds_no_state_to_the_pod();
bool tests_color_cie_lab_construction_is_constexpr();
bool tests_color_cie_lab_model_type_identity();
bool tests_color_cie_models_are_distinct_types();

// II.  COMPARISON                     (color_cie_lab_tests_comparison.cpp)
bool tests_color_cie_xyz_equality_is_reflexive();
bool tests_color_cie_xyz_equality_detects_each_channel();
bool tests_color_cie_xyz_equality_signed_zero_and_nan();
bool tests_color_cie_xyz_equality_is_constexpr();
bool tests_color_cie_lab_equality_is_reflexive();
bool tests_color_cie_lab_equality_detects_each_channel();
bool tests_color_cie_lab_equality_is_exact_not_approximate();
bool tests_color_cie_lab_equality_signed_zero_and_nan();
bool tests_color_cie_lab_equality_is_constexpr();
bool tests_color_cie_lab_inequality_is_the_negation_of_equality();

// III. VALIDATION                     (color_cie_lab_tests_validation.cpp)
bool tests_color_cie_xyz_is_valid_accepts_non_negative();
bool tests_color_cie_xyz_is_valid_zero_is_valid();
bool tests_color_cie_xyz_is_valid_has_no_upper_bound();
bool tests_color_cie_xyz_is_valid_rejects_negative_per_channel();
bool tests_color_cie_xyz_is_valid_rejects_nan();
bool tests_color_cie_xyz_is_valid_agrees_with_kernel();
bool tests_color_cie_lab_is_valid_accepts_lightness_range();
bool tests_color_cie_lab_is_valid_lightness_bounds_are_inclusive();
bool tests_color_cie_lab_is_valid_rejects_out_of_range_lightness();
bool tests_color_cie_lab_is_valid_ignores_the_chroma_axes();
bool tests_color_cie_lab_is_valid_rejects_nan_lightness_only();
bool tests_color_cie_lab_is_valid_is_constexpr();
bool tests_color_cie_lab_is_valid_agrees_with_kernel();

// IV.  CLAMPING                       (color_cie_lab_tests_clamping.cpp)
bool tests_color_cie_xyz_clamp_lifts_negative_to_zero();
bool tests_color_cie_xyz_clamp_preserves_non_negative();
bool tests_color_cie_xyz_clamp_has_no_upper_bound();
bool tests_color_cie_xyz_clamp_is_idempotent();
bool tests_color_cie_xyz_clamp_result_is_valid();
bool tests_color_cie_xyz_clamp_is_constexpr();
bool tests_color_cie_xyz_clamp_agrees_with_kernel();
bool tests_color_cie_lab_clamp_constrains_lightness();
bool tests_color_cie_lab_clamp_constrains_the_chroma_axes();
bool tests_color_cie_lab_clamp_is_stricter_than_is_valid();
bool tests_color_cie_lab_clamp_is_idempotent();
bool tests_color_cie_lab_clamp_result_is_valid();
bool tests_color_cie_lab_clamp_non_finite();
bool tests_color_cie_lab_clamp_is_constexpr();
bool tests_color_cie_lab_clamp_agrees_with_kernel();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_cie_lab_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_cie_lab_spec()
{
    return dt::module_spec{
        "color_cie_lab",
        "The C++ ergonomic layer for the two CIE models. cie_xyz is the "
        "device-independent tristimulus hub, valid on the non-negative "
        "orthant with no upper bound; cie_lab is the perceptual L*a*b* space, "
        "whose is_valid constrains ONLY the L* axis to [0, 100] while clamp "
        "additionally bounds a* and b* to [-128, 127].  Both wrappers derive "
        "from their kernel PODs without adding state, and each block below "
        "covers both types.",
        {
            dt::block_spec{
                "construction",
                "Constructors, the no-added-state guarantee over each kernel "
                "POD, and the type identities that enrol both models in the "
                "conversion graph as distinct participants.",
                {
                    { "tests_color_cie_xyz_default_is_zeroed",
                      "the cie_xyz default constructor zeroes all three "
                      "tristimulus values",
                      &tests_color_cie_xyz_default_is_zeroed },
                    { "tests_color_cie_xyz_parameterized_stores_channels_verbatim",
                      "the cie_xyz channel constructor stores its arguments "
                      "unchanged, imposing no range policy",
                      &tests_color_cie_xyz_parameterized_stores_channels_verbatim },
                    { "tests_color_cie_xyz_converting_constructor_wraps_pod",
                      "a kernel-produced d_color_xyz implicitly wraps into a "
                      "cie_xyz with identical channels",
                      &tests_color_cie_xyz_converting_constructor_wraps_pod },
                    { "tests_color_cie_xyz_adds_no_state_to_the_pod",
                      "cie_xyz derives publicly from d_color_xyz and adds no "
                      "members",
                      &tests_color_cie_xyz_adds_no_state_to_the_pod },
                    { "tests_color_cie_xyz_construction_is_constexpr",
                      "every cie_xyz constructor is usable in a constant "
                      "expression",
                      &tests_color_cie_xyz_construction_is_constexpr },
                    { "tests_color_cie_xyz_model_type_identity",
                      "cie_xyz carries cie_xyz_tag and channel_t, and is "
                      "accepted by is_color_model",
                      &tests_color_cie_xyz_model_type_identity },
                    { "tests_color_cie_lab_default_is_zeroed",
                      "the cie_lab default constructor zeroes L*, a* and b*",
                      &tests_color_cie_lab_default_is_zeroed },
                    { "tests_color_cie_lab_parameterized_stores_channels_verbatim",
                      "the cie_lab channel constructor stores its arguments "
                      "unchanged, including far out-of-gamut chroma",
                      &tests_color_cie_lab_parameterized_stores_channels_verbatim },
                    { "tests_color_cie_lab_converting_constructor_wraps_pod",
                      "a kernel-produced d_color_lab implicitly wraps into a "
                      "cie_lab with identical channels",
                      &tests_color_cie_lab_converting_constructor_wraps_pod },
                    { "tests_color_cie_lab_adds_no_state_to_the_pod",
                      "cie_lab derives publicly from d_color_lab and adds no "
                      "members",
                      &tests_color_cie_lab_adds_no_state_to_the_pod },
                    { "tests_color_cie_lab_construction_is_constexpr",
                      "every cie_lab constructor is usable in a constant "
                      "expression",
                      &tests_color_cie_lab_construction_is_constexpr },
                    { "tests_color_cie_lab_model_type_identity",
                      "cie_lab carries cie_lab_tag and channel_t, and is "
                      "accepted by is_color_model",
                      &tests_color_cie_lab_model_type_identity },
                    { "tests_color_cie_models_are_distinct_types",
                      "cie_xyz and cie_lab are unrelated types with different "
                      "tags, so neither converts silently to the other",
                      &tests_color_cie_models_are_distinct_types },
                }
            },
            dt::block_spec{
                "comparison",
                "operator== as exact channel equality on both models: "
                "reflexivity, per-channel discrimination, IEEE corner cases, "
                "constant-expression use, and the C++20 rewritten operator!=.",
                {
                    { "tests_color_cie_xyz_equality_is_reflexive",
                      "a cie_xyz equals itself and equals a separately built "
                      "copy",
                      &tests_color_cie_xyz_equality_is_reflexive },
                    { "tests_color_cie_xyz_equality_detects_each_channel",
                      "changing X, Y or Z alone breaks cie_xyz equality",
                      &tests_color_cie_xyz_equality_detects_each_channel },
                    { "tests_color_cie_xyz_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and a NaN channel "
                      "makes a cie_xyz unequal to itself",
                      &tests_color_cie_xyz_equality_signed_zero_and_nan },
                    { "tests_color_cie_xyz_equality_is_constexpr",
                      "cie_xyz operator== is usable in a constant expression",
                      &tests_color_cie_xyz_equality_is_constexpr },
                    { "tests_color_cie_lab_equality_is_reflexive",
                      "a cie_lab equals itself and equals a separately built "
                      "copy",
                      &tests_color_cie_lab_equality_is_reflexive },
                    { "tests_color_cie_lab_equality_detects_each_channel",
                      "changing L*, a* or b* alone breaks cie_lab equality",
                      &tests_color_cie_lab_equality_detects_each_channel },
                    { "tests_color_cie_lab_equality_is_exact_not_approximate",
                      "a one-ulp channel difference compares unequal - there "
                      "is no perceptual tolerance built into operator==",
                      &tests_color_cie_lab_equality_is_exact_not_approximate },
                    { "tests_color_cie_lab_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and a NaN channel "
                      "makes a cie_lab unequal to itself",
                      &tests_color_cie_lab_equality_signed_zero_and_nan },
                    { "tests_color_cie_lab_equality_is_constexpr",
                      "cie_lab operator== is usable in a constant expression",
                      &tests_color_cie_lab_equality_is_constexpr },
                    { "tests_color_cie_lab_inequality_is_the_negation_of_equality",
                      "operator!= agrees with the negation of operator== "
                      "(C++20 rewrites it from the single declared operator)",
                      &tests_color_cie_lab_inequality_is_the_negation_of_equality },
                }
            },
            dt::block_spec{
                "validation",
                "The two very different is_valid predicates: cie_xyz admits "
                "the whole non-negative orthant with no ceiling, while "
                "cie_lab constrains only L* and ignores the chroma axes "
                "entirely.",
                {
                    { "tests_color_cie_xyz_is_valid_accepts_non_negative",
                      "non-negative tristimulus values are valid",
                      &tests_color_cie_xyz_is_valid_accepts_non_negative },
                    { "tests_color_cie_xyz_is_valid_zero_is_valid",
                      "the all-zero tristimulus (and the default-constructed "
                      "value) is valid - the bound is inclusive",
                      &tests_color_cie_xyz_is_valid_zero_is_valid },
                    { "tests_color_cie_xyz_is_valid_has_no_upper_bound",
                      "arbitrarily large tristimulus values remain valid; the "
                      "predicate constrains only the sign",
                      &tests_color_cie_xyz_is_valid_has_no_upper_bound },
                    { "tests_color_cie_xyz_is_valid_rejects_negative_per_channel",
                      "a negative value on any single axis invalidates the "
                      "whole tristimulus",
                      &tests_color_cie_xyz_is_valid_rejects_negative_per_channel },
                    { "tests_color_cie_xyz_is_valid_rejects_nan",
                      "NaN is rejected on every axis, and negative infinity "
                      "with it",
                      &tests_color_cie_xyz_is_valid_rejects_nan },
                    { "tests_color_cie_xyz_is_valid_agrees_with_kernel",
                      "the wrapper's verdict matches d_color_xyz_is_valid on "
                      "the sliced POD for every probe",
                      &tests_color_cie_xyz_is_valid_agrees_with_kernel },
                    { "tests_color_cie_lab_is_valid_accepts_lightness_range",
                      "L* inside [0, 100] is valid",
                      &tests_color_cie_lab_is_valid_accepts_lightness_range },
                    { "tests_color_cie_lab_is_valid_lightness_bounds_are_inclusive",
                      "L* of exactly 0 and exactly 100 is valid",
                      &tests_color_cie_lab_is_valid_lightness_bounds_are_inclusive },
                    { "tests_color_cie_lab_is_valid_rejects_out_of_range_lightness",
                      "L* below 0 or above 100 invalidates the color",
                      &tests_color_cie_lab_is_valid_rejects_out_of_range_lightness },
                    { "tests_color_cie_lab_is_valid_ignores_the_chroma_axes",
                      "a* and b* are NOT checked at all - even absurd chroma "
                      "leaves the value valid so long as L* is in range",
                      &tests_color_cie_lab_is_valid_ignores_the_chroma_axes },
                    { "tests_color_cie_lab_is_valid_rejects_nan_lightness_only",
                      "a NaN L* is rejected while a NaN on a* or b* is not, "
                      "following directly from the predicate's scope",
                      &tests_color_cie_lab_is_valid_rejects_nan_lightness_only },
                    { "tests_color_cie_lab_is_valid_is_constexpr",
                      "cie_lab is_valid is usable in a constant expression",
                      &tests_color_cie_lab_is_valid_is_constexpr },
                    { "tests_color_cie_lab_is_valid_agrees_with_kernel",
                      "the wrapper's verdict matches d_color_lab_is_valid on "
                      "the sliced POD for every probe",
                      &tests_color_cie_lab_is_valid_agrees_with_kernel },
                }
            },
            dt::block_spec{
                "clamping",
                "The two clamps: cie_xyz projects onto the non-negative "
                "orthant with no ceiling, while cie_lab bounds L* to "
                "[0, 100] AND the chroma axes to [-128, 127] - strictly more "
                "than its own is_valid inspects.",
                {
                    { "tests_color_cie_xyz_clamp_lifts_negative_to_zero",
                      "negative tristimulus values are raised to exactly zero",
                      &tests_color_cie_xyz_clamp_lifts_negative_to_zero },
                    { "tests_color_cie_xyz_clamp_preserves_non_negative",
                      "non-negative values pass through bit-exact",
                      &tests_color_cie_xyz_clamp_preserves_non_negative },
                    { "tests_color_cie_xyz_clamp_has_no_upper_bound",
                      "large values are NOT lowered - clamp mirrors is_valid "
                      "in constraining only the sign",
                      &tests_color_cie_xyz_clamp_has_no_upper_bound },
                    { "tests_color_cie_xyz_clamp_is_idempotent",
                      "clamping an already-clamped tristimulus changes nothing",
                      &tests_color_cie_xyz_clamp_is_idempotent },
                    { "tests_color_cie_xyz_clamp_result_is_valid",
                      "for any non-NaN input the clamped tristimulus satisfies "
                      "is_valid",
                      &tests_color_cie_xyz_clamp_result_is_valid },
                    { "tests_color_cie_xyz_clamp_is_constexpr",
                      "cie_xyz clamp is usable in a constant expression",
                      &tests_color_cie_xyz_clamp_is_constexpr },
                    { "tests_color_cie_xyz_clamp_agrees_with_kernel",
                      "the wrapper's result matches d_color_xyz_clamp on the "
                      "sliced POD for every probe",
                      &tests_color_cie_xyz_clamp_agrees_with_kernel },
                    { "tests_color_cie_lab_clamp_constrains_lightness",
                      "L* saturates onto [0, 100] in both directions",
                      &tests_color_cie_lab_clamp_constrains_lightness },
                    { "tests_color_cie_lab_clamp_constrains_the_chroma_axes",
                      "a* and b* saturate onto the asymmetric [-128, 127] "
                      "interval",
                      &tests_color_cie_lab_clamp_constrains_the_chroma_axes },
                    { "tests_color_cie_lab_clamp_is_stricter_than_is_valid",
                      "a value can be valid yet still be changed by clamp, "
                      "because clamp bounds chroma that is_valid never "
                      "inspects",
                      &tests_color_cie_lab_clamp_is_stricter_than_is_valid },
                    { "tests_color_cie_lab_clamp_is_idempotent",
                      "clamping an already-clamped cie_lab changes nothing",
                      &tests_color_cie_lab_clamp_is_idempotent },
                    { "tests_color_cie_lab_clamp_result_is_valid",
                      "for any non-NaN input the clamped result satisfies "
                      "is_valid",
                      &tests_color_cie_lab_clamp_result_is_valid },
                    { "tests_color_cie_lab_clamp_non_finite",
                      "infinities saturate to the bounds while NaN passes "
                      "through unchanged",
                      &tests_color_cie_lab_clamp_non_finite },
                    { "tests_color_cie_lab_clamp_is_constexpr",
                      "cie_lab clamp is usable in a constant expression",
                      &tests_color_cie_lab_clamp_is_constexpr },
                    { "tests_color_cie_lab_clamp_agrees_with_kernel",
                      "the wrapper's result matches d_color_lab_clamp on the "
                      "sliced POD for every probe",
                      &tests_color_cie_lab_clamp_agrees_with_kernel },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_CIE_LAB_TESTS_
