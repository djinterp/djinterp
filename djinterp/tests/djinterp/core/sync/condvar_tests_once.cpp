/*******************************************************************************
* djinterp [core]                                         condvar_tests_once.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_once.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                            condvar_tests_once.cpp
//   Section III: portable_once - run this exactly once, however many threads
// arrive.
//
//   The class is four lines of forwarding around std::call_once, and that is
// precisely why it needs testing: everything interesting about it is inherited,
// and inherited guarantees are the ones people assume rather than read.  Three
// of them are pinned here because code gets them wrong:
//
//     - the flag guards the FLAG, not the callable.  Once it has fired, a later
//       call with a DIFFERENT callable does nothing at all.
//     - an exception from the callable does NOT consume the flag.  It
//       propagates, and the next caller runs the callable again.  "Exactly
//       once" means "exactly one SUCCESSFUL completion".
//     - the arguments are FORWARDED, not decay-copied.  Unlike std::thread and
//       std::async, a reference passed here arrives as a reference, and the
//       callable can write through it.
//
//   And the guarantee the whole thing exists for - one winner, and every other
// caller sees the winner's writes when call() returns - is tested under real
// contention rather than asserted.

// std
#include <stdexcept>
#include <string>
// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // contender
    //   function: a thread that waits at the gate, then races everyone else
    // into the same portable_once.  The gate is a plain atomic spin - the
    // suite's rendezvous may not be built from the primitives under test.
    void
    contender(
        portable_once*          _once,
        const std::atomic<bool>* _go,
        std::atomic<int>*       _body_runs,
        std::atomic<int>*       _returned,
        int*                    _shared_value,
        std::atomic<int>*       _saw_the_value
    )
    {
        while (!_go->load())
        {
            d_thread_yield();
        }

        _once->call([_body_runs, _shared_value]()
                    {
                        *_shared_value = 42;
                        _body_runs->fetch_add(1);
                    });

        // call() has returned, so the winner's write happens-before this read:
        // a PLAIN int is enough, and that it is enough is the guarantee
        if (*_shared_value == 42)
        {
            _saw_the_value->fetch_add(1);
        }

        _returned->fetch_add(1);

        return;
    }

    // greeter
    //   type: a functor - one of the callable shapes std::call_once's INVOKE
    // must accept.
    struct greeter
    {
        std::atomic<int>* calls;

        void
        operator()() const
        {
            calls->fetch_add(1);

            return;
        }
    };

    // target
    //   type: an object with a member function, for the pointer-to-member form
    // of INVOKE.
    struct target
    {
        int hits;

        target()
            : hits(0)
        {}

        void
        hit()
        {
            ++hits;

            return;
        }
    };

    // free_bump
    //   function: a plain function pointer, for the third form of INVOKE.
    void
    free_bump(
        std::atomic<int>* _calls
    )
    {
        _calls->fetch_add(1);

        return;
    }
}

/*
tests_once_is_default_constructible
  a fresh portable_once is armed and has not fired.
  Tests the following:
  - it is default-constructible (the class defaults that constructor)
  - constructing one and then calling it runs the callable, so the fresh state
    really is "not yet run"
*/
bool
tests_once_is_default_constructible()
{
    D_CONSTEXPR bool constructible =
        std::is_default_constructible<portable_once>::value;

    static_assert(constructible,
                  "portable_once must be default-constructible");

    std::atomic<int> calls(0);
    portable_once    once;

    once.call(call_counter(&calls));

    return ( constructible &&
             (calls.load() == 1) );
}

/*
tests_once_is_not_copyable
  a portable_once cannot be copied - the flag is the identity.
  Tests the following:
  - the copy constructor and copy assignment are deleted, and the traits say so
  - which is right: copying a fired flag would hand out a second "already done"
    to a caller whose work has not been done
*/
bool
tests_once_is_not_copyable()
{
    D_CONSTEXPR bool no_copy_ctor =
        !std::is_copy_constructible<portable_once>::value;

    D_CONSTEXPR bool no_copy_assign =
        !std::is_copy_assignable<portable_once>::value;

    static_assert(no_copy_ctor && no_copy_assign,
                  "portable_once must not be copyable");

    return (no_copy_ctor && no_copy_assign);
}

