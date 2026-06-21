/******************************************************************************
* djinterp [color]                                          color_convert.hpp
*
*   C++ conversion facade for the djinterp color module. This is a thin
* compile-time dispatch over the single C conversion kernel
* (color_convert.h): each wrapper is sliced to its POD, the appropriate
* d_color_convert_* routine is invoked, and the POD result is re-wrapped.
* No color math is duplicated.
*
*   RGB is the conversion hub. Same-type casts are identity; XYZ and LAB use
* their direct kernel path; every other pair routes through linear RGB.
* Conversions among RGB/HSL/HSV/CMYK/YCbCr are constexpr; any path touching
* LAB (or XYZ produced from LAB) is runtime-only (transcendental math) but
* uses the identical entry points.
*
*   Public surface:
*     color_convert<To, From>::apply(from)   - explicit conversion
*     color_cast<To>(from)                   - convenience wrapper
*     is_convertible_color[_v]<From, To>     - trait
*     color_model_type / convertible_color_type (C++20 concepts)
*
*
* path:      /inc/djinterp/util/color/color_convert.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    KERNEL DISPATCH HELPERS (internal)
      --------------------------------
      a. to_rgb            (per-model overloads)
      b. rgb_to            (per-model re-wrap)

II.   CONVERSION IMPLEMENTATION (internal)
      ---------------------------------
      a. color_convert_impl              (primary: route via RGB)
      b. color_convert_impl<_Type,_Type> (identity)
      c. color_convert_impl<xyz, lab>    (direct)
      d. color_convert_impl<lab, xyz>    (direct)

III.  PUBLIC INTERFACE
      ----------------
      a. color_convert
      b. color_cast

IV.   TRAITS
      ------
      a. color_convert_helper (internal)
      b. is_convertible_color
         a. is_convertible_color_v

V.    CONCEPTS (C++20)
      ----------------
      a. color_model_type
      b. convertible_color_type
*/

#ifndef DJINTERP_COLOR_CONVERT_HPP_
#define DJINTERP_COLOR_CONVERT_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.hpp"
#include "./color_convert.h"

