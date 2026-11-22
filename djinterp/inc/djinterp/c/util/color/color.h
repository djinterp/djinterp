/*******************************************************************************
* djinterp [c][utility]                                                  color.h
*
*   Umbrella header for the djinterp color module (C). Including this single
* header pulls in every color model, the shared conversion kernel, and the
* cross-model operations defined below. C translation units should include
* this; C++ translation units should include color.hpp, which includes this
* and adds the C++ models and the conversion facade.
*
*   The operations here are the ones that span more than one color space:
* HSL-mediated saturation/brightness/hue manipulation of RGB, and the
* CIEDE2000 perceptual difference metric (which lives in L*a*b*). They are
* kept out of color_convert.h so that header stays a pure conversion kernel.
*
*
* path:      /inc/djinterp/c/util/color/color.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTANTS
      ---------
      a. D_COLOR_PI

II.   RGB MANIPULATION (HSL-mediated)
      -------------------------------
      a. d_color_rgb_adjust_saturation
      b. d_color_rgb_adjust_brightness
      c.    d_color_rgb_rotate_hue

III.  PERCEPTUAL DIFFERENCE
      ---------------------
      a. d_internal_color_de_hue_delta
      b. d_internal_color_de_hue_mean
      c. d_internal_color_de_hue_weight
      d. d_internal_color_de_rotation
      e. d_color_delta_e        (CIEDE2000, on L*a*b*)
      f. d_color_rgb_delta_e    (convenience, via L*a*b*)
*/

#ifndef DJINTERP_C_UTIL_COLOR_H
#define DJINTERP_C_UTIL_COLOR_H 1

// FLOOR, FOR NOW: its C++ face is empty below C++11, rather than an
// error (README rule 5). The owner's ruling: compile at every level first;
// port to C++98 only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if ( (!D_ENV_LANG_USING_CPP) ||                                         \
      (D_ENV_LANG_IS_CPP11_OR_HIGHER) )

// std
#include <math.h>              // sqrtf, atan2f, sinf, cosf, expf
// djinterp
#include "../../djinterp.h"    // framework root
#include "./color_common.h"    // D_COLOR_FN, D_COLOR_EPSILON, d_color_fabsf
#include "./color_rgb.h"       // struct dj_color_rgb
#include "./color_cmyk.h"      // struct dj_color_cmyk
#include "./color_hsv.h"       // struct dj_color_hsv
#include "./color_hsl.h"       // struct dj_color_hsl
#include "./color_lab.h"       // struct dj_color_lab
#include "./color_ycbcr.h"     // struct dj_color_ycbcr
#include "./color_convert.h"   // d_color_convert_*


D_COLOR_NS_OPEN

///////////////////////////////////////////////////////////////////////////////
///                        I.    CONSTANTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_COLOR_PI
//   constant: pi, as used by the CIEDE2000 trigonometry.
#define D_COLOR_PI 3.14159265f


///////////////////////////////////////////////////////////////////////////////
///              II.    RGB MANIPULATION (HSL-mediated)                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_adjust_saturation
  Scales an RGB color's saturation in HSL space.

Parameter(s):
  _rgb:    source color.
  _amount: saturation multiplier (1.0 = unchanged, 0.0 = grayscale,
           > 1.0 = more saturated).
Return:
  A dj_color_rgb with adjusted saturation.
*/
D_COLOR_FN
struct dj_color_rgb d_color_rgb_adjust_saturation(
    struct dj_color_rgb _rgb,
    float               _amount
)
{
    struct dj_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

    hsl.s = d_color_clamp_01(hsl.s * _amount);

    return d_color_convert_hsl_to_rgb(hsl);
}

/*
d_color_rgb_adjust_brightness
  Scales an RGB color's lightness in HSL space.

Parameter(s):
  _rgb:    source color.
  _amount: brightness multiplier (1.0 = unchanged, 0.0 = black,
           > 1.0 = brighter).
Return:
  A dj_color_rgb with adjusted brightness.
*/
D_COLOR_FN
struct dj_color_rgb d_color_rgb_adjust_brightness(
    struct dj_color_rgb _rgb,
    float               _amount
)
{
    struct dj_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

    hsl.l = d_color_clamp_01(hsl.l * _amount);

    return d_color_convert_hsl_to_rgb(hsl);
}

