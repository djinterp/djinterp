/******************************************************************************
* djinterp [utility]                                               color_rgb.h
*
*   RGB family shared kernel for the djinterp color module. Defines the
* linear-RGB POD and its companions (straight-alpha RGBA, premultiplied
* RGBA, 8-bit integer forms, and packed hex forms) together with every
* routine that operates purely within the RGB family: construction,
* validation, clamping, sRGB gamma transfer, alpha (pre/un-multiply,
* compositing), 8-bit and hex (de)serialization, luminance, contrast,
* grayscale, inversion, interpolation, and blackbody temperature.
*
*   Convention: `d_color_rgb` holds LINEAR RGB in [0, 1]. sRGB-encoded
* values are produced/consumed by the gamma routines and by the 8-bit/hex
* paths (which gamma-encode). Routines that touch only arithmetic are
* constexpr (D_COLOR_FN); routines that call <math.h> transcendentals are
* runtime-only (D_COLOR_FN_RT).
*
*
* path:      /inc/djinterp/c/util/color/color_rgb.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    POD TYPES
      ---------
      a. d_color_rgb
      b. d_color_rgba
      c. d_color_rgba_premul
      d. d_color_rgb_u8
      e. d_color_rgba_u8
      f. d_color_rgb_hex, d_color_rgba_hex
II.   CONSTRUCTION
      ------------
      a. d_color_rgb_make
      b. d_color_rgba_make
      c. d_color_rgba_premul_make
      d. d_color_rgb_u8_make
      e. d_color_rgba_u8_make
III.  VALIDATION & CLAMPING
      ---------------------
      a. d_color_rgb_is_valid / d_color_rgba_is_valid
      b. d_color_rgb_clamp / d_color_rgba_clamp
IV.   sRGB GAMMA TRANSFER
      -------------------
      a. d_color_srgb_to_linear_component / d_color_linear_to_srgb_component
      b. d_color_rgb_from_srgb / d_color_rgb_to_srgb
V.    ALPHA
      -----
      a. d_color_rgba_from_rgb / d_color_rgb_from_rgba
      b. d_color_rgba_premultiply / d_color_rgba_unpremultiply
      c. d_color_rgba_blend_over / d_color_rgba_blend_over_straight
VI.   8-BIT (de)serialization
      ----------------------
      a. d_color_rgb_from_u8 / d_color_rgb_to_u8
      b. d_color_rgba_from_u8 / d_color_rgba_to_u8
VII.  HEX (de)serialization
      --------------------
      a. d_color_rgb_from_hex / d_color_rgb_to_hex
      b. d_color_rgba_from_hex / d_color_rgba_to_hex
      c. d_color_rgb_from_hex_string / d_color_rgba_from_hex_string
      d. d_color_rgb_hex_from_string
VIII. OPERATIONS
      ----------
      a. d_color_rgb_luminance / d_color_rgba_luminance
      b. d_color_rgb_contrast_ratio
      c. d_color_rgb_to_grayscale
      d. d_color_rgb_invert
      e. d_color_rgb_lerp / d_color_rgba_lerp
      f. d_color_rgb_from_temperature
*/

#ifndef  DJINTERP_C_COLOR_RGB_
#define  DJINTERP_C_COLOR_RGB_ 1

// std
#include <math.h>
#include <string.h>
#include <stdint.h>
// djinterp
#include "../../djinterp.h"
#include "./color_common.h"

D_COLOR_NS_OPEN


///////////////////////////////////////////////////////////////////////////////
///                       I.   POD TYPES                                    ///
///////////////////////////////////////////////////////////////////////////////

// d_color_rgb
//   POD: linear RGB color, channels in [0, 1].
struct d_color_rgb
{
    float r;
    float g;
    float b;
};

// d_color_rgba
//   POD: linear RGB with straight (non-premultiplied) alpha.
struct d_color_rgba
{
    float r;
    float g;
    float b;
    float a;
};

// d_color_rgba_premul
//   POD: linear RGB with premultiplied alpha.
struct d_color_rgba_premul
{
    float r;
    float g;
    float b;
    float a;
};

// d_color_rgb_u8
//   POD: 8-bit-per-channel sRGB-encoded RGB.
struct d_color_rgb_u8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// d_color_rgba_u8
//   POD: 8-bit-per-channel sRGB-encoded RGBA.
struct d_color_rgba_u8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

