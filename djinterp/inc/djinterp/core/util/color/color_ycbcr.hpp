/******************************************************************************
* djinterp [color]                                              color_ycbcr.hpp
*
*   C++ ergonomic layer for the YCbCr color model (full-range BT.601).
* `ycbcr` derives from the shared-kernel POD (color_ycbcr.h) without adding
* state, providing constexpr construction, comparison, validation, and
* clamping that forward to the C kernel. Conversions are supplied by the
* color_convert facade.
*
*
* path:      /inc/djinterp/util/color/color_ycbcr.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ycbcr
      -----
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c. operator==
      d. is_valid / clamp
*/

#ifndef DJINTERP_COLOR_YCBCR_HPP_
#define DJINTERP_COLOR_YCBCR_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.hpp"
#include "./color_ycbcr.h"


NS_DJINTERP


// ycbcr
//   struct: YCbCr color model. Wraps d_color_ycbcr with constexpr
// construction and self-operations.
struct ycbcr : d_color_ycbcr
{
    using model_tag  = ycbcr_tag;
    using value_type = channel_t;

    // ycbcr (default)
    //   constructor: zeroed.
    D_CONSTEXPR ycbcr()
        : d_color_ycbcr{ value_type(0), value_type(0), value_type(0) }
    {}

    // ycbcr (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR ycbcr(
        value_type _y,
        value_type _cb,
        value_type _cr
    )
        : d_color_ycbcr{ _y, _cb, _cr }
    {}

    // ycbcr (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR ycbcr(
        const d_color_ycbcr& _pod
    )
        : d_color_ycbcr(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const ycbcr& _other
    ) const
    {
        return ( (y  == _other.y)  &&
                 (cb == _other.cb) &&
                 (cr == _other.cr) );
    }

    // is_valid
    //   query: Y in [0, 1], Cb and Cr in [-0.5, 0.5].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_ycbcr_is_valid(*this);
    }

    // clamp
    //   transform: Y to [0, 1], Cb and Cr to [-0.5, 0.5].
    D_CONSTEXPR_INLINE ycbcr
    clamp() const
    {
        return d_color_ycbcr_clamp(*this);
    }
};


NS_END  // djinterp


#endif  // DJINTERP_COLOR_YCBCR_HPP_
