/*******************************************************************************
* djinterp [cpp][utility]                                 timer_tests_parity.cpp
*
* The timer suite's parity tests. The C++ timer is the C timer's counterpart,
* not a face over it (the owner's ruling of 2026-11-12), so the two are kept one
* behaviour by test: a timer<Clock, nanoseconds> and a struct dj_timer
* (c/util/timer/timer.h), read from the same hand-driven clock and given the
* same operations, must agree after every one on the elapsed time, whether
* they run, whether they have expired and the time remaining, and must report
* the same cuts.
*
*
* path:      /tests/djinterp/cpp/util/timer/timer_tests_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
// djinterp
#include "timer_tests.hpp"  // the suite's declarations
#include "../../../../../inc/djinterp/c/util/timer/timer.h"  // dj_timer
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // int64_t


NS_DJINTERP
NS_TESTING


namespace
{

// parity_clock
//   helper: a hand-driven nanosecond clock that both timers read: the C++
// timer through now(), the C timer through parity_c_clock.
struct parity_clock
{
    using duration   = std::chrono::nanoseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<parity_clock, duration>;

    // is_steady
    //   member: the tests only ever move the clock forward.
    static constexpr bool is_steady = true;

    // now
    //   returns the clock's reading.
    static
    time_point now() D_NOEXCEPT
    {
        return time_point(duration(s_ticks));
    }

    inline static re_std::int64_t s_ticks = 0;
};

// DParityTimer
//   type: the C++ timer under comparison.
using DParityTimer = timer<parity_clock, DNsec>;

// parity_c_clock
//   function: the same clock, as an fn_timer_clock.
re_std::int64_t
parity_c_clock(
    void* _context
)
{
    (void)_context;

    return parity_clock::s_ticks;
}

// k_parity_amounts
//   value: the amounts increment and decrement are given: both ends of the
// int64_t range and their neighbours, and small steps either side of zero.
const re_std::int64_t k_parity_amounts[] =
{
    std::numeric_limits<re_std::int64_t>::lowest(),
    std::numeric_limits<re_std::int64_t>::lowest() + 1,
    -1000,
    -7,
    -1,
    0,
    1,
    7,
    1000,
    std::numeric_limits<re_std::int64_t>::max() - 1,
    std::numeric_limits<re_std::int64_t>::max()
};

// k_parity_limits
//   value: the limits the timers are made with; -1 is none.
const re_std::int64_t k_parity_limits[] =
{
    -1,
    0,
    1,
    1000,
    1000000,
    std::numeric_limits<re_std::int64_t>::max()
};

// k_parity_advances
//   value: how far the clock moves at a time.
const re_std::int64_t k_parity_advances[] =
{
    0,
    1,
    7,
    1000,
    999999,
    (static_cast<re_std::int64_t>(1) << 40)
};

// k_parity_scripts / k_parity_script_length
//   value: the scripts run per limit, and the operations in each.
const std::size_t k_parity_scripts       = 200u;
const std::size_t k_parity_script_length = 40u;

// parity_pair
//   struct: the two timers under comparison, built alike.
struct parity_pair
{
    DParityTimer    cpp;
    struct dj_timer c;
};

// parity_make
//   function: makes both timers stopped at zero with `_limit` (-1 for none),
// the C timer on parity_c_clock.
void
parity_make(
    parity_pair& _pair,
    re_std::int64_t _limit
)
{
    if (_limit < 0)
    {
        _pair.cpp = DParityTimer();
        d_timer_init(&_pair.c);
    }
    else
    {
        _pair.cpp = DParityTimer(DNsec(_limit));
        (void)d_timer_init_limited(&_pair.c,
                                   _limit);
    }

    d_timer_set_clock(&_pair.c,
                      &parity_c_clock,
                      NULL);

    return;
}

// parity_same
//   function: whether the two timers agree on everything they report.
bool
parity_same(
    const parity_pair& _pair
)
{
    return ( (_pair.cpp.elapsed().count() == d_timer_elapsed(&_pair.c))     &&
             (_pair.cpp.running()         == d_timer_running(&_pair.c))     &&
             (_pair.cpp.expired()         == d_timer_expired(&_pair.c))     &&
             (_pair.cpp.remaining().count() == d_timer_remaining(&_pair.c)) );
}

// parity_next
//   function: the next number of a fixed xorshift sequence, so every run
// tries the same scripts.
re_std::uint64_t
parity_next(
    re_std::uint64_t& _state
)
{
    _state ^= (_state << 13);
    _state ^= (_state >> 7);
    _state ^= (_state << 17);

    return _state;
}

// parity_apply
//   function: applies operation `_op` with `_amount` to `_pair` (and, for
// compare and merge, `_other`). Returns true when both timers reported the
// same result.
bool
parity_apply(
    parity_pair& _pair,
    parity_pair& _other,
    unsigned     _op,
    re_std::int64_t _amount
)
{
    switch (_op)
    {
        case 0:
            _pair.cpp.start();
            (void)d_timer_start(&_pair.c);

            return true;

        case 1:
            _pair.cpp.stop();
            (void)d_timer_stop(&_pair.c);

            return true;

        case 2:
            return ( _pair.cpp.increment(DNsec(_amount)) ==
                     d_timer_increment(&_pair.c, _amount) );

        case 3:
            return ( _pair.cpp.decrement(DNsec(_amount)) ==
                     d_timer_decrement(&_pair.c, _amount) );

        case 4:
            _pair.cpp.reset();
            d_timer_reset(&_pair.c);

            return true;

        case 5:
            return ( _pair.cpp.merge(_other.cpp) ==
                     d_timer_merge(&_pair.c, &_other.c) );

        case 6:
            return ( _pair.cpp.merge(_pair.cpp) ==
                     d_timer_merge(&_pair.c, &_pair.c) );

        default:
            return ( _pair.cpp.compare(_other.cpp) ==
                     d_timer_compare(&_pair.c, &_other.c) );
    }
}

}  // namespace


/*
tests_timer_parity_scripts
  Scripted operations, against the C timer.
  Tests the following:
  - for every limit, two pairs of timers are driven by fixed pseudo-random
    scripts of starts, stops, clock advances, increments and decrements by any
    amount, resets, merges (of the other timer and of itself) and compares
  - after every operation both timers of a pair report the same elapsed time,
    running state, expiry and remaining time
  - increment, decrement and merge report the same cuts, and compare the same
    order
*/
bool tests_timer_parity_scripts()
{
    const std::size_t amount_count =
        (sizeof(k_parity_amounts) / sizeof(k_parity_amounts[0]));
    const std::size_t limit_count =
        (sizeof(k_parity_limits) / sizeof(k_parity_limits[0]));
    const std::size_t advance_count =
        (sizeof(k_parity_advances) / sizeof(k_parity_advances[0]));
    re_std::uint64_t     state = 0x9E3779B97F4A7C15ull;
    std::size_t       l     = 0;

    for (l = 0; l < limit_count; ++l)
    {
        std::size_t script = 0;

        for (script = 0; script < k_parity_scripts; ++script)
        {
            parity_pair pairs[2];
            std::size_t step = 0;

            parity_clock::s_ticks = 0;
            parity_make(pairs[0], k_parity_limits[l]);
            parity_make(pairs[1], k_parity_limits[script % limit_count]);

            for (step = 0; step < k_parity_script_length; ++step)
            {
                const re_std::uint64_t r      = parity_next(state);
                const unsigned      op     = static_cast<unsigned>(r % 9u);
                const std::size_t   which  = static_cast<std::size_t>(
                                                 (r >> 8) & 1u);
                const re_std::int64_t  amount =
                    k_parity_amounts[(r >> 16) % amount_count];
                parity_pair&        pair   = pairs[which];
                parity_pair&        other  = pairs[1u - which];

                // the ninth operation moves the clock
                if (op == 8u)
                {
                    parity_clock::s_ticks +=
                        k_parity_advances[(r >> 24) % advance_count];

                    continue;
                }

                if ( (!parity_apply(pair, other, op, amount)) ||
                     (!parity_same(pairs[0])) ||
                     (!parity_same(pairs[1])) )
                {
                    std::printf("    [parity] limit %lld, script %zu, step "
                                "%zu: operation %u, amount %lld: C++ %lld, "
                                "C %lld\n",
                                static_cast<long long>(k_parity_limits[l]),
                                script,
                                step,
                                op,
                                static_cast<long long>(amount),
                                static_cast<long long>(
                                    pair.cpp.elapsed().count()),
                                static_cast<long long>(
                                    d_timer_elapsed(&pair.c)));

                    return false;
                }
            }
        }
    }

    parity_clock::s_ticks = 0;

    return true;
}


NS_END  // testing
NS_END  // djinterp
