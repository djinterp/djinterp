/******************************************************************************
* djinterp [color]                                             color_common.hpp
*
*   C++ ergonomic foundation for the djinterp color module. Layered on top
* of color_common.h, it adds the channel type alias, generic clamp/compare
* helpers, the color-model tag hierarchy, and the is_color_model detection
* trait used by the conversion layer and the polymorphic color type.
*
*   All color-space math itself lives in the shared C kernels (the .h
* headers); this header only provides the type-level scaffolding that the
* C++ wrapper types and conversion dispatch are built from.
*
*
* path:      /inc/djinterp/util/color/color_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & UTILITIES
      -------------------------
      i.    channel_t
      ii.   clamp_channel
      iii.  approx_equal

II.   COLOR MODEL TAGS
      -----------------
      a. color_model_tag
      b. rgb_tag
      c. cmyk_tag
      d. hsl_tag
      e. hsv_tag
      f. ycbcr_tag
      g. cie_xyz_tag
      h. cie_lab_tag

III.  COLOR MODEL DETECTION
      ----------------------
      i.    has_model_tag_helper (internal)
      ii.   is_color_model
            a. is_color_model_v
*/

#ifndef DJINTERP_COLOR_COMMON_HPP_
#define DJINTERP_COLOR_COMMON_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.h"


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & UTILITIES                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP


// ================================================================
//  channel_t
// ================================================================

// channel_t
//   type: underlying floating-point type for all color channel
// values. Unified on `float` to share the C kernel's POD layout
// at zero cost; define D_COLOR_CHANNEL_TYPE before inclusion to
// override.
#ifndef D_COLOR_CHANNEL_TYPE
    using channel_t = float;
#else
    using channel_t = D_COLOR_CHANNEL_TYPE;
#endif


// ================================================================
//  clamp_channel
// ================================================================

// clamp_channel
//   function: constrains a channel value to the closed
// interval [_min, _max]. Fully constexpr-portable.
template<typename _Type>
D_CONSTEXPR_INLINE _Type
clamp_channel(
    _Type _value,
    _Type _min,
    _Type _max
)
{
    return (_value < _min) ? _min
         : (_value > _max) ? _max
         :                   _value;
}


// ================================================================
//  approx_equal
// ================================================================

// approx_equal
//   function: approximate floating-point equality within a
// specified epsilon. Used for channel comparisons where exact
// equality is inappropriate.
template<typename _Type>
D_CONSTEXPR_INLINE bool
approx_equal(
    _Type _a,
    _Type _b,
    _Type _epsilon
)
{
    _Type diff = (_a > _b) ? (_a - _b) : (_b - _a);

    return (diff <= _epsilon);
}


///////////////////////////////////////////////////////////////////////////////
///                   II.   COLOR MODEL TAGS                                ///
///////////////////////////////////////////////////////////////////////////////

// color_model_tag
//   struct: empty base tag for all color model tag types.
// Serves as the root of the tag hierarchy for dispatch and
// detection.
struct color_model_tag
{};

// rgb_tag
//   struct: tag type identifying the RGB color model.
struct rgb_tag : color_model_tag
{};

// cmyk_tag
//   struct: tag type identifying the CMYK color model.
struct cmyk_tag : color_model_tag
{};

// hsl_tag
//   struct: tag type identifying the HSL color model.
struct hsl_tag : color_model_tag
{};

// hsv_tag
//   struct: tag type identifying the HSV color model.
struct hsv_tag : color_model_tag
{};

// ycbcr_tag
//   struct: tag type identifying the YCbCr color model.
struct ycbcr_tag : color_model_tag
{};

// cie_xyz_tag
//   struct: tag type identifying the CIE 1931 XYZ color model.
struct cie_xyz_tag : color_model_tag
{};

// cie_lab_tag
//   struct: tag type identifying the CIE L*a*b* color model.
struct cie_lab_tag : color_model_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                III.   COLOR MODEL DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// is_color_model
//   trait: detects whether _Type is a color model type by
// checking for a nested `model_tag` alias.

NS_INTERNAL

    // has_model_tag_helper
    //   trait: SFINAE helper; primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_model_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_model_tag_helper (specialization)
    //   trait: success case when _Type::model_tag exists.
    template<typename _Type>
    struct has_model_tag_helper<_Type,
                                void_t<typename _Type::model_tag>>
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_color_model
//   trait: true if _Type has a nested model_tag type.
template<typename _Type>
struct is_color_model
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_model_tag_helper<clean_t<_Type>>::value;
};

// is_color_model_v
//   constant: convenience accessor for
// is_color_model<_Type>::value.
template<typename _Type>
D_STATIC_CONSTEXPR bool is_color_model_v =
    is_color_model<_Type>::value;


NS_END  // djinterp


#endif  // DJINTERP_COLOR_COMMON_HPP_
