/*******************************************************************************
* djinterp [c][utility]                                                  color.c
*
*   External-definition unit for color.h. The cross-model operation bodies
* live in the header as `inline`; this unit re-declares their prototypes so
* one out-of-line external definition of each is emitted for the compiled C
* library (no effect under D_COLOR_HEADER_ONLY). See color_common.c for the
* rationale.
*
*
* path:      /src/djinterp/c/util/color/color.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color.h"  // the header


#ifndef D_COLOR_HEADER_ONLY

struct dj_color_rgb d_color_rgb_adjust_saturation(
    struct dj_color_rgb _rgb,
    float               _amount
);
struct dj_color_rgb d_color_rgb_adjust_brightness(
    struct dj_color_rgb _rgb,
    float               _amount
);
struct dj_color_rgb d_color_rgb_rotate_hue(
    struct dj_color_rgb _rgb,
    float               _degrees
);
float d_internal_color_de_hue_delta(
    float _h1,
    float _h2,
    float _c1c2
);
float d_internal_color_de_hue_mean(
    float _h1,
    float _h2,
    float _c1c2
);
float d_internal_color_de_hue_weight(
    float _h_mean
);
float d_internal_color_de_rotation(
    float _h_mean,
    float _c_prime_mean
);
float d_color_delta_e(
    struct dj_color_lab _lab1,
    struct dj_color_lab _lab2
);
float d_color_rgb_delta_e(
    struct dj_color_rgb _rgb1,
    struct dj_color_rgb _rgb2
);

#endif  // D_COLOR_HEADER_ONLY
