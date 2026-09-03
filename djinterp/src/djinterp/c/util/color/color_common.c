/******************************************************************************
* djinterp [utility]                                            color_common.c
*
*   External-definition unit for color_common.h. The scalar helper bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_common.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_common.h"


#ifndef D_COLOR_HEADER_ONLY

float d_color_max3(float _a, float _b, float _c);
float d_color_min3(float _a, float _b, float _c);
float d_color_clamp_01(float _value);
float d_color_clamp_range(float _value, float _min, float _max);
float d_color_fmodf(float _x, float _y);
float d_color_fabsf(float _x);

#endif  // D_COLOR_HEADER_ONLY
