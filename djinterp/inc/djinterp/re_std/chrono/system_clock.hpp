/******************************************************************************
* djinterp [re_std]                                             system_clock.hpp
*
* the system_clock class:
*   The wall clock. Its epoch is the Unix epoch -- 1970-01-01 00:00:00
* UTC -- which C++20 finally made a requirement and which every known
* implementation had already adopted. re_std guarantees it at every tier,
* so to_time_t and from_time_t are exact on both sides.
*
*   THIS CLOCK IS NOT STEADY, AND is_steady SAYS SO:
*   NTP corrections, manual clock changes and daylight-saving transitions
* all move it, backwards included. Two successive now() calls can
* therefore return a DECREASING pair, which makes this the wrong clock
* for measuring how long something took -- a negative elapsed time is a
* real outcome, not a hypothetical one. Use steady_clock for intervals
* and this clock only for time of day.
*
*   THE TIME SOURCE IS NECESSARILY PLATFORM CODE:
*   Everything else in re_std is portable C++. A clock cannot be: the
* current time exists outside the program. Three tiers are tried, best
* first, and each is a compile-time choice with no run-time dispatch:
*
*     1. POSIX clock_gettime(CLOCK_REALTIME)  -- nanosecond source
*     2. C++17 std::timespec_get(TIME_UTC)    -- standard, nanosecond API
*     3. std::time()                          -- ONE SECOND resolution
*
*   Tier 3 is the honest floor. On a pre-C++17 non-POSIX platform there
* is no portable sub-second source, so now() returns whole seconds. It is
* still a correct system_clock -- just a coarse one. D_RE_STD_CLOCK_SOURCE
* reports which tier compiled in, so a program that needs better can
* detect the situation rather than discover it in its measurements.
*
*   Windows is where tier 3 bites: QueryPerformanceCounter and
* GetSystemTimeAsFileTime would both do better, and neither can be
* reached without including <windows.h> -- a header re_std will not pull
* into every translation unit that wants a duration. On C++17 and later
* MSVC reaches tier 2, which is fine.
*
*   THE PRECISION OF THE TYPE IS NOT THE PRECISION OF THE SOURCE:
*   duration is microseconds regardless of which tier compiled in. A
* microsecond period was chosen over nanoseconds because int64
*   nanoseconds since 1970 runs out in 2262, and a wall clock that cannot
* represent the next three centuries is a worse trade than a thousand-fold
* precision reduction no wall-clock source delivers anyway.
*
*
* path:      /inc/djinterp/re_std/chrono/system_clock.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_SYSTEM_CLOCK_
#define DJINTERP_RE_STD_CHRONO_SYSTEM_CLOCK_ 1

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
#include "../ratio/ratio_typedefs.hpp"
#include "../cstdint/cstdint.hpp"


// D_RE_STD_HAS_POSIX_CLOCK_GETTIME
//   constant: 1 if ::clock_gettime and CLOCK_REALTIME are available.
#ifndef D_RE_STD_HAS_POSIX_CLOCK_GETTIME
    #if ( defined(__unix__) || defined(__linux__) || defined(__APPLE__) ||    \
          defined(__QNX__)  || defined(_POSIX_VERSION) )
        #define D_RE_STD_HAS_POSIX_CLOCK_GETTIME  1
    #else
        #define D_RE_STD_HAS_POSIX_CLOCK_GETTIME  0
    #endif
#endif

#if D_RE_STD_HAS_POSIX_CLOCK_GETTIME
    // POSIX declares clock_gettime in <time.h>, at global scope. <ctime>
    // is only required to put the C++98 subset in std::, so the POSIX
    // header is included directly for this one call.
    #include <time.h>
#endif

// D_RE_STD_CLOCK_SOURCE
//   constant: which time source compiled in. 3 = POSIX clock_gettime,
// 2 = C++17 timespec_get, 1 = std::time (one-second resolution).
#ifndef D_RE_STD_CLOCK_SOURCE
    #if D_RE_STD_HAS_POSIX_CLOCK_GETTIME
        #define D_RE_STD_CLOCK_SOURCE  3
    #elif D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_RE_STD_CLOCK_SOURCE  2
    #else
        #define D_RE_STD_CLOCK_SOURCE  1
    #endif
#endif

// The macro above is user-overridable, which means it can be set to a
// tier the translation unit cannot actually support -- forcing 2 on
// C++11, where std::timespec and std::timespec_get do not exist, or
// forcing 3 where clock_gettime is not declared. Rather than let that
// become a compile error inside now(), an unsupportable choice is
// DEMOTED to the best tier that does work. Degrade or omit, never error.
#if D_RE_STD_CLOCK_SOURCE == 3 && !D_RE_STD_HAS_POSIX_CLOCK_GETTIME
    #undef D_RE_STD_CLOCK_SOURCE
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_RE_STD_CLOCK_SOURCE  2
    #else
        #define D_RE_STD_CLOCK_SOURCE  1
    #endif
#endif

#if D_RE_STD_CLOCK_SOURCE == 2 && !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #undef D_RE_STD_CLOCK_SOURCE
    #define D_RE_STD_CLOCK_SOURCE  1
#endif


NS_RESTD

namespace chrono
{

    // system_clock
    //   class: the wall clock, counting microseconds from the Unix epoch.
    // Not steady -- see the header comment.
    class system_clock
    {
    public:
        // rep / period / duration / time_point
        //   typedef: the clock's fixed precision. Microseconds; see the
        // header comment for why not nanoseconds.
        typedef std::int_least64_t              rep;
        typedef micro                           period;
        typedef chrono::duration<rep, period>   duration;
        typedef chrono::time_point<system_clock> time_point;

        // is_steady
        //   constant: false. This clock can jump, in either direction.
        static D_CONSTEXPR const bool is_steady = false;

        // now
        //   function: the current wall-clock time. Resolution depends on
        // which source compiled in -- see D_RE_STD_CLOCK_SOURCE.
        static time_point now() D_NOEXCEPT
        {
#if D_RE_STD_CLOCK_SOURCE == 3

            ::timespec _ts;
            if (::clock_gettime(CLOCK_REALTIME, &_ts) != 0)
            {
                // A failing CLOCK_REALTIME means the platform lied about
                // supporting it. Fall back rather than return garbage.
                return from_time_t(::std::time(D_NULLPTR));
            }
            return time_point(
                duration_cast<duration>(
                    seconds(static_cast<rep>(_ts.tv_sec)) +
                    nanoseconds(static_cast<rep>(_ts.tv_nsec))));

#elif D_RE_STD_CLOCK_SOURCE == 2

            ::std::timespec _ts;
            if (::std::timespec_get(&_ts, TIME_UTC) != TIME_UTC)
            {
                return from_time_t(::std::time(D_NULLPTR));
            }
            return time_point(
                duration_cast<duration>(
                    seconds(static_cast<rep>(_ts.tv_sec)) +
                    nanoseconds(static_cast<rep>(_ts.tv_nsec))));

#else

            // One-second resolution. Correct, just coarse.
            return from_time_t(::std::time(D_NULLPTR));

#endif
        }

        // to_time_t
        //   function: convert to a C time_t, truncating to whole seconds
        // toward the epoch. Exact because the epochs agree.
        static ::std::time_t to_time_t(const time_point& _t) D_NOEXCEPT
        {
            return static_cast< ::std::time_t >(
                duration_cast<seconds>(_t.time_since_epoch()).count());
        }

        // from_time_t
        //   function: convert from a C time_t. Exact; time_t carries no
        // sub-second part to lose.
        static time_point from_time_t(::std::time_t _t) D_NOEXCEPT
        {
            return time_point(
                duration_cast<duration>(
                    seconds(static_cast<rep>(_t))));
        }
    };

    // Out-of-class definition. Before C++17 an odr-used static const
    // member still needs one; from C++17 the in-class initialiser is the
    // definition and repeating it is deprecated. Same gate as ratio's.
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool system_clock::is_steady;
#endif

    // sys_time / sys_seconds / sys_days
    //   typedef: C++20 spellings for a system_clock time_point at a given
    // precision, back-ported. sys_days is the type the calendar interface
    // is built on, so the names are established here even though the
    // calendar itself is not yet implemented.
#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    template<typename _Duration>
    using sys_time = time_point<system_clock, _Duration>;
#endif

    typedef time_point<system_clock, seconds>   sys_seconds;
    typedef time_point<system_clock, days>      sys_days;

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_SYSTEM_CLOCK_
