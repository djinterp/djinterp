/*******************************************************************************
* djinterp [test]                                         color_tests_beside.cpp
*
*   The color umbrella suite, block "beside": color.hpp in one translation
* unit with the headers it gathers, some included before it and some after,
* and with the PDF and CSS layers, which include it themselves.  That every
* one of these compiles together is half the test; the bodies check that the
* types are then the model headers' own, and that they work across all of it.
*
*
* path:      /tests/djinterp/cpp/util/color/color_tests_beside.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

// djinterp: two of the headers the umbrella gathers, BEFORE it ...
#include <djinterp/cpp/util/color/color_cmyk.hpp>     // cmyk
#include <djinterp/cpp/util/color/color_convert.hpp>  // the facade
// ... the umbrella, through the suite header ...
#include "color_tests.hpp"                            // color.hpp
// ... and three more, AFTER it
#include <djinterp/cpp/util/color/color_hsl.hpp>      // hsl
#include <djinterp/cpp/util/color/color_ycbcr.hpp>    // ycbcr
#include <djinterp/c/util/color/color.h>              // the C operations
// the two layers that include the umbrella (each empty below C++17)
#include <djinterp/cpp/util/pdf/pdf_primitives.hpp>   // pdf_color
#include <djinterp/cpp/text/css/css.hpp>              // css::to_css_color


NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_sits_beside_the_model_headers
  Verifies that with color_cmyk.hpp and color_convert.hpp included before the
  umbrella, and color_hsl.hpp and color_ycbcr.hpp after it, there is one of
  each type, the model header's own, and conversions run between all of
  them.
  Tests the following:
  - cmyk is color_cmyk.hpp's, layered on dj_color_cmyk
  - red ink converts to HSL with red's known values
  - it goes on to YCbCr and comes back to the same ink
  - color_cast returns the type named, whichever side of the umbrella its
    header was included on
*/
bool tests_color_umbrella_sits_beside_the_model_headers()
{
    D_CU_CHECK((std::is_base_of<dj_color_cmyk, cmyk>::value));
    D_CU_CHECK((std::is_base_of<dj_color_ycbcr, ycbcr>::value));

    const cmyk ink(channel_t(0), channel_t(1), channel_t(1), channel_t(0));
    const hsl  h = color_cast<hsl>(ink);

    D_CU_CHECK(cu_near(h.h, channel_t(0)));
    D_CU_CHECK(cu_near(h.s, channel_t(1)));
    D_CU_CHECK(cu_near(h.l, channel_t(0.5)));

    const ycbcr y    = color_cast<ycbcr>(h);
    const cmyk  back = color_cast<cmyk>(y);

    D_CU_CHECK(cu_near(back.c, ink.c));
    D_CU_CHECK(cu_near(back.m, ink.m));
    D_CU_CHECK(cu_near(back.y, ink.y));
    D_CU_CHECK(cu_near(back.k, ink.k));

    D_CU_CHECK((std::is_same<decltype(color_cast<cmyk>(y)), cmyk>::value));
    D_CU_CHECK((std::is_same<decltype(color_cast<ycbcr>(ink)),
                             ycbcr>::value));

    return true;
}

/*
tests_color_umbrella_sits_beside_the_c_header
  Verifies that with c/util/color/color.h included after the umbrella, the
  C++ models pass straight to the C kernel's conversions, and each gives
  exactly what the facade gives.
  Tests the following:
  - RGB to HSL, HSV, CMYK, YCbCr, XYZ and L*a*b* through the C kernel
    equal color_cast's results, bit for bit
*/
bool tests_color_umbrella_sits_beside_the_c_header()
{
    const rgb col(channel_t(0.2), channel_t(0.6), channel_t(0.4));

    D_CU_CHECK(hsl(d_color_convert_rgb_to_hsl(col)) == color_cast<hsl>(col));
    D_CU_CHECK(hsv(d_color_convert_rgb_to_hsv(col)) == color_cast<hsv>(col));
    D_CU_CHECK(cmyk(d_color_convert_rgb_to_cmyk(col))
               == color_cast<cmyk>(col));
    D_CU_CHECK(ycbcr(d_color_convert_rgb_to_ycbcr(col))
               == color_cast<ycbcr>(col));
    D_CU_CHECK(cie_xyz(d_color_convert_rgb_to_xyz(col))
               == color_cast<cie_xyz>(col));
    D_CU_CHECK(cie_lab(d_color_convert_rgb_to_lab(col))
               == color_cast<cie_lab>(col));

    return true;
}

/*
tests_color_umbrella_sits_beside_the_pdf_and_css
  Verifies the PDF and CSS layers, which include the umbrella, sit beside the
  model headers and take any model.  Both layers start at C++17; below it
  their headers are empty and this test checks nothing.
  Tests the following:
  - a cmyk stays DeviceCMYK in pdf_color, with its channels kept
  - an hsl goes to pdf_color through the RGB hub, as DeviceRGB
  - CSS writes an hsl as the RGB color it converts to
*/
bool tests_color_umbrella_sits_beside_the_pdf_and_css()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const cmyk      ink(channel_t(0), channel_t(1), channel_t(1),
                        channel_t(0));
    const pdf_color inked(ink);

    D_CU_CHECK(inked.space == DPdfColorSpace::cmyk);
    D_CU_CHECK(inked.m == 1.0);
    D_CU_CHECK(inked.k == 0.0);

    const hsl       red(channel_t(0), channel_t(1), channel_t(0.5));
    const pdf_color routed(red);

    D_CU_CHECK(routed.space == DPdfColorSpace::rgb);
    D_CU_CHECK(cu_near(channel_t(routed.r), channel_t(1)));
    D_CU_CHECK(cu_near(channel_t(routed.g), channel_t(0)));
    D_CU_CHECK(cu_near(channel_t(routed.b), channel_t(0)));

    D_CU_CHECK(css::to_css_color(red)
               == css::to_css_color(color_cast<rgb>(red)));
#endif

    return true;
}

NS_END   // testing
NS_END   // djinterp
