/******************************************************************************
* djinterp [re_std]                                        duration_typedefs.hpp
*
* the predefined duration typedefs:
*   nanoseconds through hours (C++11), plus days, weeks, months and years
* (C++20, back-ported here to C++11).
*
*   THE REPRESENTATION WIDTHS ARE MINIMA THE STANDARD FIXES, NOT CHOICES:
*   [time.syn] requires each typedef's rep to hold at least a stated
* range -- nanoseconds must reach +/-292 years, hours +/-40000 years, and
* so on. Those minima are what make the family interconvertible without
* surprises. re_std uses int_least64_t and int_least32_t rather than
* int64_t / int32_t so the typedefs still exist on a target without
* exact-width types, where the exact-width names are not declared at all.
*
*   MONTHS AND YEARS ARE AVERAGES, AND THAT IS NOT A BUG:
*   years is 365.2425 days -- the mean Gregorian year -- expressed as the
* exact ratio 31556952/1. months is exactly years/12, or 2629746 seconds.
* Neither is a calendar month or a calendar year, and neither knows about
* leap days:
*
*       sys_days(...) + years(1)     NOT the same date next year
*
*   Calendar-correct arithmetic needs year_month_day, which is C++20's
* calendar and is NOT part of this module (see the umbrella). These
* typedefs are for scaling and for expressing coarse timeouts; they are
* the right tool for "no more than 2 days" and the wrong tool for "this
* time next year".
*
*   WHY days AND weeks ARE EXACT WHERE months AND years ARE NOT:
*   A day is defined as exactly 86400 seconds, which sidesteps leap
* seconds by not modelling them at all -- the standard's own decision.
* Months and years cannot be exact at any fixed length, so an average was
* chosen instead.
*
*   BACK-PORT: std added days / weeks / months / years in C++20. re_std
* provides all four from C++11, since each is a ratio typedef and needs
* nothing C++20 supplies -- a nine-year lead.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_typedefs.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_TYPEDEFS_
#define DJINTERP_RE_STD_CHRONO_DURATION_TYPEDEFS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "../ratio/ratio.hpp"
#include "../ratio/ratio_typedefs.hpp"
#include "../cstdint/cstdint.hpp"


NS_RESTD

namespace chrono
{

    // nanoseconds
    //   typedef: 1/1000000000 second. Range at least +/-292 years.
    typedef duration<std::int_least64_t, nano>              nanoseconds;

    // microseconds
    //   typedef: 1/1000000 second. Range at least +/-292000 years.
    typedef duration<std::int_least64_t, micro>             microseconds;

    // milliseconds
    //   typedef: 1/1000 second. Range at least +/-292000000 years.
    typedef duration<std::int_least64_t, milli>             milliseconds;

    // seconds
    //   typedef: one second. Range at least +/-292000000000 years.
    typedef duration<std::int_least64_t>                    seconds;

    // minutes
    //   typedef: 60 seconds. A 32-bit rep suffices for the required range.
    typedef duration<std::int_least32_t, ratio<60> >        minutes;

    // hours
    //   typedef: 3600 seconds.
    typedef duration<std::int_least32_t, ratio<3600> >      hours;

    // days
    //   typedef: exactly 86400 seconds. C++20, back-ported. Not a
    // calendar day -- leap seconds are not modelled.
    typedef duration<std::int_least32_t, ratio<86400> >     days;

    // weeks
    //   typedef: exactly 7 days. C++20, back-ported.
    typedef duration<std::int_least32_t, ratio<604800> >    weeks;

    // months
    //   typedef: the AVERAGE Gregorian month, 2629746 seconds -- exactly
    // years/12. C++20, back-ported. See the header comment before using
    // this for date arithmetic.
    typedef duration<std::int_least32_t, ratio<2629746> >   months;

    // years
    //   typedef: the AVERAGE Gregorian year, 31556952 seconds
    // (365.2425 days). C++20, back-ported. Not a calendar year.
    typedef duration<std::int_least32_t, ratio<31556952> >  years;

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_TYPEDEFS_
