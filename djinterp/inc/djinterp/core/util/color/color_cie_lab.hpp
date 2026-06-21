/******************************************************************************
* djinterp [color]                                            color_cie_lab.hpp
*
*   C++ ergonomic layer for the CIE XYZ and CIE L*a*b* color models. Both
* wrappers derive from their shared-kernel PODs (color_lab.h) without adding
* state. XYZ is the device-independent hub through which L*a*b* converts, so
* the two types are grouped together here. Construction, comparison,
* validation, and clamping forward to the C kernel; conversions are supplied
* by the color_convert facade.
*
*
* path:      /inc/djinterp/util/color/color_cie_lab.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cie_xyz
      -------
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c. operator==
      d. is_valid / clamp

II.   cie_lab
      -------
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c. operator==
      d. is_valid / clamp
*/

#ifndef DJINTERP_COLOR_CIE_LAB_HPP_
#define DJINTERP_COLOR_CIE_LAB_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.hpp"
#include "./color_lab.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                        I.   cie_xyz                                     ///
///////////////////////////////////////////////////////////////////////////////

// cie_xyz
//   struct: CIE 1931 XYZ color model. Wraps d_color_xyz with
// constexpr construction and self-operations.
struct cie_xyz : d_color_xyz
{
    using model_tag  = cie_xyz_tag;
    using value_type = channel_t;

    // cie_xyz (default)
    //   constructor: zeroed.
    D_CONSTEXPR cie_xyz()
        : d_color_xyz{ value_type(0), value_type(0), value_type(0) }
    {}

    // cie_xyz (parameterized)
    //   constructor: from raw tristimulus values.
    D_CONSTEXPR cie_xyz(
        value_type _x,
        value_type _y,
        value_type _z
    )
        : d_color_xyz{ _x, _y, _z }
    {}

    // cie_xyz (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR cie_xyz(
        const d_color_xyz& _pod
    )
        : d_color_xyz(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const cie_xyz& _other
    ) const
    {
        return ( (x == _other.x) &&
                 (y == _other.y) &&
                 (z == _other.z) );
    }

    // is_valid
    //   query: all tristimulus values non-negative.
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_xyz_is_valid(*this);
    }

    // clamp
    //   transform: tristimulus values made non-negative.
    D_CONSTEXPR_INLINE cie_xyz
    clamp() const
    {
        return d_color_xyz_clamp(*this);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                        II.   cie_lab                                    ///
///////////////////////////////////////////////////////////////////////////////

// cie_lab
//   struct: CIE L*a*b* color model. Wraps d_color_lab with
// constexpr construction and self-operations.
struct cie_lab : d_color_lab
{
    using model_tag  = cie_lab_tag;
    using value_type = channel_t;

    // cie_lab (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR cie_lab()
        : d_color_lab{ value_type(0), value_type(0), value_type(0) }
    {}

    // cie_lab (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR cie_lab(
        value_type _l,
        value_type _a,
        value_type _b
    )
        : d_color_lab{ _l, _a, _b }
    {}

    // cie_lab (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR cie_lab(
        const d_color_lab& _pod
    )
        : d_color_lab(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const cie_lab& _other
    ) const
    {
        return ( (l == _other.l) &&
                 (a == _other.a) &&
                 (b == _other.b) );
    }

    // is_valid
    //   query: L* within [0, 100].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_lab_is_valid(*this);
    }

    // clamp
    //   transform: L* to [0, 100], a*/b* to [-128, 127].
    D_CONSTEXPR_INLINE cie_lab
    clamp() const
    {
        return d_color_lab_clamp(*this);
    }
};


NS_END  // djinterp


#endif  // DJINTERP_COLOR_CIE_LAB_HPP_
