/******************************************************************************
* djinterp [color]                                                color_rgb.hpp
*
*   C++ ergonomic layer for the RGB family. Each wrapper type derives from
* its shared-kernel POD (color_rgb.h) without adding data members, so it is
* layout-compatible and slices to the POD for free when calling kernel
* routines. Construction, comparison, and the family operations are thin
* constexpr/inline facades that forward to the single C kernel
* implementation -- no color math is duplicated here.
*
*   `rgb` is a full color model (carries model_tag and participates in the
* conversion graph). `rgba` and `rgba_premul` are alpha-carrying transport
* types for compositing and (de)serialization; they deliberately have no
* model_tag and are converted to/from `rgb` explicitly.
*
*
* path:      /inc/djinterp/util/color/color_rgb.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    rgb
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c. operator==
      d. is_valid / clamp
      e. luminance / invert / grayscale
      f. to_srgb / to_8bit / to_hex
      g. from_srgb / from_8bit / from_hex / from_hex_string / from_temperature

II.   rgba
      ----
      a. value_type, channels
      b. constructors / converting constructors
      c. operator==
      d. is_valid / clamp / luminance
      e. to_rgb / premultiply / to_8bit / to_hex
      f. from_rgb / from_8bit / from_hex / from_hex_string

III.  rgba_premul
      -----------
      a. constructors / converting constructor
      b. operator==
      c. unpremultiply

IV.   FAMILY OPERATIONS (free functions)
      ----------------------------------
      a. luminance / grayscale / invert
      b. contrast_ratio
      c. lerp (rgb, rgba)
      d. premultiply / unpremultiply
      e. blend_over (premultiplied, straight)
      f. from_temperature
*/

#ifndef DJINTERP_COLOR_RGB_HPP_
#define DJINTERP_COLOR_RGB_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.hpp"
#include "./color_rgb.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                          I.   rgb                                       ///
///////////////////////////////////////////////////////////////////////////////

// rgb
//   struct: linear RGB color model. Wraps d_color_rgb with
// constexpr construction and the RGB self-operations.
struct rgb : d_color_rgb
{
    using model_tag  = rgb_tag;
    using value_type = channel_t;

    // rgb (default)
    //   constructor: black.
    D_CONSTEXPR rgb()
        : d_color_rgb{ value_type(0), value_type(0), value_type(0) }
    {}

    // rgb (parameterized)
    //   constructor: from raw channels (no clamping; use clamp()).
    D_CONSTEXPR rgb(
        value_type _r,
        value_type _g,
        value_type _b
    )
        : d_color_rgb{ _r, _g, _b }
    {}

