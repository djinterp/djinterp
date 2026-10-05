/*******************************************************************************
* djinterp [core]                                              color_convert.hpp
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
* path:      /inc/djinterp/core/util/color/color_convert.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    KERNEL DISPATCH HELPERS (internal)
      ----------------------------------
      a. to_rgb            (per-model overloads)
      b. rgb_to            (per-model re-wrap)

II.   CONVERSION IMPLEMENTATION (internal)
      ------------------------------------
      a. color_convert_impl              (primary: route via RGB)
      b. color_convert_impl<Type,Type> (identity)
      c.    color_convert_impl<xyz, lab>    (direct)
            d. color_convert_impl<lab, xyz>    (direct)

III.  PUBLIC INTERFACE
      ----------------
      a. color_convert
      b. color_cast

IV.   TRAITS
      ------
      a. color_convert_impl (internal)
      b. is_convertible_color
      a. is_convertible_color_v

V.    CONCEPTS (C++20)
      ----------------
      a. color_model_type
      b. convertible_color_type
*/

#ifndef DJINTERP_UTIL_COLOR_COLOR_CONVERT_HPP
#define DJINTERP_UTIL_COLOR_COLOR_CONVERT_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/color/color_convert.h"
#include "./color_common.hpp"
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
    template<typename To>
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

    // color_convert_helper
    //   trait: primary template. Routes an arbitrary source model
    // to an arbitrary target model through the linear RGB hub.
    template<typename From,
             typename To>
    struct color_convert_helper
    {
        D_STATIC D_CONSTEXPR_INLINE To
        apply(
            const From& _from
        )
        {
            return rgb_to<To>::apply(to_rgb(_from));
        }
    };

    // color_convert_helper (identity)
    //   trait: same source and target model returns the input
    // unchanged (no lossy round trip).
    template<typename Type>
    struct color_convert_helper<Type, Type>
    {
        D_STATIC D_CONSTEXPR_INLINE Type
        apply(
            const Type& _from
        )
        {
            return _from;
        }
    };

    // color_convert_helper (XYZ -> LAB)
    //   trait: direct kernel path, avoiding the RGB round trip.
    template<>
    struct color_convert_helper<cie_xyz, cie_lab>
    {
        D_STATIC D_INLINE cie_lab
        apply(
            const cie_xyz& _from
        )
        {
            return d_color_convert_xyz_to_lab(_from);
        }
    };

    // color_convert_helper (LAB -> XYZ)
    //   trait: direct kernel path, avoiding the RGB round trip.
    template<>
    struct color_convert_helper<cie_lab, cie_xyz>
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
template<typename To,
         typename From>
struct color_convert
{
    D_STATIC D_CONSTEXPR_INLINE To
    apply(
        const From& _from
    )
    {
        return internal::color_convert_helper<clean_t<From>,
                                            clean_t<To>>::apply(_from);
    }
};

// color_cast
//   function: convenience wrapper deducing the source type, e.g.
// auto h = color_cast<hsl>(my_rgb).
template<typename To,
         typename From>
D_CONSTEXPR_INLINE To
color_cast(
    const From& _from
)
{
    return color_convert<To, From>::apply(_from);
}


///////////////////////////////////////////////////////////////////////////////
///                          IV.   TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // color_convert_impl
    //   trait: SFINAE detector; true when both operands are color
    // models (and therefore inter-convertible through RGB).
    template<typename From,
             typename To,
             typename = void>
    struct color_convert_impl
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // color_convert_impl (specialization)
    //   trait: success case when both types expose model_tag.
    template<typename From,
             typename To>
    struct color_convert_impl<From,
                                To,
                                void_t<typename clean_t<From>::model_tag,
                                       typename clean_t<To>::model_tag>>
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_convertible_color
//   trait: true if a conversion from From to To is supported.
template<typename From,
         typename To>
struct is_convertible_color
{
    D_STATIC_CONSTEXPR bool value =
        internal::color_convert_impl<From, To>::value;
};

// is_convertible_color_v
//   constant: convenience accessor for
// is_convertible_color<From, To>::value.
template<typename From,
         typename To>
D_STATIC_CONSTEXPR bool is_convertible_color_v =
    is_convertible_color<From, To>::value;


///////////////////////////////////////////////////////////////////////////////
///                      V.   CONCEPTS (C++20)                              ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// color_model_type
//   concept: satisfied by any color model wrapper.
template<typename Type>
concept color_model_type = is_color_model_v<Type>;

// convertible_color_type
//   concept: satisfied when From converts to To.
template<typename From,
         typename To>
concept convertible_color_type = is_convertible_color_v<From, To>;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COLOR_COLOR_CONVERT_HPP
