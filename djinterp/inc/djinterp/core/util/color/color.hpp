/******************************************************************************
* djinterp [color]                                                    color.hpp
*
*   Umbrella header for the djinterp color module (C++). Including this one
* header brings in every color model wrapper, the conversion facade, the
* shared C kernel, and the cross-model operations. It is the single entry
* point most C++ users want.
*
*   Two forms of polymorphism are provided over the same kernel:
*
*     - Compile-time: the color_cast / color_convert template dispatch (from
*       color_convert.hpp) selects conversions statically with no runtime
*       cost and constexpr support where the math allows.
*
*     - Runtime: the abstract `color` base with `color_value<Model>` lets
*       heterogeneous colors be stored and manipulated behind one interface,
*       each delegating to the same kernel via color_cast.
*
*
* path:      /inc/djinterp/util/color/color.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    MODULE INCLUDES

II.   CROSS-MODEL OPERATIONS (free functions)
      --------------------------------------
      a. adjust_saturation
      b. adjust_brightness
      c. rotate_hue
      d. delta_e (cie_lab, cie_lab)
      e. delta_e (rgb, rgb)

III.  RUNTIME POLYMORPHISM
      --------------------
      a. color                 (abstract base)
      b. color_value<_Model>   (concrete holder)
      c. make_color            (factory)
*/

#ifndef DJINTERP_COLOR_HPP_
#define DJINTERP_COLOR_HPP_ 1


///////////////////////////////////////////////////////////////////////////////
///                     I.   MODULE INCLUDES                                ///
///////////////////////////////////////////////////////////////////////////////

#include "../../djinterp.hpp"

#include "./color_common.hpp"
#include "./color_rgb.hpp"
#include "./color_cmyk.hpp"
#include "./color_hsv.hpp"
#include "./color_hsl.hpp"
#include "./color_ycbcr.hpp"
#include "./color_cie_lab.hpp"
#include "./color_convert.hpp"

#include "./color.h"

#include <memory>


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///            II.   CROSS-MODEL OPERATIONS (free functions)                ///
///////////////////////////////////////////////////////////////////////////////

// adjust_saturation
//   function: scales an rgb color's saturation in HSL space
// (1.0 = unchanged, 0.0 = grayscale).
D_CONSTEXPR_INLINE rgb
adjust_saturation(
    const rgb& _rgb,
    channel_t  _amount
)
{
    return d_color_rgb_adjust_saturation(_rgb, _amount);
}

// adjust_brightness
//   function: scales an rgb color's lightness in HSL space
// (1.0 = unchanged, 0.0 = black).
D_CONSTEXPR_INLINE rgb
adjust_brightness(
    const rgb& _rgb,
    channel_t  _amount
)
{
    return d_color_rgb_adjust_brightness(_rgb, _amount);
}

// rotate_hue
//   function: rotates an rgb color's hue by signed degrees in HSL
// space.
D_CONSTEXPR_INLINE rgb
rotate_hue(
    const rgb& _rgb,
    channel_t  _degrees
)
{
    return d_color_rgb_rotate_hue(_rgb, _degrees);
}

// delta_e
//   function: CIEDE2000 perceptual difference between two L*a*b*
// colors (runtime).
D_INLINE channel_t
delta_e(
    const cie_lab& _a,
    const cie_lab& _b
)
{
    return d_color_delta_e(_a, _b);
}

// delta_e
//   function: CIEDE2000 perceptual difference between two rgb
// colors, via L*a*b* (runtime).
D_INLINE channel_t
delta_e(
    const rgb& _a,
    const rgb& _b
)
{
    return d_color_rgb_delta_e(_a, _b);
}


///////////////////////////////////////////////////////////////////////////////
///                  III.   RUNTIME POLYMORPHISM                            ///
///////////////////////////////////////////////////////////////////////////////

// color
//   class: abstract base for type-erased colors. Concrete colors
// of any model are handled uniformly through this interface; each
// delegates to the shared kernel via color_cast.
class color
{
public:
    // ~color
    //   destructor: virtual for safe polymorphic deletion.
    virtual ~color() = default;

    // to_rgb
    //   query: convert the held color to linear RGB.
    virtual rgb
    to_rgb() const = 0;

    // clone
    //   factory: deep copy of the held color.
    virtual std::unique_ptr<color>
    clone() const = 0;

    // to
    //   query: convert the held color to any target model at
    // runtime (routes through RGB).
    template<typename _To>
    _To
    to() const
    {
        return color_cast<_To>(to_rgb());
    }
};

// color_value
//   class: concrete color holder parameterized on its model.
// Stores one value of _Model and implements the runtime interface
// by forwarding to the compile-time conversion facade.
template<typename _Model>
class color_value : public color
{
public:
    using value_type = _Model;

    // color_value (default)
    //   constructor: default-constructed model value.
    color_value() = default;

    // color_value (parameterized)
    //   constructor: from an existing model value.
    explicit color_value(
        const _Model& _value
    )
        : m_value(_value)
    {}

    // to_rgb
    //   query: convert the held value to linear RGB.
    rgb
    to_rgb() const override
    {
        return color_cast<rgb>(m_value);
    }

    // clone
    //   factory: deep copy as a new color_value.
    std::unique_ptr<color>
    clone() const override
    {
        return std::unique_ptr<color>(new color_value<_Model>(m_value));
    }

    // value (const)
    //   accessor: the held model value.
    const _Model&
    value() const
    {
        return m_value;
    }

    // value
    //   accessor: mutable held model value.
    _Model&
    value()
    {
        return m_value;
    }

private:
    _Model m_value;
};

// make_color
//   function: constructs a type-erased color from any model value,
// deducing the model type.
template<typename _Model>
std::unique_ptr<color>
make_color(
    const _Model& _value
)
{
    return std::unique_ptr<color>(new color_value<_Model>(_value));
}


NS_END  // djinterp


#endif  // DJINTERP_COLOR_HPP_