    // rgb (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR rgb(
        const d_color_rgb& _pod
    )
        : d_color_rgb(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const rgb& _other
    ) const
    {
        return ( (r == _other.r) &&
                 (g == _other.g) &&
                 (b == _other.b) );
    }

    // is_valid
    //   query: all channels within [0, 1].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_rgb_is_valid(*this);
    }

    // clamp
    //   transform: channels constrained to [0, 1].
    D_CONSTEXPR_INLINE rgb
    clamp() const
    {
        return d_color_rgb_clamp(*this);
    }

    // luminance
    //   query: Rec. 709 relative luminance.
    D_CONSTEXPR_INLINE value_type
    luminance() const
    {
        return d_color_rgb_luminance(*this);
    }

    // invert
    //   transform: per-channel inversion about 1.0.
    D_CONSTEXPR_INLINE rgb
    invert() const
    {
        return d_color_rgb_invert(*this);
    }

    // grayscale
    //   transform: luminance-preserving desaturation.
    D_CONSTEXPR_INLINE rgb
    grayscale() const
    {
        return d_color_rgb_to_grayscale(*this);
    }

    // to_srgb
    //   transform: linear -> sRGB encoding (runtime; uses pow).
    D_INLINE rgb
    to_srgb() const
    {
        return d_color_rgb_to_srgb(*this);
    }

    // to_8bit
    //   transform: linear -> 8-bit sRGB (runtime).
    D_INLINE d_color_rgb_u8
    to_8bit() const
    {
        return d_color_rgb_to_u8(*this);
    }

    // to_hex
    //   transform: linear -> 0x00RRGGBB sRGB (runtime).
    D_INLINE d_color_rgb_hex
    to_hex() const
    {
        return d_color_rgb_to_hex(*this);
    }

    // from_srgb
    //   factory: sRGB-encoded -> linear (runtime).
    D_STATIC D_INLINE rgb
    from_srgb(
        const rgb& _srgb
    )
    {
        return d_color_rgb_from_srgb(_srgb);
    }

    // from_8bit
    //   factory: 8-bit sRGB -> linear (runtime).
    D_STATIC D_INLINE rgb
    from_8bit(
        unsigned _r,
        unsigned _g,
        unsigned _b
    )
    {
        return d_color_rgb_from_u8(
            d_color_rgb_u8_make(
                (uint8_t)_r,
                (uint8_t)_g,
                (uint8_t)_b
            )
        );
    }

    // from_hex
    //   factory: 0x00RRGGBB sRGB -> linear (runtime).
    D_STATIC D_INLINE rgb
    from_hex(
        d_color_rgb_hex _hex
    )
    {
        return d_color_rgb_from_hex(_hex);
    }

    // from_hex_string
    //   factory: "#RRGGBB" -> linear; returns black on failure
    // (runtime).
    D_STATIC D_INLINE rgb
    from_hex_string(
        const char* _str
    )
    {
        d_color_rgb out = d_color_rgb_make(0.0f, 0.0f, 0.0f);
        d_color_rgb_from_hex_string(_str, &out);

        return out;
    }

    // from_temperature
    //   factory: blackbody color temperature -> linear (runtime).
    D_STATIC D_INLINE rgb
    from_temperature(
        value_type _kelvin
    )
    {
        return d_color_rgb_from_temperature(_kelvin);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                         II.   rgba                                      ///
///////////////////////////////////////////////////////////////////////////////

// rgba
//   struct: linear RGB with straight alpha. Transport/compositing
// type; not a conversion-graph color model.
struct rgba : d_color_rgba
{
    using value_type = channel_t;

    // rgba (default)
    //   constructor: transparent black.
    D_CONSTEXPR rgba()
        : d_color_rgba{ value_type(0), value_type(0),
                        value_type(0), value_type(0) }
    {}

    // rgba (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR rgba(
        value_type _r,
        value_type _g,
        value_type _b,
        value_type _a
    )
        : d_color_rgba{ _r, _g, _b, _a }
    {}

    // rgba (from rgb + alpha)
    //   constructor: attaches alpha to an rgb.
    D_CONSTEXPR rgba(
        const rgb& _rgb,
        value_type _a
    )
        : d_color_rgba{ _rgb.r, _rgb.g, _rgb.b, _a }
    {}

    // rgba (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR rgba(
        const d_color_rgba& _pod
    )
        : d_color_rgba(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const rgba& _other
    ) const
    {
        return ( (r == _other.r) &&
                 (g == _other.g) &&
                 (b == _other.b) &&
                 (a == _other.a) );
    }

    // is_valid
    //   query: all channels within [0, 1].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_rgba_is_valid(*this);
    }

    // clamp
    //   transform: channels constrained to [0, 1].
    D_CONSTEXPR_INLINE rgba
    clamp() const
    {
        return d_color_rgba_clamp(*this);
    }

    // luminance
    //   query: Rec. 709 relative luminance (alpha ignored).
    D_CONSTEXPR_INLINE value_type
    luminance() const
    {
        return d_color_rgba_luminance(*this);
    }

    // to_rgb
    //   transform: discard alpha.
    D_CONSTEXPR_INLINE rgb
    to_rgb() const
    {
        return d_color_rgb_from_rgba(*this);
    }

    // premultiply
    //   transform: straight -> premultiplied alpha.
    D_CONSTEXPR_INLINE d_color_rgba_premul
    premultiply() const
    {
        return d_color_rgba_premultiply(*this);
    }

    // to_8bit
    //   transform: linear -> 8-bit sRGB (runtime).
    D_INLINE d_color_rgba_u8
    to_8bit() const
    {
        return d_color_rgba_to_u8(*this);
    }

    // to_hex
    //   transform: linear -> 0xRRGGBBAA sRGB (runtime).
    D_INLINE d_color_rgba_hex
    to_hex() const
    {
        return d_color_rgba_to_hex(*this);
    }

    // from_rgb
    //   factory: rgb + alpha.
    D_STATIC D_CONSTEXPR_INLINE rgba
    from_rgb(
        const rgb& _rgb,
        value_type _a
    )
    {
        return d_color_rgba_from_rgb(_rgb, _a);
    }

    // from_8bit
    //   factory: 8-bit sRGB -> linear straight alpha (runtime).
    D_STATIC D_INLINE rgba
    from_8bit(
        unsigned _r,
        unsigned _g,
        unsigned _b,
        unsigned _a
    )
    {
        return d_color_rgba_from_u8(
            d_color_rgba_u8_make(
                (uint8_t)_r,
                (uint8_t)_g,
                (uint8_t)_b,
                (uint8_t)_a
            )
        );
    }

    // from_hex
    //   factory: 0xRRGGBBAA sRGB -> linear straight alpha (runtime).
    D_STATIC D_INLINE rgba
    from_hex(
        d_color_rgba_hex _hex
    )
    {
        return d_color_rgba_from_hex(_hex);
    }

    // from_hex_string
    //   factory: "#RRGGBBAA" -> linear; transparent black on failure
    // (runtime).
    D_STATIC D_INLINE rgba
    from_hex_string(
        const char* _str
    )
    {
        d_color_rgba out = d_color_rgba_make(0.0f, 0.0f, 0.0f, 0.0f);
        d_color_rgba_from_hex_string(_str, &out);

        return out;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                      III.   rgba_premul                                 ///
///////////////////////////////////////////////////////////////////////////////

// rgba_premul
//   struct: linear RGB with premultiplied alpha. Compositing
// type; convert to rgba via unpremultiply().
struct rgba_premul : d_color_rgba_premul
{
    using value_type = channel_t;

    // rgba_premul (default)
    //   constructor: transparent black.
    D_CONSTEXPR rgba_premul()
        : d_color_rgba_premul{ value_type(0), value_type(0),
                               value_type(0), value_type(0) }
    {}

    // rgba_premul (parameterized)
    //   constructor: from raw premultiplied channels.
    D_CONSTEXPR rgba_premul(
        value_type _r,
        value_type _g,
        value_type _b,
        value_type _a
    )
        : d_color_rgba_premul{ _r, _g, _b, _a }
    {}

    // rgba_premul (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR rgba_premul(
        const d_color_rgba_premul& _pod
    )
        : d_color_rgba_premul(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const rgba_premul& _other
    ) const
    {
        return ( (r == _other.r) &&
                 (g == _other.g) &&
                 (b == _other.b) &&
                 (a == _other.a) );
    }

    // unpremultiply
    //   transform: premultiplied -> straight alpha.
    D_CONSTEXPR_INLINE rgba
    unpremultiply() const
    {
        return d_color_rgba_unpremultiply(*this);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   FAMILY OPERATIONS (free functions)                    ///
///////////////////////////////////////////////////////////////////////////////

// luminance
//   function: Rec. 709 relative luminance of an rgb.
D_CONSTEXPR_INLINE channel_t
luminance(
    const rgb& _rgb
)
{
    return d_color_rgb_luminance(_rgb);
}

// grayscale
//   function: luminance-preserving desaturation of an rgb.
D_CONSTEXPR_INLINE rgb
grayscale(
    const rgb& _rgb
)
{
    return d_color_rgb_to_grayscale(_rgb);
}

// invert
//   function: per-channel inversion of an rgb.
D_CONSTEXPR_INLINE rgb
invert(
    const rgb& _rgb
)
{
    return d_color_rgb_invert(_rgb);
}

// contrast_ratio
//   function: WCAG contrast ratio between two rgb colors.
D_CONSTEXPR_INLINE channel_t
contrast_ratio(
    const rgb& _a,
    const rgb& _b
)
{
    return d_color_rgb_contrast_ratio(_a, _b);
}

// lerp
//   function: linear interpolation between two rgb colors.
D_CONSTEXPR_INLINE rgb
lerp(
    const rgb& _a,
    const rgb& _b,
    channel_t  _t
)
{
    return d_color_rgb_lerp(_a, _b, _t);
}

// lerp
//   function: linear interpolation between two rgba colors.
D_CONSTEXPR_INLINE rgba
lerp(
    const rgba& _a,
    const rgba& _b,
    channel_t   _t
)
{
    return d_color_rgba_lerp(_a, _b, _t);
}

// premultiply
//   function: straight -> premultiplied alpha.
D_CONSTEXPR_INLINE rgba_premul
premultiply(
    const rgba& _rgba
)
{
    return d_color_rgba_premultiply(_rgba);
}

// unpremultiply
//   function: premultiplied -> straight alpha.
D_CONSTEXPR_INLINE rgba
unpremultiply(
    const rgba_premul& _premul
)
{
    return d_color_rgba_unpremultiply(_premul);
}

// blend_over
//   function: Porter-Duff "over" of premultiplied colors.
D_CONSTEXPR_INLINE rgba_premul
blend_over(
    const rgba_premul& _src,
    const rgba_premul& _dst
)
{
    return d_color_rgba_blend_over(_src, _dst);
}

// blend_over
//   function: Porter-Duff "over" of straight-alpha colors.
D_CONSTEXPR_INLINE rgba
blend_over(
    const rgba& _src,
    const rgba& _dst
)
{
    return d_color_rgba_blend_over_straight(_src, _dst);
}

// from_temperature
//   function: blackbody color temperature -> linear rgb (runtime).
D_INLINE rgb
from_temperature(
    channel_t _kelvin
)
{
    return d_color_rgb_from_temperature(_kelvin);
}


NS_END  // djinterp


#endif  // DJINTERP_COLOR_RGB_HPP_
