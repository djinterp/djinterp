/******************************************************************************
* djinterp [utility]                                              color_cmyk.c
*
*   External-definition unit for color_cmyk.h. The CMYK bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_cmyk.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_cmyk.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_cmyk d_color_cmyk_make(float _c, float _m, float _y, float _k);
bool d_color_cmyk_is_valid(struct d_color_cmyk _cmyk);
struct d_color_cmyk d_color_cmyk_clamp(struct d_color_cmyk _cmyk);

#endif  /* D_COLOR_HEADER_ONLY */