#include "./color_rgb.hpp"
#include "./color_cmyk.hpp"
#include "./color_hsv.hpp"
#include "./color_hsl.hpp"
#include "./color_ycbcr.hpp"
#include "./color_cie_lab.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.   KERNEL DISPATCH HELPERS (internal)                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // to_rgb
    //   function: slices each color model wrapper to its POD and
    // invokes the kernel conversion into linear RGB. One overload
    // per model.
    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const rgb& _c
    )
    {
        return _c;
    }

    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const hsl& _c
    )
    {
        return d_color_convert_hsl_to_rgb(_c);
    }

    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const hsv& _c
    )
    {
        return d_color_convert_hsv_to_rgb(_c);
    }

    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const cmyk& _c
    )
    {
        return d_color_convert_cmyk_to_rgb(_c);
    }

    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const ycbcr& _c
    )
    {
        return d_color_convert_ycbcr_to_rgb(_c);
    }

    D_CONSTEXPR_INLINE rgb
    to_rgb(
        const cie_xyz& _c
    )
    {
        return d_color_convert_xyz_to_rgb(_c);
    }

    D_INLINE rgb
    to_rgb(
        const cie_lab& _c
    )
    {
        return d_color_convert_lab_to_rgb(_c);
    }


    // rgb_to
    //   trait: re-wraps a linear RGB into the requested target
    // model via the kernel. No primary definition; one full
    // specialization per model.
    template<typename _To>
    struct rgb_to;

    template<>
    struct rgb_to<rgb>
    {
        D_STATIC D_CONSTEXPR_INLINE rgb
        apply(
            const rgb& _c
        )
        {
            return _c;
        }
    };

    template<>
    struct rgb_to<hsl>
    {
        D_STATIC D_CONSTEXPR_INLINE hsl
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_hsl(_c);
        }
    };

    template<>
    struct rgb_to<hsv>
    {
        D_STATIC D_CONSTEXPR_INLINE hsv
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_hsv(_c);
        }
    };

    template<>
    struct rgb_to<cmyk>
    {
        D_STATIC D_CONSTEXPR_INLINE cmyk
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_cmyk(_c);
        }
    };

    template<>
    struct rgb_to<ycbcr>
    {
        D_STATIC D_CONSTEXPR_INLINE ycbcr
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_ycbcr(_c);
        }
    };

    template<>
    struct rgb_to<cie_xyz>
    {
        D_STATIC D_CONSTEXPR_INLINE cie_xyz
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_xyz(_c);
        }
    };

    template<>
    struct rgb_to<cie_lab>
    {
        D_STATIC D_INLINE cie_lab
        apply(
            const rgb& _c
        )
        {
            return d_color_convert_rgb_to_lab(_c);
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///            II.   CONVERSION IMPLEMENTATION (internal)                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // color_convert_impl
    //   trait: primary template. Routes an arbitrary source model
    // to an arbitrary target model through the linear RGB hub.
    template<typename _From,
             typename _To>
    struct color_convert_impl
    {
        D_STATIC D_CONSTEXPR_INLINE _To
        apply(
            const _From& _from
        )
        {
            return rgb_to<_To>::apply(to_rgb(_from));
        }
    };

    // color_convert_impl (identity)
    //   trait: same source and target model returns the input
    // unchanged (no lossy round trip).
    template<typename _Type>
    struct color_convert_impl<_Type, _Type>
    {
        D_STATIC D_CONSTEXPR_INLINE _Type
        apply(
            const _Type& _from
        )
        {
            return _from;
        }
    };

    // color_convert_impl (XYZ -> LAB)
    //   trait: direct kernel path, avoiding the RGB round trip.
    template<>
    struct color_convert_impl<cie_xyz, cie_lab>
    {
        D_STATIC D_INLINE cie_lab
        apply(
            const cie_xyz& _from
        )
        {
            return d_color_convert_xyz_to_lab(_from);
        }
    };

    // color_convert_impl (LAB -> XYZ)
    //   trait: direct kernel path, avoiding the RGB round trip.
    template<>
    struct color_convert_impl<cie_lab, cie_xyz>
    {
        D_STATIC D_INLINE cie_xyz
        apply(
            const cie_lab& _from
        )
        {
            return d_color_convert_lab_to_xyz(_from);
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    III.   PUBLIC INTERFACE                              ///
///////////////////////////////////////////////////////////////////////////////

// color_convert
//   trait: public conversion entry point. Strips qualifiers from
// the operands and delegates to the internal implementation.
// Parameterized target-first to read as color_convert<To, From>.
template<typename _To,
         typename _From>
struct color_convert
{
    D_STATIC D_CONSTEXPR_INLINE _To
    apply(
        const _From& _from
    )
    {
        return internal::color_convert_impl<clean_t<_From>,
                                            clean_t<_To>>::apply(_from);
    }
};

// color_cast
//   function: convenience wrapper deducing the source type, e.g.
// auto h = color_cast<hsl>(my_rgb).
template<typename _To,
         typename _From>
D_CONSTEXPR_INLINE _To
color_cast(
    const _From& _from
)
{
    return color_convert<_To, _From>::apply(_from);
}


///////////////////////////////////////////////////////////////////////////////
///                          IV.   TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // color_convert_helper
    //   trait: SFINAE detector; true when both operands are color
    // models (and therefore inter-convertible through RGB).
    template<typename _From,
             typename _To,
             typename = void>
    struct color_convert_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // color_convert_helper (specialization)
    //   trait: success case when both types expose model_tag.
    template<typename _From,
             typename _To>
    struct color_convert_helper<_From,
                                _To,
                                void_t<typename clean_t<_From>::model_tag,
                                       typename clean_t<_To>::model_tag>>
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_convertible_color
//   trait: true if a conversion from _From to _To is supported.
template<typename _From,
         typename _To>
struct is_convertible_color
{
    D_STATIC_CONSTEXPR bool value =
        internal::color_convert_helper<_From, _To>::value;
};

// is_convertible_color_v
//   constant: convenience accessor for
// is_convertible_color<_From, _To>::value.
template<typename _From,
         typename _To>
D_STATIC_CONSTEXPR bool is_convertible_color_v =
    is_convertible_color<_From, _To>::value;


///////////////////////////////////////////////////////////////////////////////
///                      V.   CONCEPTS (C++20)                              ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// color_model_type
//   concept: satisfied by any color model wrapper.
template<typename _Type>
concept color_model_type = is_color_model_v<_Type>;

// convertible_color_type
//   concept: satisfied when _From converts to _To.
template<typename _From,
         typename _To>
concept convertible_color_type = is_convertible_color_v<_From, _To>;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_COLOR_CONVERT_HPP_
