/******************************************************************************
* djinterp [utility]                                                color_hsv.h
*
*   HSV shared kernel for the djinterp color module. Defines the HSV POD and
* its construction, validation, and clamping. Conversions to and from other
* models live in color_convert.h.
*
*
* path:      /inc/djinterp/c/util/color/color_hsv.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    d_color_hsv          (POD)
II.   d_color_hsv_make
III.  d_color_hsv_is_valid
IV.   d_color_hsv_clamp
*/

#ifndef  DJINTERP_C_COLOR_HSV_
#define  DJINTERP_C_COLOR_HSV_ 1

// djinterp
#include "../../djinterp.h"
#include "./color_common.h"


D_COLOR_NS_OPEN


// d_color_hsv
//   POD: HSV color; h in [0, 360), s and v in [0, 1].
struct d_color_hsv
{
    float h;
    float s;
    float v;
};

/*
d_color_hsv_make
  Constructs an HSV color from raw channel values.

Parameter(s):
  _h: hue        in degrees.
  _s: saturation in [0, 1].
  _v: value      in [0, 1].
Return:
  A d_color_hsv with the given channels.
*/
D_COLOR_FN struct d_color_hsv
d_color_hsv_make(
    float _h,
    float _s,
    float _v
)
{
    return D_COLOR_LITERAL(d_color_hsv, _h, _s, _v);
}

/*
d_color_hsv_is_valid
  Tests whether the channels lie in their canonical ranges.

Parameter(s):
  _hsv: color to test.
Return:
  true if h in [0, 360) and s, v in [0, 1], false otherwise.
*/
D_COLOR_FN bool
d_color_hsv_is_valid(
    struct d_color_hsv _hsv
)
{
    return ( (_hsv.h >= 0.0f) && (_hsv.h < 360.0f) &&
             (_hsv.s >= 0.0f) && (_hsv.s <= 1.0f) &&
             (_hsv.v >= 0.0f) && (_hsv.v <= 1.0f) );
}

/*
d_color_hsv_clamp
  Wraps hue into [0, 360) and clamps s, v to [0, 1].

Parameter(s):
  _hsv: color to clamp.
Return:
  A canonicalized d_color_hsv.
*/
D_COLOR_FN struct d_color_hsv
d_color_hsv_clamp(
    struct d_color_hsv _hsv
)
{
    float h = d_color_fmodf(_hsv.h, 360.0f);

    if (h < 0.0f)
    {
        h += 360.0f;
    }

    return d_color_hsv_make(
        h,
        d_color_clamp_01(_hsv.s),
        d_color_clamp_01(_hsv.v)
    );
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_HSV_ */
