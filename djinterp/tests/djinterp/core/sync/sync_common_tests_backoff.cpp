/*******************************************************************************
* djinterp [core]                                  sync_common_tests_backoff.cpp
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests_backoff.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]               sync_common_tests_backoff.cpp
//   Section II: cpu_relax and the adaptive backoff.  This is a WAIT, not a
// synchronization primitive: it owns no state anyone else observes, just a
// step counter that lengthens the spin until it gives up and yields the core.
// So the tests are about shape and termination rather than ordering - it must
// be noexcept, cheap, copyable, and above all it must always come back
// promptly, at every step and after any number of calls.  A backoff that
// failed to terminate would hang every spin loop in the module, so the
// escalation tests are written with real deadlines.

// std
#include <chrono>
// djinterp
#include "sync_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_cpu_relax_is_callable_and_noexcept
  the hardware pause hint is callable everywhere and never throws.
  Tests the following:
  - cpu_relax() is a noexcept expression
  - calling it repeatedly returns normally on this target
  The header maps it to a pause / yield instruction where one exists and to
  nothing where none does; either way the call must compile and be safe to put
  in the tightest loop in the module.
*/
bool
tests_cpu_relax_is_callable_and_noexcept()
{
    const bool is_noexcept = noexcept(cpu_relax());

    static_assert(noexcept(cpu_relax()),
                  "cpu_relax must be noexcept");

    for (int i = 0; i < 1000; ++i)
    {
        cpu_relax();
    }

    return is_noexcept;
}

/*
tests_backoff_starts_at_the_shortest_spin
  a fresh backoff begins at its cheapest step.
  Tests the following:
  - a default-constructed backoff completes its first pause() essentially
    immediately (well inside a generous deadline)
  The first wait is a single relax; a backoff that started deep in its
  escalation would add latency to the common case, where the very first retry
  usually succeeds.
*/
bool
tests_backoff_starts_at_the_shortest_spin()
{
    backoff bo;

    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    bo.pause();

    const long long elapsed_ms =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());

    return (elapsed_ms < 100);
}

/*
tests_backoff_pause_is_noexcept
  waiting cannot throw.
  Tests the following:
  - pause() and reset() are both noexcept, as is default construction
  A backoff is used inside lock-free retry loops, which are typically noexcept
  themselves; a throwing wait would poison them.
*/
bool
tests_backoff_pause_is_noexcept()
{
    backoff bo;

    const bool ops_noexcept =
        ( noexcept(bo.pause()) &&
          noexcept(bo.reset()) &&
          std::is_nothrow_default_constructible<backoff>::value );

    static_assert(std::is_nothrow_default_constructible<backoff>::value,
                  "backoff construction must be noexcept");

    return ops_noexcept;
}

/*
tests_backoff_escalates_without_hanging
  the escalation is bounded - it never becomes an unbounded spin.
  Tests the following:
  - calling pause() far past the spin cap (200 times, where the cap is a
    handful of steps) completes well inside a deadline
  Past the cap each pause() yields rather than spinning longer, so the cost
  stops growing.  Had the escalation kept doubling without a ceiling, this
  test would hang rather than fail - which is exactly why it carries a
  deadline instead of asserting on a step count.
*/
bool
tests_backoff_escalates_without_hanging()
{
    backoff bo;

    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (int i = 0; i < 200; ++i)
    {
        bo.pause();
    }

    const long long elapsed_ms =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());

    return (elapsed_ms < 3000);
}

