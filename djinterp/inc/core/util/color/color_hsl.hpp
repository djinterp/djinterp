/******************************************************************************
* djinterp [color]                                              color_hsl.hpp
*
*   HSL (Hue, Saturation, Lightness) color model for the djinterp color
* module. Hue is stored in degrees [0, 360), saturation and lightness
* in [0, 1], with optional alpha.
*
* 
* path:      /inc/djinterp/util/color/color_hsl.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HSL COLOR MODEL
      ----------------
      i.    hsl
            a. model_tag, value_type
            b. channels: h, s, l, a
            c. hsl()                    (default constructor)
            d. hsl(_h, _s, _l, _a)     (parameterized constructor)
            e. operator==
*/

#ifndef DJINTERP_COLOR_HSL_
#define DJINTERP_COLOR_HSL_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


NS_DJINTERP
NS_COLOR


// ================================================================
//  normalize_hue  (internal)
// ================================================================

NS_INTERNAL

    // normalize_hue
    //   function: wraps a hue value into the half-open
    // interval [0, 360). Handles negative input.
    D_CONSTEXPR_INLINE channel_t
    normalize_hue(
        channel_t _h
    )
    {
        // bring into (-360, 360) first
        _h = _h - static_cast<int>(_h / channel_t(360))
                 * channel_t(360);

        // ensure non-negative
        return (_h < channel_t(0))
             ? (_h + channel_t(360))
             : _h;
    }

NS_END  // internal


// ================================================================
//  hsl
// ================================================================

// hsl
//   struct: represents a color in the HSL color model. Hue is
// in degrees [0, 360); saturation and lightness are normalized
// to [0, 1]. Carries optional alpha in [0, 1].
struct hsl
{
    using model_tag  = hsl_tag;
    using value_type = channel_t;

    value_type h;
    value_type s;
    value_type l;
    value_type a;

    // hsl (default)
    //   constructor: initializes to opaque black (h=0, s=0,
    // l=0, a=1).
    D_CONSTEXPR hsl()
        : h(0), s(0), l(0), a(1)
    {}

    // hsl (parameterized)
    //   constructor: initializes from individual channel
    // values. Hue is normalized to [0, 360); saturation,
    // lightness, and alpha are clamped to [0, 1].
    D_CONSTEXPR hsl(
        value_type _h,
        value_type _s,
        value_type _l,
        value_type _a = 1
    )
        : h(internal::normalize_hue(_h)),
          s(clamp_channel(_s, value_type(0), value_type(1))),
          l(clamp_channel(_l, value_type(0), value_type(1))),
          a(clamp_channel(_a, value_type(0), value_type(1)))
    {}

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const hsl& _other
    ) const
    {
        return ( (h == _other.h) &&
                 (s == _other.s) &&
                 (l == _other.l) &&
                 (a == _other.a) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_HSL_
