/******************************************************************************
* djinterp [utility]                                             color_ycbcr.c
*
*   External-definition unit for color_ycbcr.h. The YCbCr bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_ycbcr.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_ycbcr.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_ycbcr d_color_ycbcr_make(float _y, float _cb, float _cr);
bool d_color_ycbcr_is_valid(struct d_color_ycbcr _ycbcr);
struct d_color_ycbcr d_color_ycbcr_clamp(struct d_color_ycbcr _ycbcr);

#endif  // D_COLOR_HEADER_ONLY