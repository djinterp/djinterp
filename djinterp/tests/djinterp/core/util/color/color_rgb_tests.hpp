/*******************************************************************************
* djinterp [tests]                                           color_rgb_tests.hpp
*
*   Suite header for color_rgb.hpp.  This header supplies TWO types with
* deliberately different standing:
*
*     rgb    - a conversion-graph participant.  It carries a model_tag, so
*              is_color_model accepts it and color_convert will route through
*              it as the hub of the RGB-centred graph.
*     rgba   - a TRANSPORT type.  Same channels plus straight alpha, but NO
*              model_tag, so it is deliberately NOT a color model: alpha has
*              no meaning under a chromatic conversion, and the type system
*              is used to say so.
*
*   That distinction is the single most important thing this suite pins down,
* and the `traits` section exists for it.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/core/util/color/color_rgb_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_RGB_TESTS_
#define DJINTERP_TESTS_COLOR_RGB_TESTS_ 1

// std
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "color_rgb.hpp"                        // the header under test
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


// color_rgb_check
//   helper: reports a failed check with its expression and source location.
inline bool
color_rgb_check(
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

// D_CR_CHECK
//   macro: evaluate once, report on failure, early-return false.  Variadic so
// a top-level comma inside a trait expression passes through whole.
#define D_CR_CHECK(...)                                                       \
    do {                                                                      \
        if (!::djinterp::testing::color_rgb_check(                            \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE   // ---- fixtures (normal mode only) ----------------

// cr_nan
//   helper: a quiet NaN of the channel scalar type.
inline channel_t
cr_nan()
{
    return std::numeric_limits<channel_t>::quiet_NaN();
}

// cr_rgb_same
//   helper: exact channel-wise comparison of an rgb against three raw values,
// used where operator== is itself under test and must not be the oracle.
inline bool
cr_rgb_same(
    const rgb& _col,
    channel_t  _r,
    channel_t  _g,
    channel_t  _b
)
{
    return ( (_col.r == _r) &&
             (_col.g == _g) &&
             (_col.b == _b) );
}

// cr_rgba_same
//   helper: exact channel-wise comparison of an rgba against four raw values.
inline bool
cr_rgba_same(
    const rgba& _col,
    channel_t   _r,
    channel_t   _g,
    channel_t   _b,
    channel_t   _a
)
{
    return ( (_col.r == _r) &&
             (_col.g == _g) &&
             (_col.b == _b) &&
             (_col.a == _a) );
}

// cr_has_model_tag
//   helper: detects whether a type publishes a nested model_tag.  That single
// nested type is the whole basis on which is_color_model admits or excludes a
// type, so detecting it directly lets the traits section show WHY rgba is
// rejected rather than merely THAT it is.
template <typename T, typename = void>
struct cr_has_model_tag
    : std::false_type
{};

template <typename T>
struct cr_has_model_tag<T, void_t<typename T::model_tag> >
    : std::true_type
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   CONSTRUCTION                   (color_rgb_tests_construction.cpp)
bool tests_color_rgb_default_is_zeroed();
bool tests_color_rgb_parameterized_stores_channels_verbatim();
bool tests_color_rgb_copy_and_assignment();
bool tests_color_rgb_construction_is_constexpr();
bool tests_color_rgb_layout_is_flat();
bool tests_color_rgba_default_is_zeroed();
bool tests_color_rgba_parameterized_stores_channels_verbatim();
bool tests_color_rgba_from_rgb_and_alpha();
bool tests_color_rgba_copy_and_assignment();
bool tests_color_rgba_construction_is_constexpr();
bool tests_color_rgba_layout_is_flat();

// II.  COMPARISON                     (color_rgb_tests_comparison.cpp)
bool tests_color_rgb_equality_is_reflexive();
bool tests_color_rgb_equality_detects_each_channel();
bool tests_color_rgb_equality_is_symmetric();
bool tests_color_rgb_inequality_is_declared_not_rewritten();
bool tests_color_rgb_equality_signed_zero_and_nan();
bool tests_color_rgb_equality_is_constexpr();
bool tests_color_rgba_equality_includes_alpha();
bool tests_color_rgba_inequality_is_declared_not_rewritten();
bool tests_color_rgba_equality_is_constexpr();

// III. TYPE TRAITS                    (color_rgb_tests_traits.cpp)
bool tests_color_rgb_model_type_identity();
bool tests_color_rgb_is_recognised_as_a_color_model();
bool tests_color_rgba_is_not_a_color_model();
bool tests_color_rgba_has_value_type_but_no_model_tag();
bool tests_color_rgb_and_rgba_are_distinct_types();
bool tests_color_rgb_detection_sees_through_cv_and_reference();
bool tests_color_rgb_channel_type_is_the_common_scalar();

// IV.  ALPHA TRANSPORT                (color_rgb_tests_transport.cpp)
bool tests_color_rgba_to_rgb_drops_alpha();
bool tests_color_rgba_to_rgb_preserves_color_channels();
bool tests_color_rgba_to_rgb_is_constexpr();
bool tests_color_rgba_round_trips_through_rgb();
bool tests_color_rgba_alpha_is_independent_of_color();
bool tests_color_rgba_to_rgb_ignores_the_alpha_value();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_rgb_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline dt::module_spec
color_rgb_spec()
{
    return dt::module_spec{
        "color_rgb",
        "The RGB hub type and its alpha transport companion. `rgb` is the "
        "centre of the conversion graph and carries a model_tag; `rgba` "
        "carries the same channels plus straight alpha but deliberately has "
        "NO model_tag, so the type system refuses to treat it as a "
        "convertible color. Both are flat aggregates with exact channel "
        "equality and explicitly declared operator!=.",
        {
            dt::block_spec{
                "construction",
                "The constructors of both types, including rgba's "
                "composition from an rgb plus an alpha, and the flat "
                "aggregate layout each guarantees.",
                {
                    { "tests_color_rgb_default_is_zeroed",
                      "the rgb default constructor zeroes all three channels",
                      &tests_color_rgb_default_is_zeroed },
                    { "tests_color_rgb_parameterized_stores_channels_verbatim",
                      "the rgb channel constructor stores its arguments "
                      "unchanged and in order",
                      &tests_color_rgb_parameterized_stores_channels_verbatim },
                    { "tests_color_rgb_copy_and_assignment",
                      "rgb copy construction and assignment reproduce every "
                      "channel, and copies are independent",
                      &tests_color_rgb_copy_and_assignment },
                    { "tests_color_rgb_construction_is_constexpr",
                      "both rgb constructors are usable in a constant "
                      "expression",
                      &tests_color_rgb_construction_is_constexpr },
                    { "tests_color_rgb_layout_is_flat",
                      "rgb is a standard-layout, trivially copyable aggregate "
                      "of exactly three channels with no hidden state",
                      &tests_color_rgb_layout_is_flat },
                    { "tests_color_rgba_default_is_zeroed",
                      "the rgba default constructor zeroes colour AND alpha - "
                      "the default is fully transparent black, not opaque",
                      &tests_color_rgba_default_is_zeroed },
                    { "tests_color_rgba_parameterized_stores_channels_verbatim",
                      "the rgba channel constructor stores all four arguments "
                      "unchanged and in order",
                      &tests_color_rgba_parameterized_stores_channels_verbatim },
                    { "tests_color_rgba_from_rgb_and_alpha",
                      "the composing constructor takes an rgb and an alpha and "
                      "copies the three colour channels across intact",
                      &tests_color_rgba_from_rgb_and_alpha },
                    { "tests_color_rgba_copy_and_assignment",
                      "rgba copy construction and assignment reproduce every "
                      "channel including alpha",
                      &tests_color_rgba_copy_and_assignment },
                    { "tests_color_rgba_construction_is_constexpr",
                      "all three rgba constructors are usable in a constant "
                      "expression",
                      &tests_color_rgba_construction_is_constexpr },
                    { "tests_color_rgba_layout_is_flat",
                      "rgba is a standard-layout, trivially copyable aggregate "
                      "of exactly four channels",
                      &tests_color_rgba_layout_is_flat },
                }
            },
            dt::block_spec{
                "comparison",
                "Exact channel equality on both types, the explicitly "
                "declared operator!= (this header does not rely on the C++20 "
                "rewrite), and alpha's participation in rgba comparison.",
                {
                    { "tests_color_rgb_equality_is_reflexive",
                      "an rgb equals itself and equals a separately built copy",
                      &tests_color_rgb_equality_is_reflexive },
                    { "tests_color_rgb_equality_detects_each_channel",
                      "changing red, green or blue alone breaks equality",
                      &tests_color_rgb_equality_detects_each_channel },
                    { "tests_color_rgb_equality_is_symmetric",
                      "the comparison gives the same verdict in either operand "
                      "order",
                      &tests_color_rgb_equality_is_symmetric },
                    { "tests_color_rgb_inequality_is_declared_not_rewritten",
                      "operator!= is declared outright and agrees with the "
                      "negation of operator==, so != works below C++20 too",
                      &tests_color_rgb_inequality_is_declared_not_rewritten },
                    { "tests_color_rgb_equality_signed_zero_and_nan",
                      "negative zero equals positive zero, and a NaN channel "
                      "makes the value unequal to itself",
                      &tests_color_rgb_equality_signed_zero_and_nan },
                    { "tests_color_rgb_equality_is_constexpr",
                      "rgb operator== and operator!= are usable in a constant "
                      "expression",
                      &tests_color_rgb_equality_is_constexpr },
                    { "tests_color_rgba_equality_includes_alpha",
                      "two rgba values with identical colour but different "
                      "alpha compare unequal - alpha is a full participant",
                      &tests_color_rgba_equality_includes_alpha },
                    { "tests_color_rgba_inequality_is_declared_not_rewritten",
                      "rgba operator!= is declared outright and agrees with "
                      "the negation of operator==",
                      &tests_color_rgba_inequality_is_declared_not_rewritten },
                    { "tests_color_rgba_equality_is_constexpr",
                      "rgba operator== and operator!= are usable in a constant "
                      "expression",
                      &tests_color_rgba_equality_is_constexpr },
                }
            },
            dt::block_spec{
                "traits",
                "The type-level distinction between a conversion-graph "
                "participant and a transport type. This is the section that "
                "pins down WHY rgba is not a color model.",
                {
                    { "tests_color_rgb_model_type_identity",
                      "rgb::model_tag is rgb_tag and rgb::value_type is "
                      "channel_t",
                      &tests_color_rgb_model_type_identity },
                    { "tests_color_rgb_is_recognised_as_a_color_model",
                      "is_color_model accepts rgb and reports the "
                      "integral_constant shape a trait is expected to have",
                      &tests_color_rgb_is_recognised_as_a_color_model },
                    { "tests_color_rgba_is_not_a_color_model",
                      "is_color_model REJECTS rgba, and keeps rejecting it "
                      "through every cv-ref qualification - alpha has no "
                      "meaning under a chromatic conversion",
                      &tests_color_rgba_is_not_a_color_model },
                    { "tests_color_rgba_has_value_type_but_no_model_tag",
                      "rgba still exposes value_type for generic channel code, "
                      "so the exclusion is specifically the missing model_tag "
                      "and not a lack of type information",
                      &tests_color_rgba_has_value_type_but_no_model_tag },
                    { "tests_color_rgb_and_rgba_are_distinct_types",
                      "neither type derives from or silently converts to the "
                      "other; going from rgba to rgb requires the explicit "
                      "to_rgb call",
                      &tests_color_rgb_and_rgba_are_distinct_types },
                    { "tests_color_rgb_detection_sees_through_cv_and_reference",
                      "is_color_model<rgb> holds through const, volatile, "
                      "lvalue-reference and rvalue-reference qualification",
                      &tests_color_rgb_detection_sees_through_cv_and_reference },
                    { "tests_color_rgb_channel_type_is_the_common_scalar",
                      "rgb::value_type is the same channel_t the rest of the "
                      "color subframework uses, so channels interoperate "
                      "without conversion",
                      &tests_color_rgb_channel_type_is_the_common_scalar },
                }
            },
            dt::block_spec{
                "transport",
                "rgba's one and only bridge back to the conversion graph: "
                "to_rgb, which drops alpha explicitly rather than letting it "
                "leak silently into a chromatic computation.",
                {
                    { "tests_color_rgba_to_rgb_drops_alpha",
                      "to_rgb returns an rgb, so alpha is discarded at a "
                      "visible call site and cannot leak into a conversion",
                      &tests_color_rgba_to_rgb_drops_alpha },
                    { "tests_color_rgba_to_rgb_preserves_color_channels",
                      "the three colour channels survive to_rgb bit-exact",
                      &tests_color_rgba_to_rgb_preserves_color_channels },
                    { "tests_color_rgba_to_rgb_is_constexpr",
                      "to_rgb is usable in a constant expression",
                      &tests_color_rgba_to_rgb_is_constexpr },
                    { "tests_color_rgba_round_trips_through_rgb",
                      "composing an rgba from an rgb and then calling to_rgb "
                      "recovers the original colour exactly, for any alpha",
                      &tests_color_rgba_round_trips_through_rgb },
                    { "tests_color_rgba_alpha_is_independent_of_color",
                      "writing alpha never disturbs the colour channels, and "
                      "vice versa - the channels are genuinely orthogonal",
                      &tests_color_rgba_alpha_is_independent_of_color },
                    { "tests_color_rgba_to_rgb_ignores_the_alpha_value",
                      "to_rgb performs no premultiplication: the colour is "
                      "returned unscaled even at zero alpha",
                      &tests_color_rgba_to_rgb_ignores_the_alpha_value },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_RGB_TESTS_
