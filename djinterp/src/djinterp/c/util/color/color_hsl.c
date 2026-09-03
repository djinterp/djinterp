/******************************************************************************
* djinterp [utility]                                               color_hsl.c
*
*   External-definition unit for color_hsl.h. The HSL bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_hsl.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_hsl.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_hsl d_color_hsl_make(float _h, float _s, float _l);
bool d_color_hsl_is_valid(struct d_color_hsl _hsl);
struct d_color_hsl d_color_hsl_clamp(struct d_color_hsl _hsl);

#endif  // D_COLOR_HEADER_ONLY