/*
d_color_rgb_rotate_hue
  Rotates an RGB color's hue by a signed number of degrees in HSL space.

Parameter(s):
  _rgb:     source color.
  _degrees: hue rotation in degrees (positive or negative).
Return:
  A dj_color_rgb with rotated hue.
*/
D_COLOR_FN
struct dj_color_rgb d_color_rgb_rotate_hue(
    struct dj_color_rgb _rgb,
    float               _degrees
)
{
    struct dj_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

    hsl.h = d_color_fmodf(hsl.h + _degrees, 360.0f);

    if (hsl.h < 0.0f)
    {
        hsl.h += 360.0f;
    }

    return d_color_convert_hsl_to_rgb(hsl);
}


///////////////////////////////////////////////////////////////////////////////
///                 III.  PERCEPTUAL DIFFERENCE                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_color_de_hue_delta
  CIEDE2000's hue-angle difference, h2' - h1' taken the short way round the
  circle, in degrees. 0 when either color is achromatic (C1'C2' = 0), whose
  hue is undefined.

Parameter(s):
  _h1:   the first color's hue angle h1', in [0, 360).
  _h2:   the second color's hue angle h2', in [0, 360).
  _c1c2: the product of the two adjusted chromas, C1'C2'.
Return:
  The difference, in [-180, 180].
*/
D_COLOR_FN
float d_internal_color_de_hue_delta(
    float _h1,
    float _h2,
    float _c1c2
)
{
    if (_c1c2 < D_COLOR_EPSILON)
    {
        return 0.0f;
    }

    if (d_color_fabsf(_h2 - _h1) <= 180.0f)
    {
        return _h2 - _h1;
    }

    return (_h2 > _h1) ? (_h2 - _h1 - 360.0f)
                       : (_h2 - _h1 + 360.0f);
}

/*
d_internal_color_de_hue_mean
  CIEDE2000's mean hue angle, taken on the short arc between the two. When
  either color is achromatic it is the sum of the two angles, which is the
  other color's hue (or 0 when both are achromatic), as the formula
  specifies.

Parameter(s):
  _h1:   the first color's hue angle h1', in [0, 360).
  _h2:   the second color's hue angle h2', in [0, 360).
  _c1c2: the product of the two adjusted chromas, C1'C2'.
Return:
  The mean hue angle, in degrees.
*/
D_COLOR_FN
float d_internal_color_de_hue_mean(
    float _h1,
    float _h2,
    float _c1c2
)
{
    if (_c1c2 < D_COLOR_EPSILON)
    {
        return _h1 + _h2;
    }

    if (d_color_fabsf(_h1 - _h2) <= 180.0f)
    {
        return (_h1 + _h2) * 0.5f;
    }

    return (_h1 + _h2 < 360.0f) ? ((_h1 + _h2 + 360.0f) * 0.5f)
                                : ((_h1 + _h2 - 360.0f) * 0.5f);
}

/*
d_internal_color_de_hue_weight
  CIEDE2000's T, the periodic term in the hue weighting S_H. Runtime-only.

Parameter(s):
  _h_mean: the mean hue angle, in degrees.
Return:
  T.
*/
D_COLOR_FN_RT
float d_internal_color_de_hue_weight(
    float _h_mean
)
{
    return 1.0f
         - 0.17f * cosf((_h_mean - 30.0f)        * D_COLOR_PI / 180.0f)
         + 0.24f * cosf((2.0f * _h_mean)         * D_COLOR_PI / 180.0f)
         + 0.32f * cosf((3.0f * _h_mean + 6.0f)  * D_COLOR_PI / 180.0f)
         - 0.20f * cosf((4.0f * _h_mean - 63.0f) * D_COLOR_PI / 180.0f);
}

/*
d_internal_color_de_rotation
  CIEDE2000's rotation term R_T = -sin(2 * dtheta) * R_C, which corrects
  the interaction of chroma and hue differences in the blue region.
  R_C = 2 * sqrt(C'^7 / (C'^7 + 25^7)) is computed from the mean of the
  ADJUSTED chromas, C1' and C2'; the mean of the unadjusted chromas belongs
  only to G. Runtime-only.

Parameter(s):
  _h_mean:       the mean hue angle, in degrees.
  _c_prime_mean: the mean adjusted chroma, (C1' + C2') / 2.
Return:
  R_T.
*/
D_COLOR_FN_RT
float d_internal_color_de_rotation(
    float _h_mean,
    float _c_prime_mean
)
{
    float c7     = _c_prime_mean * _c_prime_mean * _c_prime_mean *
                   _c_prime_mean * _c_prime_mean * _c_prime_mean *
                   _c_prime_mean;
    float spread = (_h_mean - 275.0f) / 25.0f;

    return -2.0f * sqrtf(c7 / (c7 + 6103515625.0f)) *
           sinf(60.0f * expf(-spread * spread) * D_COLOR_PI / 180.0f);
}

