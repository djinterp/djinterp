/*******************************************************************************
* djinterp [core]                                     condvar_tests_hardware.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_hardware.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                        condvar_tests_hardware.cpp
//   Section II: hardware_concurrency - how many agents the machine can really
// run at once.
//
//   The header is careful, and correct, about the one thing that trips people
// up here: the answer may be ZERO.  The standard permits it ("returns 0 if the
// value cannot be determined"), and every platform branch in the header can
// produce it - std::thread's own query is allowed to, GetSystemInfo is not
// reached on a non-Windows box, and sysconf can fail.  So the suite refuses to
// assert that the value is positive.  What it asserts instead is what a CALLER
// may rely on: the value is unsigned, it is stable, it is safe to ask for from
// any thread, and a caller who needs a thread count of its own must clamp it -
// which is the edge case the whole section exists to nail down.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // k_plausible_ceiling
    //   constant: no machine this code will run on has more than this many
    // hardware threads.  A query that returned garbage - a sign-flipped
    // sysconf, say, cast into an unsigned - would land far above it.
    D_STATIC D_CONSTEXPR unsigned k_plausible_ceiling = 4096;

    // usable_thread_count
    //   function: what a caller must actually write.  hardware_concurrency() is
    // a HINT and may be 0; a pool that sized itself from it directly would
    // spawn no threads at all.  Clamping to at least one is the whole of the
    // contract.
    D_INLINE unsigned
    usable_thread_count()
    {
        const unsigned reported = hardware_concurrency();

        return (reported == 0) ? 1u : reported;
    }

    // hardware_reader
    //   function: read the value from a worker thread and publish what it saw.
    void
    hardware_reader(
        std::atomic<unsigned>* _seen,
        std::atomic<int>*      _done
    )
    {
        _seen->store(hardware_concurrency());
        _done->fetch_add(1);

        return;
    }
}

/*
yield_address_from_another_tu
  the address of d_thread_yield, taken in THIS TU, for the yield section's
  cross-TU test to compare against its own.
*/
void*
yield_address_from_another_tu()
{
    return reinterpret_cast<void*>(&d_thread_yield);
}

/*
tests_hardware_returns_unsigned
  the shape of the query.
  Tests the following:
  - it takes no arguments and returns unsigned - so the result can never be
    negative, and a failed query has to be signalled some other way (it is: with
    zero)
  - its address has type unsigned(*)()
*/
bool
tests_hardware_returns_unsigned()
{
    D_CONSTEXPR bool returns_unsigned =
        std::is_same<decltype(hardware_concurrency()), unsigned>::value;

    D_CONSTEXPR bool is_nullary =
        std::is_same<decltype(&hardware_concurrency), unsigned (*)()>::value;

    static_assert(returns_unsigned && is_nullary,
                  "hardware_concurrency must be a nullary unsigned function");

    return (returns_unsigned && is_nullary);
}

/*
tests_hardware_is_deterministic
  the query answers the same way every time.
  Tests the following:
  - sixty-four consecutive calls return one value
  - so a caller may cache it, and two parts of a program that ask separately
    will agree
*/
bool
tests_hardware_is_deterministic()
{
    const unsigned first = hardware_concurrency();
    int            i     = 0;

    for (i = 0; i < 64; ++i)
    {
        // check if the query has drifted
        if (hardware_concurrency() != first)
        {
            return false;
        }
    }

    return true;
}

/*
tests_hardware_may_return_zero
  zero is a LEGAL answer, and a caller must survive it.
  Tests the following:
  - the suite does not assert the value is positive: the header says it may be
    0 when the count cannot be determined, and the standard says the same
  - a caller that needs a thread count must clamp - usable_thread_count() is
    that caller, and it returns at least 1 whatever the query said
  - if the query DID return zero on this machine, the clamp is what saved us,
    and the test still passes
*/
bool
tests_hardware_may_return_zero()
{
    const unsigned reported = hardware_concurrency();
    const unsigned usable   = usable_thread_count();

    // the clamp holds either way - that is the entire point
    const bool clamped = (usable >= 1u);

    const bool clamp_is_faithful =
        (reported == 0u) ? (usable == 1u) : (usable == reported);

    return (clamped && clamp_is_faithful);
}

