/******************************************************************************
* djinterp [utility]                                               color_cmyk.h
*
*   CMYK shared kernel for the djinterp color module. Defines the CMYK POD
* and its construction, validation, and clamping. Conversions to and from
* other models live in color_convert.h.
*
*
* path:      /inc/djinterp/c/util/color/color_cmyk.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    d_color_cmyk          (POD)
II.   d_color_cmyk_make
III.  d_color_cmyk_is_valid
IV.   d_color_cmyk_clamp
*/

#ifndef  DJINTERP_C_COLOR_CMYK_
#define  DJINTERP_C_COLOR_CMYK_ 1

// djinterp
#include "../../djinterp.h"
#include "./color_common.h"


D_COLOR_NS_OPEN


// d_color_cmyk
//   POD: CMYK color, channels in [0, 1].
struct d_color_cmyk
{
    float c;
    float m;
    float y;
    float k;
};

/*
d_color_cmyk_make
  Constructs a CMYK color from raw channel values.

Parameter(s):
  _c: cyan    channel.
  _m: magenta channel.
  _y: yellow  channel.
  _k: key (black) channel.
Return:
  A d_color_cmyk with the given channels.
*/
D_COLOR_FN struct d_color_cmyk
d_color_cmyk_make(
    float _c,
    float _m,
    float _y,
    float _k
)
{
    return D_COLOR_LITERAL(d_color_cmyk, _c, _m, _y, _k);
}

/*
d_color_cmyk_is_valid
  Tests whether all channels lie within [0, 1].

Parameter(s):
  _cmyk: color to test.
Return:
  true if every channel is in range, false otherwise.
*/
D_COLOR_FN bool
d_color_cmyk_is_valid(
    struct d_color_cmyk _cmyk
)
{
    return ( (_cmyk.c >= 0.0f) && (_cmyk.c <= 1.0f) &&
             (_cmyk.m >= 0.0f) && (_cmyk.m <= 1.0f) &&
             (_cmyk.y >= 0.0f) && (_cmyk.y <= 1.0f) &&
             (_cmyk.k >= 0.0f) && (_cmyk.k <= 1.0f) );
}

/*
d_color_cmyk_clamp
  Clamps each channel to [0, 1].

Parameter(s):
  _cmyk: color to clamp.
Return:
  A d_color_cmyk with channels constrained to [0, 1].
*/
D_COLOR_FN struct d_color_cmyk
d_color_cmyk_clamp(
    struct d_color_cmyk _cmyk
)
{
    return d_color_cmyk_make(
        d_color_clamp_01(_cmyk.c),
        d_color_clamp_01(_cmyk.m),
        d_color_clamp_01(_cmyk.y),
        d_color_clamp_01(_cmyk.k)
    );
}


D_COLOR_NS_CLOSE


#endif  /*  DJINTERP_C_COLOR_CMYK_ */
