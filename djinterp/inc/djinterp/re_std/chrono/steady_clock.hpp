/******************************************************************************
* djinterp [re_std]                                             steady_clock.hpp
*
* the steady_clock class:
*   The clock for measuring intervals. Its epoch is unspecified -- often
* system boot -- so a single reading means nothing on its own. The
* guarantee is the one that matters for timing: it never jumps and never
* runs backwards. Subtract two readings and the difference is the elapsed
* time, with no NTP correction or daylight-saving transition able to
* corrupt it.
*
*   Because the epoch is unspecified there is no to_time_t here, and no
* conversion to system_clock. That absence is the point: a steady reading
* cannot be turned into a date, and any interface that appeared to do so
* would be lying.
*
*   is_steady IS COMPUTED, NOT ASSERTED:
*   Where a monotonic source exists, is_steady is true and now() reads
* it. Where none does, the class falls back to the wall clock -- and
* is_steady becomes FALSE, because on that platform the guarantee is
* genuinely not being met.
*
*   Reporting false is the whole design decision here. The alternative --
* claiming steadiness while silently reading a clock that jumps -- would
* let a caller's `assert(end >= start)` hold in testing and fail in
* production during a leap-second smear. A caller can test is_steady and
* choose; a caller lied to cannot.
*
*   The fallback is reached only on a non-POSIX platform below C++17 (in
* practice: older MSVC). CLOCK_MONOTONIC covers every Unix.
*
*   PRECISION: nanoseconds. Unlike system_clock, the range concern does
* not apply -- an interval clock counts from boot, so int64 nanoseconds
* covers 292 years of uptime.
*
*
* path:      /inc/djinterp/re_std/chrono/steady_clock.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_STEADY_CLOCK_
#define DJINTERP_RE_STD_CHRONO_STEADY_CLOCK_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <ctime>

// djinterp
#include "./duration.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_cast.hpp"
#include "./duration_compare.hpp"
#include "./duration_typedefs.hpp"
#include "./time_point.hpp"
#include "./system_clock.hpp"
#include "../ratio/ratio_typedefs.hpp"
#include "../cstdint/cstdint.hpp"


// D_RE_STD_HAS_MONOTONIC_CLOCK
//   constant: 1 if a genuinely monotonic time source is available. Drives
// steady_clock::is_steady -- do not define it to 1 on a platform that
// cannot honour it.
#ifndef D_RE_STD_HAS_MONOTONIC_CLOCK
    #if D_RE_STD_HAS_POSIX_CLOCK_GETTIME && defined(CLOCK_MONOTONIC)
        #define D_RE_STD_HAS_MONOTONIC_CLOCK  1
    #else
        #define D_RE_STD_HAS_MONOTONIC_CLOCK  0
    #endif
#endif


NS_RESTD

namespace chrono
{

    // steady_clock
    //   class: monotonic interval clock, counting nanoseconds from an
    // unspecified epoch.
    class steady_clock
    {
    public:
        // rep / period / duration / time_point
        //   typedef: the clock's fixed precision.
        typedef std::int_least64_t              rep;
        typedef nano                            period;
        typedef chrono::duration<rep, period>   duration;
        typedef chrono::time_point<steady_clock> time_point;

        // is_steady
        //   constant: true only where a monotonic source compiled in.
        // See the header comment -- this is reported honestly rather than
        // asserted.
        static D_CONSTEXPR const bool is_steady =
            (D_RE_STD_HAS_MONOTONIC_CLOCK != 0);

        // now
        //   function: the current reading. Meaningful only as a
        // difference against another reading.
        static time_point now() D_NOEXCEPT
        {
#if D_RE_STD_HAS_MONOTONIC_CLOCK

            ::timespec _ts;
            if (::clock_gettime(CLOCK_MONOTONIC, &_ts) != 0)
            {
                return time_point(duration::zero());
            }
            return time_point(
                seconds(static_cast<rep>(_ts.tv_sec)) +
                nanoseconds(static_cast<rep>(_ts.tv_nsec)));

#else

            // No monotonic source. is_steady is false to match; the
            // reading is the wall clock's, re-tagged onto this clock's
            // timeline.
            return time_point(
                duration_cast<duration>(
                    system_clock::now().time_since_epoch()));

#endif
        }
    };

    // Out-of-class definition; see system_clock.hpp for the gate.
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool steady_clock::is_steady;
#endif

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_STEADY_CLOCK_
