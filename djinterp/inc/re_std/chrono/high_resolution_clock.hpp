/*******************************************************************************
* djinterp [re_std]                                    high_resolution_clock.hpp
*
* the high_resolution_clock typedef:
*   The standard permits this name to be an alias for system_clock, an
* alias for steady_clock, or a distinct clock of its own, and implementations
* disagree: libstdc++ makes it system_clock, libc++ makes it steady_clock.
* That disagreement is exactly the kind of behavioural divergence re_std
* exists to remove, so re_std fixes one rule:
*
*     high_resolution_clock IS steady_clock where a monotonic source
*     exists, and system_clock otherwise.
*
*   IT IS A TYPEDEF, NOT A CLASS, AND THAT IS OBSERVABLE:
*   high_resolution_clock::time_point and steady_clock::time_point are
* therefore the SAME type on most platforms, and the two are freely
* interchangeable. Code that relies on them being distinct -- overloading
* on them, say -- will not behave as intended. Since the standard allows
* precisely this aliasing, such code was never portable.
*
*   PREFER NAMING THE CLOCK YOU MEAN:
*   The name promises resolution, which is the least useful property of
* the three: the predefined clocks already count in nanoseconds or
* microseconds, and the real question is always whether the clock is
* steady. Write steady_clock to time an operation and system_clock to
* record when it happened. This alias is provided for compatibility and
* for generic code that names it, not as a recommendation.
*
*
* path:      /inc/re_std/chrono/high_resolution_clock.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_HIGH_RESOLUTION_CLOCK_HPP
#define RE_STD_CHRONO_HIGH_RESOLUTION_CLOCK_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./steady_clock.hpp"
#include "./system_clock.hpp"


namespace re_std
{

namespace chrono
{

#if RE_STD_HAS_MONOTONIC_CLOCK

    // high_resolution_clock
    //   typedef: steady_clock. A steady source is available, so the
    // steady clock is strictly the better answer.
    typedef steady_clock            high_resolution_clock;

#else

    // high_resolution_clock
    //   typedef: system_clock. No monotonic source, so steady_clock is
    // already a re-tagged wall clock and naming it here would add a layer
    // of indirection without adding a guarantee.
    typedef system_clock            high_resolution_clock;

#endif

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_HIGH_RESOLUTION_CLOCK_HPP
