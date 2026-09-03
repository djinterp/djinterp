/******************************************************************************
* djinterp [utility]                                               color_lab.c
*
*   External-definition unit for color_lab.h. The CIE XYZ and L*a*b* bodies 
* live in the header as `inline`; this unit re-declares their prototypes so 
* one out-of-line external definition of each is emitted for the compiled C 
* library (no effect under D_COLOR_HEADER_ONLY). See color_common.c for the 
* rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_lab.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_lab.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_xyz d_color_xyz_make(float _x, float _y, float _z);
struct d_color_lab d_color_lab_make(float _l, float _a, float _b);
float d_color_lab_f(float _t);
float d_color_lab_f_inv(float _t);
bool d_color_xyz_is_valid(struct d_color_xyz _xyz);
bool d_color_lab_is_valid(struct d_color_lab _lab);
struct d_color_xyz d_color_xyz_clamp(struct d_color_xyz _xyz);
struct d_color_lab d_color_lab_clamp(struct d_color_lab _lab);

#endif  // D_COLOR_HEADER_ONLY