/*
tests_backoff_reset_returns_to_the_start
  reset() puts the wait back to its cheapest step.
  Tests the following:
  - a backoff driven deep into its escalation, then reset(), costs about what
    a FRESH backoff costs over the same number of pauses - and dramatically
    less than the same escalated backoff left un-reset
  A spin loop calls reset() when it makes progress, so the next contention
  episode starts cheap again rather than inheriting the last one's penalty.
  The comparison is relative (reset vs. not-reset, measured on this machine in
  this run) rather than an absolute duration, because absolute spin timings
  are far too fast and too load-dependent to assert on directly - a reset that
  quietly did nothing would slip straight past a wall-clock deadline.
*/
bool
tests_backoff_reset_returns_to_the_start()
{
    // how long a short burst of pauses costs from a FRESH backoff
    backoff fresh;

    const std::chrono::steady_clock::time_point fresh_start =
        std::chrono::steady_clock::now();

    for (int i = 0; i < 4; ++i)
    {
        fresh.pause();
    }

    const long long fresh_ns =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - fresh_start).count());

    // the same burst from a backoff that has been escalated then RESET
    backoff recycled;
    for (int i = 0; i < 5; ++i)
    {
        recycled.pause();
    }
    recycled.reset();

    const std::chrono::steady_clock::time_point reset_start =
        std::chrono::steady_clock::now();

    for (int i = 0; i < 4; ++i)
    {
        recycled.pause();
    }

    const long long reset_ns =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - reset_start).count());

    // and the same burst from one that was NOT reset - the control
    backoff escalated;
    for (int i = 0; i < 5; ++i)
    {
        escalated.pause();
    }

    const std::chrono::steady_clock::time_point escalated_start =
        std::chrono::steady_clock::now();

    for (int i = 0; i < 4; ++i)
    {
        escalated.pause();
    }

    const long long escalated_ns =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - escalated_start).count());

    // A reset backoff spins from step 0 again, so its burst is far cheaper
    // than the escalated one's (which is yielding to the scheduler by now).
    // Generous margins keep this robust under load: the effect is orders of
    // magnitude, so a 2x separation is a conservative bar.
    const bool reset_is_cheaper = (reset_ns * 2 < escalated_ns);

    // ... and it lands in the same league as a brand-new backoff.
    const bool reset_matches_fresh =
        (reset_ns < (fresh_ns * 8) + 1000000);

    return (reset_is_cheaper && reset_matches_fresh);
}

/*
tests_backoff_is_copyable
  a backoff is a value, not a resource.
  Tests the following:
  - backoff is copy- and move-constructible, and assignable both ways
  It holds only a step counter and owns nothing, so it is deliberately NOT
  one of the pinned types: a caller may keep one per retry loop, pass it by
  value, or store it in a struct without ceremony.
*/
bool
tests_backoff_is_copyable()
{
    const bool is_value_like =
        ( std::is_copy_constructible<backoff>::value &&
          std::is_copy_assignable<backoff>::value    &&
          std::is_move_constructible<backoff>::value &&
          std::is_move_assignable<backoff>::value );

    // and a copy is independently usable
    backoff original;
    original.pause();

    backoff duplicate(original);
    duplicate.pause();
    duplicate.reset();

    return is_value_like;
}

/*
tests_backoff_costs_only_a_counter
  the whole wait is one small integer.
  Tests the following:
  - sizeof(backoff) is no larger than an unsigned int
  There is no lock, no allocation and no atomic inside: a backoff can live in
  a register-sized slot on the stack of the hottest retry loop.
*/
bool
tests_backoff_costs_only_a_counter()
{
    return (sizeof(backoff) <= sizeof(unsigned int));
}

/*
tests_backoff_survives_a_long_spin_loop
  the counter does not misbehave when a loop spins for a very long time.
  Tests the following:
  - several thousand pause() calls on one backoff complete inside a deadline
    and leave the object usable (a following reset() and pause() still work)
  A pathological contention episode must degrade to repeated yields, never to
  overflow-driven nonsense or an ever-lengthening spin.
*/
bool
tests_backoff_survives_a_long_spin_loop()
{
    backoff bo;

    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (int i = 0; i < 5000; ++i)
    {
        bo.pause();
    }

    const long long elapsed_ms =
        static_cast<long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());

    // still usable afterwards
    bo.reset();
    bo.pause();

    return (elapsed_ms < 5000);
}


NS_END  // testing
NS_END  // djinterp
