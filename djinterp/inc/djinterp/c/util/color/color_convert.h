/******************************************************************************
* djinterp [utility]                                            color_convert.h
*
*   Cross-model conversion shared kernel for the djinterp color module.
* Every color-space conversion is implemented exactly once here, operating
* on PODs by value. RGB is the hub: HSL, HSV, CMYK, YCbCr, and XYZ convert
* directly to/from linear RGB, while HSL<->HSV route through RGB and
* L*a*b* routes through XYZ. The C++ template dispatch layer
* (color_convert.hpp) is a thin facade over these functions.
*
*   Pure-arithmetic conversions are constexpr (D_COLOR_FN). Conversions that
* touch L*a*b* require the cube-root/power helpers and are therefore
* runtime-only (D_COLOR_FN_RT). YCbCr uses full-range ITU-R BT.601.
*
*
* path:      /inc/djinterp/c/util/color/color_convert.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HSL HELPER
      ----------
      a. d_color_hsl_hue_to_rgb
II.   RGB <-> HSL
      ----------
      a. d_color_convert_rgb_to_hsl
      b. d_color_convert_hsl_to_rgb
III.  RGB <-> HSV
      ----------
      a. d_color_convert_rgb_to_hsv
      b. d_color_convert_hsv_to_rgb
IV.   HSL <-> HSV   (via RGB)
      ----------
      a. d_color_convert_hsl_to_hsv
      b. d_color_convert_hsv_to_hsl
V.    RGB <-> CMYK
      -----------
      a. d_color_convert_rgb_to_cmyk
      b. d_color_convert_cmyk_to_rgb
VI.   RGB <-> YCBCR (BT.601)
      ------------
      a. d_color_convert_rgb_to_ycbcr
      b. d_color_convert_ycbcr_to_rgb
VII.  RGB <-> XYZ
      ----------
      a. d_color_convert_rgb_to_xyz
      b. d_color_convert_xyz_to_rgb
VIII. XYZ <-> LAB
      ----------
      a. d_color_convert_xyz_to_lab
      b. d_color_convert_lab_to_xyz
IX.   RGB <-> LAB   (via XYZ)
      ----------
      a. d_color_convert_rgb_to_lab
      b. d_color_convert_lab_to_rgb
*/

#ifndef  DJINTERP_C_COLOR_CONVERT_
#define  DJINTERP_C_COLOR_CONVERT_ 1

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


D_COLOR_NS_OPEN

///////////////////////////////////////////////////////////////////////////////
///                        I.   HSL HELPER                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_hsl_hue_to_rgb
  HSL hue-sector helper mapping (p, q, t) to a single RGB component.

Parameter(s):
  _p: lower bound component.
  _q: upper bound component.
  _t: hue position in [0, 1] (wrapped internally).
Return:
  The interpolated component value.
*/
D_COLOR_FN float
d_color_hsl_hue_to_rgb(
    float _p,
    float _q,
    float _t
)
{
    if (_t < 0.0f)
    {
        _t += 1.0f;
    }
    if (_t > 1.0f)
    {
        _t -= 1.0f;
    }

    if (_t < 1.0f / 6.0f)
    {
        return _p + (_q - _p) * 6.0f * _t;
    }
    if (_t < 0.5f)
    {
        return _q;
    }
    if (_t < 2.0f / 3.0f)
    {
        return _p + (_q - _p) * (2.0f / 3.0f - _t) * 6.0f;
    }

    return _p;
}


