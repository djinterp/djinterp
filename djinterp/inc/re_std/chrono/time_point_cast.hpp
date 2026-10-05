/*******************************************************************************
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
* path:      /inc/re_std/chrono/time_point_cast.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_CAST_HPP
#define RE_STD_CHRONO_TIME_POINT_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./time_point.hpp"
#include "./duration_cast.hpp"
#include "../type_traits/enable_if.hpp"


namespace re_std
{

namespace chrono
{

    // time_point_cast
    //   function: re-express a time_point at a different precision on the
    // same clock. Truncates toward the epoch.
    template<typename ToDur,
             typename Clock,
             typename Duration>
    RE_STD_CONSTEXPR
    typename enable_if< internal::is_duration<ToDur>::value,
                        time_point<Clock, ToDur> >::type
    time_point_cast(const time_point<Clock, Duration>& _t)
    {
        return time_point<Clock, ToDur>(
            duration_cast<ToDur>(_t.time_since_epoch()));
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_CAST_HPP