/*
tests_once_is_not_movable
  ...and it cannot be MOVED either, which the header never says.
  Tests the following:
  - is_move_constructible and is_move_assignable are both false
  - not because the moves are deleted, but because DECLARING the deleted copy
    suppresses their implicit declaration, and overload resolution then falls
    back to the deleted copy
  The practical consequence is worth knowing before you meet it: a portable_once
  cannot be stored in a std::vector, returned from a factory, or held in a type
  that wants to be movable.  Hold it by reference, or behind a pointer.
*/
bool
tests_once_is_not_movable()
{
    D_CONSTEXPR bool no_move_ctor =
        !std::is_move_constructible<portable_once>::value;

    D_CONSTEXPR bool no_move_assign =
        !std::is_move_assignable<portable_once>::value;

    static_assert(no_move_ctor && no_move_assign,
                  "portable_once must not be movable");

    return (no_move_ctor && no_move_assign);
}

/*
tests_once_holds_one_flag_and_nothing_else
  the wrapper adds no state of its own.
  Tests the following:
  - sizeof(portable_once) is sizeof(std::once_flag): one member, no padding, no
    bookkeeping
  - it is a class, and it has no vtable
*/
bool
tests_once_holds_one_flag_and_nothing_else()
{
    const bool same_size = (sizeof(portable_once) == sizeof(std::once_flag));

    D_CONSTEXPR bool plain_class =
        ( std::is_class<portable_once>::value          &&
          !std::is_polymorphic<portable_once>::value );

    static_assert(plain_class, "portable_once must be a plain class");

    return (same_size && plain_class);
}

/*
tests_once_runs_the_callable
  the callable runs.
  Tests the following:
  - one call() invokes the callable exactly once
*/
bool
tests_once_runs_the_callable()
{
    std::atomic<int> calls(0);
    portable_once    once;

    once.call(call_counter(&calls));

    return (calls.load() == 1);
}

/*
tests_once_runs_it_only_once
  five calls, one invocation.
  Tests the following:
  - calling call() repeatedly from ONE thread runs the callable the first time
    and never again
  - the later calls still return normally - they are no-ops, not errors
*/
bool
tests_once_runs_it_only_once()
{
    std::atomic<int> calls(0);
    portable_once    once;
    int              i = 0;

    for (i = 0; i < 5; ++i)
    {
        once.call(call_counter(&calls));
    }

    return (calls.load() == 1);
}

/*
tests_once_ignores_a_later_callable
  the flag guards the FLAG, not the callable.
  Tests the following:
  - after a first callable has run, a call() with a DIFFERENT callable does not
    run it
  - so a portable_once is a one-shot gate on a piece of state, and not a cache
    keyed on what you passed it.  Two initializers behind one flag means the
    second one silently never happens.
*/
bool
tests_once_ignores_a_later_callable()
{
    std::atomic<int> first_calls(0);
    std::atomic<int> second_calls(0);
    portable_once    once;

    once.call(call_counter(&first_calls));
    once.call(call_counter(&second_calls));

    return ( (first_calls.load()  == 1) &&
             (second_calls.load() == 0) );
}

