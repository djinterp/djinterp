/*******************************************************************************
* djinterp [cpp][utility]                                      counter_event.hpp
*
* The event tie-in for counters: counter_handler is an event handler that
* increments, decrements, adds to or resets a counter each time it is called.
* It couples a handler to a counter and does nothing else: the caller binds
* it wherever handlers are bound, so this file includes no event header.
*   C's counterpart is c/util/counter/counter_event.h, a struct dj_event_step
* for the event core.
*
* ONE HANDLER FOR EVERY COUNTER
* -----------------------------
*   counter_handler<Counter> takes any type with value_type, increment,
* decrement and reset -- counter, atomic_counter, mutex_counter, or a type of
* the caller's -- and no counter includes this file.
*
* WHAT IS COUNTED IS THE CALLER'S CHOICE
* --------------------------------------
*   One event may move several counters (several handlers on its tag), and one
* counter may be moved by several events (a handler on each).
*
* THE CALLER OWNS THE COUNTER
* ---------------------------
*   A handler keeps a pointer to it, so a counter must outlive every handler
* bound to move it, or they are unbound first.
*
* PORTABILITY
* -----------
*   Requires C++17 or later, as the counters do; below that this file is
* empty.
*
* HOW TO USE
* ----------
*   Make a handler and bind it where handlers are bound:
*
*     D_EVENT(on_bytes, int);
*     DEventDispatcher d;
*     counter<int>     bytes;
*
*     d.bind<on_bytes>(make_counter_handler(bytes, DCounterEventOp::add));
*     d.fire<on_bytes>(512);              // bytes.value() == 512
*
* OPTIONS
* -------
*   What each call does, per handler: the operation (DCounterEventOp) and the
* amount for increment and decrement.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/util/counter/counter_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_UTIL_COUNTER_EVENT_HPP
#define DJINTERP_UTIL_COUNTER_EVENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, as counter.hpp is.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"  // framework root


NS_DJINTERP

// DCounterEventOp
//   enum: what a binding does to its counter when its event fires.
enum class DCounterEventOp
{
    increment,  // move up by the binding's amount
    decrement,  // move down by the binding's amount
    add,        // move by the occurrence's first payload value
    reset       // return to the start
};


NS_INTERNAL

/**
 * @brief The amount an `add` binding moves by, for an occurrence with no
 *        payload: zero.
 *
 * @tparam ValueType  the counter's value type.
 * @return `ValueType{0}`.
 */
template<typename ValueType>
constexpr ValueType counter_event_payload_amount() noexcept
{
    return ValueType{0};
}

/**
 * @brief The amount an `add` binding moves by: the occurrence's first payload
 *        value, where it converts to `ValueType`.
 *
 * @tparam ValueType  the counter's value type.
 * @tparam First      the first payload value's type.
 * @tparam Rest       the other payload values' types; ignored.
 * @param[in] _first  the first payload value.
 * @return `_first` as a `ValueType`, or zero when it does not convert.
 */
template<typename ValueType,
         typename First,
         typename... Rest>
constexpr ValueType counter_event_payload_amount(
    const First& _first,
    const Rest&...
) noexcept
{
    // only a value that converts can be added
    if constexpr (std::is_convertible_v<const First&, ValueType>)
    {
        return static_cast<ValueType>(_first);
    }
    else
    {
        (void)_first;

        return ValueType{0};
    }
}

/**
 * @brief Applies `_op` to `_counter` once, for one occurrence.
 *
 * @tparam Counter  the counter's type.
 * @tparam Payload  the occurrence's payload types.
 * @param[in,out] _counter  the counter.
 * @param[in]     _op       what the occurrence does.
 * @param[in]     _amount   the step, for increment and decrement.
 * @param[in]     _payload  the occurrence's payload; `add` moves by its first
 *                          value.
 */
template<typename Counter,
         typename... Payload>
void apply_counter_event(
    Counter&                     _counter,
    DCounterEventOp              _op,
    typename Counter::value_type _amount,
    const Payload&...            _payload
)
{
    using value_type = typename Counter::value_type;

    // one of the counter's functions per operation
    switch (_op)
    {
        case DCounterEventOp::increment:
        {
            (void)_counter.increment(_amount);

            break;
        }

        case DCounterEventOp::decrement:
        {
            (void)_counter.decrement(_amount);

            break;
        }

        case DCounterEventOp::add:
        {
            const value_type amount =
                counter_event_payload_amount<value_type>(_payload...);

            // a negative amount moves down
            if (amount < value_type{0})
            {
                (void)_counter.decrement(value_type{0} - amount);
            }
            else
            {
                (void)_counter.increment(amount);
            }

            break;
        }

        case DCounterEventOp::reset:
        {
            _counter.reset();

            break;
        }
    }

    return;
}

NS_END  // internal


// counter_handler
//   class: an event handler that applies one operation to one counter per
// call. Called with an occurrence's payload, it ignores it, except that `add`
// moves by its first value. It returns no verdict, so it never stops an event
// reaching the handlers bound after it.
//
// Example:
//   mutex_counter<long> hits;
//   d.bind<on_hit>(counter_handler(hits));   // C++17's deduction
template<typename Counter>
class counter_handler
{
public:
    using counter_type = Counter;
    using value_type   = typename Counter::value_type;

    /**
     * @brief Aims a handler at a counter.
     *
     * @param[in,out] _counter  the counter; must outlive the handler.
     * @param[in]     _op       what each call does; increment by default.
     * @param[in]     _amount   the step for increment and decrement.
     */
    explicit counter_handler(
        Counter&        _counter,
        DCounterEventOp _op     = DCounterEventOp::increment,
        value_type      _amount = value_type{1}
    ) noexcept
        : m_counter(&_counter),
          m_op(_op),
          m_amount(_amount)
    {
    }

    /**
     * @brief Applies the operation once, for one occurrence.
     *
     * @tparam Payload  the occurrence's payload types.
     * @param[in] _payload  the occurrence's payload; `add` moves by its first
     *                      value, the other operations ignore it.
     */
    template<typename... Payload>
    void operator()(
        const Payload&... _payload
    ) const
    {
        internal::apply_counter_event(*m_counter,
                                      m_op,
                                      m_amount,
                                      _payload...);

        return;
    }

    /**
     * @brief Returns the counter the handler moves.
     *
     * @return the counter.
     */
    Counter& target() const noexcept
    {
        return *m_counter;
    }

    /**
     * @brief Returns what each call does.
     *
     * @return the operation.
     */
    DCounterEventOp op() const noexcept
    {
        return m_op;
    }

    /**
     * @brief Returns the step for increment and decrement.
     *
     * @return the amount.
     */
    value_type amount() const noexcept
    {
        return m_amount;
    }

private:
    Counter*        m_counter;  // the caller's
    DCounterEventOp m_op;       // what a call does
    value_type      m_amount;   // for increment and decrement
};

/**
 * @brief Makes the handler that applies `_op` to `_counter` per call, to bind
 *        to an event.
 *
 * @tparam Counter  any type with value_type, increment, decrement and reset.
 * @param[in,out] _counter  the counter; must outlive the handler.
 * @param[in]     _op       what each call does; increment by default.
 * @param[in]     _amount   the step for increment and decrement.
 * @return the handler.
 */
template<typename Counter>
counter_handler<Counter> make_counter_handler(
    Counter&                     _counter,
    DCounterEventOp              _op     = DCounterEventOp::increment,
    typename Counter::value_type _amount = typename Counter::value_type{1}
) noexcept
{
    return counter_handler<Counter>(_counter,
                                    _op,
                                    _amount);
}

NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COUNTER_EVENT_HPP
