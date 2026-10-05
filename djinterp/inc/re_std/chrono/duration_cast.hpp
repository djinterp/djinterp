/*******************************************************************************
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
* path:      /inc/re_std/chrono/duration_cast.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_CHRONO_DURATION_CAST_HPP
#define RE_STD_CHRONO_DURATION_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./duration.hpp"
#include "./duration_cast_impl.hpp"
#include "../ratio/ratio_divide.hpp"
#include "../type_traits/common_type.hpp"
#include "../type_traits/enable_if.hpp"
#include "../cstdint/cstdint.hpp"


namespace re_std
{

namespace chrono
{

    // duration_cast
    //   function: convert between durations of any periods, truncating
    // toward zero. Participates only when ToDur is a duration.
    template<typename ToDur,
             typename Rep,
             typename Period>
    RE_STD_CONSTEXPR
    typename enable_if<internal::is_duration<ToDur>::value, ToDur>::type
    duration_cast(const duration<Rep, Period>& _d)
    {
        return internal::duration_cast_helper<
                    ToDur,
                    typename ratio_divide<Period,
                                          typename ToDur::period>::type,
                    typename common_type<typename ToDur::rep,
                                         Rep,
                                         intmax_t>::type,
                    ratio_divide<Period, typename ToDur::period>::num == 1,
                    ratio_divide<Period, typename ToDur::period>::den == 1
               >::cast(_d);
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_DURATION_CAST_HPP
