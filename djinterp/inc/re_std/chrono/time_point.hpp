/*******************************************************************************
* djinterp [re_std]                                               time_point.hpp
*
* the time_point class template:
*   A duration measured from a clock's epoch, tagged with the clock it was
* measured against:
*
*       time_point<system_clock, milliseconds>
*
*   THE CLOCK IS PART OF THE TYPE BECAUSE EPOCHS ARE NOT COMPARABLE:
*   A system_clock reading and a steady_clock reading are both "a number
* of ticks since some origin", but the origins are unrelated -- one is
* the Unix epoch, the other is typically system boot. Subtracting them
* would produce a plausible-looking duration that means nothing. Carrying
* the clock in the type makes that mistake a compile error instead, which
* is the single most valuable thing this class does.
*
*   For the same reason there is no conversion between time_points of
* different clocks. C++20 adds clock_cast for the conversions that are
* actually defined; that is part of the calendar and time-zone surface
* and is not implemented here.
*
*   DEFAULT CONSTRUCTION IS THE EPOCH, NOT GARBAGE:
*   This is the opposite of duration, whose default constructor leaves
* the count uninitialised. The asymmetry is the standard's and it is
* deliberate: a duration has no distinguished value, while a time_point
* has an obvious one. `time_point<C> t;` is the epoch.
*
*   CONSTEXPR: the observers are constexpr from C++11, matching std. The
* two mutators are constexpr from C++14 -- re_std ahead of std, which
* waited for C++17 -- and use RE_STD_CONSTEXPR_CPP14 rather than RE_STD_CONSTEXPR
* for the further reason that constexpr on a C++11 non-const member
* implies const.
*
*
* path:      /inc/re_std/chrono/time_point.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_HPP
#define RE_STD_CHRONO_TIME_POINT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./duration.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_convertible.hpp"


namespace re_std
{

namespace chrono
{

    // time_point
    //   class: a point on Clock's timeline, held as a Duration measured
    // from that clock's epoch.
    template<typename Clock,
             typename Duration = typename Clock::duration>
    class time_point
    {
        static_assert(internal::is_duration<Duration>::value,
            "re_std::chrono::time_point: second parameter must be a duration");

    public:
        // clock
        //   typedef: the clock this point is measured against. Present so
        // generic code can name it; the class never calls into it.
        typedef Clock                          clock;

        // duration / rep / period
        //   typedef: the offset from the epoch, and its parts.
        typedef Duration                       duration;
        typedef typename duration::rep          rep;
        typedef typename duration::period       period;

    private:
        duration    m_d;

    public:

        // time_point
        //   function: default constructor -- the clock's epoch. Unlike
        // duration, this DOES initialise. See the header comment.
        RE_STD_CONSTEXPR time_point()
            : m_d(duration::zero())
        {}

        // time_point
        //   function: construct at a given offset from the epoch.
        // Explicit, because a duration is a length and a time_point is a
        // position, and conflating them is the error this type exists to
        // prevent.
        RE_STD_CONSTEXPR explicit time_point(const duration& _d)
            : m_d(_d)
        {}

        // time_point
        //   function: converting constructor, SAME CLOCK ONLY. Allowed
        // exactly when the underlying duration conversion is allowed, so
        // a coarsening conversion is refused here too.
        template<typename Duration2,
                 typename = typename enable_if<
                     is_convertible<Duration2, duration>::value >::type>
        RE_STD_CONSTEXPR time_point(const time_point<clock, Duration2>& _t)
            : m_d(_t.time_since_epoch())
        {}

        // time_since_epoch
        //   function: the offset from the clock's epoch. Meaningful only
        // in combination with the clock -- see the header comment.
        RE_STD_CONSTEXPR duration time_since_epoch() const
        {
            return m_d;
        }

        // operator+= / operator-=
        //   function: move the point along its timeline.
        RE_STD_CONSTEXPR_CPP14 time_point& operator+=(const duration& _d)
        {
            m_d += _d;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 time_point& operator-=(const duration& _d)
        {
            m_d -= _d;
            return *this;
        }

        // min
        //   function: the earliest representable point.
        static RE_STD_CONSTEXPR time_point min() RE_STD_NOEXCEPT
        {
            return time_point(duration::min());
        }

        // max
        //   function: the latest representable point.
        static RE_STD_CONSTEXPR time_point max() RE_STD_NOEXCEPT
        {
            return time_point(duration::max());
        }
    };

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_HPP
