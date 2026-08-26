/******************************************************************************
* djinterp [re_std]                                                      abs.hpp
*
* chrono::abs for durations:
*   The magnitude of a duration, as a duration of the same type.
*
*   CONSTRAINED TO SIGNED REPRESENTATIONS:
*   For an unsigned rep the operation is meaningless -- every value is
* already its own magnitude, and negating one would wrap. The standard
* constrains it on numeric_limits<Rep>::is_signed, and expressing that as
* SFINAE means abs(an_unsigned_duration) does not match rather than
* compiling into a wrap.
*
*   THE ONE VALUE WITH NO MAGNITUDE:
*   abs(duration::min()) is undefined behaviour, for the same reason
* abs(INT_MIN) is: on a two's complement rep the most negative count has
* no representable negation. re_std adds no check, matching std -- a
* branch on every call to catch one input would be the wrong trade, and
* the check the caller actually wants is usually a range assertion
* further out. It is named here so it is known rather than discovered.
*
*   BACK-PORT: std added chrono::abs in C++17; re_std provides it from
* C++11, constexpr throughout.
*
*
* path:      /inc/djinterp/re_std/chrono/abs.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_ABS_
#define DJINTERP_RE_STD_CHRONO_ABS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_compare.hpp"
#include "../limits/numeric_limits.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD

namespace chrono
{

    // abs
    //   function: the magnitude of a duration. Signed representations
    // only; undefined for duration::min().
    template<typename _Rep,
             typename _Period>
    D_CONSTEXPR
    typename enable_if< numeric_limits<_Rep>::is_signed,
                        duration<_Rep, _Period> >::type
    abs(const duration<_Rep, _Period>& _d)
    {
        return (_d < duration<_Rep, _Period>::zero()) ? -_d : _d;
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_ABS_