// d_color_rgb_hex
//   type: 0x00RRGGBB packed sRGB color.
typedef uint32_t d_color_rgb_hex;

// d_color_rgba_hex
//   type: 0xRRGGBBAA packed sRGB color.
typedef uint32_t d_color_rgba_hex;


///////////////////////////////////////////////////////////////////////////////
///                      II.   CONSTRUCTION                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_make
  Constructs a linear RGB color from raw channel values (no clamping).

Parameter(s):
  _r: red   channel.
  _g: green channel.
  _b: blue  channel.
Return:
  A d_color_rgb with the given channels.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_make(
    float _r,
    float _g,
    float _b
)
{
    return D_COLOR_LITERAL(d_color_rgb, _r, _g, _b);
}

/*
d_color_rgba_make
  Constructs a straight-alpha RGBA color from raw channel values.

Parameter(s):
  _r: red   channel.
  _g: green channel.
  _b: blue  channel.
  _a: alpha channel.
Return:
  A d_color_rgba with the given channels.
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_make(
    float _r,
    float _g,
    float _b,
    float _a
)
{
    return D_COLOR_LITERAL(d_color_rgba, _r, _g, _b, _a);
}

/*
d_color_rgba_premul_make
  Constructs a premultiplied-alpha RGBA color from raw channel values.

Parameter(s):
  _r: premultiplied red   channel.
  _g: premultiplied green channel.
  _b: premultiplied blue  channel.
  _a: alpha channel.
Return:
  A d_color_rgba_premul with the given channels.
*/
D_COLOR_FN struct d_color_rgba_premul
d_color_rgba_premul_make(
    float _r,
    float _g,
    float _b,
    float _a
)
{
    return D_COLOR_LITERAL(d_color_rgba_premul, _r, _g, _b, _a);
}

/*
d_color_rgb_u8_make
  Constructs an 8-bit RGB color from integer channel values.

Parameter(s):
  _r: red   channel [0, 255].
  _g: green channel [0, 255].
  _b: blue  channel [0, 255].
Return:
  A d_color_rgb_u8 with the given channels.
*/
D_COLOR_FN struct d_color_rgb_u8
d_color_rgb_u8_make(
    uint8_t _r,
    uint8_t _g,
    uint8_t _b
)
{
    return D_COLOR_LITERAL(d_color_rgb_u8, _r, _g, _b);
}

/*
d_color_rgba_u8_make
  Constructs an 8-bit RGBA color from integer channel values.

Parameter(s):
  _r: red   channel [0, 255].
  _g: green channel [0, 255].
  _b: blue  channel [0, 255].
  _a: alpha channel [0, 255].
Return:
  A d_color_rgba_u8 with the given channels.
*/
D_COLOR_FN struct d_color_rgba_u8
d_color_rgba_u8_make(
    uint8_t _r,
    uint8_t _g,
    uint8_t _b,
    uint8_t _a
)
{
    return D_COLOR_LITERAL(d_color_rgba_u8, _r, _g, _b, _a);
}


///////////////////////////////////////////////////////////////////////////////
///                 III.   VALIDATION & CLAMPING                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_is_valid
  Tests whether all channels lie within [0, 1].

Parameter(s):
  _rgb: color to test.
Return:
  true if every channel is in range, false otherwise.
*/
D_COLOR_FN bool
d_color_rgb_is_valid(
    struct d_color_rgb _rgb
)
{
    return ( (_rgb.r >= 0.0f) && (_rgb.r <= 1.0f) &&
             (_rgb.g >= 0.0f) && (_rgb.g <= 1.0f) &&
             (_rgb.b >= 0.0f) && (_rgb.b <= 1.0f) );
}

/*
d_color_rgba_is_valid
  Tests whether all channels (including alpha) lie within [0, 1].

Parameter(s):
  _rgba: color to test.
Return:
  true if every channel is in range, false otherwise.
*/
D_COLOR_FN bool
d_color_rgba_is_valid(
    struct d_color_rgba _rgba
)
{
    return ( (_rgba.r >= 0.0f) && (_rgba.r <= 1.0f) &&
             (_rgba.g >= 0.0f) && (_rgba.g <= 1.0f) &&
             (_rgba.b >= 0.0f) && (_rgba.b <= 1.0f) &&
             (_rgba.a >= 0.0f) && (_rgba.a <= 1.0f) );
}

