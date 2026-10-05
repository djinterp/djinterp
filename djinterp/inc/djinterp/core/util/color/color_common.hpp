/*******************************************************************************
* djinterp [core]                                               color_common.hpp
*
*   VALIDATION SHIM - not the production color subframework.
*
*   A self-contained stand-in for the real color foundation, provided so the
* report / PDF stack (test_report_runner -> test_report -> test_render_pdf ->
* pdf -> color) can be COMPILED and exercised WITHOUT the few-dozen real color
* headers + their C kernels in the tree.  It reproduces exactly the surface the
* PDF layer consumes across the whole closure and nothing more: the channel
* scalar, the model-tag hierarchy, and the is_color_model detection trait.  The
* real subframework layers each wrapper on a C kernel (color_common.h, ...) and
* routes a D_COLOR_* macro layer; NONE of that is reachable from the PDF stack
* (verified: no D_COLOR_* token appears outside the color headers themselves),
* so the shim needs none of it and is pure C++.
*
*   Drop this directory in at inc/djinterp/core/util/color/ for a color-free
* build of the report/PDF layer; the real headers supersede it in a full tree.
*
*   Surface consumed by the PDF stack (font.hpp, pdf_primitives.hpp):
*     channel_t, rgb{r,g,b}, rgba{r,g,b,a}, cmyk{c,m,y,k},
*     color_cast<To>(from)  (identity + cmyk->rgb are the live paths),
*     is_color_model<T>::value.
*
*
* path:      /inc/djinterp/core/util/color/color_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_COLOR_COLOR_COMMON_HPP
#define DJINTERP_UTIL_COLOR_COLOR_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"   // NS_*, D_CONSTEXPR*, D_INLINE, feature gates


NS_DJINTERP


// channel_t
//   the scalar every model stores.  Overridable, exactly as the real header,
// by defining D_COLOR_CHANNEL_TYPE before inclusion.
#ifndef D_COLOR_CHANNEL_TYPE
#  define D_COLOR_CHANNEL_TYPE float
#endif
using channel_t = D_COLOR_CHANNEL_TYPE;


// clamp_channel / approx_equal
//   the two generic scalar helpers the real foundation exposes.  Single-return
// so they stay constant-expression-safe down to C++11.
template <typename Type>
D_CONSTEXPR_INLINE Type
clamp_channel(
    Type _v,
    Type _lo,
    Type _hi
)
{
    return _v < _lo ? _lo : (_hi < _v ? _hi : _v);
}

template <typename Type>
D_CONSTEXPR_INLINE bool
approx_equal(
    Type _a,
    Type _b,
    Type _eps
)
{
    return (_a < _b ? _b - _a : _a - _b) <= _eps;
}


// ---- model-tag hierarchy ----------------------------------------------------
// Empty base + one tag per model; a wrapper advertises membership of the
// conversion graph by exposing a nested model_tag.
struct color_model_tag {};

struct rgb_tag     : color_model_tag {};
struct cmyk_tag    : color_model_tag {};
struct hsl_tag     : color_model_tag {};
struct hsv_tag     : color_model_tag {};
struct ycbcr_tag   : color_model_tag {};
struct cie_xyz_tag : color_model_tag {};
struct cie_lab_tag : color_model_tag {};


NS_INTERNAL

    // color_detail_void
    //   a C++11-clean void_t: maps any well-formed type list to void so a
    // partial specialization can probe for a nested member.
    template <typename /*...*/>
    struct color_detail_void { typedef void type; };

    // has_model_tag_helper
    //   true iff Type names a nested ::model_tag.  Underpins is_color_model.
    template <typename Type, typename = void>
    struct has_model_tag_helper { static const bool value = false; };

    template <typename Type>
    struct has_model_tag_helper<
        Type,
        typename color_detail_void<typename Type::model_tag>::type>
    { static const bool value = true; };

NS_END   // internal


// is_color_model
//   a model participates in the conversion graph; transport types (rgba,
// rgba_premul) deliberately do not.  cv/ref are stripped first.
template <typename Type>
struct is_color_model
    : std::integral_constant<
          bool,
          internal::has_model_tag_helper<
              typename std::remove_cv<
                  typename std::remove_reference<Type>::type>::type>::value>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
// C++14+ variable-template shorthand (gated, matching the real header).
template <typename Type>
D_CONSTEXPR bool is_color_model_v = is_color_model<Type>::value;
#endif


NS_END   // djinterp

#endif  // floor, for now

#endif // DJINTERP_UTIL_COLOR_COLOR_COMMON_HPP
