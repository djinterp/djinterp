/******************************************************************************
* djinterp [color]                                              color_cmyk.hpp
*
*   CMYK color model for the djinterp color module. Channels c, m, y, k
* are stored as normalized channel_t values in [0, 1]. Commonly used for
* print-oriented color representation.
*
* 
* path:      /inc/djinterp/util/color/color_cmyk.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CMYK COLOR MODEL
      -----------------
      i.    cmyk
            a. model_tag, value_type
            b. channels: c, m, y, k
            c. cmyk()                       (default constructor)
            d. cmyk(_c, _m, _y, _k)        (parameterized constructor)
            e. from_percentage              (static factory)
            f. operator==
*/

#ifndef DJINTERP_COLOR_CMYK_
#define DJINTERP_COLOR_CMYK_ 1

#include "color.hpp"


NS_DJINTERP
NS_COLOR


// ================================================================
//  cmyk
// ================================================================

// cmyk
//   struct: represents a color in the CMYK color model with
// all channels normalized to [0, 1]. Does not carry alpha;
// CMYK is a subtractive model typically used for print where
// alpha compositing is uncommon.
struct cmyk
{
    using model_tag  = cmyk_tag;
    using value_type = channel_t;

    value_type c;
    value_type m;
    value_type y;
    value_type k;

    // cmyk (default)
    //   constructor: initializes to white (all channels zero,
    // key zero).
    D_CONSTEXPR cmyk()
        : c(0), m(0), y(0), k(0)
    {}

    // cmyk (parameterized)
    //   constructor: initializes from individual channel
    // values, each clamped to [0, 1].
    D_CONSTEXPR cmyk(
        value_type _c,
        value_type _m,
        value_type _y,
        value_type _k
    )
        : c(clamp_channel(_c, value_type(0), value_type(1))),
          m(clamp_channel(_m, value_type(0), value_type(1))),
          y(clamp_channel(_y, value_type(0), value_type(1))),
          k(clamp_channel(_k, value_type(0), value_type(1)))
    {}

    // from_percentage
    //   function: constructs a cmyk from percentage values
    // [0, 100].
    D_STATIC_CONSTEXPR_INLINE cmyk
    from_percentage(
        value_type _c,
        value_type _m,
        value_type _y,
        value_type _k
    )
    {
        return cmyk(_c / value_type(100),
                    _m / value_type(100),
                    _y / value_type(100),
                    _k / value_type(100));
    }

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const cmyk& _other
    ) const
    {
        return ( (c == _other.c) &&
                 (m == _other.m) &&
                 (y == _other.y) &&
                 (k == _other.k) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_CMYK_
