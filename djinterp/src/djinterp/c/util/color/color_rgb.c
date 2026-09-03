/******************************************************************************
* djinterp [utility]                                               color_rgb.c
*
*   External-definition unit for color_rgb.h. The RGB-family bodies live in the
* header as `inline`; this unit re-declares their prototypes so one out-of-
* line external definition of each is emitted for the compiled C library (no
* effect under D_COLOR_HEADER_ONLY). See color_common.c for the rationale.
*
*
* path:      /inc/djinterp/c/util/color/color_rgb.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/
#include "../../../../../inc/djinterp/c/util/color/color_rgb.h"


#ifndef D_COLOR_HEADER_ONLY

struct d_color_rgb d_color_rgb_make(float _r, float _g, float _b);
struct d_color_rgba d_color_rgba_make(float _r, float _g, float _b, float _a);
struct d_color_rgba_premul d_color_rgba_premul_make(float _r, float _g, float _b, float _a);
struct d_color_rgb_u8 d_color_rgb_u8_make(uint8_t _r, uint8_t _g, uint8_t _b);
struct d_color_rgba_u8 d_color_rgba_u8_make(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a);
bool d_color_rgb_is_valid(struct d_color_rgb _rgb);
bool d_color_rgba_is_valid(struct d_color_rgba _rgba);
struct d_color_rgb d_color_rgb_clamp(struct d_color_rgb _rgb);
struct d_color_rgba d_color_rgba_clamp(struct d_color_rgba _rgba);
float d_color_srgb_to_linear_component(float _component);
float d_color_linear_to_srgb_component(float _component);
struct d_color_rgb d_color_rgb_from_srgb(struct d_color_rgb _srgb);
struct d_color_rgb d_color_rgb_to_srgb(struct d_color_rgb _linear);
struct d_color_rgba d_color_rgba_from_rgb(struct d_color_rgb _rgb, float _alpha);
struct d_color_rgb d_color_rgb_from_rgba(struct d_color_rgba _rgba);
struct d_color_rgba_premul d_color_rgba_premultiply(struct d_color_rgba _rgba);
struct d_color_rgba d_color_rgba_unpremultiply(struct d_color_rgba_premul _premul);
struct d_color_rgba_premul d_color_rgba_blend_over(struct d_color_rgba_premul _src, struct d_color_rgba_premul _dst);
struct d_color_rgba d_color_rgba_blend_over_straight(struct d_color_rgba _src, struct d_color_rgba _dst);
struct d_color_rgb d_color_rgb_from_u8(struct d_color_rgb_u8 _u8);
struct d_color_rgb_u8 d_color_rgb_to_u8(struct d_color_rgb _rgb);
struct d_color_rgba d_color_rgba_from_u8(struct d_color_rgba_u8 _u8);
struct d_color_rgba_u8 d_color_rgba_to_u8(struct d_color_rgba _rgba);
struct d_color_rgb d_color_rgb_from_hex(struct d_color_rgb_hex _hex);
struct d_color_rgb_hex d_color_rgb_to_hex(struct d_color_rgb _rgb);
struct d_color_rgba d_color_rgba_from_hex(struct d_color_rgba_hex _hex);
struct d_color_rgba_hex d_color_rgba_to_hex(struct d_color_rgba _rgba);
bool d_color_rgb_from_hex_string(const char* _str, struct d_color_rgb* _out_rgb);
bool d_color_rgba_from_hex_string(const char* _str, struct d_color_rgba* _out_rgba);
struct d_color_rgb_hex d_color_rgb_hex_from_string(const char* _str);
float d_color_rgb_luminance(struct d_color_rgb _rgb);
float d_color_rgba_luminance(struct d_color_rgba _rgba);
float d_color_rgb_contrast_ratio(struct d_color_rgb _a, struct d_color_rgb _b);
struct d_color_rgb d_color_rgb_to_grayscale(struct d_color_rgb _rgb);
struct d_color_rgb d_color_rgb_invert(struct d_color_rgb _rgb);
struct d_color_rgb d_color_rgb_lerp(struct d_color_rgb _a, struct d_color_rgb _b, float _t);
struct d_color_rgba d_color_rgba_lerp(struct d_color_rgba _a, struct d_color_rgba _b, float _t);
struct d_color_rgb d_color_rgb_from_temperature(float _kelvin);

#endif  // D_COLOR_HEADER_ONLY