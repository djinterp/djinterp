/*******************************************************************************
* djinterp [tests]                                       color_convert_tests.hpp
*
*   Suite header for color_convert.hpp - the conversion graph over the seven
* color models.  Its shape is a hub-and-spoke: every model converts to and
* from linear RGB, and any other pair is composed from those two legs.  Two
* edges are shortcut deliberately (XYZ <-> LAB, which the kernel does
* directly) and one is a no-op (a model to itself).
*
*   Two API orders coexist here and are easy to confuse; the traits section
* pins both down:
*
*     color_convert<To, From>::apply(from)     - TARGET first
*     is_convertible_color<From, To>::value    - SOURCE first
*
*   Numeric expectations were measured against the C kernels rather than
* derived from idealised colour arithmetic, so the round-trip residuals below
* are the real ones (see the `hub` section).
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_CONVERT_TESTS_
#define DJINTERP_TESTS_COLOR_CONVERT_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color_convert.hpp"                    // the header under test
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


// color_convert_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_convert_check(
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

// D_CX_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CX_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_convert_check(                        \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cx_near
//   helper: absolute-tolerance comparison of two channel values.  Conversions
// are lossy, so almost nothing here can be compared exactly; the default
// tolerance is loose enough for a full round trip through the RGB hub.
inline bool
cx_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-4)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// cx_rgb_near
//   helper: tolerance comparison of an rgb against three expected channels.
inline bool
cx_rgb_near(
    const rgb& _col,
    channel_t  _r,
    channel_t  _g,
    channel_t  _b,
    channel_t  _eps = channel_t(1e-4)
)
{
    return ( cx_near(_col.r, _r, _eps) &&
             cx_near(_col.g, _g, _eps) &&
             cx_near(_col.b, _b, _eps) );
}

// cx_rgb_round_trip
//   helper: converts an rgb out to _Model and straight back, reporting
// whether every channel survived within tolerance.  The workhorse of the hub
// section: it exercises both legs of a spoke in one call.
template <typename _Model>
bool
cx_rgb_round_trip(
    const rgb& _col,
    channel_t  _eps = channel_t(1e-4)
)
{
    const _Model out  = color_cast<_Model>(_col);
    const rgb    back = color_cast<rgb>(out);

    return cx_rgb_near(back, _col.r, _col.g, _col.b, _eps);
}

// cx_primaries
//   helper: the eight corners of the RGB cube plus mid grey, as the standard
// probe set for a round trip.  Returned by index so the caller can loop.
inline rgb
cx_primaries(
    std::size_t _i
)
{
    switch (_i)
    {
        case 0:  return rgb(channel_t(0), channel_t(0), channel_t(0));
        case 1:  return rgb(channel_t(1), channel_t(0), channel_t(0));
        case 2:  return rgb(channel_t(0), channel_t(1), channel_t(0));
        case 3:  return rgb(channel_t(0), channel_t(0), channel_t(1));
        case 4:  return rgb(channel_t(1), channel_t(1), channel_t(0));
        case 5:  return rgb(channel_t(0), channel_t(1), channel_t(1));
        case 6:  return rgb(channel_t(1), channel_t(0), channel_t(1));
        case 7:  return rgb(channel_t(1), channel_t(1), channel_t(1));
        default: return rgb(channel_t(0.5), channel_t(0.5), channel_t(0.5));
    }
}

// cx_primary_count
//   constant: the number of probes cx_primaries offers.
D_STATIC_CONSTEXPR std::size_t cx_primary_count = 9;

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   KERNEL DISPATCH                (color_convert_tests_dispatch.cpp)
bool tests_color_convert_to_rgb_overload_per_model();
bool tests_color_convert_to_rgb_from_rgb_is_the_identity();
bool tests_color_convert_to_rgb_returns_a_wrapped_rgb();
bool tests_color_convert_rgb_to_specialization_per_model();
bool tests_color_convert_rgb_to_rgb_is_the_identity();
bool tests_color_convert_rgb_to_returns_the_wrapped_target();
bool tests_color_convert_dispatch_matches_the_public_entry_point();

// II.  IDENTITY                       (color_convert_tests_identity.cpp)
bool tests_color_convert_identity_returns_the_input_unchanged();
bool tests_color_convert_identity_covers_every_model();
bool tests_color_convert_identity_does_not_round_trip_through_rgb();
bool tests_color_convert_identity_preserves_out_of_gamut_values();
bool tests_color_convert_identity_is_constexpr();

// III. THE RGB HUB                    (color_convert_tests_hub.cpp)
bool tests_color_convert_rgb_to_hsl_known_values();
bool tests_color_convert_rgb_to_hsv_known_values();
bool tests_color_convert_rgb_to_cmyk_known_values();
bool tests_color_convert_rgb_to_ycbcr_known_values();
bool tests_color_convert_rgb_to_xyz_known_values();
bool tests_color_convert_hsl_round_trip();
bool tests_color_convert_hsv_round_trip();
bool tests_color_convert_cmyk_round_trip();
bool tests_color_convert_ycbcr_round_trip();
bool tests_color_convert_xyz_round_trip();
bool tests_color_convert_round_trip_residual_is_not_exactly_zero();
bool tests_color_convert_grey_has_no_saturation();

// IV.  THE CIE SHORTCUT               (color_convert_tests_lab.cpp)
bool tests_color_convert_xyz_to_lab_is_a_direct_edge();
bool tests_color_convert_lab_to_xyz_is_a_direct_edge();
bool tests_color_convert_xyz_lab_round_trip();
bool tests_color_convert_white_point_maps_to_lightness_100();
bool tests_color_convert_rgb_to_lab_known_values();
bool tests_color_convert_lab_round_trip_through_the_hub();
bool tests_color_convert_lab_lightness_tracks_luminance();

// V.   CROSS-MODEL ROUTING            (color_convert_tests_crossmodel.cpp)
bool tests_color_convert_hsl_to_hsv_composes_the_two_legs();
bool tests_color_convert_cmyk_to_hsl_composes_the_two_legs();
bool tests_color_convert_every_ordered_pair_is_reachable();
bool tests_color_convert_cross_model_equals_manual_composition();
bool tests_color_convert_routing_is_not_symmetric_in_precision();

// VI.  TRAITS & CASTING               (color_convert_tests_traits.cpp)
bool tests_color_convert_is_convertible_color_accepts_every_model_pair();
bool tests_color_convert_is_convertible_color_rejects_non_models();
bool tests_color_convert_is_convertible_color_v_matches_the_trait();
bool tests_color_convert_traits_strip_cv_and_reference();
bool tests_color_convert_argument_order_differs_between_api_and_trait();
bool tests_color_cast_deduces_the_source_type();
bool tests_color_cast_accepts_const_and_reference_sources();
bool tests_color_convert_concepts_agree_with_the_traits();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_convert_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_convert_spec()
{
    return dt::module_spec{
        "color_convert",
        "The conversion graph over the seven color models. Every model has an "
        "edge to and from linear RGB; arbitrary pairs are composed from those "
        "two legs; XYZ <-> LAB is shortcut to a direct kernel call; and a "
        "model to itself is a no-op that deliberately avoids the lossy round "
        "trip. Numeric expectations here were measured against the C kernels, "
        "not derived from idealised arithmetic.",
        {
            dt::block_spec{
                "dispatch",
                "The two internal layers the public entry point is built "
                "from: the to_rgb overload set that brings any model to the "
                "hub, and the rgb_to specialisations that take it back out.",
                {
                    { "tests_color_convert_to_rgb_overload_per_model",
                      "every non-RGB model has a to_rgb overload that reaches "
                      "the hub",
                      &tests_color_convert_to_rgb_overload_per_model },
                    { "tests_color_convert_to_rgb_from_rgb_is_the_identity",
                      "to_rgb on an rgb returns it unchanged rather than "
                      "routing it through a conversion",
                      &tests_color_convert_to_rgb_from_rgb_is_the_identity },
                    { "tests_color_convert_to_rgb_returns_a_wrapped_rgb",
                      "the overloads return the C++ wrapper, not the bare "
                      "kernel POD, so results compose without manual rewrapping",
                      &tests_color_convert_to_rgb_returns_a_wrapped_rgb },
                    { "tests_color_convert_rgb_to_specialization_per_model",
                      "rgb_to is specialised for every model, so the hub can "
                      "reach any target",
                      &tests_color_convert_rgb_to_specialization_per_model },
                    { "tests_color_convert_rgb_to_rgb_is_the_identity",
                      "rgb_to<rgb> returns its input unchanged",
                      &tests_color_convert_rgb_to_rgb_is_the_identity },
                    { "tests_color_convert_rgb_to_returns_the_wrapped_target",
                      "each specialisation returns the wrapper type named in "
                      "its parameter, not a POD",
                      &tests_color_convert_rgb_to_returns_the_wrapped_target },
                    { "tests_color_convert_dispatch_matches_the_public_entry_point",
                      "composing the two internal layers by hand gives exactly "
                      "what color_convert produces, confirming the public API "
                      "adds no arithmetic of its own",
                      &tests_color_convert_dispatch_matches_the_public_entry_point },
                }
            },
            dt::block_spec{
                "identity",
                "The same-model specialisation: converting a model to itself "
                "returns the input untouched instead of taking the lossy "
                "detour through RGB.",
                {
                    { "tests_color_convert_identity_returns_the_input_unchanged",
                      "converting a model to its own type yields a bit-exact "
                      "copy",
                      &tests_color_convert_identity_returns_the_input_unchanged },
                    { "tests_color_convert_identity_covers_every_model",
                      "the identity specialisation is available for all seven "
                      "models",
                      &tests_color_convert_identity_covers_every_model },
                    { "tests_color_convert_identity_does_not_round_trip_through_rgb",
                      "a value that would NOT survive an RGB round trip "
                      "survives the identity conversion exactly, proving the "
                      "specialisation is really taken",
                      &tests_color_convert_identity_does_not_round_trip_through_rgb },
                    { "tests_color_convert_identity_preserves_out_of_gamut_values",
                      "the identity path applies no validation or clamping, so "
                      "out-of-range channels pass through untouched",
                      &tests_color_convert_identity_preserves_out_of_gamut_values },
                    { "tests_color_convert_identity_is_constexpr",
                      "the identity conversion is usable in a constant "
                      "expression",
                      &tests_color_convert_identity_is_constexpr },
                }
            },
            dt::block_spec{
                "hub",
                "The five spokes that run through linear RGB, checked against "
                "kernel-measured values in the outbound direction and by full "
                "round trip in both.",
                {
                    { "tests_color_convert_rgb_to_hsl_known_values",
                      "the RGB primaries map to their expected HSL "
                      "coordinates",
                      &tests_color_convert_rgb_to_hsl_known_values },
                    { "tests_color_convert_rgb_to_hsv_known_values",
                      "the RGB primaries map to their expected HSV "
                      "coordinates",
                      &tests_color_convert_rgb_to_hsv_known_values },
                    { "tests_color_convert_rgb_to_cmyk_known_values",
                      "the RGB primaries map to their expected CMYK "
                      "coordinates, including the black-generation branch",
                      &tests_color_convert_rgb_to_cmyk_known_values },
                    { "tests_color_convert_rgb_to_ycbcr_known_values",
                      "the RGB primaries map to their expected BT.601 luma and "
                      "chroma",
                      &tests_color_convert_rgb_to_ycbcr_known_values },
                    { "tests_color_convert_rgb_to_xyz_known_values",
                      "white maps to the D65 tristimulus and black to the "
                      "origin",
                      &tests_color_convert_rgb_to_xyz_known_values },
                    { "tests_color_convert_hsl_round_trip",
                      "every cube corner survives RGB -> HSL -> RGB within "
                      "tolerance",
                      &tests_color_convert_hsl_round_trip },
                    { "tests_color_convert_hsv_round_trip",
                      "every cube corner survives RGB -> HSV -> RGB within "
                      "tolerance",
                      &tests_color_convert_hsv_round_trip },
                    { "tests_color_convert_cmyk_round_trip",
                      "every cube corner survives RGB -> CMYK -> RGB within "
                      "tolerance",
                      &tests_color_convert_cmyk_round_trip },
                    { "tests_color_convert_ycbcr_round_trip",
                      "every cube corner survives RGB -> YCbCr -> RGB within "
                      "tolerance",
                      &tests_color_convert_ycbcr_round_trip },
                    { "tests_color_convert_xyz_round_trip",
                      "every cube corner survives RGB -> XYZ -> RGB within "
                      "tolerance",
                      &tests_color_convert_xyz_round_trip },
                    { "tests_color_convert_round_trip_residual_is_not_exactly_zero",
                      "the round trip is close but NOT bit-exact - pure red "
                      "returns with a small non-zero blue channel, which is "
                      "why every check here carries a tolerance",
                      &tests_color_convert_round_trip_residual_is_not_exactly_zero },
                    { "tests_color_convert_grey_has_no_saturation",
                      "an achromatic input converts to zero saturation in HSL "
                      "and HSV and to zero chroma in YCbCr",
                      &tests_color_convert_grey_has_no_saturation },
                }
            },
            dt::block_spec{
                "lab",
                "The one shortcut in the graph: XYZ <-> LAB is a direct kernel "
                "edge rather than a detour through RGB, which matters because "
                "the detour would clip everything outside the RGB gamut.",
                {
                    { "tests_color_convert_xyz_to_lab_is_a_direct_edge",
                      "XYZ to LAB uses the direct kernel path",
                      &tests_color_convert_xyz_to_lab_is_a_direct_edge },
                    { "tests_color_convert_lab_to_xyz_is_a_direct_edge",
                      "LAB to XYZ uses the direct kernel path",
                      &tests_color_convert_lab_to_xyz_is_a_direct_edge },
                    { "tests_color_convert_xyz_lab_round_trip",
                      "XYZ -> LAB -> XYZ recovers the tristimulus closely, "
                      "including for values outside the RGB gamut that the "
                      "hub route would have clipped",
                      &tests_color_convert_xyz_lab_round_trip },
                    { "tests_color_convert_white_point_maps_to_lightness_100",
                      "the D65 white point converts to L* = 100 with "
                      "vanishing chroma",
                      &tests_color_convert_white_point_maps_to_lightness_100 },
                    { "tests_color_convert_rgb_to_lab_known_values",
                      "the RGB primaries map to their expected L*a*b* "
                      "coordinates",
                      &tests_color_convert_rgb_to_lab_known_values },
                    { "tests_color_convert_lab_round_trip_through_the_hub",
                      "in-gamut colours survive RGB -> LAB -> RGB within "
                      "tolerance",
                      &tests_color_convert_lab_round_trip_through_the_hub },
                    { "tests_color_convert_lab_lightness_tracks_luminance",
                      "L* increases monotonically with luminance across a grey "
                      "ramp, and is non-linear - mid grey sits well above 50",
                      &tests_color_convert_lab_lightness_tracks_luminance },
                }
            },
            dt::block_spec{
                "crossmodel",
                "Pairs with no direct edge, which the graph composes from the "
                "two hub legs. The point of these checks is that composition "
                "is what actually happens - no pair silently acquires a "
                "shortcut of its own.",
                {
                    { "tests_color_convert_hsl_to_hsv_composes_the_two_legs",
                      "HSL to HSV routes through RGB and lands on the expected "
                      "coordinates - including the cyclic case, where a hue of "
                      "zero returns just BELOW 360 rather than just above 0, so "
                      "hue tolerances must be taken modulo 360",
                      &tests_color_convert_hsl_to_hsv_composes_the_two_legs },
                    { "tests_color_convert_cmyk_to_hsl_composes_the_two_legs",
                      "CMYK to HSL routes through RGB and lands on the "
                      "expected coordinates",
                      &tests_color_convert_cmyk_to_hsl_composes_the_two_legs },
                    { "tests_color_convert_every_ordered_pair_is_reachable",
                      "all forty-nine ordered model pairs compile and produce "
                      "a value, so the graph has no missing edge",
                      &tests_color_convert_every_ordered_pair_is_reachable },
                    { "tests_color_convert_cross_model_equals_manual_composition",
                      "a cross-model conversion equals the two legs performed "
                      "by hand, bit for bit",
                      &tests_color_convert_cross_model_equals_manual_composition },
                    { "tests_color_convert_routing_is_not_symmetric_in_precision",
                      "converting out and back through a different model "
                      "accumulates error rather than cancelling it, so the "
                      "graph is not a group and chained casts should be "
                      "avoided",
                      &tests_color_convert_routing_is_not_symmetric_in_precision },
                }
            },
            dt::block_spec{
                "traits",
                "The compile-time surface: which pairs are declared "
                "convertible, the _v shorthand, qualifier stripping, the "
                "target-first vs source-first ordering trap, and the C++20 "
                "concepts layered on top.",
                {
                    { "tests_color_convert_is_convertible_color_accepts_every_model_pair",
                      "all forty-nine ordered pairs of the seven models are "
                      "reported convertible",
                      &tests_color_convert_is_convertible_color_accepts_every_model_pair },
                    { "tests_color_convert_is_convertible_color_rejects_non_models",
                      "types without a model_tag - including rgba and the bare "
                      "kernel PODs - are rejected in either position",
                      &tests_color_convert_is_convertible_color_rejects_non_models },
                    { "tests_color_convert_is_convertible_color_v_matches_the_trait",
                      "the _v shorthand agrees with the trait it abbreviates "
                      "on every probe",
                      &tests_color_convert_is_convertible_color_v_matches_the_trait },
                    { "tests_color_convert_traits_strip_cv_and_reference",
                      "the trait answers the same for a model however it is "
                      "cv-ref qualified, because clean_t is applied first",
                      &tests_color_convert_traits_strip_cv_and_reference },
                    { "tests_color_convert_argument_order_differs_between_api_and_trait",
                      "color_convert is TARGET-first while "
                      "is_convertible_color is SOURCE-first; both orders are "
                      "exercised head-on so the asymmetry is documented rather "
                      "than discovered",
                      &tests_color_convert_argument_order_differs_between_api_and_trait },
                    { "tests_color_cast_deduces_the_source_type",
                      "color_cast needs only the target named explicitly and "
                      "agrees with the fully spelled-out call",
                      &tests_color_cast_deduces_the_source_type },
                    { "tests_color_cast_accepts_const_and_reference_sources",
                      "const, reference and cv-qualified sources all convert, "
                      "exercising the qualifier stripping in the entry point",
                      &tests_color_cast_accepts_const_and_reference_sources },
                    { "tests_color_convert_concepts_agree_with_the_traits",
                      "the C++20 concepts color_model_type and "
                      "convertible_color_type accept exactly what the "
                      "underlying traits accept",
                      &tests_color_convert_concepts_agree_with_the_traits },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_CONVERT_TESTS_
