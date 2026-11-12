/*******************************************************************************
* djinterp [cpp][utility]                                       atomic_timer.hpp
*
* Lock-free thread-safe timer.
*   timer.hpp's operations over a timer whose whole state is one atomic
* word, changed by compare-and-swap. A timer's state is two things -- the
* time so far, and whether an interval is open -- and timer keeps them in
* separate members; here both are folded into one integer of the duration's
* rep, so a start, a stop or a step is one CAS:
*
*     state = 2 * v + running
*
* where, stopped, v is the elapsed ticks and, running, v is the elapsed ticks
* less the clock's reading when the interval opened, so that the time now is
* v plus the clock's reading now. C's counterpart is c/util/timer/
* timer_atomic.h, which uses the same word.
*
* WHAT THE FOLDING COSTS
* ----------------------
*   - elapsed time saturates at half the rep's range (max_ticks), and the
*     clock's readings, in ticks since its epoch, must stay within it, as
*     steady_clock's do;
*   - a running timer no longer keeps its closed intervals apart from its
*     open one, so decrement takes time from the whole, open interval
*     included, where timer takes it from the closed intervals only, and a
*     clock wound back below an interval's start takes time off where timer
*     counts none; both stop at zero;
*   - there are no children: nesting needs coordination that one word cannot
*     carry. Use mutex_timer for that.
*
* SEE ALSO
* --------
*   timer.hpp        — unsynchronized base
*   mutex_timer.hpp  — lock-policy-based variant (supports children)
*
* PORTABILITY
* -----------
*   Requires C++11 or later, and a signed integral rep.
*
* HOW TO USE
* ----------
*     atomic_timer<> busy;
*
*     busy.start();                       // from any thread
*     busy.stop();
*
* OPTIONS
* -------
*   The clock and the duration, as template arguments, and the limit, fixed at
* construction.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/util/timer/atomic_timer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_UTIL_TIMER_ATOMIC_TIMER_HPP
#define DJINTERP_UTIL_TIMER_ATOMIC_TIMER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <atomic>
#include <chrono>
#include <limits>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "./timer.hpp"         // timer, for merge


NS_DJINTERP


// =========================================================================
// atomic_timer
//   class: a lock-free timer with configurable clock and duration, and an
// optional maximum time limit; timer's operations, without children.
//
//   Template parameter `Clock` must satisfy the Clock named requirement.
//   Template parameter `Duration` must be a std::chrono::duration whose rep
// is a signed integer.
// =========================================================================
template<typename Clock    = std::chrono::steady_clock,
         typename Duration = typename Clock::duration>
class atomic_timer
{
private:
    using self_type = atomic_timer<Clock, Duration>;
    using base_type = timer<Clock, Duration>;
    using rep       = typename Duration::rep;

    static_assert( (std::is_integral<rep>::value) &&
                   (std::is_signed<rep>::value),
                   "atomic_timer: the duration's rep must be a signed "
                   "integer, so the state fits one atomic word" );

public:
    using clock_type    = Clock;
    using duration_type = Duration;
    using rep_type      = rep;

    // max_ticks
    //   constant: the most ticks an atomic_timer holds: half the rep's range,
    // so that v and the running bit fit one word.
    static constexpr rep max_ticks = std::numeric_limits<rep>::max() / 2;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    /**
     * @brief Makes a stopped timer at zero with no maximum limit.
     */
    atomic_timer() noexcept
        : m_state(0),
          m_max(Duration::zero()),
          m_has_max(false)
    {
    }

    /**
     * @brief Makes a stopped timer at zero with a maximum duration limit.
     *
     * @param[in] _max  the limit; the timer is expired once its elapsed time
     *                  reaches or exceeds it.
     */
    explicit atomic_timer(
        Duration _max
    ) noexcept
        : m_state(0),
          m_max(_max),
          m_has_max(true)
    {
    }

    // non-copyable (the state is atomic)
    atomic_timer(const atomic_timer&)            = delete;
    atomic_timer& operator=(const atomic_timer&) = delete;

    // -----------------------------------------------------------------
    // operations
    // -----------------------------------------------------------------

    /**
     * @brief Starts or resumes the timer; does nothing if it is running or
     *        expired.
     */
    void start() noexcept
    {
        (void)try_start();

        return;
    }

    /**
     * @brief Stops the timer, closing the open interval into the elapsed
     *        time; does nothing if it is not running.
     */
    void stop() noexcept
    {
        (void)try_stop();

        return;
    }

    /**
     * @brief Starts the timer, reporting whether this call did.
     *
     * @note Of two threads starting a stopped timer, one starts it and the
     *       other is refused.
     *
     * @return `true` when this call started the timer; `false` when it was
     *         running or expired.
     */
    bool try_start() noexcept
    {
        const rep now   = now_ticks();
        rep       state = m_state.load(std::memory_order_acquire);

        // compute from what was loaded; retry when someone else moved it
        for (;;)
        {
            const rep v = value_of(state);

            // the two states that cannot start
            if ( (running_of(state)) ||
                 ( (m_has_max) &&
                   (m_max.count() <= v) ) )
            {
                return false;
            }

            bool      is_exact = true;
            const rep opened   = held_difference(v,
                                                 now,
                                                 -max_ticks,
                                                 max_ticks,
                                                 is_exact);

            // publish; on failure `state` holds the newer word
            if (m_state.compare_exchange_weak(state,
                                              encode(opened,
                                                     true),
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire))
            {
                return true;
            }
        }
    }

    /**
     * @brief Stops the timer, reporting whether this call did.
     *
     * @return `true` when this call stopped the timer; `false` when it was
     *         not running.
     */
    bool try_stop() noexcept
    {
        const rep now   = now_ticks();
        rep       state = m_state.load(std::memory_order_acquire);

        // compute from what was loaded; retry when someone else moved it
        for (;;)
        {
            // nothing to stop
            if (!running_of(state))
            {
                return false;
            }

            const rep closed = time_of(state,
                                       now);

            // publish; on failure `state` holds the newer word
            if (m_state.compare_exchange_weak(state,
                                              encode(closed,
                                                     false),
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire))
            {
                return true;
            }
        }
    }

    /**
     * @brief Stops the timer and clears all accumulated time, in one store.
     *
     * @post The timer is stopped at zero; its limit is unchanged.
     */
    void reset() noexcept
    {
        m_state.store(0,
                      std::memory_order_release);

        return;
    }

    /**
     * @brief Adds `_amount` to the elapsed time, in one successful CAS.
     *
     * @param[in] _amount  the duration; negative removes time.
     * @return `true` when the full amount applied; `false` when the elapsed
     *         time was held at zero or at max_ticks.
     */
    bool increment(
        Duration _amount
    ) noexcept
    {
        return step(_amount.count(),
                    false);
    }

    /**
     * @brief Removes `_amount` from the elapsed time, in one successful CAS.
     *
     * @note Running, the time comes off the whole, so the open interval may
     *       be shortened.
     *
     * @param[in] _amount  the duration; negative adds time.
     * @return as increment.
     */
    bool decrement(
        Duration _amount
    ) noexcept
    {
        return step(_amount.count(),
                    true);
    }

    /**
     * @brief Three-way comparison by elapsed time, each timer read once.
     *
     * @param[in] _other  the timer to compare with.
     * @return -1, 0 or 1 as this timer has run less than, as long as, or
     *         longer than `_other`.
     */
    int compare(
        const self_type& _other
    ) const noexcept
    {
        const Duration mine   = elapsed();
        const Duration theirs = _other.elapsed();

        return (mine < theirs) ? -1 : ( (theirs < mine) ? 1 : 0 );
    }

    /**
     * @brief Adds `_other`'s elapsed time, read once, in one step.
     *
     * @note Merging a timer into itself doubles its time once.
     *
     * @param[in] _other  the timer whose elapsed time is added.
     * @return as increment.
     */
    bool merge(
        const self_type& _other
    ) noexcept
    {
        return increment(_other.elapsed());
    }

    /**
     * @brief Adds a plain timer's elapsed time, read before the step.
     *
     * @param[in] _other  the plain timer; the caller's, and not synchronized.
     * @return as increment.
     */
    bool merge(
        const base_type& _other
    ) noexcept
    {
        return increment(_other.elapsed());
    }

    // -----------------------------------------------------------------
    // accessors
    // -----------------------------------------------------------------

    /**
     * @brief Returns the elapsed duration of one load.
     *
     * @return the elapsed time, reading the clock only when the timer is
     *         running.
     */
    Duration elapsed() const noexcept
    {
        const rep state = m_state.load(std::memory_order_acquire);

        return Duration(time_of(state,
                                (running_of(state)) ? now_ticks() : rep(0)));
    }

    /**
     * @brief Returns the maximum duration limit.
     *
     * @return the limit, or zero when none is set.
     */
    Duration max() const noexcept
    {
        return m_max;
    }

    /**
     * @brief Whether a maximum duration limit is set.
     *
     * @return `true` when the timer can expire.
     */
    bool has_max() const noexcept
    {
        return m_has_max;
    }

    /**
     * @brief Whether the timer is running.
     *
     * @return the running bit of one load.
     */
    bool running() const noexcept
    {
        return running_of(m_state.load(std::memory_order_acquire));
    }

    /**
     * @brief Whether the timer has reached its limit.
     *
     * @return `true` when a limit is set and the elapsed time meets or
     *         exceeds it.
     */
    bool expired() const noexcept
    {
        return ( (m_has_max) &&
                 (m_max <= elapsed()) );
    }

    /**
     * @brief Returns the time remaining before expiry.
     *
     * @return the limit less the elapsed time; zero when no limit is set or
     *         the timer is expired.
     */
    Duration remaining() const noexcept
    {
        // no limit to approach
        if (!m_has_max)
        {
            return Duration::zero();
        }

        const Duration done = elapsed();

        return (done < m_max) ? (m_max - done) : Duration::zero();
    }

