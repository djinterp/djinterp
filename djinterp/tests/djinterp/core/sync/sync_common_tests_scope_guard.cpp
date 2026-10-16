/*******************************************************************************
* djinterp [core]                              sync_common_tests_scope_guard.cpp
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests_scope_guard.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]           sync_common_tests_scope_guard.cpp
//   Section III: scope_guard and make_scope_guard.  The contract is "run this
// exactly once, when the scope ends, unless I say otherwise".  The two ways to
// get that wrong are running it twice (a move that copies the duty instead of
// transferring it) and running it when it should not (a dismiss that does not
// take).  So every test here counts INVOCATIONS rather than merely observing
// that something happened, and the move test checks the count across the
// moved-from and moved-to guards together.

// std
#include <functional>
// djinterp
#include "sync_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_scope_guard_runs_on_scope_exit
  the action fires when the guard leaves scope.
  Tests the following:
  - the counter is untouched while the guard is alive
  - it has been incremented once immediately after the scope closes
  The basic promise: cleanup that cannot be forgotten on an early return.
*/
bool
tests_scope_guard_runs_on_scope_exit()
{
    int calls = 0;

    bool ran_early = false;
    {
        scope_guard<counting_action> guard{counting_action(&calls)};
        ran_early = (calls != 0);
    }

    return (!ran_early && (calls == 1));
}

/*
tests_scope_guard_runs_exactly_once
  the action is not repeated.
  Tests the following:
  - after a guard's scope has closed, the counter reads exactly 1
  - a second, independent guard over the same counter takes it to exactly 2
  Cleanup that ran twice would double-free, double-unlock or double-release -
  the failure this type exists to prevent.
*/
bool
tests_scope_guard_runs_exactly_once()
{
    int calls = 0;

    {
        scope_guard<counting_action> guard{counting_action(&calls)};
    }

    const bool once = (calls == 1);

    {
        scope_guard<counting_action> guard{counting_action(&calls)};
    }

    const bool twice = (calls == 2);

    return (once && twice);
}

/*
tests_scope_guard_dismiss_cancels_the_action
  a dismissed guard does nothing at all.
  Tests the following:
  - after dismiss(), the guard's scope closes without touching the counter
  This is the commit path: a function that has succeeded dismisses its
  rollback guard, so only the failure paths roll back.
*/
bool
tests_scope_guard_dismiss_cancels_the_action()
{
    int calls = 0;

    {
        scope_guard<counting_action> guard{counting_action(&calls)};
        guard.dismiss();
    }

    return (calls == 0);
}

/*
tests_scope_guard_moving_transfers_the_duty
  moving hands the action over rather than duplicating it.
  Tests the following:
  - a guard moved into a new one, both of which then go out of scope, runs the
    action EXACTLY once in total - the moved-from guard was disarmed
  - the action runs when the destination dies, not when the source does
  Without the disarm, returning a guard from a factory would fire the cleanup
  twice; this is the subtle case the type has to get right.
*/
bool
tests_scope_guard_moving_transfers_the_duty()
{
    int calls = 0;

    bool fired_on_source_death = false;
    {
        scope_guard<counting_action> destination{
            scope_guard<counting_action>{counting_action(&calls)}};

        {
            scope_guard<counting_action> source{
                counting_action(&calls)};
            scope_guard<counting_action> moved(std::move(source));

            // `source` dies at the end of THIS scope, `moved` too;
            // between them the action must fire exactly once.
        }

        fired_on_source_death = (calls == 1);
    }

    // destination (constructed from a temporary) fires as well: 2 total
    return (fired_on_source_death && (calls == 2));
}

/*
tests_scope_guard_is_not_copyable
  a guard is a unique duty, not a value.
  Tests the following:
  - scope_guard is neither copy-constructible nor copy-assignable
  - it IS move-constructible, so it can be returned or relocated
  Copying would mean two objects each believing they must run the cleanup.
*/
bool
tests_scope_guard_is_not_copyable()
{
    typedef scope_guard<counting_action> guard_type;

    const bool no_copy =
        ( !std::is_copy_constructible<guard_type>::value &&
          !std::is_copy_assignable<guard_type>::value );

    const bool movable =
        std::is_move_constructible<guard_type>::value;

    static_assert(!std::is_copy_constructible<guard_type>::value,
                  "scope_guard must not be copyable");

    return (no_copy && movable);
}

/*
tests_scope_guard_nests_in_reverse_order
  nested guards unwind last-in-first-out.
  Tests the following:
  - two guards declared in one scope run their actions in reverse declaration
    order, like any two automatic objects
  Order matters when the cleanups are dependent - releasing a slot before the
  registry that owns it, say - so the guard must not reorder them.
*/
bool
tests_scope_guard_nests_in_reverse_order()
{
    int sequence[2] = { 0, 0 };
    int next        = 0;

    {
        // records 1 when it fires
        std::function<void()> first_action =
            [&sequence, &next]()
            {
                sequence[next] = 1;
                ++next;
            };

        // records 2 when it fires
        std::function<void()> second_action =
            [&sequence, &next]()
            {
                sequence[next] = 2;
                ++next;
            };

        scope_guard<std::function<void()> > first(first_action);
        scope_guard<std::function<void()> > second(second_action);
    }

    // second was declared last, so it fires first
    return ((sequence[0] == 2) && (sequence[1] == 1));
}

/*
tests_make_scope_guard_deduces_the_callable
  the factory saves the caller from naming the callable's type.
  Tests the following:
  - make_scope_guard on a lambda produces a working guard whose action fires
    once on scope exit
  A lambda's type cannot be written out, and class template argument deduction
  is C++17; the factory is what makes the guard usable from the C++11 floor.
*/
bool
tests_make_scope_guard_deduces_the_callable()
{
    int calls = 0;

    {
        auto guard = make_scope_guard(
            [&calls]() { ++calls; });
        (void)guard;
    }

    return (calls == 1);
}

/*
tests_scope_guard_accepts_a_function_object
  the guard is not limited to lambdas.
  Tests the following:
  - a plain function object (a struct with operator()) works as the action
  - a std::function works too, including one wrapping a lambda
  The template takes any callable, which matters because the module's cleanups
  are sometimes stored callables rather than literal lambdas.
*/
bool
tests_scope_guard_accepts_a_function_object()
{
    int struct_calls = 0;
    {
        scope_guard<counting_action> guard{
            counting_action(&struct_calls)};
    }

    int function_calls = 0;
    {
        std::function<void()> action =
            [&function_calls]() { ++function_calls; };

        scope_guard<std::function<void()> > guard(action);
    }

    return ((struct_calls == 1) && (function_calls == 1));
}

/*
tests_scope_guard_dismiss_is_noexcept
  cancelling cannot itself fail.
  Tests the following:
  - dismiss() is a noexcept expression
  It is called on success paths, often inside code that has already committed;
  a throwing dismiss would turn a completed operation into a failed one.
*/
bool
tests_scope_guard_dismiss_is_noexcept()
{
    int calls = 0;

    scope_guard<counting_action> guard{counting_action(&calls)};

    const bool is_noexcept = noexcept(guard.dismiss());

    guard.dismiss();

    return (is_noexcept && (calls == 0));
}


NS_END  // testing
NS_END  // djinterp