/*
d_color_rgb_clamp
  Clamps each channel to [0, 1].

Parameter(s):
  _rgb: color to clamp.
Return:
  A d_color_rgb with channels constrained to [0, 1].
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_clamp(
    struct d_color_rgb _rgb
)
{
    return d_color_rgb_make(
        d_color_clamp_01(_rgb.r),
        d_color_clamp_01(_rgb.g),
        d_color_clamp_01(_rgb.b)
    );
}

/*
d_color_rgba_clamp
  Clamps each channel (including alpha) to [0, 1].

Parameter(s):
  _rgba: color to clamp.
Return:
  A d_color_rgba with channels constrained to [0, 1].
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_clamp(
    struct d_color_rgba _rgba
)
{
    return d_color_rgba_make(
        d_color_clamp_01(_rgba.r),
        d_color_clamp_01(_rgba.g),
        d_color_clamp_01(_rgba.b),
        d_color_clamp_01(_rgba.a)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                  IV.   sRGB GAMMA TRANSFER                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_srgb_to_linear_component
  Applies the sRGB-to-linear transfer function to one component.

Parameter(s):
  _component: sRGB component in [0, 1].
Return:
  The corresponding linear component in [0, 1].
*/
D_COLOR_FN_RT float
d_color_srgb_to_linear_component(
    float _component
)
{
    if (_component <= 0.04045f)
    {
        return _component / 12.92f;
    }

    return powf((_component + 0.055f) / 1.055f, 2.4f);
}

/*
d_color_linear_to_srgb_component
  Applies the linear-to-sRGB transfer function to one component.

Parameter(s):
  _component: linear component in [0, 1].
Return:
  The corresponding sRGB component in [0, 1].
*/
D_COLOR_FN_RT float
d_color_linear_to_srgb_component(
    float _component
)
{
    if (_component <= 0.0031308f)
    {
        return _component * 12.92f;
    }

    return 1.055f * powf(_component, 1.0f / 2.4f) - 0.055f;
}

/*
d_color_rgb_from_srgb
  Converts an sRGB-encoded color to linear RGB.

Parameter(s):
  _srgb: sRGB color, channels in [0, 1].
Return:
  A linear d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_rgb_from_srgb(
    struct d_color_rgb _srgb
)
{
    return d_color_rgb_make(
        d_color_srgb_to_linear_component(_srgb.r),
        d_color_srgb_to_linear_component(_srgb.g),
        d_color_srgb_to_linear_component(_srgb.b)
    );
}

/*
d_color_rgb_to_srgb
  Converts a linear RGB color to sRGB encoding.

Parameter(s):
  _linear: linear color, channels in [0, 1].
Return:
  An sRGB-encoded d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_rgb_to_srgb(
    struct d_color_rgb _linear
)
{
    return d_color_rgb_make(
        d_color_linear_to_srgb_component(_linear.r),
        d_color_linear_to_srgb_component(_linear.g),
        d_color_linear_to_srgb_component(_linear.b)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                          V.   ALPHA                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgba_from_rgb
  Attaches a straight alpha to an RGB color.

Parameter(s):
  _rgb:   source color.
  _alpha: alpha value in [0, 1].
Return:
  A d_color_rgba combining _rgb and _alpha.
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_from_rgb(
    struct d_color_rgb _rgb,
    float       _alpha
)
{
    return d_color_rgba_make(_rgb.r, _rgb.g, _rgb.b, _alpha);
}

/*
d_color_rgb_from_rgba
  Drops the alpha channel from an RGBA color.

Parameter(s):
  _rgba: source color.
Return:
  A d_color_rgb with _rgba's color channels.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_from_rgba(
    struct d_color_rgba _rgba
)
{
    return d_color_rgb_make(_rgba.r, _rgba.g, _rgba.b);
}

/*
d_color_rgba_premultiply
  Converts straight alpha to premultiplied alpha.

Parameter(s):
  _rgba: straight-alpha color.
Return:
  A d_color_rgba_premul with color channels scaled by alpha.
*/
D_COLOR_FN struct d_color_rgba_premul
d_color_rgba_premultiply(
    struct d_color_rgba _rgba
)
{
    return d_color_rgba_premul_make(
        _rgba.r * _rgba.a,
        _rgba.g * _rgba.a,
        _rgba.b * _rgba.a,
        _rgba.a
    );
}

