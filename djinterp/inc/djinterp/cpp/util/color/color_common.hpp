/*******************************************************************************
* djinterp [cpp][utility]                                       color_common.hpp
*
*   The foundation of the C++ color module, which every model's header
* includes: the channel scalar (channel_t, the C kernel's float), the
* model-tag hierarchy, the is_color_model detection trait, and two scalar
* helpers (clamp_channel, approx_equal).  Each model's header layers a C++
* type on its C kernel's struct; color.hpp gathers them all.
*
*
* path:      /inc/djinterp/cpp/util/color/color_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.11.17
*******************************************************************************/

#ifndef DJINTERP_UTIL_COLOR_COMMON_HPP
#define DJINTERP_UTIL_COLOR_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>  // std::is_same
// djinterp
#include "../../djinterp.hpp"   // framework root


NS_DJINTERP


// channel_t
//   the scalar every model stores: the C kernel's float, since each model
// derives from its C struct.  D_COLOR_CHANNEL_TYPE once offered to change it,
// which those structs make impossible; it is retired, and defined as anything
// but float it stops the build, saying why.
using channel_t = float;

#ifdef D_COLOR_CHANNEL_TYPE
static_assert(std::is_same<D_COLOR_CHANNEL_TYPE, float>::value,
              "D_COLOR_CHANNEL_TYPE is retired: channel_t is the C color "
              "kernel's float, which every color model stores. Remove the "
              "definition.");
#endif


// clamp_channel / approx_equal
//   the two generic scalar helpers the foundation exposes.  Single-return
// so they stay constant-expression-safe down to C++11.
template <typename Type>
D_CONSTEXPR_INLINE
Type clamp_channel(
    Type _v,
    Type _lo,
    Type _hi
)
{
    return _v < _lo ? _lo : (_hi < _v ? _hi : _v);
}

template <typename Type>
D_CONSTEXPR_INLINE
bool approx_equal(
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
struct DColorModelTag {};

struct rgb_tag     : DColorModelTag {};
struct cmyk_tag    : DColorModelTag {};
struct hsl_tag     : DColorModelTag {};
struct hsv_tag     : DColorModelTag {};
struct ycbcr_tag   : DColorModelTag {};
struct cie_xyz_tag : DColorModelTag {};
struct cie_lab_tag : DColorModelTag {};


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
// C++14+ variable-template shorthand (gated).
template <typename Type>
D_CONSTEXPR bool is_color_model_v = is_color_model<Type>::value;
#endif


NS_END   // djinterp

#endif  // floor, for now

#endif // DJINTERP_UTIL_COLOR_COMMON_HPP
