/*******************************************************************************
* djinterp [core]                                                  color_rgb.hpp
*
*   C++ ergonomic layer for the RGB color model. `rgb` derives from the
* shared-kernel POD (color_rgb.h) without adding state, providing constexpr
* construction, comparison, validation, and clamping that forward to the C
* kernel. `rgba` is the straight-alpha TRANSPORT type: it deliberately
* carries no model_tag, so it is not a conversion-graph model. Conversions
* are supplied by the color_convert facade.
*
*   NOTE (why this file exists): the snapshot under test shipped the PDF
* VALIDATION SHIM of color_rgb.hpp - a standalone `struct rgb` that does not
* derive from d_color_rgb and cannot be constructed from one. color_convert.hpp
* (the production facade) requires the production shape, because every kernel
* dispatch returns a POD that must re-wrap:
*     D_CONSTEXPR_INLINE rgb to_rgb(const hsl& _c)
*     { return d_color_convert_hsl_to_rgb(_c); }   // d_color_rgb -> rgb
* This header restores that shape, mirroring color_cmyk.hpp exactly.
*
*
* path:      /inc/djinterp/core/util/color/color_rgb.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    rgb
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c.    operator==
            d. is_valid / clamp

II.   rgba
      ----
      a. value_type, channels (no model_tag - transport type)
      b. constructors / converting constructor
      c.    operator==
            d. is_valid / clamp / to_rgb

III.  interpolation
      -------------
      a. lerp (rgb, rgba)
*/

#ifndef DJINTERP_UTIL_COLOR_COLOR_RGB_HPP
#define DJINTERP_UTIL_COLOR_COLOR_RGB_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/color/color_rgb.h"
#include "./color_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                          I.   rgb                                       ///
///////////////////////////////////////////////////////////////////////////////

// rgb
//   struct: linear RGB color model. Wraps d_color_rgb with constexpr
// construction and self-operations.
struct rgb : d_color_rgb
{
    using model_tag  = rgb_tag;
    using value_type = channel_t;

    // rgb (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR rgb()
        : d_color_rgb{ value_type(0), value_type(0), value_type(0) }
    {}

    // rgb (parameterized)
    //   constructor: from raw channels.
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
};


///////////////////////////////////////////////////////////////////////////////
///                         II.   rgba                                      ///
///////////////////////////////////////////////////////////////////////////////

// rgba
//   struct: linear RGB with straight alpha. A TRANSPORT type - it exposes no
// model_tag, so is_color_model<rgba>::value is false and it does not
// participate in the conversion graph.
struct rgba : d_color_rgba
{
    using value_type = channel_t;

    // rgba (default)
    //   constructor: zeroed (transparent black).
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
    //   constructor: attaches straight alpha to a color model.
    D_CONSTEXPR rgba(
        const rgb& _c,
        value_type _a
    )
        : d_color_rgba{ _c.r, _c.g, _c.b, _a }
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

    // to_rgb
    //   transform: drops the alpha channel.
    D_CONSTEXPR_INLINE rgb
    to_rgb() const
    {
        return rgb(r, g, b);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                        III.   interpolation                             ///
///////////////////////////////////////////////////////////////////////////////

// lerp
//   function: linear interpolation per channel, _a at _t = 0 to _b at _t = 1,
// forwarding to the C kernel. Alpha interpolates with the colour channels.
D_NODISCARD D_CONSTEXPR_INLINE rgb
lerp(
    const rgb& _a,
    const rgb& _b,
    channel_t  _t
)
{
    return d_color_rgb_lerp(_a, _b, _t);
}

D_NODISCARD D_CONSTEXPR_INLINE rgba
lerp(
    const rgba& _a,
    const rgba& _b,
    channel_t   _t
)
{
    return d_color_rgba_lerp(_a, _b, _t);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_COLOR_RGB_HPP
