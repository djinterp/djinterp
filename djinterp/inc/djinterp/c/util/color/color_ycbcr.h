/******************************************************************************
* djinterp [utility]                                              color_ycbcr.h
*
*   YCbCr shared kernel for the djinterp color module. Defines the YCbCr POD
* (full-range ITU-R BT.601) and its construction, validation, and clamping.
* Conversions to and from other models live in color_convert.h.
*
*
* path:      /inc/djinterp/c/util/color/color_ycbcr.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    d_color_ycbcr          (POD)
II.   d_color_ycbcr_make
III.  d_color_ycbcr_is_valid
IV.   d_color_ycbcr_clamp
*/

#ifndef  DJINTERP_C_COLOR_YCBCR_
#define  DJINTERP_C_COLOR_YCBCR_ 1

// djinterp
#include "../../djinterp.h"
#include "./color_common.h"


D_COLOR_NS_OPEN


// d_color_ycbcr
//   POD: YCbCr color; Y in [0, 1], Cb and Cr in [-0.5, 0.5].
struct d_color_ycbcr
{
    float y;
    float cb;
    float cr;
};

/*
d_color_ycbcr_make
  Constructs a YCbCr color from raw channel values.

Parameter(s):
  _y:  luma          in [0, 1].
  _cb: blue-difference chroma in [-0.5, 0.5].
  _cr: red-difference  chroma in [-0.5, 0.5].
Return:
  A d_color_ycbcr with the given channels.
*/
D_COLOR_FN struct d_color_ycbcr
d_color_ycbcr_make(
    float _y,
    float _cb,
    float _cr
)
{
    return D_COLOR_LITERAL(d_color_ycbcr, _y, _cb, _cr);
}

/*
d_color_ycbcr_is_valid
  Tests whether the channels lie in their canonical ranges.

Parameter(s):
  _ycbcr: color to test.
Return:
  true if Y in [0, 1] and Cb, Cr in [-0.5, 0.5], false otherwise.
*/
D_COLOR_FN bool
d_color_ycbcr_is_valid(
    struct d_color_ycbcr _ycbcr
)
{
    return ( (_ycbcr.y  >=  0.0f) && (_ycbcr.y  <= 1.0f) &&
             (_ycbcr.cb >= -0.5f) && (_ycbcr.cb <= 0.5f) &&
             (_ycbcr.cr >= -0.5f) && (_ycbcr.cr <= 0.5f) );
}

/*
d_color_ycbcr_clamp
  Clamps Y to [0, 1] and Cb, Cr to [-0.5, 0.5].

Parameter(s):
  _ycbcr: color to clamp.
Return:
  A d_color_ycbcr with channels constrained to canonical ranges.
*/
D_COLOR_FN struct d_color_ycbcr
d_color_ycbcr_clamp(
    struct d_color_ycbcr _ycbcr
)
{
    return d_color_ycbcr_make(
        d_color_clamp_01(_ycbcr.y),
        d_color_clamp_range(_ycbcr.cb, -0.5f, 0.5f),
        d_color_clamp_range(_ycbcr.cr, -0.5f, 0.5f)
    );
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_YCBCR_ */
