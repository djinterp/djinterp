/******************************************************************************
* djinterp [re_std]                                          time_point_cast.hpp
*
* the time_point_cast function template:
*   Changes a time_point's precision, keeping its clock:
*
*       auto secs = time_point_cast<seconds>(a_millisecond_point);
*
*   The target is spelled as a DURATION, not as a time_point -- the clock
* is carried over from the argument and cannot be changed. That is the
* interface making the mixed-clock error unspellable rather than merely
* diagnosable.
*
*   TRUNCATION IS TOWARD THE EPOCH, NOT TOWARD THE PAST:
*   The conversion is duration_cast applied to the offset from the epoch,
* so it truncates toward zero -- and zero is the epoch. For a point after
* the epoch that rounds earlier; for a point BEFORE the epoch it rounds
* LATER. Pre-1970 system_clock values therefore move forward in time
* under a coarsening cast, which is rarely what a caller wants.
*
*   floor() is almost always the better tool for a time_point: it always
* moves toward the past, which is what "which second is this in" means.
* This function is the standard's, and it is provided with its standard
* behaviour; the header comment is here so the behaviour is chosen rather
* than discovered.
*
*
* path:      /inc/djinterp/re_std/chrono/time_point_cast.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TIME_POINT_CAST_
#define DJINTERP_RE_STD_CHRONO_TIME_POINT_CAST_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./time_point.hpp"
#include "./duration_cast.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD

namespace chrono
{

    // time_point_cast
    //   function: re-express a time_point at a different precision on the
    // same clock. Truncates toward the epoch.
    template<typename _ToDur,
             typename _Clock,
             typename _Duration>
    D_CONSTEXPR
    typename enable_if< internal::is_duration<_ToDur>::value,
                        time_point<_Clock, _ToDur> >::type
    time_point_cast(const time_point<_Clock, _Duration>& _t)
    {
        return time_point<_Clock, _ToDur>(
            duration_cast<_ToDur>(_t.time_since_epoch()));
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TIME_POINT_CAST_
