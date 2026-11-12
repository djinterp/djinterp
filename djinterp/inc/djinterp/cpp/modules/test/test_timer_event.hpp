/*******************************************************************************
* djinterp [cpp][test]                                      test_timer_event.hpp
*
* The event tie-in for DTest's timer on the C++ face: a start and a stop
* handler on one test timer, so the span between two events is timed. It
* couples timer_event.hpp's handler to test_timer.hpp and does nothing else:
* the caller binds the handlers wherever handlers are bound, so this file
* includes no event header. Neither test_timer.hpp nor DTest's runners
* include it.
*   C's counterpart is c/modules/test/test_timer_event.h.
*
* THE CALLER OWNS THE TIMER
* -------------------------
*   The handlers keep a pointer to it, so it must outlive every handler bound to
* act on it, or they are unbound first.
*
* PORTABILITY
* -----------
*   Requires C++17 or later, as timer_event.hpp does; below that this file
* is empty.
*
* HOW TO USE
* ----------
*     DTestTimer<> span;
*     auto handlers = make_test_timer_handlers(span);
*
*     d.bind<on_open>(handlers.first);    // starts it
*     d.bind<on_close>(handlers.second);  // stops it
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_timer_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TEST_TIMER_EVENT_HPP
#define DJINTERP_TEST_TIMER_EVENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, as timer_event.hpp is.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <utility>
// djinterp
#include "../../djinterp.hpp"                // framework root
#include "../../util/timer/timer_event.hpp"  // timer_handler
#include "./test_timer.hpp"                  // DTestTimer


NS_DJINTERP
NS_TEST


/**
 * @brief Makes the two handlers that time a span on a test timer: the first
 *        starts it, the second stops it.
 *
 * @note Bind each to its own event. A start while running, or a stop while
 *       stopped, does nothing, so an event that arrives twice does not
 *       corrupt the time.
 *
 * @tparam Clock     the timer's clock; deduced.
 * @tparam Duration  the timer's duration; deduced.
 * @param[in,out] _timer  the timer; must outlive the handlers.
 * @return the start handler, then the stop handler.
 */
template<typename Clock,
         typename Duration>
std::pair<timer_handler<DTestTimer<Clock, Duration>>,
          timer_handler<DTestTimer<Clock, Duration>>>
make_test_timer_handlers(
    DTestTimer<Clock, Duration>& _timer
) noexcept
{
    return std::make_pair(make_timer_handler(_timer,
                                             DTimerEventOp::start),
                          make_timer_handler(_timer,
                                             DTimerEventOp::stop));
}


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_TIMER_EVENT_HPP
