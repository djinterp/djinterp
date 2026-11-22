/*******************************************************************************
* djinterp [cpp][utility]                                              color.hpp
*
*   The color module's umbrella: one include for all of it.  It brings every
* model (rgb and rgba, cmyk, hsv, hsl, ycbcr, cie_xyz and cie_lab), the
* conversion facade (color_cast, color_convert, is_convertible_color and,
* from C++20, the concepts), and the C kernel's operations across models
* (c/util/color/color.h: saturation, brightness and hue through HSL, and the
* CIEDE2000 difference).
*   It defines nothing of its own, so it can be included beside any of the
* headers it gathers, in either order.  A unit that needs only one model can
* include that model's header instead.
*
*
* path:      /inc/djinterp/cpp/util/color/color.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.11.17
*******************************************************************************/

#ifndef DJINTERP_UTIL_COLOR_HPP
#define DJINTERP_UTIL_COLOR_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "./color_common.hpp"   // channel_t, the model tags, is_color_model
#include "./color_rgb.hpp"      // rgb, rgba, lerp
#include "./color_cmyk.hpp"     // cmyk
#include "./color_hsv.hpp"      // hsv
#include "./color_hsl.hpp"      // hsl
#include "./color_ycbcr.hpp"    // ycbcr
#include "./color_cie_lab.hpp"  // cie_xyz, cie_lab
#include "./color_convert.hpp"  // color_cast, color_convert, the traits
#include "../../../c/util/color/color.h"  // the operations across models


#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_HPP