/*
d_color_rgba_unpremultiply
  Converts premultiplied alpha back to straight alpha. Returns fully
  transparent black when alpha is (near) zero.

Parameter(s):
  _premul: premultiplied-alpha color.
Return:
  A straight-alpha d_color_rgba.
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_unpremultiply(
    struct d_color_rgba_premul _premul
)
{
    if (_premul.a < D_COLOR_EPSILON)
    {
        return d_color_rgba_make(0.0f, 0.0f, 0.0f, 0.0f);
    }

    return d_color_rgba_make(
        _premul.r / _premul.a,
        _premul.g / _premul.a,
        _premul.b / _premul.a,
        _premul.a
    );
}

/*
d_color_rgba_blend_over
  Porter-Duff "over" composite of premultiplied source onto premultiplied
  destination.

Parameter(s):
  _src: source color (premultiplied).
  _dst: destination color (premultiplied).
Return:
  The composited d_color_rgba_premul.
*/
D_COLOR_FN struct d_color_rgba_premul
d_color_rgba_blend_over(
    struct d_color_rgba_premul _src,
    struct d_color_rgba_premul _dst
)
{
    float one_minus_src_a = 1.0f - _src.a;

    return d_color_rgba_premul_make(
        _src.r + _dst.r * one_minus_src_a,
        _src.g + _dst.g * one_minus_src_a,
        _src.b + _dst.b * one_minus_src_a,
        _src.a + _dst.a * one_minus_src_a
    );
}

/*
d_color_rgba_blend_over_straight
  Porter-Duff "over" composite using straight-alpha inputs and output.

Parameter(s):
  _src: source color (straight alpha).
  _dst: destination color (straight alpha).
Return:
  The composited straight-alpha d_color_rgba.
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_blend_over_straight(
    struct d_color_rgba _src,
    struct d_color_rgba _dst
)
{
    struct d_color_rgba_premul src_premul = d_color_rgba_premultiply(_src);
    struct d_color_rgba_premul dst_premul = d_color_rgba_premultiply(_dst);

    return d_color_rgba_unpremultiply(
        d_color_rgba_blend_over(src_premul, dst_premul)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                  VI.   8-BIT (de)serialization                          ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_from_u8
  Decodes an 8-bit sRGB color to linear RGB (applies gamma).

Parameter(s):
  _u8: 8-bit sRGB color.
Return:
  A linear d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_rgb_from_u8(
    struct d_color_rgb_u8 _u8
)
{
    struct d_color_rgb srgb = d_color_rgb_make(
        _u8.r / 255.0f,
        _u8.g / 255.0f,
        _u8.b / 255.0f
    );

    return d_color_rgb_from_srgb(srgb);
}

/*
d_color_rgb_to_u8
  Encodes a linear RGB color to 8-bit sRGB (applies gamma).

Parameter(s):
  _rgb: linear color.
Return:
  An 8-bit sRGB d_color_rgb_u8.
*/
D_COLOR_FN_RT struct d_color_rgb_u8
d_color_rgb_to_u8(
    struct d_color_rgb _rgb
)
{
    struct d_color_rgb srgb = d_color_rgb_to_srgb(_rgb);

    return d_color_rgb_u8_make(
        (uint8_t)(d_color_clamp_01(srgb.r) * 255.0f + 0.5f),
        (uint8_t)(d_color_clamp_01(srgb.g) * 255.0f + 0.5f),
        (uint8_t)(d_color_clamp_01(srgb.b) * 255.0f + 0.5f)
    );
}

/*
d_color_rgba_from_u8
  Decodes an 8-bit sRGB RGBA color to linear straight-alpha RGBA. Color
  channels are gamma-decoded; alpha is linear.

Parameter(s):
  _u8: 8-bit sRGB RGBA color.
Return:
  A linear straight-alpha d_color_rgba.
*/
D_COLOR_FN_RT struct d_color_rgba
d_color_rgba_from_u8(
    struct d_color_rgba_u8 _u8
)
{
    struct d_color_rgb_u8 rgb_u8 = d_color_rgb_u8_make(_u8.r, _u8.g, _u8.b);
    struct d_color_rgb    rgb    = d_color_rgb_from_u8(rgb_u8);

    return d_color_rgba_from_rgb(rgb, _u8.a / 255.0f);
}

