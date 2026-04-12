/******************************************************************************
* djinterp [color]                                                   color.hpp
*
*   Master color module for the djinterp framework. Provides shared
* channel types, clamping utilities, color model tag dispatch, and the
* is_color_model detection trait. Individual color model headers and the
* conversion layer are included at the bottom.
*
* 
* path:      /inc/djinterp/util/color/color.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
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
      g. cie_lab_tag

III.  COLOR MODEL DETECTION
      ----------------------
      i.    has_model_tag_helper (internal)
      ii.   is_color_model
            a. is_color_model_v

IV.   SUB-MODULE INCLUDES
      ---------------------
      a. color_rgb.hpp
      b. color_cmyk.hpp
      c. color_hsl.hpp
      d. color_hsv.hpp
      e. color_ycbcr.hpp
      f. color_cie_lab.hpp
      g. color_convert.hpp
*/

#ifndef DJINTERP_COLOR_
#define DJINTERP_COLOR_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & UTILITIES                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP
NS_COLOR


// ================================================================
//  channel_t
// ================================================================

// channel_t
//   type: underlying floating-point type for all color channel
// values. Defaults to double; define D_COLOR_CHANNEL_TYPE
// before inclusion to override.
#ifndef D_COLOR_CHANNEL_TYPE
    using channel_t = double;
#else
    using channel_t = D_COLOR_CHANNEL_TYPE;
#endif


// ================================================================
//  clamp_channel
// ================================================================

// clamp_channel
//   function: constrains a channel value to the closed
// interval [_min, _max]. Fully constexpr-portable via
// D_CONSTEXPR.
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


NS_END  // color
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                  IV.   SUB-MODULE INCLUDES                              ///
///////////////////////////////////////////////////////////////////////////////

#include "color_rgb.hpp"
#include "color_cmyk.hpp"
#include "color_hsl.hpp"
#include "color_hsv.hpp"
#include "color_ycbcr.hpp"
#include "color_cie_lab.hpp"
#include "color_convert.hpp"


#endif  // DJINTERP_COLOR_