private:
    /**
     * @brief Reads the clock.
     *
     * @return the reading, in ticks of Duration since the clock's epoch.
     */
    static rep now_ticks() noexcept
    {
        return std::chrono::duration_cast<Duration>(
                   Clock::now().time_since_epoch()).count();
    }

    /**
     * @brief Encodes v and the running bit as a state word.
     *
     * @param[in] _v           the value; within +/-max_ticks, so the word
     *                         cannot overflow.
     * @param[in] _is_running  whether an interval is open.
     * @return 2 * `_v` + `_is_running`.
     */
    static constexpr rep encode(
        rep  _v,
        bool _is_running
    ) noexcept
    {
        return static_cast<rep>( (_v * 2) + ( (_is_running) ? 1 : 0 ) );
    }

    /**
     * @brief Whether a state word's interval is open.
     *
     * @param[in] _state  the word.
     * @return its running bit.
     */
    static constexpr bool running_of(
        rep _state
    ) noexcept
    {
        return ( (_state % 2) != 0 );
    }

    /**
     * @brief A state word's v.
     *
     * @param[in] _state  the word.
     * @return (`_state` - its running bit) / 2, exact for either sign.
     */
    static constexpr rep value_of(
        rep _state
    ) noexcept
    {
        return static_cast<rep>( (_state - ( (running_of(_state)) ? 1 : 0 ))
                                 / 2 );
    }

    /**
     * @brief `_a` + `_b`, held to [`_lo`, `_hi`].
     *
     * @note The sum is formed only where it cannot overflow.
     *
     * @param[in]  _a         the value.
     * @param[in]  _b         the amount.
     * @param[in]  _lo        the lower bound.
     * @param[in]  _hi        the upper bound; not below `_lo`.
     * @param[out] _is_exact  cleared when the sum was held.
     * @return the held sum.
     */
    static rep held_sum(
        rep   _a,
        rep   _b,
        rep   _lo,
        rep   _hi,
        bool& _is_exact
    ) noexcept
    {
        // past the rep's top: held at the upper bound
        if ( (rep(0) < _b) &&
             ((std::numeric_limits<rep>::max() - _b) < _a) )
        {
            _is_exact = false;

            return _hi;
        }

        // past the rep's bottom: held at the lower bound
        if ( (_b < rep(0)) &&
             (_a < (std::numeric_limits<rep>::min() - _b)) )
        {
            _is_exact = false;

            return _lo;
        }

        const rep sum = static_cast<rep>(_a + _b);

        // within the rep; now within the bounds
        if (sum < _lo)
        {
            _is_exact = false;

            return _lo;
        }

        // above the upper bound
        if (_hi < sum)
        {
            _is_exact = false;

            return _hi;
        }

        return sum;
    }

    /**
     * @brief `_a` - `_b`, held to [`_lo`, `_hi`], as held_sum.
     *
     * @note The rep's minimum, which has no negation, is taken as a step down
     *       by the maximum and then by one.
     *
     * @param[in]  _a         the value.
     * @param[in]  _b         the amount.
     * @param[in]  _lo        the lower bound.
     * @param[in]  _hi        the upper bound; not below `_lo`.
     * @param[out] _is_exact  cleared when the difference was held.
     * @return the held difference.
     */
    static rep held_difference(
        rep   _a,
        rep   _b,
        rep   _lo,
        rep   _hi,
        bool& _is_exact
    ) noexcept
    {
        // every other amount negates safely
        if (_b != std::numeric_limits<rep>::min())
        {
            return held_sum(_a,
                            static_cast<rep>(-_b),
                            _lo,
                            _hi,
                            _is_exact);
        }

        const rep first = held_sum(_a,
                                   std::numeric_limits<rep>::max(),
                                   _lo,
                                   _hi,
                                   _is_exact);

        return held_sum(first,
                        rep(1),
                        _lo,
                        _hi,
                        _is_exact);
    }

    /**
     * @brief The elapsed ticks a state word stands for.
     *
     * @param[in] _state  the word.
     * @param[in] _now    the clock's reading; used only when running.
     * @return v when stopped; v + `_now`, held to [0, max_ticks], when
     *         running.
     */
    static rep time_of(
        rep _state,
        rep _now
    ) noexcept
    {
        bool is_exact = true;

        return (running_of(_state))
                   ? held_sum(value_of(_state),
                              _now,
                              rep(0),
                              max_ticks,
                              is_exact)
                   : value_of(_state);
    }

    /**
     * @brief Moves the elapsed time by `_amount` ticks with one successful
     *        CAS.
     *
     * @note Stopped, v is the time and moves within [0, max_ticks]; running,
     *       v moves within the range that keeps v + now there. The clock is
     *       read only when a running word is first seen.
     *
     * @param[in] _amount        the ticks.
     * @param[in] _is_decrement  `true` to subtract `_amount`, `false` to add
     *                           it.
     * @return whether the published step applied in full.
     */
    bool step(
        rep  _amount,
        bool _is_decrement
    ) noexcept
    {
        rep  state   = m_state.load(std::memory_order_acquire);
        rep  now     = 0;
        bool has_now = false;

        // compute from what was loaded; retry when someone else moved it
        for (;;)
        {
            const bool is_running = running_of(state);

            // the clock, once, and only for a running timer
            if ( (is_running) &&
                 (!has_now) )
            {
                now     = now_ticks();
                has_now = true;
            }

            bool      is_ignored = true;
            const rep lo         = (is_running)
                                       ? held_difference(rep(0),
                                                         now,
                                                         -max_ticks,
                                                         max_ticks,
                                                         is_ignored)
                                       : rep(0);
            const rep hi         = (is_running)
                                       ? held_difference(max_ticks,
                                                         now,
                                                         -max_ticks,
                                                         max_ticks,
                                                         is_ignored)
                                       : max_ticks;
            bool      is_exact   = true;
            const rep next       = (_is_decrement)
                                       ? held_difference(value_of(state),
                                                         _amount,
                                                         lo,
                                                         hi,
                                                         is_exact)
                                       : held_sum(value_of(state),
                                                  _amount,
                                                  lo,
                                                  hi,
                                                  is_exact);

            // publish; on failure `state` holds the newer word
            if (m_state.compare_exchange_weak(state,
                                              encode(next,
                                                     is_running),
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire))
            {
                return is_exact;
            }
        }
    }

    std::atomic<rep> m_state;    // 2 * v + running; see the file's banner
    Duration         m_max;      // the limit, when m_has_max
    bool             m_has_max;  // whether the timer can expire
};

// before C++17 a static constexpr member that is odr-used needs a definition
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename Clock,
             typename Duration>
    constexpr typename Duration::rep atomic_timer<Clock, Duration>::max_ticks;
#endif  // !D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_TIMER_ATOMIC_TIMER_HPP
