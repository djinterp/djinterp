/******************************************************************************
* djinterp [re_std]                                            duration_cast.hpp
*
* the duration_cast function template:
*   The explicit conversion between durations -- the one that is allowed
* to lose precision, and therefore the one the caller must write out:
*
*       auto ms = duration_cast<milliseconds>(some_micros);
*
*   TRUNCATION IS TOWARD ZERO, WHICH IS NOT ROUNDING:
*   duration_cast<seconds>(milliseconds(1999)) is 1 second, and
* duration_cast<seconds>(milliseconds(-1999)) is -1 second. The magnitude
* always shrinks. For -1999 milliseconds that means the result is LATER
* than the input, which is the behaviour that surprises people writing
* timeout arithmetic around negative offsets.
*
*   Where truncation toward zero is not what is wanted, C++17's floor,
* ceil and round from this same module take a consistent direction
* instead; re_std back-ports all three to C++11.
*
*   THE CONSTRAINT IS ON THE TARGET, NOT THE SOURCE:
*   duration_cast<int>(d) is not a compile error inside the function body
* -- it is a substitution failure, so the name simply does not match and
* the diagnostic points at the call rather than at library internals.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_cast.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_CAST_
#define DJINTERP_RE_STD_CHRONO_DURATION_CAST_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_cast_impl.hpp"
#include "../ratio/ratio_divide.hpp"
#include "../type_traits/common_type.hpp"
#include "../type_traits/enable_if.hpp"
#include "../cstdint/cstdint.hpp"


NS_RESTD

namespace chrono
{

    // duration_cast
    //   function: convert between durations of any periods, truncating
    // toward zero. Participates only when _ToDur is a duration.
    template<typename _ToDur,
             typename _Rep,
             typename _Period>
    D_CONSTEXPR
    typename enable_if<internal::is_duration<_ToDur>::value, _ToDur>::type
    duration_cast(const duration<_Rep, _Period>& _d)
    {
        return internal::duration_cast_helper<
                    _ToDur,
                    typename ratio_divide<_Period,
                                          typename _ToDur::period>::type,
                    typename common_type<typename _ToDur::rep,
                                         _Rep,
                                         std::intmax_t>::type,
                    ratio_divide<_Period, typename _ToDur::period>::num == 1,
                    ratio_divide<_Period, typename _ToDur::period>::den == 1
               >::cast(_d);
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_CAST_