/*
d_color_delta_e
  CIEDE2000 perceptual color difference between two L*a*b* colors, with the
  parametric factors k_L = k_C = k_H = 1. Symmetric in its arguments. The
  implementation follows Sharma, Wu and Dalal, "The CIEDE2000
  Color-Difference Formula: Implementation Notes, Supplementary Test Data,
  and Mathematical Observations", Color Research and Application 30(1),
  2005, and reproduces that paper's 34 test pairs to four decimal places.
  Runtime-only (uses sqrt, atan2, sin, cos and exp).

Parameter(s):
  _lab1: first color.
  _lab2: second color.
Return:
  The Delta E 2000 value (0 = identical; larger = more different).
*/
D_COLOR_FN_RT
float d_color_delta_e(
    struct dj_color_lab _lab1,
    struct dj_color_lab _lab2
)
{
    float c1      = sqrtf(_lab1.a * _lab1.a + _lab1.b * _lab1.b);
    float c2      = sqrtf(_lab2.a * _lab2.a + _lab2.b * _lab2.b);
    float c_mean  = (c1 + c2) * 0.5f;
    float c_mean7 = c_mean * c_mean * c_mean * c_mean *
                    c_mean * c_mean * c_mean;
    float g       = 0.5f * (1.0f - sqrtf(c_mean7 /
                                         (c_mean7 + 6103515625.0f)));
    float a1_prime = _lab1.a * (1.0f + g);
    float a2_prime = _lab2.a * (1.0f + g);
    float c1_prime     = sqrtf(a1_prime * a1_prime + _lab1.b * _lab1.b);
    float c2_prime     = sqrtf(a2_prime * a2_prime + _lab2.b * _lab2.b);
    float c_prime_mean = (c1_prime + c2_prime) * 0.5f;
    float h1_prime = atan2f(_lab1.b, a1_prime) * 180.0f / D_COLOR_PI;
    float h2_prime = atan2f(_lab2.b, a2_prime) * 180.0f / D_COLOR_PI;
    float h_mean   = 0.0f;
    float l_dev    = 0.0f;
    float d_l      = 0.0f;
    float d_c      = 0.0f;
    float d_h      = 0.0f;

    if (h1_prime < 0.0f)
    {
        h1_prime += 360.0f;
    }

    if (h2_prime < 0.0f)
    {
        h2_prime += 360.0f;
    }

    h_mean = d_internal_color_de_hue_mean(h1_prime,
                                          h2_prime,
                                          c1_prime * c2_prime);
    l_dev  = (_lab1.l + _lab2.l) * 0.5f - 50.0f;

    // each difference over its weighting function S_L, S_C or S_H
    d_l = (_lab2.l - _lab1.l) /
          (1.0f + (0.015f * l_dev * l_dev) / sqrtf(20.0f + l_dev * l_dev));
    d_c = (c2_prime - c1_prime) / (1.0f + 0.045f * c_prime_mean);
    d_h = 2.0f * sqrtf(c1_prime * c2_prime) *
          sinf(d_internal_color_de_hue_delta(h1_prime,
                                             h2_prime,
                                             c1_prime * c2_prime) *
               D_COLOR_PI / 360.0f) /
          (1.0f + 0.015f * c_prime_mean *
                  d_internal_color_de_hue_weight(h_mean));

    return sqrtf(d_l * d_l + d_c * d_c + d_h * d_h +
                 d_internal_color_de_rotation(h_mean, c_prime_mean) *
                     d_c * d_h);
}

/*
d_color_rgb_delta_e
  CIEDE2000 difference between two linear RGB colors (converts to L*a*b*
  first). Runtime-only.

Parameter(s):
  _rgb1: first color.
  _rgb2: second color.
Return:
  The Delta E 2000 value.
*/
D_COLOR_FN_RT
float d_color_rgb_delta_e(
    struct dj_color_rgb _rgb1,
    struct dj_color_rgb _rgb2
)
{
    return d_color_delta_e(
        d_color_convert_rgb_to_lab(_rgb1),
        d_color_convert_rgb_to_lab(_rgb2)
    );
}


D_COLOR_NS_CLOSE

#endif  // floor, for now


#endif  // DJINTERP_C_UTIL_COLOR_H
