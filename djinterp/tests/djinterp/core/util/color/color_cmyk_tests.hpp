/*******************************************************************************
* djinterp [tests]                                          color_cmyk_tests.hpp
*
*   Suite header for color_cmyk.hpp - the C++ ergonomic layer over the CMYK
* shared kernel: `cmyk` derives from d_color_cmyk without adding state and
* forwards construction, comparison, validation and clamping to the C kernel.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_cmyk_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_CMYK_TESTS_
#define DJINTERP_TESTS_COLOR_CMYK_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_cmyk.hpp"                       // the header under test
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


// color_cmyk_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_cmyk_check(
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

// D_CK_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CK_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_cmyk_check(                           \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// ck_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
ck_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// ck_inf
//   helper: positive infinity of the channel scalar type.
inline channel_t
ck_inf()
{
    return std::numeric_limits<channel_t>::infinity();
}

// ck_near
//   helper: absolute-tolerance comparison of two channel values.
inline bool
ck_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-6)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// ck_same
//   helper: exact channel-wise comparison against four raw values, used where
// operator== is itself under test and must not be the oracle.
inline bool
ck_same(
    const cmyk& _v,
    channel_t   _c,
    channel_t   _m,
    channel_t   _y,
    channel_t   _k
)
{
    return ( (_v.c == _c) &&
             (_v.m == _m) &&
             (_v.y == _y) &&
             (_v.k == _k) );
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CONSTRUCTION & TYPE IDENTITY   (color_cmyk_tests_construction.cpp)
bool tests_color_cmyk_default_is_zeroed();
bool tests_color_cmyk_parameterized_stores_channels_verbatim();
bool tests_color_cmyk_converting_constructor_wraps_pod();
bool tests_color_cmyk_slices_back_to_pod();
bool tests_color_cmyk_adds_no_state_to_the_pod();
bool tests_color_cmyk_copy_and_assignment();
bool tests_color_cmyk_construction_is_constexpr();
bool tests_color_cmyk_model_type_identity();

// II.  COMPARISON                     (color_cmyk_tests_comparison.cpp)
bool tests_color_cmyk_equality_is_reflexive();
bool tests_color_cmyk_equality_detects_each_channel();
bool tests_color_cmyk_equality_is_symmetric();
bool tests_color_cmyk_equality_is_exact_not_approximate();
bool tests_color_cmyk_equality_signed_zero_and_nan();
bool tests_color_cmyk_equality_is_constexpr();
bool tests_color_cmyk_inequality_is_the_negation_of_equality();

// III. VALIDATION                     (color_cmyk_tests_validation.cpp)
bool tests_color_cmyk_is_valid_accepts_unit_range();
bool tests_color_cmyk_is_valid_boundaries_are_inclusive();
bool tests_color_cmyk_is_valid_rejects_below_zero_per_channel();
bool tests_color_cmyk_is_valid_rejects_above_one_per_channel();
bool tests_color_cmyk_is_valid_rejects_non_finite();
bool tests_color_cmyk_is_valid_default_constructed();
bool tests_color_cmyk_is_valid_is_constexpr();
bool tests_color_cmyk_is_valid_agrees_with_kernel();

// IV.  CLAMPING                       (color_cmyk_tests_clamping.cpp)
bool tests_color_cmyk_clamp_lifts_below_zero();
bool tests_color_cmyk_clamp_lowers_above_one();
bool tests_color_cmyk_clamp_preserves_in_range();
bool tests_color_cmyk_clamp_is_per_channel();
bool tests_color_cmyk_clamp_is_idempotent();
bool tests_color_cmyk_clamp_result_is_valid();
bool tests_color_cmyk_clamp_non_finite();
bool tests_color_cmyk_clamp_is_constexpr();
bool tests_color_cmyk_clamp_agrees_with_kernel();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_cmyk_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_cmyk_spec()
{
    return dt::module_spec{
        "color_cmyk",
        "The C++ ergonomic layer for the CMYK subtractive device model. "
        "`cmyk` derives from the shared-kernel POD d_color_cmyk without "
        "adding state, supplying constexpr construction, exact channel "
        "comparison, and is_valid / clamp that forward to the C kernel. "
        "Conversions are not part of this header - they live in the "
        "color_convert facade.",
        {
            dt::block_spec{
                "construction",
                "The three constructors, the no-added-state guarantee over "
                "the kernel POD, and the type identity (model_tag, "
                "value_type) that enrols cmyk in the conversion graph.",
                {
                    { "tests_color_cmyk_default_is_zeroed",
                      "the default constructor zeroes all four channels",
                      &tests_color_cmyk_default_is_zeroed },
                    { "tests_color_cmyk_parameterized_stores_channels_verbatim",
                      "the channel constructor stores its arguments unchanged, "
                      "imposing no range policy",
                      &tests_color_cmyk_parameterized_stores_channels_verbatim },
                    { "tests_color_cmyk_converting_constructor_wraps_pod",
                      "a kernel-produced d_color_cmyk implicitly wraps into a "
                      "cmyk with identical channels",
                      &tests_color_cmyk_converting_constructor_wraps_pod },
                    { "tests_color_cmyk_slices_back_to_pod",
                      "a cmyk slices to its d_color_cmyk base, so it can be "
                      "handed straight to a kernel routine",
                      &tests_color_cmyk_slices_back_to_pod },
                    { "tests_color_cmyk_adds_no_state_to_the_pod",
                      "cmyk derives publicly from d_color_cmyk and adds no "
                      "members, so the two are the same size and layout",
                      &tests_color_cmyk_adds_no_state_to_the_pod },
                    { "tests_color_cmyk_copy_and_assignment",
                      "copy construction and copy assignment reproduce every "
                      "channel",
                      &tests_color_cmyk_copy_and_assignment },
                    { "tests_color_cmyk_construction_is_constexpr",
                      "all three constructors are usable in a constant "
                      "expression",
                      &tests_color_cmyk_construction_is_constexpr },
                    { "tests_color_cmyk_model_type_identity",
                      "model_tag is cmyk_tag, value_type is channel_t, and "
                      "is_color_model accepts cmyk",
                      &tests_color_cmyk_model_type_identity },
                }
            },
            dt::block_spec{
                "comparison",
                "operator== as exact channel equality: reflexivity, symmetry, "
                "per-channel discrimination, floating-point exactness, and "
                "the C++20 rewritten operator!=.",
                {
                    { "tests_color_cmyk_equality_is_reflexive",
                      "a value equals itself and equals a separately built "
                      "copy of the same channels",
                      &tests_color_cmyk_equality_is_reflexive },
                    { "tests_color_cmyk_equality_detects_each_channel",
                      "changing any one of c, m, y or k alone breaks equality",
                      &tests_color_cmyk_equality_detects_each_channel },
                    { "tests_color_cmyk_equality_is_symmetric",
                      "the comparison gives the same verdict in either operand "
                      "order",
                      &tests_color_cmyk_equality_is_symmetric },
                    { "tests_color_cmyk_equality_is_exact_not_approximate",
                      "a one-ulp channel difference compares unequal - there "
                      "is no built-in tolerance",
                      &tests_color_cmyk_equality_is_exact_not_approximate },
                    { "tests_color_cmyk_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and any channel "
                      "holding NaN makes the value unequal to itself",
                      &tests_color_cmyk_equality_signed_zero_and_nan },
                    { "tests_color_cmyk_equality_is_constexpr",
                      "operator== is usable in a constant expression",
                      &tests_color_cmyk_equality_is_constexpr },
                    { "tests_color_cmyk_inequality_is_the_negation_of_equality",
                      "operator!= agrees with the negation of operator== "
                      "(C++20 rewrites it from the single declared operator)",
                      &tests_color_cmyk_inequality_is_the_negation_of_equality },
                }
            },
            dt::block_spec{
                "validation",
                "is_valid as the closed unit-interval predicate: acceptance, "
                "inclusive boundaries, per-channel rejection at both ends, "
                "non-finite operands, and agreement with the kernel.",
                {
                    { "tests_color_cmyk_is_valid_accepts_unit_range",
                      "interior values in [0, 1] on every channel are valid",
                      &tests_color_cmyk_is_valid_accepts_unit_range },
                    { "tests_color_cmyk_is_valid_boundaries_are_inclusive",
                      "channels of exactly 0 and exactly 1 are valid - the "
                      "interval is closed at both ends",
                      &tests_color_cmyk_is_valid_boundaries_are_inclusive },
                    { "tests_color_cmyk_is_valid_rejects_below_zero_per_channel",
                      "a negative value on any single channel invalidates the "
                      "whole color",
                      &tests_color_cmyk_is_valid_rejects_below_zero_per_channel },
                    { "tests_color_cmyk_is_valid_rejects_above_one_per_channel",
                      "an above-unity value on any single channel invalidates "
                      "the whole color",
                      &tests_color_cmyk_is_valid_rejects_above_one_per_channel },
                    { "tests_color_cmyk_is_valid_rejects_non_finite",
                      "NaN and the infinities are rejected on every channel",
                      &tests_color_cmyk_is_valid_rejects_non_finite },
                    { "tests_color_cmyk_is_valid_default_constructed",
                      "a default-constructed cmyk is valid",
                      &tests_color_cmyk_is_valid_default_constructed },
                    { "tests_color_cmyk_is_valid_is_constexpr",
                      "is_valid is usable in a constant expression",
                      &tests_color_cmyk_is_valid_is_constexpr },
                    { "tests_color_cmyk_is_valid_agrees_with_kernel",
                      "the wrapper's verdict matches d_color_cmyk_is_valid on "
                      "the sliced POD for every probe",
                      &tests_color_cmyk_is_valid_agrees_with_kernel },
                }
            },
            dt::block_spec{
                "clamping",
                "clamp as the per-channel projection onto [0, 1]: both "
                "saturating directions, pass-through, independence, "
                "idempotence, the validity postcondition, and non-finite "
                "behaviour.",
                {
                    { "tests_color_cmyk_clamp_lifts_below_zero",
                      "negative channels are raised to exactly zero",
                      &tests_color_cmyk_clamp_lifts_below_zero },
                    { "tests_color_cmyk_clamp_lowers_above_one",
                      "above-unity channels are lowered to exactly one",
                      &tests_color_cmyk_clamp_lowers_above_one },
                    { "tests_color_cmyk_clamp_preserves_in_range",
                      "channels already inside [0, 1], including the bounds, "
                      "pass through bit-exact",
                      &tests_color_cmyk_clamp_preserves_in_range },
                    { "tests_color_cmyk_clamp_is_per_channel",
                      "each channel is clamped independently; one saturating "
                      "channel does not disturb the others",
                      &tests_color_cmyk_clamp_is_per_channel },
                    { "tests_color_cmyk_clamp_is_idempotent",
                      "clamping an already-clamped value changes nothing",
                      &tests_color_cmyk_clamp_is_idempotent },
                    { "tests_color_cmyk_clamp_result_is_valid",
                      "for any finite input the clamped result satisfies "
                      "is_valid",
                      &tests_color_cmyk_clamp_result_is_valid },
                    { "tests_color_cmyk_clamp_non_finite",
                      "infinities saturate to the bounds while NaN passes "
                      "through unchanged and stays invalid",
                      &tests_color_cmyk_clamp_non_finite },
                    { "tests_color_cmyk_clamp_is_constexpr",
                      "clamp is usable in a constant expression",
                      &tests_color_cmyk_clamp_is_constexpr },
                    { "tests_color_cmyk_clamp_agrees_with_kernel",
                      "the wrapper's result matches d_color_cmyk_clamp on the "
                      "sliced POD for every probe",
                      &tests_color_cmyk_clamp_agrees_with_kernel },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_CMYK_TESTS_
