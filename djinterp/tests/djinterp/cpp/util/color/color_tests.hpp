/*******************************************************************************
* djinterp [cpp][utility]                                        color_tests.hpp
*
*   Suite header for color.hpp, the color module's umbrella.  The umbrella
* defines nothing of its own: what it promises is reach and coexistence, so
* that is what this suite checks.
*
*     1. One include gives all of it: every model, the conversion facade, and
*        the C kernel's operations across models (color_tests_include.cpp).
*
*     2. It sits beside the headers it gathers, before or after them in one
*        translation unit, and there is then one of each type; the PDF and CSS
*        layers, which include it, sit beside them too
*        (color_tests_beside.cpp).
*
*   The conversions themselves are color_convert's suite's to check, and each
* model is its own suite's.
*
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3).
*
*
* path:      /tests/djinterp/cpp/util/color/color_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.17
*******************************************************************************/

#ifndef DJINTERP_TESTS_COLOR_TESTS_
#define DJINTERP_TESTS_COLOR_TESTS_ 1

// std
#include <cstdio>       // std::printf
#include <type_traits>  // std::is_same, std::is_base_of
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/util/color/color.hpp>  // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec
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
inline
bool color_umbrella_check(
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
inline
bool cu_near(
    channel_t _a,
    channel_t _b,
    channel_t _eps = channel_t(1e-4)
)
{
    const channel_t d = (_a < _b) ? (_b - _a) : (_a - _b);

    return (d <= _eps);
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ------------------------------------

// I.   ONE INCLUDE                    (color_tests_include.cpp)
bool tests_color_umbrella_gives_every_model();
bool tests_color_umbrella_gives_the_facade();
bool tests_color_umbrella_gives_the_c_operations();
bool tests_color_umbrella_exposes_the_foundation();

// II.  BESIDE THE HEADERS IT GATHERS  (color_tests_beside.cpp)
bool tests_color_umbrella_sits_beside_the_model_headers();
bool tests_color_umbrella_sits_beside_the_c_header();
bool tests_color_umbrella_sits_beside_the_pdf_and_css();


// -- the spec provider - spec mode only --------------------------------------
#ifdef DTEST_SPEC_MODE

// color_spec
//   provider: the declarative description of this suite, one block per
// section translation unit.
inline
dt::module_spec color_spec()
{
    return dt::module_spec{
        "color",
        "The color module's umbrella, color.hpp. It defines nothing of its "
        "own: it gathers every model, the conversion facade and the C "
        "kernel's operations across models, and it sits beside each header "
        "it gathers, before or after it.",
        {
            dt::block_spec{
                "include",
                "One include gives the whole module: every model, the "
                "conversion facade, the C operations across models, and the "
                "foundation the models share.",
                {
                    { "tests_color_umbrella_gives_every_model",
                      "all seven models are complete types that report "
                      "themselves models, and rgba is reachable but is not "
                      "one",
                      &tests_color_umbrella_gives_every_model },
                    { "tests_color_umbrella_gives_the_facade",
                      "color_cast, color_convert, is_convertible_color and "
                      "its _v form, and from C++20 the concepts, all work "
                      "between models no other include brought in",
                      &tests_color_umbrella_gives_the_facade },
                    { "tests_color_umbrella_gives_the_c_operations",
                      "the C kernel's operations across models (CIEDE2000, "
                      "hue rotation through HSL) take the C++ models "
                      "directly",
                      &tests_color_umbrella_gives_the_c_operations },
                    { "tests_color_umbrella_exposes_the_foundation",
                      "channel_t, the model tags, is_color_model, the scalar "
                      "helpers and the rgba transport bridge are all in "
                      "reach",
                      &tests_color_umbrella_exposes_the_foundation },
                }
            },
            dt::block_spec{
                "beside",
                "The umbrella included before and after the headers it "
                "gathers, in one translation unit, leaves one of each type; "
                "the PDF and CSS layers, which include it, sit beside the "
                "model headers too.",
                {
                    { "tests_color_umbrella_sits_beside_the_model_headers",
                      "with cmyk and the facade included before the umbrella "
                      "and hsl and ycbcr after, each type is the model "
                      "header's own and every conversion between them works",
                      &tests_color_umbrella_sits_beside_the_model_headers },
                    { "tests_color_umbrella_sits_beside_the_c_header",
                      "with c/util/color/color.h included after the "
                      "umbrella, the C++ models pass to its operations and "
                      "agree with the conversions",
                      &tests_color_umbrella_sits_beside_the_c_header },
                    { "tests_color_umbrella_sits_beside_the_pdf_and_css",
                      "pdf_primitives.hpp and css.hpp included beside the "
                      "model headers take any model: cmyk stays DeviceCMYK, "
                      "hsl goes through the hub, and CSS writes hsl as rgb",
                      &tests_color_umbrella_sits_beside_the_pdf_and_css },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif  // DJINTERP_TESTS_COLOR_TESTS_