///////////////////////////////////////////////////////////////////////////////
///                       II.   RGB <-> HSL                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_hsl
  Converts linear RGB to HSL.

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_hsl.
*/
D_COLOR_FN struct d_color_hsl
d_color_convert_rgb_to_hsl(
    struct d_color_rgb _rgb
)
{
    float max   = d_color_max3(_rgb.r, _rgb.g, _rgb.b);
    float min   = d_color_min3(_rgb.r, _rgb.g, _rgb.b);
    float delta = max - min;
    float l      = (max + min) * 0.5f;
    float s      = 0.0f;
    float h      = 0.0f;

    if (delta >= D_COLOR_EPSILON)
    {
        if (l < 0.5f)
        {
            s = delta / (max + min);
        }
        else
        {
            s = delta / (2.0f - max - min);
        }

        if (d_color_fabsf(_rgb.r - max) < D_COLOR_EPSILON)
        {
            h = 60.0f * d_color_fmodf((_rgb.g - _rgb.b) / delta, 6.0f);
        }
        else if (d_color_fabsf(_rgb.g - max) < D_COLOR_EPSILON)
        {
            h = 60.0f * ( ((_rgb.b - _rgb.r) / delta) + 2.0f );
        }
        else
        {
            h = 60.0f * ( ((_rgb.r - _rgb.g) / delta) + 4.0f );
        }

        if (h < 0.0f)
        {
            h += 360.0f;
        }
    }

    return d_color_hsl_make(h, s, l);
}

/*
d_color_convert_hsl_to_rgb
  Converts HSL to linear RGB.

Parameter(s):
  _hsl: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_convert_hsl_to_rgb(
    struct d_color_hsl _hsl
)
{
    float q      = 0.0f;
    float p      = 0.0f;
    float h_norm = 0.0f;

    if (_hsl.s < D_COLOR_EPSILON)
    {
        return d_color_rgb_make(_hsl.l, _hsl.l, _hsl.l);
    }

    q = (_hsl.l < 0.5f)
      ? _hsl.l * (1.0f + _hsl.s)
      : _hsl.l + _hsl.s - _hsl.l * _hsl.s;
    p = 2.0f * _hsl.l - q;
    h_norm = _hsl.h / 360.0f;

    return d_color_rgb_make(
        d_color_hsl_hue_to_rgb(p, q, h_norm + 1.0f / 3.0f),
        d_color_hsl_hue_to_rgb(p, q, h_norm),
        d_color_hsl_hue_to_rgb(p, q, h_norm - 1.0f / 3.0f)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                       III.   RGB <-> HSV                                ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_hsv
  Converts linear RGB to HSV.

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_hsv.
*/
D_COLOR_FN struct d_color_hsv
d_color_convert_rgb_to_hsv(
    struct d_color_rgb _rgb
)
{
    float max   = d_color_max3(_rgb.r, _rgb.g, _rgb.b);
    float min   = d_color_min3(_rgb.r, _rgb.g, _rgb.b);
    float delta = max - min;
    float v      = max;
    float s      = 0.0f;
    float h      = 0.0f;

    if (delta >= D_COLOR_EPSILON)
    {
        s = delta / max;

        if (d_color_fabsf(_rgb.r - max) < D_COLOR_EPSILON)
        {
            h = 60.0f * d_color_fmodf((_rgb.g - _rgb.b) / delta, 6.0f);
        }
        else if (d_color_fabsf(_rgb.g - max) < D_COLOR_EPSILON)
        {
            h = 60.0f * ( ((_rgb.b - _rgb.r) / delta) + 2.0f );
        }
        else
        {
            h = 60.0f * ( ((_rgb.r - _rgb.g) / delta) + 4.0f );
        }

        if (h < 0.0f)
        {
            h += 360.0f;
        }
    }

    return d_color_hsv_make(h, s, v);
}

/*
d_color_convert_hsv_to_rgb
  Converts HSV to linear RGB.

Parameter(s):
  _hsv: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_convert_hsv_to_rgb(
    struct d_color_hsv _hsv
)
{
    float h      = 0.0f;
    int   sector = 0;
    float f      = 0.0f;
    float p      = 0.0f;
    float q      = 0.0f;
    float t      = 0.0f;

    if (_hsv.s < D_COLOR_EPSILON)
    {
        return d_color_rgb_make(_hsv.v, _hsv.v, _hsv.v);
    }

    h      = d_color_fmodf(_hsv.h, 360.0f) / 60.0f;
    sector = (int)h;
    f      = h - (float)sector;

    p = _hsv.v * (1.0f - _hsv.s);
    q = _hsv.v * (1.0f - _hsv.s * f);
    t = _hsv.v * (1.0f - _hsv.s * (1.0f - f));

    switch (sector)
    {
        case 0:  return d_color_rgb_make(_hsv.v, t,      p     );
        case 1:  return d_color_rgb_make(q,      _hsv.v, p     );
        case 2:  return d_color_rgb_make(p,      _hsv.v, t     );
        case 3:  return d_color_rgb_make(p,      q,      _hsv.v);
        case 4:  return d_color_rgb_make(t,      p,      _hsv.v);
        default: return d_color_rgb_make(_hsv.v, p,      q     );
    }
}


///////////////////////////////////////////////////////////////////////////////
///                  IV.   HSL <-> HSV   (via RGB)                          ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_hsl_to_hsv
  Converts HSL to HSV via linear RGB.

Parameter(s):
  _hsl: source color.
Return:
  The equivalent d_color_hsv.
*/
D_COLOR_FN struct d_color_hsv
d_color_convert_hsl_to_hsv(
    struct d_color_hsl _hsl
)
{
    return d_color_convert_rgb_to_hsv(d_color_convert_hsl_to_rgb(_hsl));
}

