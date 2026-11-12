/*******************************************************************************
* djinterp [cpp][utility]                                        timer_event.hpp
*
* The event tie-in for timers: timer_handler is an event handler that starts,
* stops, moves or resets a timer each time it is called. It couples a handler
* to a timer and does nothing else: the caller binds it wherever handlers are
* bound, so this file includes no event header. counter_event.hpp's design,
* for time.
*   Timing the span between two events is two handlers on one timer, start
* and stop, each bound to its own event, as shown below.
*   C's counterpart is c/util/timer/timer_event.h, a struct dj_event_step for
* the event core.
*
* ONE HANDLER FOR EVERY TIMER
* ---------------------------
*   timer_handler<Timer> takes any type with duration_type, start, stop,
* increment, decrement and reset -- timer, atomic_timer, mutex_timer, or a type
* of the caller's -- and no timer includes this file.
*
* THE CALLER OWNS THE TIMER
* -------------------------
*   A handler keeps a pointer to it, so a timer must outlive every handler bound
* to act on it, or they are unbound first.
*
* PORTABILITY
* -----------
*   Requires C++17 or later; below that this file is empty.
*
* HOW TO USE
* ----------
*   Timing the span between two events is two handlers on one timer:
*
*     timer<> span;
*
*     d.bind<on_open>(make_timer_handler(span, DTimerEventOp::start));
*     d.bind<on_close>(make_timer_handler(span, DTimerEventOp::stop));
*
* OPTIONS
* -------
*   What each call does, per handler: the operation (DTimerEventOp) and the
* duration for increment and decrement.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/util/timer/timer_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_UTIL_TIMER_EVENT_HPP
#define DJINTERP_UTIL_TIMER_EVENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"  // framework root


NS_DJINTERP

// DTimerEventOp
//   enum: what a binding does to its timer when its event fires.
enum class DTimerEventOp
{
    start,      // open an interval
    stop,       // close the open interval
    increment,  // add the binding's amount
    decrement,  // remove the binding's amount
    add,        // add the occurrence's first payload value, as a duration
    reset       // stop, and return to zero
};


NS_INTERNAL

/**
 * @brief The duration an `add` binding adds, for an occurrence with no
 *        payload: zero.
 *
 * @tparam Duration  the timer's duration type.
 * @return `Duration::zero()`.
 */
template<typename Duration>
constexpr Duration timer_event_payload_amount() noexcept
{
    return Duration::zero();
}

/**
 * @brief The duration an `add` binding adds: the occurrence's first payload
 *        value, where it converts to `Duration`.
 *
 * @tparam Duration  the timer's duration type.
 * @tparam First     the first payload value's type.
 * @tparam Rest      the other payload values' types; ignored.
 * @param[in] _first  the first payload value.
 * @return `_first` as a `Duration`, or zero when it does not convert.
 */
template<typename Duration,
         typename First,
         typename... Rest>
constexpr Duration timer_event_payload_amount(
    const First& _first,
    const Rest&...
) noexcept
{
    // only a value that converts can be added
    if constexpr (std::is_convertible_v<const First&, Duration>)
    {
        return static_cast<Duration>(_first);
    }
    else
    {
        (void)_first;

        return Duration::zero();
    }
}

/**
 * @brief Applies `_op` to `_timer` once, for one occurrence.
 *
 * @tparam Timer    the timer's type.
 * @tparam Payload  the occurrence's payload types.
 * @param[in,out] _timer    the timer.
 * @param[in]     _op       what the occurrence does.
 * @param[in]     _amount   the duration, for increment and decrement.
 * @param[in]     _payload  the occurrence's payload; `add` adds its first
 *                          value.
 */
template<typename Timer,
         typename... Payload>
void apply_timer_event(
    Timer&                        _timer,
    DTimerEventOp                 _op,
    typename Timer::duration_type _amount,
    const Payload&...             _payload
)
{
    using duration_type = typename Timer::duration_type;

    // one of the timer's functions per operation
    switch (_op)
    {
        case DTimerEventOp::start:
        {
            _timer.start();

            break;
        }

        case DTimerEventOp::stop:
        {
            _timer.stop();

            break;
        }

        case DTimerEventOp::increment:
        {
            (void)_timer.increment(_amount);

            break;
        }

        case DTimerEventOp::decrement:
        {
            (void)_timer.decrement(_amount);

            break;
        }

        case DTimerEventOp::add:
        {
            (void)_timer.increment(
                timer_event_payload_amount<duration_type>(_payload...));

            break;
        }

        case DTimerEventOp::reset:
        {
            _timer.reset();

            break;
        }
    }

    return;
}

NS_END  // internal


// timer_handler
//   class: an event handler that applies one operation to one timer per
// call. Called with an occurrence's payload, it ignores it, except that `add`
// adds its first value as a duration. It returns no verdict, so it never stops
// an event reaching the handlers bound after it. A start on a running timer,
// or a stop on a stopped one, does nothing, so an event that arrives twice
// does not corrupt the time.
//
// Example:
//   atomic_timer<> busy;
//   d.bind<on_begin>(timer_handler(busy, DTimerEventOp::start));
template<typename Timer>
class timer_handler
{
public:
    using timer_type    = Timer;
    using duration_type = typename Timer::duration_type;

    /**
     * @brief Aims a handler at a timer.
     *
     * @param[in,out] _timer   the timer; must outlive the handler.
     * @param[in]     _op      what each call does.
     * @param[in]     _amount  the duration for increment and decrement.
     */
    timer_handler(
        Timer&        _timer,
        DTimerEventOp _op,
        duration_type _amount = duration_type::zero()
    ) noexcept
        : m_timer(&_timer),
          m_op(_op),
          m_amount(_amount)
    {
    }

    /**
     * @brief Applies the operation once, for one occurrence.
     *
     * @tparam Payload  the occurrence's payload types.
     * @param[in] _payload  the occurrence's payload; `add` adds its first
     *                      value, the other operations ignore it.
     */
    template<typename... Payload>
    void operator()(
        const Payload&... _payload
    ) const
    {
        internal::apply_timer_event(*m_timer,
                                    m_op,
                                    m_amount,
                                    _payload...);

        return;
    }

    /**
     * @brief Returns the timer the handler acts on.
     *
     * @return the timer.
     */
    Timer& target() const noexcept
    {
        return *m_timer;
    }

    /**
     * @brief Returns what each call does.
     *
     * @return the operation.
     */
    DTimerEventOp op() const noexcept
    {
        return m_op;
    }

    /**
     * @brief Returns the duration for increment and decrement.
     *
     * @return the amount.
     */
    duration_type amount() const noexcept
    {
        return m_amount;
    }

private:
    Timer*        m_timer;   // the caller's
    DTimerEventOp m_op;      // what a call does
    duration_type m_amount;  // for increment and decrement
};

/**
 * @brief Makes the handler that applies `_op` to `_timer` per call, to bind to
 *        an event.
 *
 * @tparam Timer  any type with duration_type, start, stop, increment,
 *                decrement and reset.
 * @param[in,out] _timer   the timer; must outlive the handler.
 * @param[in]     _op      what each call does.
 * @param[in]     _amount  the duration for increment and decrement.
 * @return the handler.
 */
template<typename Timer>
timer_handler<Timer> make_timer_handler(
    Timer&                        _timer,
    DTimerEventOp                 _op,
    typename Timer::duration_type _amount =
        Timer::duration_type::zero()
) noexcept
{
    return timer_handler<Timer>(_timer,
                                _op,
                                _amount);
}

NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_TIMER_EVENT_HPP
