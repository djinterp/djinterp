/*******************************************************************************
* djinterp [core]                                                 color_cmyk.hpp
*
*   C++ ergonomic layer for the CMYK color model. `cmyk` derives from the
* shared-kernel POD (color_cmyk.h) without adding state, providing constexpr
* construction, comparison, validation, and clamping that forward to the C
* kernel. Conversions are supplied by the color_convert facade.
*
*
* path:      /inc/djinterp/core/util/color/color_cmyk.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cmyk
      ----
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c.    operator==
            d. is_valid / clamp
*/

#ifndef DJINTERP_UTIL_COLOR_COLOR_CMYK_HPP
#define DJINTERP_UTIL_COLOR_COLOR_CMYK_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/color/color_cmyk.h"
#include "./color_common.hpp"


NS_DJINTERP


// cmyk
//   struct: CMYK color model. Wraps d_color_cmyk with constexpr
// construction and self-operations.
struct cmyk : d_color_cmyk
{
    using model_tag  = cmyk_tag;
    using value_type = channel_t;

    // cmyk (default)
    //   constructor: zeroed (white).
    D_CONSTEXPR cmyk()
        : d_color_cmyk{ value_type(0), value_type(0),
                        value_type(0), value_type(0) }
    {}

    // cmyk (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR cmyk(
        value_type _c,
        value_type _m,
        value_type _y,
        value_type _k
    )
        : d_color_cmyk{ _c, _m, _y, _k }
    {}

    // cmyk (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR cmyk(
        const d_color_cmyk& _pod
    )
        : d_color_cmyk(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
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

    // is_valid
    //   query: all channels within [0, 1].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_cmyk_is_valid(*this);
    }

    // clamp
    //   transform: channels constrained to [0, 1].
    D_CONSTEXPR_INLINE cmyk
    clamp() const
    {
        return d_color_cmyk_clamp(*this);
    }
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_COLOR_CMYK_HPP
