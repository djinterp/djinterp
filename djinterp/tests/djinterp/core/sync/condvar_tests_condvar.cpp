/*******************************************************************************
* djinterp [core]                                      condvar_tests_condvar.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_condvar.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                         condvar_tests_condvar.cpp
//   Section IV (c): portable_condvar's SURFACE - what the wrapper is, before
// anything blocks on it.
//
//   The class is a policy-keyed alias plus one member and a handful of
// forwarding templates.  Everything here is a claim about that shape: it is not
// copyable and (as a consequence nobody writes down) not movable either; it
// costs exactly what the condvar it wrapped costs - one byte under the null
// policy; its notifies are noexcept and mean it; and native() hands out the
// real
// thing, by reference, for the code that needs the standard type back.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef portable_condvar<null_lock_policy>      null_cv;
    typedef portable_condvar<exclusive_lock_policy> exclusive_cv;
    typedef portable_condvar<timed_lock_policy>     timed_cv;
}

/*
tests_condvar_is_default_constructible
  a portable_condvar is ready the moment it exists.
  Tests the following:
  - it is default-constructible under every policy
  - constructing one takes no argument: it is not keyed on a mutex INSTANCE,
    only on the policy's mutex TYPE, so it never has to be handed the mutex it
    will be waited on with
*/
bool
tests_condvar_is_default_constructible()
{
    D_CONSTEXPR bool constructible =
        ( std::is_default_constructible<null_cv>::value      &&
          std::is_default_constructible<exclusive_cv>::value &&
          std::is_default_constructible<timed_cv>::value );

    static_assert(constructible,
                  "a portable_condvar must be default-constructible");

    null_cv      a;
    exclusive_cv b;
    timed_cv     c;

    a.notify_all();
    b.notify_all();
    c.notify_all();

    return constructible;
}

/*
tests_condvar_is_not_copyable
  a condvar is a rendezvous point, and a rendezvous point cannot be copied.
  Tests the following:
  - the copy constructor and copy assignment are deleted under every policy
  - the underlying std condvars are non-copyable too, so the deletion is not an
    added restriction - it is the wrapper refusing to hide one
*/
bool
tests_condvar_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<null_cv>::value      &&
          !std::is_copy_assignable<null_cv>::value         &&
          !std::is_copy_constructible<exclusive_cv>::value &&
          !std::is_copy_assignable<exclusive_cv>::value    &&
          !std::is_copy_constructible<timed_cv>::value     &&
          !std::is_copy_assignable<timed_cv>::value );

    static_assert(no_copy, "a portable_condvar must not be copyable");

    return no_copy;
}

/*
tests_condvar_is_not_movable
  ...nor moved, which the header does not say and the traits do.
  Tests the following:
  - is_move_constructible and is_move_assignable are false under every policy
  - declaring the deleted copy suppressed the implicit moves; overload
    resolution then finds the deleted copy
  - note this holds even for the NULL policy, whose no_op_condvar member is
    perfectly movable.  The wrapper's deleted copy, not the member, is what
    settles it
  So a portable_condvar cannot be returned from a factory or stored by value in
  a container.  Give the owning type a reference, or hold it in place.
*/
bool
tests_condvar_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<null_cv>::value      &&
          !std::is_move_assignable<null_cv>::value         &&
          !std::is_move_constructible<exclusive_cv>::value &&
          !std::is_move_assignable<exclusive_cv>::value );

    // ...even though the no-op condvar it wraps IS movable
    D_CONSTEXPR bool the_member_would_have_allowed_it =
        std::is_move_constructible<internal::no_op_condvar>::value;

    static_assert(no_move, "a portable_condvar must not be movable");

    return (no_move && the_member_would_have_allowed_it);
}

/*
tests_condvar_exposes_its_condvar_type
  the wrapper says which condvar it chose.
  Tests the following:
  - condvar_type is a public alias, and it agrees with the selector
  - so a consumer can name the underlying type - to write a static_assert about
    it, or to hand native() somewhere that wants the std type
*/
bool
tests_condvar_exposes_its_condvar_type()
{
    D_CONSTEXPR bool agrees_with_the_selector =
        ( std::is_same<null_cv::condvar_type,
                       internal::condvar_selector<no_op_mutex>::type>::value &&
          std::is_same<exclusive_cv::condvar_type,
                       internal::condvar_selector<std::mutex>::type>::value  &&
          std::is_same<timed_cv::condvar_type,
                       internal::condvar_selector<
                           std::timed_mutex>::type>::value );

    static_assert(agrees_with_the_selector,
                  "condvar_type must be what the selector chose");

    return agrees_with_the_selector;
}

/*
tests_condvar_notify_is_noexcept
  notifying never throws, and says so.
  Tests the following:
  - notify_one and notify_all are noexcept under every policy
  - which is only honest because all three underlying condvars have noexcept
    notifies (see tests_every_condvar_can_notify) - the wrapper is not promising
    something its member cannot keep
  - so a notify may sit in a destructor, or in a noexcept unlock path, safely
*/
bool
tests_condvar_notify_is_noexcept()
{
    null_cv      null;
    exclusive_cv exclusive;
    timed_cv     timed;

    D_CONSTEXPR bool all_noexcept =
        ( noexcept(std::declval<null_cv&>().notify_one())      &&
          noexcept(std::declval<null_cv&>().notify_all())      &&
          noexcept(std::declval<exclusive_cv&>().notify_one()) &&
          noexcept(std::declval<exclusive_cv&>().notify_all()) &&
          noexcept(std::declval<timed_cv&>().notify_one())     &&
          noexcept(std::declval<timed_cv&>().notify_all()) );

    static_assert(all_noexcept,
                  "portable_condvar's notifies must be noexcept");

    null.notify_one();
    exclusive.notify_one();
    timed.notify_one();

    return all_noexcept;
}