/*
tests_hardware_is_plausible
  when the query DOES answer, the answer is sane.
  Tests the following:
  - a non-zero result is at most k_plausible_ceiling
  - which is what would catch a platform branch whose signed count went negative
    and came back as a vast unsigned (the POSIX branch casts a long from
    sysconf, and guards it with n > 0 - this is the test of that guard)
*/
bool
tests_hardware_is_plausible()
{
    const unsigned reported = hardware_concurrency();

    // check if the platform declined to answer
    if (reported == 0u)
    {
        return true;   // zero is legal; nothing further to say
    }

    return (reported <= k_plausible_ceiling);
}

/*
tests_hardware_matches_the_standard
  under C++11 the wrapper IS std::thread::hardware_concurrency().
  Tests the following:
  - the two agree exactly
  - so the wrapper adds portability below C++11 and nothing else above it - it
    neither clamps, nor caches, nor defaults
*/
bool
tests_hardware_matches_the_standard()
{
    return (hardware_concurrency() == std::thread::hardware_concurrency());
}

/*
tests_hardware_does_not_throw
  the query never throws - and, like the yield, does not SAY so.
  Tests the following:
  - a hundred calls inside a try block reach the end of it
  - noexcept(hardware_concurrency()) is FALSE, even though
    std::thread::hardware_concurrency() is noexcept
  Current behaviour, pinned: a caller cannot put this call in its own noexcept
  specification and have it mean anything.
*/
bool
tests_hardware_does_not_throw()
{
    int i = 0;

    try
    {
        for (i = 0; i < 100; ++i)
        {
            (void)hardware_concurrency();
        }
    }
    catch (...)
    {
        return false;
    }

    D_CONSTEXPR bool declared_noexcept = noexcept(hardware_concurrency());

    D_CONSTEXPR bool std_query_is_noexcept =
        noexcept(std::thread::hardware_concurrency());

    static_assert(std_query_is_noexcept,
                  "std::thread::hardware_concurrency() is noexcept - the "
                  "wrapper simply does not say so");

    return (!declared_noexcept);
}

/*
tests_hardware_is_callable_from_many_threads
  the query is safe to ask concurrently, and every asker gets the same answer.
  Tests the following:
  - four threads read it at once; all four return within the deadline
  - every one of them saw the value this thread sees
*/
bool
tests_hardware_is_callable_from_many_threads()
{
    const unsigned              expected = hardware_concurrency();
    std::atomic<int>            done(0);
    std::vector<std::atomic<unsigned>*> seen;
    std::vector<std::thread>    workers;
    int                         i     = 0;
    bool                        agree = true;

    for (i = 0; i < k_waiters; ++i)
    {
        seen.push_back(new std::atomic<unsigned>(0));
    }

    for (i = 0; i < k_waiters; ++i)
    {
        workers.push_back(std::thread(&hardware_reader,
                                      seen[static_cast<std::size_t>(i)],
                                      &done));
    }

    const bool all_returned = spin_until_count(done, k_waiters, k_deadline_ms);

    for (i = 0; i < k_waiters; ++i)
    {
        workers[static_cast<std::size_t>(i)].join();
    }

    for (i = 0; i < k_waiters; ++i)
    {
        // check if any thread disagreed with this one
        if (seen[static_cast<std::size_t>(i)]->load() != expected)
        {
            agree = false;
        }

        delete seen[static_cast<std::size_t>(i)];
    }

    return (all_returned && agree);
}

/*
tests_hardware_has_one_definition_across_tus
  `inline` is doing its job here too.
  Tests the following:
  - the address of hardware_concurrency taken HERE equals the address taken in
    the thread-yield TU
*/
bool
tests_hardware_has_one_definition_across_tus()
{
    void* const here      = reinterpret_cast<void*>(&hardware_concurrency);
    void* const elsewhere = hardware_address_from_another_tu();

    return ( (here != nullptr) &&
             (here == elsewhere) );
}


NS_END  // testing
NS_END  // djinterp
