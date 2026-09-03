/******************************************************************************
* djinterp [utility]                                               color_hsv.c
*
*   External-definition unit for color_hsv.h. The HSV bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_hsv.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_hsv.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_hsv d_color_hsv_make(float _h, float _s, float _v);
bool d_color_hsv_is_valid(struct d_color_hsv _hsv);
struct d_color_hsv d_color_hsv_clamp(struct d_color_hsv _hsv);

#endif  // D_COLOR_HEADER_ONLY