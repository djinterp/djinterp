/******************************************************************************
* djinterp [color]                                             color_ycbcr.hpp
*
*   YCbCr color model for the djinterp color module. Luma (Y) is in
* [0, 1]; chroma-blue (Cb) and chroma-red (Cr) are in [-0.5, 0.5].
* Follows the ITU-R BT.601 convention by default.
*
* 
* path:      /inc/djinterp/util/color/color_ycbcr.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    YCBCR COLOR MODEL
      ------------------
      i.    ycbcr
            a. model_tag, value_type
            b. channels: y, cb, cr
            c. ycbcr()                      (default constructor)
            d. ycbcr(_y, _cb, _cr)          (parameterized constructor)
            e. from_8bit                     (static factory)
            f. operator==
*/

#ifndef DJINTERP_COLOR_YCBCR_
#define DJINTERP_COLOR_YCBCR_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


NS_DJINTERP
NS_COLOR


// ================================================================
//  ycbcr
// ================================================================

// ycbcr
//   struct: represents a color in the YCbCr color model.
// Luma (Y) is normalized to [0, 1]. Chroma-blue (Cb) and
// chroma-red (Cr) are in the signed range [-0.5, 0.5].
// Does not carry alpha, as YCbCr is primarily a signal-level
// encoding.
struct ycbcr
{
    using model_tag  = ycbcr_tag;
    using value_type = channel_t;

    value_type y;
    value_type cb;
    value_type cr;

    // ycbcr (default)
    //   constructor: initializes to black (y=0, cb=0, cr=0).
    D_CONSTEXPR ycbcr()
        : y(0), cb(0), cr(0)
    {}

    // ycbcr (parameterized)
    //   constructor: initializes from individual channel
    // values. Y is clamped to [0, 1]; Cb and Cr are clamped
    // to [-0.5, 0.5].
    D_CONSTEXPR ycbcr(
        value_type _y,
        value_type _cb,
        value_type _cr
    )
        : y (clamp_channel(_y,  value_type(0),    value_type(1))),
          cb(clamp_channel(_cb, value_type(-0.5), value_type(0.5))),
          cr(clamp_channel(_cr, value_type(-0.5), value_type(0.5)))
    {}

    // from_8bit
    //   function: constructs a ycbcr from 8-bit studio-swing
    // values where Y is in [16, 235] and Cb/Cr are in
    // [16, 240] with 128 as the zero-chroma midpoint.
    D_STATIC_CONSTEXPR_INLINE ycbcr
    from_8bit(
        unsigned _y,
        unsigned _cb,
        unsigned _cr
    )
    {
        return ycbcr(
            (static_cast<value_type>(_y)  - value_type(16))
                / value_type(219),
            (static_cast<value_type>(_cb) - value_type(128))
                / value_type(224),
            (static_cast<value_type>(_cr) - value_type(128))
                / value_type(224)
        );
    }

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const ycbcr& _other
    ) const
    {
        return ( (y  == _other.y)  &&
                 (cb == _other.cb) &&
                 (cr == _other.cr) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_YCBCR_
