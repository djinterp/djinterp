/*******************************************************************************
* djinterp [test]                                        color_tests_include.cpp
*
*   The color umbrella suite, block "include": color.hpp alone, included
* through the suite header and nothing else from the module, gives every
* model, the conversion facade, the C kernel's operations across models, and
* the foundation the models share.
*
*
* path:      /tests/djinterp/cpp/util/color/color_tests_include.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

// djinterp
#include "color_tests.hpp"  // the suite; brings color.hpp alone


NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_gives_every_model
  Verifies the one include brings all seven models, each layered on its C
  kernel's struct and carrying its own tag, and the rgba transport type, which
  is not a model.
  Tests the following:
  - each of the seven models reports itself a model
  - each derives from its C kernel's struct, and so is the production type
  - each carries its own model tag
  - rgba is reachable, derives from dj_color_rgba, and is not a model
*/
bool tests_color_umbrella_gives_every_model()
{
    D_CU_CHECK(is_color_model<rgb>::value);
    D_CU_CHECK(is_color_model<cmyk>::value);
    D_CU_CHECK(is_color_model<hsv>::value);
    D_CU_CHECK(is_color_model<hsl>::value);
    D_CU_CHECK(is_color_model<ycbcr>::value);
    D_CU_CHECK(is_color_model<cie_xyz>::value);
    D_CU_CHECK(is_color_model<cie_lab>::value);

    D_CU_CHECK((std::is_base_of<dj_color_rgb, rgb>::value));
    D_CU_CHECK((std::is_base_of<dj_color_cmyk, cmyk>::value));
    D_CU_CHECK((std::is_base_of<dj_color_hsv, hsv>::value));
    D_CU_CHECK((std::is_base_of<dj_color_hsl, hsl>::value));
    D_CU_CHECK((std::is_base_of<dj_color_ycbcr, ycbcr>::value));
    D_CU_CHECK((std::is_base_of<dj_color_xyz, cie_xyz>::value));
    D_CU_CHECK((std::is_base_of<dj_color_lab, cie_lab>::value));

    D_CU_CHECK((std::is_same<rgb::model_tag, rgb_tag>::value));
    D_CU_CHECK((std::is_same<cmyk::model_tag, cmyk_tag>::value));
    D_CU_CHECK((std::is_same<hsv::model_tag, hsv_tag>::value));
    D_CU_CHECK((std::is_same<hsl::model_tag, hsl_tag>::value));
    D_CU_CHECK((std::is_same<ycbcr::model_tag, ycbcr_tag>::value));
    D_CU_CHECK((std::is_same<cie_xyz::model_tag, cie_xyz_tag>::value));
    D_CU_CHECK((std::is_same<cie_lab::model_tag, cie_lab_tag>::value));

    D_CU_CHECK((std::is_base_of<dj_color_rgba, rgba>::value));
    D_CU_CHECK(!is_color_model<rgba>::value);

    return true;
}

/*
tests_color_umbrella_gives_the_facade
  Verifies the conversion facade arrives with the umbrella and reaches every
  model, including the pairs outside RGB and CMYK.
  Tests the following:
  - color_cast converts RGB to HSL with the known values for pure red
  - color_convert spelled out agrees with color_cast, and with the C kernel
  - a CMYK ink reaches YCbCr and returns
  - is_convertible_color and its _v form answer for models and refuse rgba
  - from C++20, the two concepts agree
*/
bool tests_color_umbrella_gives_the_facade()
{
    const rgb red(channel_t(1), channel_t(0), channel_t(0));

    const hsl h = color_cast<hsl>(red);

    D_CU_CHECK(cu_near(h.h, channel_t(0)));
    D_CU_CHECK(cu_near(h.s, channel_t(1)));
    D_CU_CHECK(cu_near(h.l, channel_t(0.5)));

    const cie_lab spelled = color_convert<cie_lab, rgb>::apply(red);

    D_CU_CHECK(spelled == color_cast<cie_lab>(red));
    D_CU_CHECK(spelled == cie_lab(d_color_convert_rgb_to_lab(red)));

    const cmyk  ink(channel_t(0), channel_t(1), channel_t(1), channel_t(0));
    const cmyk  back = color_cast<cmyk>(color_cast<ycbcr>(ink));

    D_CU_CHECK(cu_near(back.c, ink.c));
    D_CU_CHECK(cu_near(back.m, ink.m));
    D_CU_CHECK(cu_near(back.y, ink.y));
    D_CU_CHECK(cu_near(back.k, ink.k));

    D_CU_CHECK((is_convertible_color<ycbcr, cie_xyz>::value));
    D_CU_CHECK((is_convertible_color_v<cmyk, hsv>));
    D_CU_CHECK(!(is_convertible_color<rgba, rgb>::value));
    D_CU_CHECK(!(is_convertible_color_v<rgb, rgba>));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CU_CHECK(color_model_type<hsv>);
    D_CU_CHECK(!color_model_type<rgba>);
    D_CU_CHECK((convertible_color_type<hsv, cie_lab>));
#endif

    return true;
}