/*
tests_once_runs_exactly_once_under_contention
  the whole point: many threads, one invocation.
  Tests the following:
  - four threads wait at a gate and then race into the same portable_once
  - the body runs exactly ONCE
  - all four calls return (nobody is left holding the flag)
*/
bool
tests_once_runs_exactly_once_under_contention()
{
    portable_once            once;
    std::atomic<bool>        go(false);
    std::atomic<int>         body_runs(0);
    std::atomic<int>         returned(0);
    std::atomic<int>         saw_the_value(0);
    int                      shared_value = 0;
    std::vector<std::thread> racers;
    int                      i = 0;

    for (i = 0; i < k_waiters; ++i)
    {
        racers.push_back(std::thread(&contender,
                                     &once,
                                     &go,
                                     &body_runs,
                                     &returned,
                                     &shared_value,
                                     &saw_the_value));
    }

    go.store(true);

    const bool all_returned =
        spin_until_count(returned, k_waiters, k_deadline_ms);

    for (i = 0; i < k_waiters; ++i)
    {
        racers[static_cast<std::size_t>(i)].join();
    }

    return ( all_returned            &&
             (body_runs.load() == 1) &&
             (returned.load()  == k_waiters) );
}

/*
tests_once_publishes_the_result_to_every_caller
  the memory guarantee: the losers see the winner's writes.
  Tests the following:
  - four threads race into one portable_once whose body writes a PLAIN int
  - every thread, after its call() returned, reads that int and sees the value
  - a plain (non-atomic) int is sufficient, and that it is sufficient IS the
    guarantee: the completion of the winner's invocation synchronizes-with the
    return of every other call on the same flag.  Without it, this test would be
    a data race, and the losers could read a stale zero.
*/
bool
tests_once_publishes_the_result_to_every_caller()
{
    portable_once            once;
    std::atomic<bool>        go(false);
    std::atomic<int>         body_runs(0);
    std::atomic<int>         returned(0);
    std::atomic<int>         saw_the_value(0);
    int                      shared_value = 0;
    std::vector<std::thread> racers;
    int                      i = 0;

    for (i = 0; i < k_waiters; ++i)
    {
        racers.push_back(std::thread(&contender,
                                     &once,
                                     &go,
                                     &body_runs,
                                     &returned,
                                     &shared_value,
                                     &saw_the_value));
    }

    go.store(true);

    const bool all_returned =
        spin_until_count(returned, k_waiters, k_deadline_ms);

    for (i = 0; i < k_waiters; ++i)
    {
        racers[static_cast<std::size_t>(i)].join();
    }

    return ( all_returned                     &&
             (body_runs.load() == 1)          &&
             (saw_the_value.load() == k_waiters) );
}

/*
tests_once_forwards_its_arguments
  the arguments reach the callable.
  Tests the following:
  - a by-value argument arrives with its value
  - several arguments arrive in order
  - an rvalue is forwarded as an rvalue (the callable's rvalue overload is the
    one that runs)
*/
bool
tests_once_forwards_its_arguments()
{
    std::atomic<int> sum(0);
    std::atomic<int> took_an_rvalue(0);
    portable_once    once;
    portable_once    once_rvalue;

    once.call([&sum](int _a, int _b, int _c)
              {
                  sum.store((_a * 100) + (_b * 10) + _c);
              },
              1, 2, 3);

    // an rvalue argument keeps its value category through the forward
    once_rvalue.call([&took_an_rvalue](std::string&& _moved)
                     {
                         took_an_rvalue.store(
                             static_cast<int>(_moved.size()));
                     },
                     std::string("four"));

    return ( (sum.load() == 123) &&
             (took_an_rvalue.load() == 4) );
}

/*
tests_once_forwards_by_reference
  a reference passed to call() arrives AS a reference.
  Tests the following:
  - a callable taking int& writes through it, and the caller's variable changes
  - so portable_once does NOT decay-copy its arguments the way std::thread and
    std::async do, and std::ref is not needed here
  This is the difference that bites: the same argument list handed to a thread
  would have been copied, and the write would have gone into the copy.
*/
bool
tests_once_forwards_by_reference()
{
    portable_once once;
    int           value = 0;

    once.call([](int& _out)
              {
                  _out = 42;
              },
              value);

    return (value == 42);
}

