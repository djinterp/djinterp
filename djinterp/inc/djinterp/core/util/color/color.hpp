/*******************************************************************************
* djinterp [core]                                                      color.hpp
*
*   VALIDATION SHIM - not the production color subframework.  See
* color_common.hpp for the rationale and scope.
*
*   The umbrella the PDF layer includes: pulls the foundation + rgb/rgba, adds
* the cmyk model, and provides the compile-time conversion facade
* (color_convert<To,From>::apply) with the color_cast<To>(from) convenience
* the real header exposes.  Only the graph edges the PDF stack exercises are
* defined - identity (any model to itself) and rgb<->cmyk; the primary
* color_convert is intentionally left undefined so an unsupported pair fails to
* compile rather than silently mis-converting.  pdf_color's model constructors
* are not constexpr, so color_cast is a plain (runtime) inline.
*
*   NOT reproduced (unused by the PDF/report closure, verified): the other
* native models (hsl/hsv/ycbcr/cie_*), the polymorphic color / make_color /
* color_value surface, and all channel-algebra free functions.  Extend here if
* a future consumer needs them.
*
*
* path:      /inc/djinterp/core/util/color/color.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_COLOR_COLOR_HPP
#define DJINTERP_UTIL_COLOR_COLOR_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "./color_common.hpp"   // channel_t, tags, is_color_model
#include "./color_rgb.hpp"      // rgb, rgba


NS_DJINTERP


// cmyk
//   subtractive device model; channels in [0,1].  Kept as its own device
// space by pdf_color, with an rgb view derived through the hub.
struct cmyk
{
    typedef cmyk_tag  model_tag;    // => is_color_model<cmyk>::value == true
    typedef channel_t value_type;

    channel_t c;
    channel_t m;
    channel_t y;
    channel_t k;

    D_CONSTEXPR_INLINE cmyk()
        : c(0), m(0), y(0), k(0)
    {}

    D_CONSTEXPR_INLINE cmyk(
        channel_t _c,
        channel_t _m,
        channel_t _y,
        channel_t _k
    )
        : c(_c), m(_m), y(_y), k(_k)
    {}

    D_CONSTEXPR_INLINE bool
    operator==(const cmyk& _o) const
    { return c == _o.c && m == _o.m && y == _o.y && k == _o.k; }

    D_CONSTEXPR_INLINE bool
    operator!=(const cmyk& _o) const
    { return !(*this == _o); }
};


// ===========================================================================
//  conversion facade  (RGB hub; identity + rgb<->cmyk)
// ===========================================================================

// color_convert<To, From>
//   primary left UNDEFINED: only the specializations below are usable, so an
// unsupported pair is a compile error rather than a wrong answer.
template <typename To, typename From>
struct color_convert;

// identity  (any model -> itself)
template <typename Type>
struct color_convert<Type, Type>
{
    static D_CONSTEXPR_INLINE Type
    apply(const Type& _c)
    { return _c; }
};

// cmyk -> rgb   (the live path: pdf_color's cmyk view fallback)
template <>
struct color_convert<rgb, cmyk>
{
    static D_CONSTEXPR_INLINE rgb
    apply(const cmyk& _c)
    {
        return rgb(
            static_cast<channel_t>((channel_t(1) - _c.c) * (channel_t(1) - _c.k)),
            static_cast<channel_t>((channel_t(1) - _c.m) * (channel_t(1) - _c.k)),
            static_cast<channel_t>((channel_t(1) - _c.y) * (channel_t(1) - _c.k)));
    }
};

// rgb -> cmyk   (completeness; runtime - multi-statement)
template <>
struct color_convert<cmyk, rgb>
{
    static D_INLINE cmyk
    apply(const rgb& _c)
    {
        const channel_t max_rg = _c.r < _c.g ? _c.g : _c.r;
        const channel_t max_v  = max_rg < _c.b ? _c.b : max_rg;
        const channel_t k      = channel_t(1) - max_v;
        const channel_t denom  = channel_t(1) - k;

        if (denom <= channel_t(0))
        {
            return cmyk(0, 0, 0, channel_t(1));
        }

        return cmyk(
            (channel_t(1) - _c.r - k) / denom,
            (channel_t(1) - _c.g - k) / denom,
            (channel_t(1) - _c.b - k) / denom,
            k);
    }
};


NS_INTERNAL

    // color_clean
    //   strip cv/ref so color_cast can be called on references / const models.
    template <typename Type>
    struct color_clean
    {
        typedef typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type type;
    };

NS_END   // internal


// color_cast<To>(from)
//   the convenience the PDF layer calls (e.g. color_cast<rgb>(cmyk)).  Routes
// through color_convert after normalizing cv/ref.  Runtime inline - see header
// note; pdf_color's model constructors are not constexpr.
template <typename To, typename From>
D_NODISCARD D_INLINE To
color_cast(const From& _from)
{
    return color_convert<
               typename internal::color_clean<To>::type,
               typename internal::color_clean<From>::type>::apply(_from);
}


// is_convertible_color
//   both endpoints are conversion-graph models.  (Edge existence is a separate
// question - only the specializations above are defined in this shim.)
template <typename From, typename To>
struct is_convertible_color
    : std::integral_constant<
          bool,
          is_color_model<From>::value && is_color_model<To>::value>
{};


NS_END   // djinterp

#endif  // floor, for now

#endif // DJINTERP_UTIL_COLOR_COLOR_HPP
