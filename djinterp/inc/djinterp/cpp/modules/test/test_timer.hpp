/*******************************************************************************
* djinterp [cpp][test]                                            test_timer.hpp
*
*   DTest's timer for the C++ face: the generic timer
* (cpp/util/timer/timer.hpp), and nothing DTest-specific beyond the
* milliseconds DTest reports in. C's counterpart is
* c/modules/test/test_timer.h, also a wrapper.
*
* DERIVED
* -------
*   DTestTimer<Clock, Duration> is a djinterp::timer<Clock, Duration>: starting,
* stopping, the five operations, the limit and the children are the generic
* timer's own.
*
* TIMED BY HAND
* -------------
*   Nothing here knows about events: a suite starts and stops a test timer
* itself, or binds the handlers of test_timer_event.hpp, the tie-in, to time the
* span between two events.
*
* PORTABILITY
* -----------
*   Requires C++11 or later.
*
* HOW TO USE
* ----------
*     DTestTimer<> t;
*
*     t.start();
*     // the work
*     t.stop();
*     t.elapsed_ms();                     // for the report
*
*   test_timer_event.hpp times a span between two events instead.
*
* OPTIONS
* -------
*   The clock and the duration, as template arguments, and the limit given to
* the constructor.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_timer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.08
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TEST_TIMER_HPP
#define DJINTERP_TEST_TIMER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <chrono>
// djinterp
#include "../../djinterp.hpp"          // framework root
#include "../../util/timer/timer.hpp"  // timer


NS_DJINTERP
NS_TEST


// DTestTimer
//   class: the generic timer, as DTest's, reporting in milliseconds.
//
// Example:
//   DTestTimer<> t;
//   t.start();
//   // ... work ...
//   t.stop();
//   double ms = t.elapsed_ms();
template<typename Clock    = std::chrono::steady_clock,
         typename Duration = typename Clock::duration>
class DTestTimer : public timer<Clock, Duration>
{
public:
    using base_type     = timer<Clock, Duration>;
    using clock_type    = Clock;
    using duration_type = Duration;
    using rep_type      = typename Duration::rep;

    // the generic timer's constructors: (), and (max)
    using base_type::base_type;

    /**
     * @brief Returns the elapsed time in milliseconds, for reports.
     *
     * @return the elapsed duration, an open interval included, in
     *         milliseconds.
     */
    double elapsed_ms() const
    {
        return std::chrono::duration<double, std::milli>(
                   this->elapsed()).count();
    }
};


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_TIMER_HPP