/*
d_color_convert_hsv_to_hsl
  Converts HSV to HSL via linear RGB.

Parameter(s):
  _hsv: source color.
Return:
  The equivalent d_color_hsl.
*/
D_COLOR_FN struct d_color_hsl
d_color_convert_hsv_to_hsl(
    struct d_color_hsv _hsv
)
{
    return d_color_convert_rgb_to_hsl(d_color_convert_hsv_to_rgb(_hsv));
}


///////////////////////////////////////////////////////////////////////////////
///                      V.   RGB <-> CMYK                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_cmyk
  Converts linear RGB to CMYK (black extracted first).

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_cmyk.
*/
D_COLOR_FN struct d_color_cmyk
d_color_convert_rgb_to_cmyk(
    struct d_color_rgb _rgb
)
{
    float k     = 1.0f - d_color_max3(_rgb.r, _rgb.g, _rgb.b);
    float k_inv = 0.0f;

    if (k >= 1.0f - D_COLOR_EPSILON)
    {
        return d_color_cmyk_make(0.0f, 0.0f, 0.0f, 1.0f);
    }

    k_inv = 1.0f / (1.0f - k);

    return d_color_cmyk_make(
        (1.0f - _rgb.r - k) * k_inv,
        (1.0f - _rgb.g - k) * k_inv,
        (1.0f - _rgb.b - k) * k_inv,
        k
    );
}

/*
d_color_convert_cmyk_to_rgb
  Converts CMYK to linear RGB.

Parameter(s):
  _cmyk: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_convert_cmyk_to_rgb(
    struct d_color_cmyk _cmyk
)
{
    float k_comp = 1.0f - _cmyk.k;

    return d_color_rgb_make(
        (1.0f - _cmyk.c) * k_comp,
        (1.0f - _cmyk.m) * k_comp,
        (1.0f - _cmyk.y) * k_comp
    );
}


///////////////////////////////////////////////////////////////////////////////
///                 VI.   RGB <-> YCBCR (BT.601)                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_ycbcr
  Converts linear RGB to full-range ITU-R BT.601 YCbCr.

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_ycbcr.
*/
D_COLOR_FN struct d_color_ycbcr
d_color_convert_rgb_to_ycbcr(
    struct d_color_rgb _rgb
)
{
    return d_color_ycbcr_make(
         0.299f    * _rgb.r + 0.587f    * _rgb.g + 0.114f    * _rgb.b,
        -0.168736f * _rgb.r - 0.331264f * _rgb.g + 0.5f      * _rgb.b,
         0.5f      * _rgb.r - 0.418688f * _rgb.g - 0.081312f * _rgb.b
    );
}

