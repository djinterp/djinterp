/******************************************************************************
* djinterp [utility]                                                color_lab.h
*
*   CIE XYZ and CIE L*a*b* shared kernel for the djinterp color module. XYZ
* is the device-independent hub through which L*a*b* conversions route, so
* both PODs and the shared lab transfer helpers live together here. Cross-
* model conversions live in color_convert.h.
*
*   The D65 white point and the lab_f / lab_f_inv helpers are defined here
* because they are intrinsic to the XYZ<->LAB relationship and are shared by
* the conversion kernels.
*
*
* path:      /inc/djinterp/c/util/color/color_lab.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    POD TYPES
      ---------
      a. d_color_xyz
      b. d_color_lab
II.   CONSTRUCTION
      ------------
      a. d_color_xyz_make
      b. d_color_lab_make
III.  D65 WHITE POINT
      ---------------
IV.   LAB TRANSFER HELPERS
      -------------------
      a. d_color_lab_f
      b. d_color_lab_f_inv
V.    VALIDATION & CLAMPING
      ---------------------
      a. d_color_xyz_is_valid / d_color_lab_is_valid
      b. d_color_xyz_clamp / d_color_lab_clamp
*/

#ifndef  DJINTERP_C_COLOR_LAB_
#define  DJINTERP_C_COLOR_LAB_ 1

// std
#include <math.h>
// djinterp
#include "../../djinterp.h"
#include "./color_common.h"


D_COLOR_NS_OPEN


///////////////////////////////////////////////////////////////////////////////
///                       I.   POD TYPES                                    ///
///////////////////////////////////////////////////////////////////////////////

// d_color_xyz
//   POD: CIE 1931 XYZ tristimulus color (D65, sRGB primaries).
struct d_color_xyz
{
    float x;
    float y;
    float z;
};

// d_color_lab
//   POD: CIE L*a*b* color; L* in [0, 100], a*/b* ~ [-128, 127].
struct d_color_lab
{
    float l;
    float a;
    float b;
};


///////////////////////////////////////////////////////////////////////////////
///                      II.   CONSTRUCTION                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_xyz_make
  Constructs an XYZ color from raw tristimulus values.

Parameter(s):
  _x: X tristimulus value.
  _y: Y tristimulus value.
  _z: Z tristimulus value.
Return:
  A d_color_xyz with the given values.
*/
D_COLOR_FN struct d_color_xyz
d_color_xyz_make(
    float _x,
    float _y,
    float _z
)
{
    return D_COLOR_LITERAL(d_color_xyz, _x, _y, _z);
}

/*
d_color_lab_make
  Constructs a L*a*b* color from raw channel values.

Parameter(s):
  _l: lightness L*.
  _a: green-red a*.
  _b: blue-yellow b*.
Return:
  A d_color_lab with the given channels.
*/
D_COLOR_FN struct d_color_lab
d_color_lab_make(
    float _l,
    float _a,
    float _b
)
{
    return D_COLOR_LITERAL(d_color_lab, _l, _a, _b);
}


///////////////////////////////////////////////////////////////////////////////
///                    III.   D65 WHITE POINT                               ///
///////////////////////////////////////////////////////////////////////////////

// D65 illuminant white point (sRGB reference white).
#define D_COLOR_XYZ_WHITE_X 0.95047f
#define D_COLOR_XYZ_WHITE_Y 1.00000f
#define D_COLOR_XYZ_WHITE_Z 1.08883f


///////////////////////////////////////////////////////////////////////////////
///                  IV.   LAB TRANSFER HELPERS                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_lab_f
  Forward L*a*b* nonlinearity f(t). Runtime-only (uses cube root via powf).

Parameter(s):
  _t: normalized tristimulus ratio.
Return:
  f(_t) per the CIE L*a*b* definition.
*/
D_COLOR_FN_RT float
d_color_lab_f(
    float _t
)
{
    const float delta        = 6.0f / 29.0f;
    const float delta_cubed  = delta * delta * delta;

    if (_t > delta_cubed)
    {
        return powf(_t, 1.0f / 3.0f);
    }

    return _t / (3.0f * delta * delta) + 4.0f / 29.0f;
}

/*
d_color_lab_f_inv
  Inverse L*a*b* nonlinearity f^-1(t).

Parameter(s):
  _t: value in f-space.
Return:
  f^-1(_t) per the CIE L*a*b* definition.
*/
D_COLOR_FN_RT float
d_color_lab_f_inv(
    float _t
)
{
    const float delta = 6.0f / 29.0f;

    if (_t > delta)
    {
        return _t * _t * _t;
    }

    return 3.0f * delta * delta * (_t - 4.0f / 29.0f);
}


///////////////////////////////////////////////////////////////////////////////
///                 V.   VALIDATION & CLAMPING                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_xyz_is_valid
  Tests whether all tristimulus values are non-negative.

Parameter(s):
  _xyz: color to test.
Return:
  true if x, y, z are all >= 0, false otherwise.
*/
D_COLOR_FN bool
d_color_xyz_is_valid(
    struct d_color_xyz _xyz
)
{
    return ( (_xyz.x >= 0.0f) &&
             (_xyz.y >= 0.0f) &&
             (_xyz.z >= 0.0f) );
}

/*
d_color_lab_is_valid
  Tests whether L* lies within [0, 100].

Parameter(s):
  _lab: color to test.
Return:
  true if L* is in range, false otherwise.
*/
D_COLOR_FN bool
d_color_lab_is_valid(
    struct d_color_lab _lab
)
{
    return ( (_lab.l >= 0.0f) && (_lab.l <= 100.0f) );
}

/*
d_color_xyz_clamp
  Clamps each tristimulus value to be non-negative.

Parameter(s):
  _xyz: color to clamp.
Return:
  A d_color_xyz with non-negative values.
*/
D_COLOR_FN struct d_color_xyz
d_color_xyz_clamp(
    struct d_color_xyz _xyz
)
{
    return d_color_xyz_make(
        (_xyz.x < 0.0f) ? 0.0f : _xyz.x,
        (_xyz.y < 0.0f) ? 0.0f : _xyz.y,
        (_xyz.z < 0.0f) ? 0.0f : _xyz.z
    );
}

/*
d_color_lab_clamp
  Clamps L* to [0, 100] and a*, b* to [-128, 127].

Parameter(s):
  _lab: color to clamp.
Return:
  A d_color_lab with channels constrained to canonical ranges.
*/
D_COLOR_FN struct d_color_lab
d_color_lab_clamp(
    struct d_color_lab _lab
)
{
    return d_color_lab_make(
        d_color_clamp_range(_lab.l, 0.0f, 100.0f),
        d_color_clamp_range(_lab.a, -128.0f, 127.0f),
        d_color_clamp_range(_lab.b, -128.0f, 127.0f)
    );
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_LAB_ */