/*
d_color_rgba_to_u8
  Encodes a linear straight-alpha RGBA color to 8-bit sRGB RGBA.

Parameter(s):
  _rgba: linear straight-alpha color.
Return:
  An 8-bit sRGB d_color_rgba_u8.
*/
D_COLOR_FN_RT struct d_color_rgba_u8
d_color_rgba_to_u8(
    struct d_color_rgba _rgba
)
{
    struct d_color_rgb    rgb    = d_color_rgb_make(_rgba.r, _rgba.g, _rgba.b);
    struct d_color_rgb_u8 rgb_u8 = d_color_rgb_to_u8(rgb);

    return d_color_rgba_u8_make(
        rgb_u8.r,
        rgb_u8.g,
        rgb_u8.b,
        (uint8_t)(d_color_clamp_01(_rgba.a) * 255.0f + 0.5f)
    );
}


///////////////////////////////////////////////////////////////////////////////
///                  VII.   HEX (de)serialization                           ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_from_hex
  Decodes a 0x00RRGGBB packed color to linear RGB.

Parameter(s):
  _hex: packed sRGB color.
Return:
  A linear d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_rgb_from_hex(
    d_color_rgb_hex _hex
)
{
    struct d_color_rgb_u8 rgb_u8 = d_color_rgb_u8_make(
        (uint8_t)((_hex >> 16) & 0xFFu),
        (uint8_t)((_hex >>  8) & 0xFFu),
        (uint8_t)( _hex        & 0xFFu)
    );

    return d_color_rgb_from_u8(rgb_u8);
}

/*
d_color_rgb_to_hex
  Encodes a linear RGB color to a 0x00RRGGBB packed value.

Parameter(s):
  _rgb: linear color.
Return:
  The packed sRGB color.
*/
D_COLOR_FN_RT d_color_rgb_hex
d_color_rgb_to_hex(
    struct d_color_rgb _rgb
)
{
    struct d_color_rgb_u8 rgb_u8 = d_color_rgb_to_u8(_rgb);

    return ( ((d_color_rgb_hex)rgb_u8.r << 16) |
             ((d_color_rgb_hex)rgb_u8.g <<  8) |
              (d_color_rgb_hex)rgb_u8.b );
}

/*
d_color_rgba_from_hex
  Decodes a 0xRRGGBBAA packed color to linear straight-alpha RGBA.

Parameter(s):
  _hex: packed sRGB color.
Return:
  A linear straight-alpha d_color_rgba.
*/
D_COLOR_FN_RT struct d_color_rgba
d_color_rgba_from_hex(
    d_color_rgba_hex _hex
)
{
    struct d_color_rgba_u8 rgba_u8 = d_color_rgba_u8_make(
        (uint8_t)((_hex >> 24) & 0xFFu),
        (uint8_t)((_hex >> 16) & 0xFFu),
        (uint8_t)((_hex >>  8) & 0xFFu),
        (uint8_t)( _hex        & 0xFFu)
    );

    return d_color_rgba_from_u8(rgba_u8);
}

/*
d_color_rgba_to_hex
  Encodes a linear straight-alpha RGBA color to a 0xRRGGBBAA packed value.

Parameter(s):
  _rgba: linear straight-alpha color.
Return:
  The packed sRGB color.
*/
D_COLOR_FN_RT d_color_rgba_hex
d_color_rgba_to_hex(
    struct d_color_rgba _rgba
)
{
    struct d_color_rgba_u8 rgba_u8 = d_color_rgba_to_u8(_rgba);

    return ( ((d_color_rgba_hex)rgba_u8.r << 24) |
             ((d_color_rgba_hex)rgba_u8.g << 16) |
             ((d_color_rgba_hex)rgba_u8.b <<  8) |
              (d_color_rgba_hex)rgba_u8.a );
}

