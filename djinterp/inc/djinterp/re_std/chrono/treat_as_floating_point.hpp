/******************************************************************************
* djinterp [re_std]                                  treat_as_floating_point.hpp
*
* the treat_as_floating_point trait:
*   Answers one question: may a duration with this representation convert
* to any other duration implicitly?
*
*   WHY THE ANSWER GOVERNS IMPLICIT CONVERSION:
*   An integral duration converts implicitly only when the conversion
* cannot lose anything -- seconds to milliseconds is exact, so it is
* implicit; milliseconds to seconds truncates, so it requires an explicit
* duration_cast. That rule is what stops a truncation from happening
* silently inside an argument list.
*
*   Floating-point representations are exempt, because every conversion
* among them is already approximate. Refusing the implicit conversion
* would not prevent a loss that has already been accepted by choosing a
* floating-point rep at all.
*
*   THIS IS THE CUSTOMISATION POINT FOR USER REPRESENTATIONS:
*   A program with its own fixed-point or checked-arithmetic type
* specialises this trait to true and gets the same freedom. The
* specialisation must be written in re_std::chrono, and specialising it
* for a fundamental type is undefined -- the library already knows the
* answer for those.
*
*
* path:      /inc/djinterp/re_std/chrono/treat_as_floating_point.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TREAT_AS_FLOATING_POINT_
#define DJINTERP_RE_STD_CHRONO_TREAT_AS_FLOATING_POINT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../type_traits/is_floating_point.hpp"


NS_RESTD

namespace chrono
{

    // treat_as_floating_point
    //   trait: true if _Rep may take part in implicit duration
    // conversions that are not exact. Defaults to is_floating_point;
    // specialise for a user-defined representation.
    template<typename _Rep>
    struct treat_as_floating_point
        : is_floating_point<_Rep>
    {};

    // treat_as_floating_point_v (C++17, back-ported to C++14)
    //   variable: the trait's value. std added the _v spelling in C++17;
    // re_std provides it wherever variable templates exist, which is
    // C++14.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Rep>
    D_CONSTEXPR bool treat_as_floating_point_v =
        treat_as_floating_point<_Rep>::value;
#endif

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TREAT_AS_FLOATING_POINT_
