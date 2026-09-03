/******************************************************************************
* djinterp [utility]                                                    color.h
*
*   Umbrella header for the djinterp color module (C). Including this single
* header pulls in every color model, the shared conversion kernel, and the
* cross-model operations defined below. C translation units should include
* this; C++ translation units should include color.hpp (which includes this
* and adds the ergonomic and polymorphic layers).
*
*   The operations here are the ones that span more than one color space:
* HSL-mediated saturation/brightness/hue manipulation of RGB, and the
* CIEDE2000 perceptual difference metric (which lives in L*a*b*). They are
* kept out of color_convert.h so that header stays a pure conversion kernel.
*
*
* path:      /inc/djinterp/c/util/color/color.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTANTS
      ---------
      a. D_COLOR_PI
II.   RGB MANIPULATION (HSL-mediated)
      ------------------------------
      a. d_color_rgb_adjust_saturation
      b. d_color_rgb_adjust_brightness
      c. d_color_rgb_rotate_hue
III.  PERCEPTUAL DIFFERENCE
      --------------------
      a. d_color_delta_e        (CIEDE2000, on L*a*b*)
      b. d_color_rgb_delta_e    (convenience, via L*a*b*)
*/

#ifndef  DJINTERP_C_COLOR_
#define  DJINTERP_C_COLOR_ 1

