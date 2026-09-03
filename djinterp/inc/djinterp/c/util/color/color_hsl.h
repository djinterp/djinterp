/******************************************************************************
* djinterp [utility]                                                color_hsl.h
*
*   HSL shared kernel for the djinterp color module. Defines the HSL POD and
* its construction, validation, and clamping. Conversions to and from other
* models live in color_convert.h.
*
*
* path:      /inc/djinterp/c/util/color/color_hsl.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    d_color_hsl          (POD)
II.   d_color_hsl_make
III.  d_color_hsl_is_valid
IV.   d_color_hsl_clamp
*/

#ifndef  DJINTERP_C_COLOR_HSL_
#define  DJINTERP_C_COLOR_HSL_ 1

// djinterp
#include "../../djinterp.h"
#include "./color_common.h"


D_COLOR_NS_OPEN


// d_color_hsl
//   POD: HSL color; h in [0, 360), s and l in [0, 1].
struct d_color_hsl
{
    float h;
    float s;
    float l;
};

/*
d_color_hsl_make
  Constructs an HSL color from raw channel values.

Parameter(s):
  _h: hue        in degrees.
  _s: saturation in [0, 1].
  _l: lightness  in [0, 1].
Return:
  A d_color_hsl with the given channels.
*/
D_COLOR_FN struct d_color_hsl
d_color_hsl_make(
    float _h,
    float _s,
    float _l
)
{
    return D_COLOR_LITERAL(d_color_hsl, _h, _s, _l);
}

/*
d_color_hsl_is_valid
  Tests whether the channels lie in their canonical ranges.

Parameter(s):
  _hsl: color to test.
Return:
  true if h in [0, 360) and s, l in [0, 1], false otherwise.
*/
D_COLOR_FN bool
d_color_hsl_is_valid(
    struct d_color_hsl _hsl
)
{
    return ( (_hsl.h >= 0.0f) && (_hsl.h < 360.0f) &&
             (_hsl.s >= 0.0f) && (_hsl.s <= 1.0f) &&
             (_hsl.l >= 0.0f) && (_hsl.l <= 1.0f) );
}

/*
d_color_hsl_clamp
  Wraps hue into [0, 360) and clamps s, l to [0, 1].

Parameter(s):
  _hsl: color to clamp.
Return:
  A canonicalized d_color_hsl.
*/
D_COLOR_FN struct d_color_hsl
d_color_hsl_clamp(
    struct d_color_hsl _hsl
)
{
    float h = d_color_fmodf(_hsl.h, 360.0f);

    if (h < 0.0f)
    {
        h += 360.0f;
    }

    return d_color_hsl_make(
        h,
        d_color_clamp_01(_hsl.s),
        d_color_clamp_01(_hsl.l)
    );
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_HSL_ */