/*
d_color_rgb_from_hex_string
  Parses a "#RRGGBB" string into a linear RGB color.

Parameter(s):
  _str:     NUL-terminated candidate string.
  _out_rgb: destination color (written only on success).
Return:
  true on a valid "#RRGGBB" string, false otherwise.
*/
D_COLOR_FN_RT bool
d_color_rgb_from_hex_string(
    const char*  _str,
    struct d_color_rgb* _out_rgb
)
{
    d_color_rgb_hex hex = 0;
    int             i   = 0;

    if ( (_str == NULL) || (_out_rgb == NULL) )
    {
        return false;
    }
    if (_str[0] != '#')
    {
        return false;
    }
    if (strlen(_str) != 7)
    {
        return false;
    }

    for (i = 1; i < 7; ++i)
    {
        char c     = _str[i];
        int  digit = 0;

        if ( (c >= '0') && (c <= '9') )
        {
            digit = c - '0';
        }
        else if ( (c >= 'a') && (c <= 'f') )
        {
            digit = c - 'a' + 10;
        }
        else if ( (c >= 'A') && (c <= 'F') )
        {
            digit = c - 'A' + 10;
        }
        else
        {
            return false;
        }

        hex = (hex << 4) | (d_color_rgb_hex)digit;
    }

    *_out_rgb = d_color_rgb_from_hex(hex);

    return true;
}

/*
d_color_rgba_from_hex_string
  Parses a "#RRGGBBAA" string into a linear straight-alpha RGBA color.

Parameter(s):
  _str:      NUL-terminated candidate string.
  _out_rgba: destination color (written only on success).
Return:
  true on a valid "#RRGGBBAA" string, false otherwise.
*/
D_COLOR_FN_RT bool
d_color_rgba_from_hex_string(
    const char*   _str,
    struct d_color_rgba* _out_rgba
)
{
    d_color_rgba_hex hex = 0;
    int              i   = 0;

    if ( (_str == NULL) || (_out_rgba == NULL) )
    {
        return false;
    }
    if (_str[0] != '#')
    {
        return false;
    }
    if (strlen(_str) != 9)
    {
        return false;
    }

    for (i = 1; i < 9; ++i)
    {
        char c     = _str[i];
        int  digit = 0;

        if ( (c >= '0') && (c <= '9') )
        {
            digit = c - '0';
        }
        else if ( (c >= 'a') && (c <= 'f') )
        {
            digit = c - 'a' + 10;
        }
        else if ( (c >= 'A') && (c <= 'F') )
        {
            digit = c - 'A' + 10;
        }
        else
        {
            return false;
        }

        hex = (hex << 4) | (d_color_rgba_hex)digit;
    }

    *_out_rgba = d_color_rgba_from_hex(hex);

    return true;
}

