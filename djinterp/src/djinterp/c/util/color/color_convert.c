/******************************************************************************
* djinterp [utility]                                           color_convert.c
*
*   External-definition unit for color_convert.h. The cross-model conversion bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_convert.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_convert.h"


#ifndef D_COLOR_HEADER_ONLY

float d_color_hsl_hue_to_rgb(float _p, float _q, float _t);
struct d_color_hsl d_color_convert_rgb_to_hsl(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_hsl_to_rgb(struct d_color_hsl _hsl);
struct d_color_hsv d_color_convert_rgb_to_hsv(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_hsv_to_rgb(struct d_color_hsv _hsv);
struct d_color_hsv d_color_convert_hsl_to_hsv(struct d_color_hsl _hsl);
struct d_color_hsl d_color_convert_hsv_to_hsl(struct d_color_hsv _hsv);
struct d_color_cmyk d_color_convert_rgb_to_cmyk(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_cmyk_to_rgb(struct d_color_cmyk _cmyk);
struct d_color_ycbcr d_color_convert_rgb_to_ycbcr(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_ycbcr_to_rgb(struct d_color_ycbcr _ycbcr);
struct d_color_xyz d_color_convert_rgb_to_xyz(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_xyz_to_rgb(struct d_color_xyz _xyz);
struct d_color_lab d_color_convert_xyz_to_lab(struct d_color_xyz _xyz);
struct d_color_xyz d_color_convert_lab_to_xyz(struct d_color_lab _lab);
struct d_color_lab d_color_convert_rgb_to_lab(struct d_color_rgb _rgb);
struct d_color_rgb d_color_convert_lab_to_rgb(struct d_color_lab _lab);

#endif  // D_COLOR_HEADER_ONLY