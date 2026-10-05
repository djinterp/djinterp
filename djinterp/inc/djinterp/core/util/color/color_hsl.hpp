/*******************************************************************************
* djinterp [core]                                                  color_hsl.hpp
*
*   C++ ergonomic layer for the HSL color model. `hsl` derives from the
* shared-kernel POD (color_hsl.h) without adding state, providing constexpr
* construction, comparison, validation, and canonicalizing clamp that
* forward to the C kernel. Conversions are supplied by the color_convert
* facade.
*
*
* path:      /inc/djinterp/core/util/color/color_hsl.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    hsl
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c.    operator==
            d. is_valid / clamp
*/

#ifndef DJINTERP_UTIL_COLOR_COLOR_HSL_HPP
#define DJINTERP_UTIL_COLOR_COLOR_HSL_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/color/color_hsl.h"
#include "./color_common.hpp"


NS_DJINTERP


// hsl
//   struct: HSL color model. Wraps d_color_hsl with constexpr
// construction and self-operations.
struct hsl : d_color_hsl
{
    using model_tag  = hsl_tag;
    using value_type = channel_t;

    // hsl (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR hsl()
        : d_color_hsl{ value_type(0), value_type(0), value_type(0) }
    {}

    // hsl (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR hsl(
        value_type _h,
        value_type _s,
        value_type _l
    )
        : d_color_hsl{ _h, _s, _l }
    {}

    // hsl (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR hsl(
        const d_color_hsl& _pod
    )
        : d_color_hsl(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const hsl& _other
    ) const
    {
        return ( (h == _other.h) &&
                 (s == _other.s) &&
                 (l == _other.l) );
    }

    // is_valid
    //   query: h in [0, 360), s and l in [0, 1].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_hsl_is_valid(*this);
    }

    // clamp
    //   transform: wrap hue, clamp s and l to [0, 1].
    D_CONSTEXPR_INLINE hsl
    clamp() const
    {
        return d_color_hsl_clamp(*this);
    }
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_COLOR_HSL_HPP
