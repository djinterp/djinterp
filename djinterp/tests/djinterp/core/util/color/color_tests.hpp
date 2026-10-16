/*******************************************************************************
* djinterp [tests]                                               color_tests.hpp
*
*   Suite header for color.hpp - the umbrella the PDF layer includes.  It
* pulls in the foundation and rgb/rgba, adds a self-contained cmyk, and
* supplies a deliberately MINIMAL conversion graph: three specialisations over
* an undefined primary template.
*
*   Two design decisions drive most of this suite:
*
*     1. The primary color_convert is declared but never defined, so a pair
*        with no specialisation is a COMPILE error rather than a silent
*        mis-conversion.  The `convert` section detects that incompleteness
*        directly through cu_edge_exists rather than merely asserting the
*        edges that do work.
*
*     2. is_convertible_color asks only whether both endpoints are models.
*        The header says so outright - edge existence is a separate question -
*        and the two answers genuinely disagree in one direction, which the
*        `traits` section pins down.
*
*   NOTE: this header and color_convert.hpp both define color_convert,
* color_cast and is_convertible_color at djinterp scope.  They can never share
* a translation unit, which is why this suite links its own executable.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_TESTS_
#define DJINTERP_TESTS_COLOR_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/color/color.hpp"                            // the header under test
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


// color_umbrella_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_umbrella_check(
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

// D_CU_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CU_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_umbrella_check(                       \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cu_near
//   helper: absolute-tolerance comparison of two channel values.
inline bool
cu_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-5)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

// cu_rgb_near
//   helper: tolerance comparison of an rgb against three expected channels.
inline bool
cu_rgb_near(
    const rgb& _col,
    channel_t  _r,
    channel_t  _g,
    channel_t  _b,
    channel_t  _eps = channel_t(1e-5)
)
{
    return ( cu_near(_col.r, _r, _eps) &&
             cu_near(_col.g, _g, _eps) &&
             cu_near(_col.b, _b, _eps) );
}

// cu_cmyk_near
//   helper: tolerance comparison of a cmyk against four expected channels.
inline bool
cu_cmyk_near(
    const cmyk& _col,
    channel_t   _c,
    channel_t   _m,
    channel_t   _y,
    channel_t   _k,
    channel_t   _eps = channel_t(1e-5)
)
{
    return ( cu_near(_col.c, _c, _eps) &&
             cu_near(_col.m, _m, _eps) &&
             cu_near(_col.y, _y, _eps) &&
             cu_near(_col.k, _k, _eps) );
}

// cu_edge_exists
//   helper: detects whether a color_convert specialisation is COMPLETE.  The
// primary template is declared and never defined, so taking its size is a
// substitution failure - which is precisely how an unsupported pair is turned
// into a compile error.  Reifying that into a bool lets the suite assert the
// absence of an edge instead of only the presence of one.
template <typename T, typename = void>
struct cu_edge_exists
    : std::false_type
{};

template <typename T>
struct cu_edge_exists<T, void_t<decltype(sizeof(T))> >
    : std::true_type
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   THE SHIM CMYK MODEL            (color_tests_cmyk.cpp)
bool tests_color_umbrella_cmyk_default_is_zeroed();
bool tests_color_umbrella_cmyk_parameterized_stores_channels_verbatim();
bool tests_color_umbrella_cmyk_copy_and_assignment();
bool tests_color_umbrella_cmyk_equality_detects_each_channel();
bool tests_color_umbrella_cmyk_inequality_is_declared_not_rewritten();
bool tests_color_umbrella_cmyk_equality_is_constexpr();
bool tests_color_umbrella_cmyk_construction_is_constexpr();
bool tests_color_umbrella_cmyk_layout_is_flat();

// II.  THE MINIMAL GRAPH              (color_tests_convert.cpp)
bool tests_color_umbrella_identity_returns_the_input_unchanged();
bool tests_color_umbrella_identity_covers_every_model();
bool tests_color_umbrella_identity_accepts_rgba_despite_it_not_being_a_model();
bool tests_color_umbrella_cmyk_to_rgb_known_values();
bool tests_color_umbrella_cmyk_to_rgb_is_constexpr();
bool tests_color_umbrella_rgb_to_cmyk_known_values();
bool tests_color_umbrella_rgb_to_cmyk_takes_the_black_branch();
bool tests_color_umbrella_rgb_cmyk_round_trip();
bool tests_color_umbrella_only_the_declared_edges_exist();

// III. THE CAST CONVENIENCE           (color_tests_cast.cpp)
bool tests_color_umbrella_color_cast_deduces_the_source_type();
bool tests_color_umbrella_color_cast_strips_const_and_reference();
bool tests_color_umbrella_color_cast_agrees_with_color_convert();
bool tests_color_umbrella_color_clean_strips_cv_and_reference();
bool tests_color_umbrella_color_cast_round_trips_through_both_edges();
bool tests_color_umbrella_color_cast_identity_needs_no_edge();

// IV.  TRAITS                         (color_tests_traits.cpp)
bool tests_color_umbrella_is_convertible_color_accepts_model_pairs();
bool tests_color_umbrella_is_convertible_color_rejects_rgba();
bool tests_color_umbrella_is_convertible_color_has_integral_constant_shape();
bool tests_color_umbrella_is_convertible_color_strips_cv_and_reference();
bool tests_color_umbrella_trait_and_edge_existence_can_disagree();
bool tests_color_umbrella_exposes_the_foundation();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_spec()
{
    return dt::module_spec{
        "color",
        "The umbrella header the PDF layer includes. It re-exports the "
        "foundation and rgb/rgba, adds a self-contained cmyk, and supplies a "
        "deliberately minimal conversion graph: three specialisations over an "
        "UNDEFINED primary template, so an unsupported pair fails to compile "
        "rather than silently mis-converting. Its is_convertible_color asks "
        "only whether both endpoints are models, which is a weaker question "
        "than whether an edge exists - and the two answers do disagree.",
        {
            dt::block_spec{
                "cmyk",
                "The self-contained cmyk model this header defines: "
                "constructors, exact channel equality with an explicitly "
                "declared operator!=, and its flat four-channel layout.",
                {
                    { "tests_color_umbrella_cmyk_default_is_zeroed",
                      "the default constructor zeroes all four ink channels",
                      &tests_color_umbrella_cmyk_default_is_zeroed },
                    { "tests_color_umbrella_cmyk_parameterized_stores_channels_verbatim",
                      "the channel constructor stores its four arguments "
                      "unchanged and in order",
                      &tests_color_umbrella_cmyk_parameterized_stores_channels_verbatim },
                    { "tests_color_umbrella_cmyk_copy_and_assignment",
                      "copy construction and assignment reproduce every "
                      "channel, and copies are independent",
                      &tests_color_umbrella_cmyk_copy_and_assignment },
                    { "tests_color_umbrella_cmyk_equality_detects_each_channel",
                      "changing any one of the four inks alone breaks equality",
                      &tests_color_umbrella_cmyk_equality_detects_each_channel },
                    { "tests_color_umbrella_cmyk_inequality_is_declared_not_rewritten",
                      "operator!= is declared outright and agrees with the "
                      "negation of operator==, so != works below C++20 too",
                      &tests_color_umbrella_cmyk_inequality_is_declared_not_rewritten },
                    { "tests_color_umbrella_cmyk_equality_is_constexpr",
                      "both comparison operators are usable in a constant "
                      "expression",
                      &tests_color_umbrella_cmyk_equality_is_constexpr },
                    { "tests_color_umbrella_cmyk_construction_is_constexpr",
                      "both constructors are usable in a constant expression",
                      &tests_color_umbrella_cmyk_construction_is_constexpr },
                    { "tests_color_umbrella_cmyk_layout_is_flat",
                      "cmyk is a standard-layout, trivially copyable aggregate "
                      "of exactly four channels",
                      &tests_color_umbrella_cmyk_layout_is_flat },
                }
            },
            dt::block_spec{
                "convert",
                "The three edges this shim actually defines - the identity, "
                "CMYK to RGB and RGB to CMYK - and, just as importantly, the "
                "absence of every other edge.",
                {
                    { "tests_color_umbrella_identity_returns_the_input_unchanged",
                      "converting a type to itself yields a bit-exact copy",
                      &tests_color_umbrella_identity_returns_the_input_unchanged },
                    { "tests_color_umbrella_identity_covers_every_model",
                      "the identity specialisation serves both models this "
                      "header defines",
                      &tests_color_umbrella_identity_covers_every_model },
                    { "tests_color_umbrella_identity_accepts_rgba_despite_it_not_being_a_model",
                      "the identity specialisation is keyed on type equality "
                      "alone, so it accepts rgba even though rgba carries no "
                      "model_tag - a real gap between the graph and the trait",
                      &tests_color_umbrella_identity_accepts_rgba_despite_it_not_being_a_model },
                    { "tests_color_umbrella_cmyk_to_rgb_known_values",
                      "the subtractive-to-additive formula produces the "
                      "expected values for the ink primaries",
                      &tests_color_umbrella_cmyk_to_rgb_known_values },
                    { "tests_color_umbrella_cmyk_to_rgb_is_constexpr",
                      "the CMYK to RGB edge is a single expression and so is "
                      "usable in a constant expression",
                      &tests_color_umbrella_cmyk_to_rgb_is_constexpr },
                    { "tests_color_umbrella_rgb_to_cmyk_known_values",
                      "the additive-to-subtractive formula produces the "
                      "expected inks for the RGB primaries",
                      &tests_color_umbrella_rgb_to_cmyk_known_values },
                    { "tests_color_umbrella_rgb_to_cmyk_takes_the_black_branch",
                      "an input with no luminance takes the guarded branch and "
                      "returns pure black ink rather than dividing by zero",
                      &tests_color_umbrella_rgb_to_cmyk_takes_the_black_branch },
                    { "tests_color_umbrella_rgb_cmyk_round_trip",
                      "the two edges are mutual inverses across the cube "
                      "corners and the greys",
                      &tests_color_umbrella_rgb_cmyk_round_trip },
                    { "tests_color_umbrella_only_the_declared_edges_exist",
                      "the primary template is left INCOMPLETE, so exactly the "
                      "declared pairs are usable and any other pair is a "
                      "compile error - detected here rather than assumed",
                      &tests_color_umbrella_only_the_declared_edges_exist },
                }
            },
            dt::block_spec{
                "cast",
                "color_cast, the convenience the PDF layer actually calls, and "
                "the color_clean normalisation that lets it accept references "
                "and const models.",
                {
                    { "tests_color_umbrella_color_cast_deduces_the_source_type",
                      "only the target need be named, and the result matches "
                      "the fully spelled-out call",
                      &tests_color_umbrella_color_cast_deduces_the_source_type },
                    { "tests_color_umbrella_color_cast_strips_const_and_reference",
                      "const, reference and const-reference sources all "
                      "convert identically",
                      &tests_color_umbrella_color_cast_strips_const_and_reference },
                    { "tests_color_umbrella_color_cast_agrees_with_color_convert",
                      "color_cast adds no arithmetic of its own - it is pure "
                      "routing",
                      &tests_color_umbrella_color_cast_agrees_with_color_convert },
                    { "tests_color_umbrella_color_clean_strips_cv_and_reference",
                      "the color_clean helper reduces every cv-ref spelling of "
                      "a type to the bare type",
                      &tests_color_umbrella_color_clean_strips_cv_and_reference },
                    { "tests_color_umbrella_color_cast_round_trips_through_both_edges",
                      "casting out to CMYK and back recovers the original "
                      "colour within tolerance",
                      &tests_color_umbrella_color_cast_round_trips_through_both_edges },
                    { "tests_color_umbrella_color_cast_identity_needs_no_edge",
                      "a same-type cast resolves to the identity and so works "
                      "for any type, including the transport type rgba",
                      &tests_color_umbrella_color_cast_identity_needs_no_edge },
                }
            },
            dt::block_spec{
                "traits",
                "is_convertible_color as this shim defines it - a question "
                "about the endpoints, not about the graph - and the "
                "foundation types the umbrella re-exports.",
                {
                    { "tests_color_umbrella_is_convertible_color_accepts_model_pairs",
                      "every ordered pair of the two models is reported "
                      "convertible",
                      &tests_color_umbrella_is_convertible_color_accepts_model_pairs },
                    { "tests_color_umbrella_is_convertible_color_rejects_rgba",
                      "rgba is rejected in either position, because it carries "
                      "no model_tag",
                      &tests_color_umbrella_is_convertible_color_rejects_rgba },
                    { "tests_color_umbrella_is_convertible_color_has_integral_constant_shape",
                      "the trait derives from integral_constant, so it is "
                      "usable for tag dispatch and not merely readable",
                      &tests_color_umbrella_is_convertible_color_has_integral_constant_shape },
                    { "tests_color_umbrella_is_convertible_color_strips_cv_and_reference",
                      "the trait answers the same however its operands are "
                      "cv-ref qualified",
                      &tests_color_umbrella_is_convertible_color_strips_cv_and_reference },
                    { "tests_color_umbrella_trait_and_edge_existence_can_disagree",
                      "the trait is NOT a proxy for edge existence: rgba to "
                      "rgba is reported non-convertible yet has a working "
                      "identity edge, exactly as the header's own note warns",
                      &tests_color_umbrella_trait_and_edge_existence_can_disagree },
                    { "tests_color_umbrella_exposes_the_foundation",
                      "including the umbrella alone brings in channel_t, the "
                      "model tags, is_color_model, rgb and rgba",
                      &tests_color_umbrella_exposes_the_foundation },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_TESTS_
