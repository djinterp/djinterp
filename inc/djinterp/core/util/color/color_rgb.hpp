/******************************************************************************
* djinterp [color]                                              color_rgb.hpp
*
*   RGB color model for the djinterp color module. Channels are stored
* as normalized channel_t values in [0, 1] with optional alpha. Provides
* constexpr construction from both normalized and 8-bit integer values.
*
* 
* path:      /inc/djinterp/util/color/color_rgb.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RGB COLOR MODEL
      ----------------
      i.    rgb
            a. model_tag, value_type
            b. channels: r, g, b, a
            c. rgb()                    (default constructor)
            d. rgb(_r, _g, _b, _a)     (parameterized constructor)
            e. from_8bit               (static factory)
            f. to_8bit_r, to_8bit_g, to_8bit_b, to_8bit_a
            g. operator==
*/

#ifndef DJINTERP_COLOR_RGB_
#define DJINTERP_COLOR_RGB_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


NS_DJINTERP
NS_COLOR


// ================================================================
//  rgb
// ================================================================

// rgb
//   struct: represents a color in the RGB color model with
// channels normalized to the [0, 1] range. Fully constexpr-
// constructible for compile-time color expressions.
struct rgb
{
    using model_tag  = rgb_tag;
    using value_type = channel_t;

    value_type r;
    value_type g;
    value_type b;
    value_type a;

    // rgb (default)
    //   constructor: initializes to opaque black.
    D_CONSTEXPR rgb()
        : r(0), g(0), b(0), a(1)
    {}

    // rgb (parameterized)
    //   constructor: initializes from individual channel
    // values, clamped to [0, 1].
    D_CONSTEXPR rgb(
        value_type _r,
        value_type _g,
        value_type _b,
        value_type _a = 1
    )
        : r(clamp_channel(_r, value_type(0), value_type(1))),
          g(clamp_channel(_g, value_type(0), value_type(1))),
          b(clamp_channel(_b, value_type(0), value_type(1))),
          a(clamp_channel(_a, value_type(0), value_type(1)))
    {}

    // from_8bit
    //   function: constructs an rgb from 8-bit unsigned
    // integer channels [0, 255].
    D_STATIC_CONSTEXPR_INLINE rgb
    from_8bit(
        unsigned _r,
        unsigned _g,
        unsigned _b,
        unsigned _a = 255
    )
    {
        return rgb(
            static_cast<value_type>(_r) / value_type(255),
            static_cast<value_type>(_g) / value_type(255),
            static_cast<value_type>(_b) / value_type(255),
            static_cast<value_type>(_a) / value_type(255)
        );
    }

    // to_8bit_r
    //   function: returns the red channel as an 8-bit value.
    D_CONSTEXPR_INLINE unsigned
    to_8bit_r() const
    {
        return static_cast<unsigned>(
            r * value_type(255) + value_type(0.5)
        );
    }

    // to_8bit_g
    //   function: returns the green channel as an 8-bit value.
    D_CONSTEXPR_INLINE unsigned
    to_8bit_g() const
    {
        return static_cast<unsigned>(
            g * value_type(255) + value_type(0.5)
        );
    }

    // to_8bit_b
    //   function: returns the blue channel as an 8-bit value.
    D_CONSTEXPR_INLINE unsigned
    to_8bit_b() const
    {
        return static_cast<unsigned>(
            b * value_type(255) + value_type(0.5)
        );
    }

    // to_8bit_a
    //   function: returns the alpha channel as an 8-bit value.
    D_CONSTEXPR_INLINE unsigned
    to_8bit_a() const
    {
        return static_cast<unsigned>(
            a * value_type(255) + value_type(0.5)
        );
    }

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const rgb& _other
    ) const
    {
        return ( (r == _other.r) &&
                 (g == _other.g) &&
                 (b == _other.b) &&
                 (a == _other.a) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_RGB_
