/******************************************************************************
* djinterp [utility]                                                   color.c
*
*   External-definition unit for color.h. The cross-model operation bodies 
* live in the header as `inline`; this unit re-declares their prototypes so 
* one out-of-line external definition of each is emitted for the compiled C 
* library (no effect under D_COLOR_HEADER_ONLY). See color_common.c for the 
* rationale.
* 
*
* path:      /inc/djinterp/c/util/color/color.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_rgb d_color_rgb_adjust_saturation(struct d_color_rgb _rgb, float _amount);
struct d_color_rgb d_color_rgb_adjust_brightness(struct d_color_rgb _rgb, float _amount);
struct d_color_rgb d_color_rgb_rotate_hue(struct d_color_rgb _rgb, float _degrees);
float d_color_delta_e(struct d_color_lab _lab1, struct d_color_lab _lab2);
float d_color_rgb_delta_e(struct d_color_rgb _rgb1, struct d_color_rgb _rgb2);

#endif  // D_COLOR_HEADER_ONLY