/*
d_color_rgb_hex_from_string
  Parses a "#RRGGBB" string directly to a packed hex value, returning 0 on
  failure.

Parameter(s):
  _str: NUL-terminated candidate string.
Return:
  The packed color, or 0 if the string is invalid.
*/
D_COLOR_FN_RT d_color_rgb_hex
d_color_rgb_hex_from_string(
    const char* _str
)
{
    struct d_color_rgb rgb = d_color_rgb_make(0.0f, 0.0f, 0.0f);

    if (d_color_rgb_from_hex_string(_str, &rgb))
    {
        return d_color_rgb_to_hex(rgb);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///                      VIII.   OPERATIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_rgb_luminance
  Relative luminance (Rec. 709 coefficients) of a linear RGB color.

Parameter(s):
  _rgb: linear color.
Return:
  Luminance in [0, 1].
*/
D_COLOR_FN float
d_color_rgb_luminance(
    struct d_color_rgb _rgb
)
{
    return ( 0.2126729f * _rgb.r +
             0.7151522f * _rgb.g +
             0.0721750f * _rgb.b );
}

/*
d_color_rgba_luminance
  Relative luminance of a linear RGBA color (alpha ignored).

Parameter(s):
  _rgba: linear color.
Return:
  Luminance in [0, 1].
*/
D_COLOR_FN float
d_color_rgba_luminance(
    struct d_color_rgba _rgba
)
{
    return ( 0.2126729f * _rgba.r +
             0.7151522f * _rgba.g +
             0.0721750f * _rgba.b );
}

/*
d_color_rgb_contrast_ratio
  WCAG contrast ratio between two linear RGB colors.

Parameter(s):
  _a: first color.
  _b: second color.
Return:
  Contrast ratio in [1, 21].
*/
D_COLOR_FN float
d_color_rgb_contrast_ratio(
    struct d_color_rgb _a,
    struct d_color_rgb _b
)
{
    float l1 = d_color_rgb_luminance(_a) + 0.05f;
    float l2 = d_color_rgb_luminance(_b) + 0.05f;

    return (l1 > l2) ? (l1 / l2) : (l2 / l1);
}

/*
d_color_rgb_to_grayscale
  Replaces every channel with the color's luminance.

Parameter(s):
  _rgb: linear color.
Return:
  A neutral-gray d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_to_grayscale(
    struct d_color_rgb _rgb
)
{
    float lum = d_color_rgb_luminance(_rgb);

    return d_color_rgb_make(lum, lum, lum);
}

/*
d_color_rgb_invert
  Inverts each channel about 1.0.

Parameter(s):
  _rgb: linear color.
Return:
  The inverted d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_invert(
    struct d_color_rgb _rgb
)
{
    return d_color_rgb_make(
        1.0f - _rgb.r,
        1.0f - _rgb.g,
        1.0f - _rgb.b
    );
}

/*
d_color_rgb_lerp
  Linearly interpolates between two RGB colors.

Parameter(s):
  _a: start color (returned at _t = 0).
  _b: end color   (returned at _t = 1).
  _t: interpolation factor.
Return:
  The interpolated d_color_rgb.
*/
D_COLOR_FN struct d_color_rgb
d_color_rgb_lerp(
    struct d_color_rgb _a,
    struct d_color_rgb _b,
    float       _t
)
{
    float u = 1.0f - _t;

    return d_color_rgb_make(
        _a.r * u + _b.r * _t,
        _a.g * u + _b.g * _t,
        _a.b * u + _b.b * _t
    );
}

/*
d_color_rgba_lerp
  Linearly interpolates between two straight-alpha RGBA colors.

Parameter(s):
  _a: start color (returned at _t = 0).
  _b: end color   (returned at _t = 1).
  _t: interpolation factor.
Return:
  The interpolated d_color_rgba.
*/
D_COLOR_FN struct d_color_rgba
d_color_rgba_lerp(
    struct d_color_rgba _a,
    struct d_color_rgba _b,
    float        _t
)
{
    float u = 1.0f - _t;

    return d_color_rgba_make(
        _a.r * u + _b.r * _t,
        _a.g * u + _b.g * _t,
        _a.b * u + _b.b * _t,
        _a.a * u + _b.a * _t
    );
}

/*
d_color_rgb_from_temperature
  Approximates the linear RGB of a blackbody radiator at a given color
  temperature (Tanner Helland approximation).

Parameter(s):
  _kelvin: color temperature in kelvin (roughly 1000-40000).
Return:
  The approximate linear d_color_rgb.
*/
D_COLOR_FN_RT struct d_color_rgb
d_color_rgb_from_temperature(
    float _kelvin
)
{
    float temp = _kelvin / 100.0f;
    float r    = 0.0f;
    float g    = 0.0f;
    float b    = 0.0f;

    if (temp <= 66.0f)
    {
        r = 1.0f;
    }
    else
    {
        float rr = temp - 60.0f;
        rr = 329.698727446f * powf(rr, -0.1332047592f);
        r  = d_color_clamp_01(rr / 255.0f);
    }

    if (temp <= 66.0f)
    {
        float gg = temp;
        gg = 99.4708025861f * logf(gg) - 161.1195681661f;
        g  = d_color_clamp_01(gg / 255.0f);
    }
    else
    {
        float gg = temp - 60.0f;
        gg = 288.1221695283f * powf(gg, -0.0755148492f);
        g  = d_color_clamp_01(gg / 255.0f);
    }

    if (temp >= 66.0f)
    {
        b = 1.0f;
    }
    else if (temp <= 19.0f)
    {
        b = 0.0f;
    }
    else
    {
        float bb = temp - 10.0f;
        bb = 138.5177312231f * logf(bb) - 305.0447927307f;
        b  = d_color_clamp_01(bb / 255.0f);
    }

    return d_color_rgb_make(r, g, b);
}


D_COLOR_NS_CLOSE


#endif  // DJINTERP_C_COLOR_RGB_ */
