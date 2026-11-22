/*******************************************************************************
* djinterp [cpp][utility]                                          color_hsv.hpp
*
*   C++ ergonomic layer for the HSV color model. `hsv` derives from the
* shared-kernel POD (color_hsv.h) without adding state, providing constexpr
* construction, comparison, validation, and canonicalizing clamp that
* forward to the C kernel. Conversions are supplied by the color_convert
* facade.
*
*
* path:      /inc/djinterp/cpp/util/color/color_hsv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    hsv
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c.    operator==, operator!=
            d. is_valid / clamp
*/

#ifndef DJINTERP_UTIL_COLOR_HSV_HPP
#define DJINTERP_UTIL_COLOR_HSV_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../../../c/util/color/color_hsv.h"  // struct dj_color_hsv
#include "./color_common.hpp"  // channel_t, model tags


NS_DJINTERP


// hsv
//   struct: HSV color model. Wraps dj_color_hsv with constexpr
// construction and self-operations.
struct hsv : dj_color_hsv
{
    using model_tag  = hsv_tag;
    using value_type = channel_t;

    // hsv (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR hsv()
        : dj_color_hsv{ value_type(0), value_type(0), value_type(0) }
    {}

    // hsv (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR hsv(
        value_type _h,
        value_type _s,
        value_type _v
    )
        : dj_color_hsv{ _h, _s, _v }
    {}

    // hsv (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR hsv(
        const dj_color_hsv& _pod
    )
        : dj_color_hsv(_pod)
    {}

    // operator==
    //   compare: exact channel equality. A hidden friend rather than a
    // member, so either operand may be the C struct, converted, at every
    // level; found only when one operand is a hsv.
    friend D_CONSTEXPR
    bool operator==(
        const hsv& _lhs,
        const hsv& _rhs
    )
    {
        return ( (_lhs.h == _rhs.h) &&
                 (_lhs.s == _rhs.s) &&
                 (_lhs.v == _rhs.v) );
    }

    // operator!=
    //   compare: the negation of operator==. Declared, so that it exists at
    // every supported level; C++20 would otherwise rewrite a != b from ==,
    // and earlier levels would not compile it at all.
    friend D_CONSTEXPR
    bool operator!=(
        const hsv& _lhs,
        const hsv& _rhs
    )
    {
        return !(_lhs == _rhs);
    }

    // is_valid
    //   query: h in [0, 360), s and v in [0, 1].
    D_CONSTEXPR_INLINE
    bool is_valid() const
    {
        return d_color_hsv_is_valid(*this);
    }

    // clamp
    //   transform: wrap hue, clamp s and v to [0, 1].
    D_CONSTEXPR_INLINE
    hsv clamp() const
    {
        return d_color_hsv_clamp(*this);
    }
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_HSV_HPP
