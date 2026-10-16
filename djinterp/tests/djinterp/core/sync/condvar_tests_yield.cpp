/*******************************************************************************
* djinterp [core]                                        condvar_tests_yield.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_yield.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                           condvar_tests_yield.cpp
//   Section I: d_thread_yield - the portable yield hint.
//
//   There is almost nothing to observe about a yield: it takes nothing, returns
// nothing, and its only effect is on a scheduler that is free to ignore it.  So
// the tests here pin the two things that ARE observable and that a spin-loop
// depends on - that it is callable from any number of threads without throwing,
// and that a loop which calls it makes progress rather than starving the thread
// it is waiting for - plus the one structural fact that its `inline` earns: one
// definition, one address, across every TU that includes the header.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // yield_storm
    //   function: hammer the yield from a worker thread and record that the
    // worker came back.
    void
    yield_storm(
        std::atomic<int>* _done,
        int               _iterations
    )
    {
        int i = 0;

        for (i = 0; i < _iterations; ++i)
        {
            d_thread_yield();
        }

        _done->fetch_add(1);

        return;
    }

    // yielding_spinner
    //   function: the documented use - spin on a flag, yielding, until another
    // agent sets it.  A yield that starved its peer would hang here; the
    // deadline is what turns that into a reported failure.
    void
    yielding_spinner(
        const std::atomic<bool>* _flag,
        std::atomic<bool>*       _saw_it,
        const std::atomic<bool>* _abort
    )
    {
        while ( (!_flag->load()) &&
                (!_abort->load()) )
        {
            d_thread_yield();
        }

        _saw_it->store(_flag->load());

        return;
    }
}

/*
hardware_address_from_another_tu
  the address of hardware_concurrency, taken in THIS TU, for the hardware
  section's cross-TU test to compare against its own.
*/
void*
hardware_address_from_another_tu()
{
    return reinterpret_cast<void*>(&hardware_concurrency);
}

/*
tests_yield_is_a_nullary_void_function
  the shape of the yield.
  Tests the following:
  - it takes no arguments and returns void
  - its address has type void(*)(), so it can be stored in a function pointer
    and handed to a backoff policy
*/
bool
tests_yield_is_a_nullary_void_function()
{
    D_CONSTEXPR bool returns_void =
        std::is_same<decltype(d_thread_yield()), void>::value;

    D_CONSTEXPR bool is_nullary =
        std::is_same<decltype(&d_thread_yield), void (*)()>::value;

    static_assert(returns_void && is_nullary,
                  "d_thread_yield must be a nullary void function");

    // and it really can be stored and called through that pointer
    void (*as_pointer)() = &d_thread_yield;

    as_pointer();

    return (returns_void && is_nullary);
}

/*
tests_yield_is_callable
  the yield can simply be called.
  Tests the following:
  - a single call returns
  - a thousand calls in a row return
  - it carries no state: nothing accumulates, nothing to reset
*/
bool
tests_yield_is_callable()
{
    int i = 0;

    d_thread_yield();

    for (i = 0; i < 1000; ++i)
    {
        d_thread_yield();
    }

    return true;
}

/*
tests_yield_does_not_throw
  the yield never throws.
  Tests the following:
  - a thousand calls inside a try block reach the end of it
  - no exception of any type escapes
  This matters because a yield sits inside spin loops, which sit inside
  noexcept paths; a throwing yield would terminate the process rather than
  unwind.
*/
bool
tests_yield_does_not_throw()
{
    int i = 0;

    try
    {
        for (i = 0; i < 1000; ++i)
        {
            d_thread_yield();
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

/*
tests_yield_is_not_declared_noexcept
  ...but it is not DECLARED noexcept, and the difference is visible.
  Tests the following:
  - noexcept(d_thread_yield()) is FALSE
  - even though the C++11 body it forwards to, std::this_thread::yield(), IS
    noexcept
  This test pins current behaviour rather than endorsing it: the function could
  carry noexcept and does not, so a caller cannot rely on it inside a noexcept
  specification of its own.  If the qualifier is ever added, this test is the
  one that should be flipped.
*/
bool
tests_yield_is_not_declared_noexcept()
{
    D_CONSTEXPR bool declared_noexcept = noexcept(d_thread_yield());

    D_CONSTEXPR bool std_yield_is_noexcept =
        noexcept(std::this_thread::yield());

    static_assert(std_yield_is_noexcept,
                  "std::this_thread::yield() is noexcept - the wrapper simply "
                  "does not say so");

    return (!declared_noexcept);
}

/*
tests_yield_from_many_threads
  the yield is safe under concurrency - it has no state to race on.
  Tests the following:
  - four threads each call it a thousand times, concurrently
  - all four return within the deadline
*/
bool
tests_yield_from_many_threads()
{
    std::atomic<int>         done(0);
    std::vector<std::thread> workers;
    int                      i = 0;

    for (i = 0; i < k_waiters; ++i)
    {
        workers.push_back(std::thread(&yield_storm, &done, 1000));
    }

    const bool all_returned = spin_until_count(done, k_waiters, k_deadline_ms);

    for (i = 0; i < static_cast<int>(workers.size()); ++i)
    {
        workers[static_cast<std::size_t>(i)].join();
    }

    return ( all_returned &&
             (done.load() == k_waiters) );
}

/*
tests_yield_in_a_spin_loop_makes_progress
  the documented use: a spin loop that yields sees the other agent's write.
  Tests the following:
  - a worker spins on a flag, yielding each turn, while this thread sets it
  - the worker observes the flag and exits, well inside the deadline
  - a yield that never returned to the scheduler would starve the setter on a
    single-core box and hang here; the deadline turns that into a failure
*/
bool
tests_yield_in_a_spin_loop_makes_progress()
{
    std::atomic<bool> flag(false);
    std::atomic<bool> saw_it(false);
    std::atomic<bool> abort(false);

    std::thread spinner(&yielding_spinner, &flag, &saw_it, &abort);

    sleep_ms(5);
    flag.store(true);

    const bool finished = spin_until(saw_it, k_deadline_ms);

    // rescue: whatever happened, the spinner must be able to leave
    abort.store(true);
    spinner.join();

    return ( finished &&
             saw_it.load() );
}

/*
tests_yield_has_one_definition_across_tus
  `inline` is doing its job: every TU got a definition, and the linker folded
  them into one.
  Tests the following:
  - the address of d_thread_yield taken HERE equals the address taken in the
    hardware-concurrency TU
  - which is also why the suite links at all: without `inline`, ten TUs each
    carrying a definition would be ten duplicate symbols
*/
bool
tests_yield_has_one_definition_across_tus()
{
    void* const here      = reinterpret_cast<void*>(&d_thread_yield);
    void* const elsewhere = yield_address_from_another_tu();

    return ( (here != nullptr) &&
             (here == elsewhere) );
}


NS_END  // testing
NS_END  // djinterp