/*
tests_color_umbrella_gives_the_c_operations
  Verifies the C kernel's operations across models (c/util/color/color.h)
  arrive with the umbrella and take the C++ models directly.
  Tests the following:
  - CIEDE2000 on two cie_lab values gives the first reference pair's 2.0425
    (Sharma, Wu and Dalal, 2005), and zero for a color against itself
  - the RGB convenience agrees with converting to L*a*b* first
  - rotating red's hue by 120 degrees gives green
  - saturation scaled to zero gives grey; lightness scaled to zero, black
*/
bool tests_color_umbrella_gives_the_c_operations()
{
    const cie_lab one(channel_t(50), channel_t(2.6772), channel_t(-79.7751));
    const cie_lab two(channel_t(50), channel_t(0), channel_t(-82.7485));

    D_CU_CHECK(cu_near(d_color_delta_e(one, two), channel_t(2.0425),
                       channel_t(2e-4)));
    D_CU_CHECK(d_color_delta_e(one, one) == 0.0f);

    const rgb red(channel_t(1), channel_t(0), channel_t(0));
    const rgb teal(channel_t(0.2), channel_t(0.6), channel_t(0.6));

    D_CU_CHECK(cu_near(d_color_rgb_delta_e(red, teal),
                       d_color_delta_e(color_cast<cie_lab>(red),
                                       color_cast<cie_lab>(teal))));

    const rgb green = d_color_rgb_rotate_hue(red, 120.0f);

    D_CU_CHECK(cu_near(green.r, channel_t(0)));
    D_CU_CHECK(cu_near(green.g, channel_t(1)));
    D_CU_CHECK(cu_near(green.b, channel_t(0)));

    const rgb grey  = d_color_rgb_adjust_saturation(red, 0.0f);
    const rgb black = d_color_rgb_adjust_brightness(red, 0.0f);

    D_CU_CHECK(cu_near(grey.r, channel_t(0.5)));
    D_CU_CHECK(cu_near(grey.g, channel_t(0.5)));
    D_CU_CHECK(cu_near(grey.b, channel_t(0.5)));
    D_CU_CHECK(black == rgb(channel_t(0), channel_t(0), channel_t(0)));

    return true;
}

/*
tests_color_umbrella_exposes_the_foundation
  Verifies the foundation every model shares is in reach through the
  umbrella.
  Tests the following:
  - channel_t is the C kernel's float
  - each of the seven tags derives from DColorModelTag
  - is_color_model strips cv and reference, and has its _v form
  - clamp_channel and approx_equal work on channels
  - the flat layouts, and rgba's bridge to rgb
*/
bool tests_color_umbrella_exposes_the_foundation()
{
    D_CU_CHECK((std::is_same<channel_t, float>::value));

    D_CU_CHECK((std::is_base_of<DColorModelTag, rgb_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, cmyk_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, hsv_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, hsl_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, ycbcr_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, cie_xyz_tag>::value));
    D_CU_CHECK((std::is_base_of<DColorModelTag, cie_lab_tag>::value));

    D_CU_CHECK(is_color_model<const hsl&>::value);
    D_CU_CHECK(is_color_model_v<cie_lab>);
    D_CU_CHECK(!is_color_model_v<rgba>);

    D_CU_CHECK(clamp_channel(channel_t(1.5), channel_t(0), channel_t(1))
               == channel_t(1));
    D_CU_CHECK(approx_equal(channel_t(0.5), channel_t(0.50001),
                            channel_t(1e-4)));

    D_CU_CHECK(sizeof(rgb) == 3u * sizeof(channel_t));
    D_CU_CHECK(sizeof(rgba) == 4u * sizeof(channel_t));
    D_CU_CHECK(sizeof(cmyk) == 4u * sizeof(channel_t));

    const rgba transport(channel_t(1), channel_t(0), channel_t(0),
                         channel_t(0.5));

    D_CU_CHECK(transport.to_rgb() == rgb(channel_t(1), channel_t(0),
                                         channel_t(0)));

    return true;
}

NS_END   // testing
NS_END   // djinterp