// std
#include <math.h>
// djinterp
#include "../../djinterp.h"
#include "./color_common.h"
#include "./color_rgb.h"
#include "./color_cmyk.h"
#include "./color_hsv.h"
#include "./color_hsl.h"
#include "./color_lab.h"
#include "./color_ycbcr.h"
#include "./color_convert.h"


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
  A d_color_rgb with adjusted saturation.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_adjust_saturation(
    struct d_color_rgb _rgb,
    float       _amount
)
{
    struct d_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

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
  A d_color_rgb with adjusted brightness.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_adjust_brightness(
    struct d_color_rgb _rgb,
    float       _amount
)
{
    struct d_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

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
  A d_color_rgb with rotated hue.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_rotate_hue(
    struct d_color_rgb _rgb,
    float       _degrees
)
{
    struct d_color_hsl hsl = d_color_convert_rgb_to_hsl(_rgb);

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
d_color_delta_e
  CIEDE2000 perceptual color difference between two L*a*b* colors. Runtime-
  only (uses sqrt/atan2/sin/cos/exp).

Parameter(s):
  _lab1: first color.
  _lab2: second color.
Return:
  The Delta E 2000 value (0 = identical; larger = more different).
*/
D_COLOR_FN_RT float
d_color_delta_e(
    struct d_color_lab _lab1,
    struct d_color_lab _lab2
)
{
    float delta_l = _lab2.l - _lab1.l;
    float l_mean  = (_lab1.l + _lab2.l) * 0.5f;

    float c1     = sqrtf(_lab1.a * _lab1.a + _lab1.b * _lab1.b);
    float c2     = sqrtf(_lab2.a * _lab2.a + _lab2.b * _lab2.b);
    float c_mean = (c1 + c2) * 0.5f;

    float c_mean7 = c_mean * c_mean * c_mean * c_mean *
                    c_mean * c_mean * c_mean;
    float g       = 0.5f * (1.0f - sqrtf(c_mean7 / (c_mean7 + 6103515625.0f)));

    float a1_prime = _lab1.a * (1.0f + g);
    float a2_prime = _lab2.a * (1.0f + g);

    float c1_prime      = sqrtf(a1_prime * a1_prime + _lab1.b * _lab1.b);
    float c2_prime      = sqrtf(a2_prime * a2_prime + _lab2.b * _lab2.b);
    float c_prime_mean  = (c1_prime + c2_prime) * 0.5f;
    float delta_c_prime = c2_prime - c1_prime;

    float h1_prime = atan2f(_lab1.b, a1_prime) * 180.0f / D_COLOR_PI;
    float h2_prime = atan2f(_lab2.b, a2_prime) * 180.0f / D_COLOR_PI;

    float delta_h_prime     = 0.0f;
    float delta_big_h_prime = 0.0f;
    float h_prime_mean      = 0.0f;
    float t                 = 0.0f;
    float s_l               = 0.0f;
    float s_c               = 0.0f;
    float s_h               = 0.0f;
    float r_t               = 0.0f;
    float k_l               = 1.0f;
    float k_c               = 1.0f;
    float k_h               = 1.0f;

    if (h1_prime < 0.0f)
    {
        h1_prime += 360.0f;
    }
    if (h2_prime < 0.0f)
    {
        h2_prime += 360.0f;
    }

    if (c1_prime * c2_prime < D_COLOR_EPSILON)
    {
        delta_h_prime = 0.0f;
    }
    else if (d_color_fabsf(h2_prime - h1_prime) <= 180.0f)
    {
        delta_h_prime = h2_prime - h1_prime;
    }
    else if (h2_prime > h1_prime)
    {
        delta_h_prime = h2_prime - h1_prime - 360.0f;
    }
    else
    {
        delta_h_prime = h2_prime - h1_prime + 360.0f;
    }

    delta_big_h_prime = 2.0f * sqrtf(c1_prime * c2_prime) *
                        sinf(delta_h_prime * D_COLOR_PI / 360.0f);

    if (c1_prime * c2_prime < D_COLOR_EPSILON)
    {
        h_prime_mean = h1_prime + h2_prime;
    }
    else if (d_color_fabsf(h2_prime - h1_prime) <= 180.0f)
    {
        h_prime_mean = (h1_prime + h2_prime) * 0.5f;
    }
    else if (h1_prime + h2_prime < 360.0f)
    {
        h_prime_mean = (h1_prime + h2_prime + 360.0f) * 0.5f;
    }
    else
    {
        h_prime_mean = (h1_prime + h2_prime - 360.0f) * 0.5f;
    }

    t = 1.0f
      - 0.17f * cosf((h_prime_mean - 30.0f)        * D_COLOR_PI / 180.0f)
      + 0.24f * cosf((2.0f * h_prime_mean)         * D_COLOR_PI / 180.0f)
      + 0.32f * cosf((3.0f * h_prime_mean + 6.0f)  * D_COLOR_PI / 180.0f)
      - 0.20f * cosf((4.0f * h_prime_mean - 63.0f) * D_COLOR_PI / 180.0f);

    s_l = 1.0f + (0.015f * (l_mean - 50.0f) * (l_mean - 50.0f)) /
                 sqrtf(20.0f + (l_mean - 50.0f) * (l_mean - 50.0f));
    s_c = 1.0f + 0.045f * c_prime_mean;
    s_h = 1.0f + 0.015f * c_prime_mean * t;

    r_t = -2.0f * sqrtf(c_mean7 / (c_mean7 + 6103515625.0f)) *
          sinf(60.0f * expf(-((h_prime_mean - 275.0f) / 25.0f) *
                             ((h_prime_mean - 275.0f) / 25.0f)) *
               D_COLOR_PI / 180.0f);

    return sqrtf(
        (delta_l / (k_l * s_l)) * (delta_l / (k_l * s_l)) +
        (delta_c_prime / (k_c * s_c)) * (delta_c_prime / (k_c * s_c)) +
        (delta_big_h_prime / (k_h * s_h)) * (delta_big_h_prime / (k_h * s_h)) +
        r_t * (delta_c_prime / (k_c * s_c)) *
              (delta_big_h_prime / (k_h * s_h))
    );
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
D_COLOR_FN_RT float
d_color_rgb_delta_e(
    struct d_color_rgb _rgb1,
    struct d_color_rgb _rgb2
)
{
    return d_color_delta_e(
        d_color_convert_rgb_to_lab(_rgb1),
        d_color_convert_rgb_to_lab(_rgb2)
    );
}


D_COLOR_NS_CLOSE


#endif  // DJINTERP_C_COLOR_