/*
d_color_convert_ycbcr_to_rgb
  Converts full-range ITU-R BT.601 YCbCr to linear RGB.

Parameter(s):
  _ycbcr: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_convert_ycbcr_to_rgb(
    struct d_color_ycbcr _ycbcr
)
{
    return d_color_rgb_make(
        _ycbcr.y + 1.402f    * _ycbcr.cr,
        _ycbcr.y - 0.344136f * _ycbcr.cb - 0.714136f * _ycbcr.cr,
        _ycbcr.y + 1.772f    * _ycbcr.cb
    );
}


///////////////////////////////////////////////////////////////////////////////
///                      VII.   RGB <-> XYZ                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_xyz
  Converts linear RGB to CIE XYZ (sRGB primaries, D65).

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_xyz.
*/
D_COLOR_FN struct d_color_xyz
d_color_convert_rgb_to_xyz(
    struct d_color_rgb _rgb
)
{
    return d_color_xyz_make(
        0.4124564f * _rgb.r + 0.3575761f * _rgb.g + 0.1804375f * _rgb.b,
        0.2126729f * _rgb.r + 0.7151522f * _rgb.g + 0.0721750f * _rgb.b,
        0.0193339f * _rgb.r + 0.1191920f * _rgb.g + 0.9503041f * _rgb.b
    );
}

/*
d_color_convert_xyz_to_rgb
  Converts CIE XYZ to linear RGB (sRGB primaries, D65).

Parameter(s):
  _xyz: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_convert_xyz_to_rgb(
    struct d_color_xyz _xyz
)
{
    return d_color_rgb_make(
         3.2404542f * _xyz.x - 1.5371385f * _xyz.y - 0.4985314f * _xyz.z,
        -0.9692660f * _xyz.x + 1.8760108f * _xyz.y + 0.0415560f * _xyz.z,
         0.0556434f * _xyz.x - 0.2040259f * _xyz.y + 1.0572252f * _xyz.z
    );
}


///////////////////////////////////////////////////////////////////////////////
///                      VIII.   XYZ <-> LAB                                ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_xyz_to_lab
  Converts CIE XYZ to CIE L*a*b* (D65). Runtime-only (cube root).

Parameter(s):
  _xyz: source color.
Return:
  The equivalent d_color_lab.
*/
D_COLOR_FN_RT struct d_color_lab
d_color_convert_xyz_to_lab(
    struct d_color_xyz _xyz
)
{
    float fx = d_color_lab_f(_xyz.x / D_COLOR_XYZ_WHITE_X);
    float fy = d_color_lab_f(_xyz.y / D_COLOR_XYZ_WHITE_Y);
    float fz = d_color_lab_f(_xyz.z / D_COLOR_XYZ_WHITE_Z);

    return d_color_lab_make(
        116.0f * fy - 16.0f,
        500.0f * (fx - fy),
        200.0f * (fy - fz)
    );
}

/*
d_color_convert_lab_to_xyz
  Converts CIE L*a*b* to CIE XYZ (D65). Runtime-only.

Parameter(s):
  _lab: source color.
Return:
  The equivalent d_color_xyz.
*/
D_COLOR_FN_RT struct d_color_xyz
d_color_convert_lab_to_xyz(
    struct d_color_lab _lab
)
{
    float fy = (_lab.l + 16.0f) / 116.0f;
    float fx = _lab.a / 500.0f + fy;
    float fz = fy - _lab.b / 200.0f;

    return d_color_xyz_make(
        D_COLOR_XYZ_WHITE_X * d_color_lab_f_inv(fx),
        D_COLOR_XYZ_WHITE_Y * d_color_lab_f_inv(fy),
        D_COLOR_XYZ_WHITE_Z * d_color_lab_f_inv(fz)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                  IX.   RGB <-> LAB   (via XYZ)                          ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_convert_rgb_to_lab
  Converts linear RGB to CIE L*a*b* via XYZ. Runtime-only.

Parameter(s):
  _rgb: source color.
Return:
  The equivalent d_color_lab.
*/
D_COLOR_FN_RT struct d_color_lab
d_color_convert_rgb_to_lab(
    struct d_color_rgb _rgb
)
{
    return d_color_convert_xyz_to_lab(d_color_convert_rgb_to_xyz(_rgb));
}

/*
d_color_convert_lab_to_rgb
  Converts CIE L*a*b* to linear RGB via XYZ. Runtime-only.

Parameter(s):
  _lab: source color.
Return:
  The equivalent d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_convert_lab_to_rgb(
    struct d_color_lab _lab
)
{
    return d_color_convert_xyz_to_rgb(d_color_convert_lab_to_xyz(_lab));
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_CONVERT_ */
