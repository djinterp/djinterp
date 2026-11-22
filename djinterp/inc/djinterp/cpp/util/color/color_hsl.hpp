/*******************************************************************************
* djinterp [cpp][utility]                                          color_hsl.hpp
*
*   C++ ergonomic layer for the HSL color model. `hsl` derives from the
* shared-kernel POD (color_hsl.h) without adding state, providing constexpr
* construction, comparison, validation, and canonicalizing clamp that
* forward to the C kernel. Conversions are supplied by the color_convert
* facade.
*
*
* path:      /inc/djinterp/cpp/util/color/color_hsl.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    hsl
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c.    operator==, operator!=
            d. is_valid / clamp
*/

#ifndef DJINTERP_UTIL_COLOR_HSL_HPP
#define DJINTERP_UTIL_COLOR_HSL_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../../../c/util/color/color_hsl.h"  // struct dj_color_hsl
#include "./color_common.hpp"  // channel_t, model tags


NS_DJINTERP


// hsl
//   struct: HSL color model. Wraps dj_color_hsl with constexpr
// construction and self-operations.
struct hsl : dj_color_hsl
{
    using model_tag  = hsl_tag;
    using value_type = channel_t;

    // hsl (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR hsl()
        : dj_color_hsl{ value_type(0), value_type(0), value_type(0) }
    {}

    // hsl (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR hsl(
        value_type _h,
        value_type _s,
        value_type _l
    )
        : dj_color_hsl{ _h, _s, _l }
    {}

    // hsl (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR hsl(
        const dj_color_hsl& _pod
    )
        : dj_color_hsl(_pod)
    {}

    // operator==
    //   compare: exact channel equality. A hidden friend rather than a
    // member, so either operand may be the C struct, converted, at every
    // level; found only when one operand is a hsl.
    friend D_CONSTEXPR
    bool operator==(
        const hsl& _lhs,
        const hsl& _rhs
    )
    {
        return ( (_lhs.h == _rhs.h) &&
                 (_lhs.s == _rhs.s) &&
                 (_lhs.l == _rhs.l) );
    }

    // operator!=
    //   compare: the negation of operator==. Declared, so that it exists at
    // every supported level; C++20 would otherwise rewrite a != b from ==,
    // and earlier levels would not compile it at all.
    friend D_CONSTEXPR
    bool operator!=(
        const hsl& _lhs,
        const hsl& _rhs
    )
    {
        return !(_lhs == _rhs);
    }

    // is_valid
    //   query: h in [0, 360), s and l in [0, 1].
    D_CONSTEXPR_INLINE
    bool is_valid() const
    {
        return d_color_hsl_is_valid(*this);
    }

    // clamp
    //   transform: wrap hue, clamp s and l to [0, 1].
    D_CONSTEXPR_INLINE
    hsl clamp() const
    {
        return d_color_hsl_clamp(*this);
    }
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_HSL_HPP