/*
tests_condvar_native_returns_the_underlying
  native() hands back the real condvar, not a copy of it.
  Tests the following:
  - native() returns condvar_type& - a reference, so a caller can use the std
    condvar directly (with a stop_token, say, or with an overload the wrapper
    does not forward)
  - the reference is stable: two calls return the same address
*/
bool
tests_condvar_native_returns_the_underlying()
{
    exclusive_cv cv;

    D_CONSTEXPR bool returns_a_reference =
        std::is_same<decltype(std::declval<exclusive_cv&>().native()),
                     exclusive_cv::condvar_type&>::value;

    static_assert(returns_a_reference,
                  "native() must return a reference to the condvar");

    const bool stable = (&cv.native() == &cv.native());

    // and it is the real thing: notifying through it compiles and runs
    cv.native().notify_all();

    return (returns_a_reference && stable);
}

/*
tests_condvar_native_has_a_const_overload
  a const portable_condvar still hands out its condvar - as const.
  Tests the following:
  - the const overload returns const condvar_type&
  - it names the same object as the non-const one: same address, one member
*/
bool
tests_condvar_native_has_a_const_overload()
{
    exclusive_cv        cv;
    const exclusive_cv& as_const = cv;

    D_CONSTEXPR bool const_returns_const =
        std::is_same<decltype(std::declval<const exclusive_cv&>().native()),
                     const exclusive_cv::condvar_type&>::value;

    static_assert(const_returns_const,
                  "the const native() must return a const reference");

    const bool same_object = (&as_const.native() == &cv.native());

    return (const_returns_const && same_object);
}

/*
tests_condvar_costs_exactly_its_condvar
  the wrapper is free.
  Tests the following:
  - sizeof(portable_condvar<P>) is sizeof(P's condvar) for every policy: one
    member, no bookkeeping, no vtable
  - under the null policy that is ONE BYTE
*/
bool
tests_condvar_costs_exactly_its_condvar()
{
    const bool exact =
        ( (sizeof(null_cv) == sizeof(null_cv::condvar_type))           &&
          (sizeof(exclusive_cv) == sizeof(exclusive_cv::condvar_type)) &&
          (sizeof(timed_cv) == sizeof(timed_cv::condvar_type)) );

    const bool the_null_one_is_a_byte = (sizeof(null_cv) == 1);

    return (exact && the_null_one_is_a_byte);
}

/*
tests_condvar_has_no_vtable
  it is a plain class.
  Tests the following:
  - not polymorphic, no virtual destructor, not abstract, under every policy
  - so it can be a member of a container, or a base, without dragging a vptr in
*/
bool
tests_condvar_has_no_vtable()
{
    D_CONSTEXPR bool plain =
        ( !std::is_polymorphic<null_cv>::value             &&
          !std::is_polymorphic<exclusive_cv>::value        &&
          !std::is_polymorphic<timed_cv>::value            &&
          !std::has_virtual_destructor<exclusive_cv>::value &&
          !std::is_abstract<exclusive_cv>::value );

    static_assert(plain, "a portable_condvar must have no vtable");

    return plain;
}

/*
tests_condvar_notify_without_waiters_is_harmless
  notifying an empty condvar is legal, and does nothing.
  Tests the following:
  - notify_one and notify_all on a condvar nobody is waiting on return normally
  - a hundred of them, under two policies, still return
  - it is NOT stored for a future waiter, which is the next test's business and
    the reason predicates exist
*/
bool
tests_condvar_notify_without_waiters_is_harmless()
{
    exclusive_cv exclusive;
    timed_cv     timed;
    int          i = 0;

    for (i = 0; i < 100; ++i)
    {
        exclusive.notify_one();
        exclusive.notify_all();
        timed.notify_one();
        timed.notify_all();
    }

    return true;
}

/*
tests_condvar_instances_are_independent
  one condvar, one queue of waiters.
  Tests the following:
  - a waiter blocked on condvar A is NOT woken by a notify_all on condvar B,
    even though both are portable_condvar<exclusive_lock_policy>
  - it is still blocked after the settle window
  - and it wakes as soon as A itself is published and notified
*/
bool
tests_condvar_instances_are_independent()
{
    channel<exclusive_lock_policy> a;
    channel<exclusive_lock_policy> b;

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &a);

    const bool reached_the_wait = spin_until(a.waiting, k_deadline_ms);

    // the WRONG condvar is notified, loudly
    b.publish_and_notify_all();

    sleep_ms(k_settle_ms);

    const bool still_waiting = (!a.woke.load());

    // now the right one
    a.publish_and_notify_all();

    const bool woke = spin_until(a.woke, k_deadline_ms);

    a.rescue();
    b.rescue();
    waiter.join();

    return ( reached_the_wait &&
             still_waiting    &&
             woke );
}


NS_END  // testing
NS_END  // djinterp
