/*******************************************************************************
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
* path:      /inc/re_std/chrono/abs.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_ABS_HPP
#define RE_STD_CHRONO_ABS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./duration.hpp"
#include "./duration_compare.hpp"
#include "../limits/numeric_limits.hpp"
#include "../type_traits/enable_if.hpp"


namespace re_std
{

namespace chrono
{

    // abs
    //   function: the magnitude of a duration. Signed representations
    // only; undefined for duration::min().
    template<typename Rep,
             typename Period>
    RE_STD_CONSTEXPR
    typename enable_if< numeric_limits<Rep>::is_signed,
                        duration<Rep, Period> >::type
    abs(const duration<Rep, Period>& _d)
    {
        return (_d < duration<Rep, Period>::zero()) ? -_d : _d;
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_ABS_HPP
