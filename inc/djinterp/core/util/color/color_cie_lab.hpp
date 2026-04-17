/******************************************************************************
* djinterp [color]                                           color_cie_lab.hpp
*
*   CIE L*a*b* color model for the djinterp color module. Lightness (L)
* is in [0, 100]; a* and b* are unbounded but practically fall within
* approximately [-128, 127]. Uses the D65 standard illuminant as the
* default white point.
* 
* 
* path:      /inc/djinterp/util/color/color_cie_lab.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    D65 WHITE POINT CONSTANTS
      --------------------------
      a. d65_x, d65_y, d65_z

II.   CIE XYZ INTERMEDIATE TYPE
      --------------------------
      i.    cie_xyz
            a. x, y, z
            b. cie_xyz()                (default constructor)
            c. cie_xyz(_x, _y, _z)     (parameterized constructor)

III.  CIE L*A*B* COLOR MODEL
      ------------------------
      i.    cie_lab
            a. model_tag, value_type
            b. channels: l, a, b
            c. cie_lab()                    (default constructor)
            d. cie_lab(_l, _a, _b)          (parameterized constructor)
            e. operator==
*/

#ifndef DJINTERP_COLOR_CIE_LAB_
#define DJINTERP_COLOR_CIE_LAB_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


NS_DJINTERP
NS_COLOR


///////////////////////////////////////////////////////////////////////////////
///              I.   D65 WHITE POINT CONSTANTS                             ///
///////////////////////////////////////////////////////////////////////////////

// d65_x
//   constant: X tristimulus value of the CIE D65 standard
// illuminant (2-degree observer).
D_STATIC_CONSTEXPR channel_t d65_x = channel_t(0.95047);

// d65_y
//   constant: Y tristimulus value of the CIE D65 standard
// illuminant (2-degree observer). Defined as 1.0 by
// convention.
D_STATIC_CONSTEXPR channel_t d65_y = channel_t(1.0);

// d65_z
//   constant: Z tristimulus value of the CIE D65 standard
// illuminant (2-degree observer).
D_STATIC_CONSTEXPR channel_t d65_z = channel_t(1.08883);


///////////////////////////////////////////////////////////////////////////////
///             II.   CIE XYZ INTERMEDIATE TYPE                             ///
///////////////////////////////////////////////////////////////////////////////

// cie_xyz
//   struct: CIE 1931 XYZ tristimulus values. Used as the
// intermediate color space for conversions between RGB and
// CIE L*a*b*. Not a full "color model" in the djinterp
// sense (no model_tag); it is a conversion waypoint.
struct cie_xyz
{
    using value_type = channel_t;

    value_type x;
    value_type y;
    value_type z;

    // cie_xyz (default)
    //   constructor: initializes to the origin (black).
    D_CONSTEXPR cie_xyz()
        : x(0), y(0), z(0)
    {}

    // cie_xyz (parameterized)
    //   constructor: initializes from individual tristimulus
    // values. No clamping; XYZ values are unbounded.
    D_CONSTEXPR cie_xyz(
        value_type _x,
        value_type _y,
        value_type _z
    )
        : x(_x), y(_y), z(_z)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///           III.   CIE L*A*B* COLOR MODEL                                ///
///////////////////////////////////////////////////////////////////////////////

// cie_lab
//   struct: represents a color in the CIE L*a*b* perceptual
// color space. L* (lightness) is in [0, 100]; a* and b* are
// nominally unbounded but typically within [-128, 127].
struct cie_lab
{
    using model_tag  = cie_lab_tag;
    using value_type = channel_t;

    value_type l;
    value_type a;
    value_type b;

    // cie_lab (default)
    //   constructor: initializes to black (L*=0, a*=0, b*=0).
    D_CONSTEXPR cie_lab()
        : l(0), a(0), b(0)
    {}

    // cie_lab (parameterized)
    //   constructor: initializes from individual L*a*b*
    // values. L* is clamped to [0, 100]; a* and b* are
    // unclamped.
    D_CONSTEXPR cie_lab(
        value_type _l,
        value_type _a,
        value_type _b
    )
        : l(clamp_channel(_l, value_type(0), value_type(100))),
          a(_a),
          b(_b)
    {}

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const cie_lab& _other
    ) const
    {
        return ( (l == _other.l) &&
                 (a == _other.a) &&
                 (b == _other.b) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_CIE_LAB_