/*
tests_once_accepts_any_callable
  every INVOKE-able shape works.
  Tests the following:
  - a lambda
  - a plain function pointer, with an argument
  - a functor object with operator()
  - a pointer-to-member-function plus the object to call it on
*/
bool
tests_once_accepts_any_callable()
{
    std::atomic<int> lambda_calls(0);
    std::atomic<int> function_calls(0);
    std::atomic<int> functor_calls(0);
    target           object;

    portable_once as_lambda;
    portable_once as_function;
    portable_once as_functor;
    portable_once as_member;

    greeter functor;
    functor.calls = &functor_calls;

    as_lambda.call([&lambda_calls]()
                   {
                       lambda_calls.fetch_add(1);
                   });

    as_function.call(&free_bump, &function_calls);

    as_functor.call(functor);

    as_member.call(&target::hit, &object);

    return ( (lambda_calls.load()   == 1) &&
             (function_calls.load() == 1) &&
             (functor_calls.load()  == 1) &&
             (object.hits           == 1) );
}

/*
tests_once_propagates_an_exception
  an exception from the callable comes out of call().
  Tests the following:
  - a throwing callable's exception escapes call() to the caller
  - it is the same exception, not a wrapped or translated one
*/
bool
tests_once_propagates_an_exception()
{
    portable_once once;
    bool          caught  = false;
    bool          message = false;

    try
    {
        once.call([]()
                  {
                      throw std::runtime_error("boom");
                  });
    }
    catch (const std::runtime_error& _e)
    {
        caught  = true;
        message = (std::string(_e.what()) == "boom");
    }
    catch (...)
    {
        return false;   // translated into something else: wrong
    }

    return (caught && message);
}

/*
tests_once_retries_after_an_exception
  a throw does NOT consume the flag.
  Tests the following:
  - a callable that throws leaves the portable_once unfired
  - the next call() runs the callable AGAIN
  - only a callable that RETURNS sets the flag; the third attempt here succeeds,
    and a fourth call does nothing
  "Exactly once" means exactly one SUCCESSFUL completion.  Initialization code
  that can fail therefore gets retried by whichever caller comes next - which is
  usually what you want, and never what you assumed.
*/
bool
tests_once_retries_after_an_exception()
{
    portable_once    once;
    std::atomic<int> attempts(0);
    int              i = 0;

    // three calls: the first two throw, the third succeeds
    for (i = 0; i < 3; ++i)
    {
        try
        {
            once.call([&attempts]()
                      {
                          const int attempt = attempts.fetch_add(1) + 1;

                          // check if this attempt is one of the failing ones
                          if (attempt < 3)
                          {
                              throw std::runtime_error("not yet");
                          }
                      });
        }
        catch (const std::runtime_error&)
        {
            // expected for the first two
        }
    }

    const bool retried_until_it_succeeded = (attempts.load() == 3);

    // ...and now that it HAS succeeded, the flag is consumed for good
    once.call([&attempts]()
              {
                  attempts.fetch_add(100);
              });

    const bool flag_now_holds = (attempts.load() == 3);

    return (retried_until_it_succeeded && flag_now_holds);
}

/*
tests_once_instances_are_independent
  one flag, one gate.
  Tests the following:
  - two portable_once objects each fire their own callable
  - firing the first does not fire, or consume, the second
*/
bool
tests_once_instances_are_independent()
{
    std::atomic<int> first_calls(0);
    std::atomic<int> second_calls(0);
    portable_once    first;
    portable_once    second;

    first.call(call_counter(&first_calls));
    first.call(call_counter(&first_calls));

    const bool first_fired_once = (first_calls.load() == 1);
    const bool second_untouched = (second_calls.load() == 0);

    second.call(call_counter(&second_calls));

    return ( first_fired_once            &&
             second_untouched            &&
             (second_calls.load() == 1)  &&
             (first_calls.load()  == 1) );
}


NS_END  // testing
NS_END  // djinterp
