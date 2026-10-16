/*******************************************************************************
* djinterp [core]                                     condvar_tests_selector.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_selector.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                        condvar_tests_selector.cpp
//   Section IV (a): internal::condvar_selector - which condition variable a
// policy's mutex gets.
//
//   Three answers, and the header explains two of them: std::mutex gets
// std::condition_variable ("the more efficient" one), no_op_mutex gets a no-op,
// and EVERYTHING ELSE gets std::condition_variable_any, which is the only one
// that can wait on a lock that is not a unique_lock<std::mutex> - a
// shared_lock,
// say.  The mapping is three lines of specialization and it is the whole reason
// portable_condvar works across the policy set.
//
//   The second half of this section is the CAPABILITY MATRIX, and it is the
// most important thing in the suite.  The three selected condvars do not have
// the same surface:
//
//                          notify  wait  wait_for  wait_until  stop_token wait
//     condition_variable      Y      Y       Y          Y             N
//     condition_variable_any  Y      Y       Y          Y             Y
//     no_op_condvar           Y      Y       Y          N             N
//
//   portable_condvar publishes all five, because its waits are member TEMPLATES
// that are only instantiated when called.  So two of its methods are latent
// compile errors, and which two depends on the policy you chose:
//
//     - wait_until under null_lock_policy    -> no_op_condvar has none;
//     - the C++20 stop_token wait under any  -> condition_variable has none.
//       std::mutex policy (exclusive_lock_policy, and default_lock_policy below
//       C++17)
//
//   Neither is a compile error until someone writes the call.  The two tests at
// the end of this file are what say so before that someone does.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // the three condvars the selector can hand back
    typedef std::condition_variable      real_condvar;
    typedef std::condition_variable_any  any_condvar;
    typedef internal::no_op_condvar      null_condvar;

    // the lock each of them is meant to be waited on with
    typedef std::unique_lock<std::mutex>       real_lock;
    typedef std::unique_lock<std::timed_mutex> any_lock;
    typedef no_op_guard                        null_lock;

    // selected
    //   type: the condvar the selector picks for a mutex - the expression under
    // test, spelled short.
    template<typename _MutexType>
    struct selected
    {
        typedef typename internal::condvar_selector<_MutexType>::type type;
    };

    // condvar_of
    //   type: the condvar a POLICY ends up with, through portable_condvar's own
    // public alias.  This is what a consumer actually sees.
    template<typename _Policy>
    struct condvar_of
    {
        typedef typename portable_condvar<_Policy>::condvar_type type;
    };
}

/*
tests_selector_maps_std_mutex_to_condition_variable
  the specialization the header calls "the more efficient" one.
  Tests the following:
  - condvar_selector<std::mutex>::type is std::condition_variable - NOT
    condition_variable_any
  - and so exclusive_lock_policy, whose mutex_type is std::mutex, gets it too
  The distinction is not cosmetic: condition_variable can only wait on a
  unique_lock<std::mutex>, and in exchange it can be implemented directly on the
  platform's futex rather than on a second, internal mutex.
*/
bool
tests_selector_maps_std_mutex_to_condition_variable()
{
    D_CONSTEXPR bool mutex_maps_to_the_real_one =
        std::is_same<selected<std::mutex>::type, real_condvar>::value;

    D_CONSTEXPR bool and_not_to_the_any =
        !std::is_same<selected<std::mutex>::type, any_condvar>::value;

    D_CONSTEXPR bool the_policy_agrees =
        std::is_same<condvar_of<exclusive_lock_policy>::type,
                     real_condvar>::value;

    static_assert(mutex_maps_to_the_real_one && the_policy_agrees,
                  "std::mutex must select std::condition_variable");

    return ( mutex_maps_to_the_real_one &&
             and_not_to_the_any         &&
             the_policy_agrees );
}

/*
tests_selector_maps_no_op_mutex_to_the_no_op_condvar
  the single-threaded specialization.
  Tests the following:
  - condvar_selector<no_op_mutex>::type is internal::no_op_condvar
  - and null_lock_policy, whose mutex_type is no_op_mutex, gets it
  - so the same source that waits under a real policy compiles - and costs
    nothing - under the null one.  What it MEANS there is another matter; see
    the no-op section.
*/
bool
tests_selector_maps_no_op_mutex_to_the_no_op_condvar()
{
    D_CONSTEXPR bool no_op_mutex_maps_to_the_no_op_condvar =
        std::is_same<selected<no_op_mutex>::type, null_condvar>::value;

    D_CONSTEXPR bool the_policy_agrees =
        std::is_same<condvar_of<null_lock_policy>::type, null_condvar>::value;

    static_assert(no_op_mutex_maps_to_the_no_op_condvar && the_policy_agrees,
                  "no_op_mutex must select the no-op condvar");

    return ( no_op_mutex_maps_to_the_no_op_condvar &&
             the_policy_agrees );
}

/*
tests_selector_falls_back_to_condition_variable_any
  everything the header did not name gets the general one.
  Tests the following:
  - std::timed_mutex, std::recursive_mutex and a user-written BasicLockable all
    map to std::condition_variable_any
  - and timed_lock_policy, whose mutex_type is std::timed_mutex, gets it
  This is the primary template doing its job: condition_variable_any is the only
  choice that can wait on a lock the standard condvar has never heard of.
*/
bool
tests_selector_falls_back_to_condition_variable_any()
{
    D_CONSTEXPR bool timed_falls_back =
        std::is_same<selected<std::timed_mutex>::type, any_condvar>::value;

    D_CONSTEXPR bool recursive_falls_back =
        std::is_same<selected<std::recursive_mutex>::type, any_condvar>::value;

    D_CONSTEXPR bool a_foreign_mutex_falls_back =
        std::is_same<selected<mock_mutex>::type, any_condvar>::value;

    D_CONSTEXPR bool the_policy_agrees =
        std::is_same<condvar_of<timed_lock_policy>::type, any_condvar>::value;

    static_assert(timed_falls_back && a_foreign_mutex_falls_back,
                  "an unrecognized mutex must select condition_variable_any");

    return ( timed_falls_back           &&
             recursive_falls_back       &&
             a_foreign_mutex_falls_back &&
             the_policy_agrees );
}

/*
tests_selector_maps_the_shared_mutexes
  the reader/writer mutexes take the general condvar, which is the whole reason
  it is the primary.
  Tests the following:
  - std::shared_timed_mutex (C++14) maps to condition_variable_any
  - std::shared_mutex (C++17) maps to condition_variable_any
  - so a READER holding a shared_lock can wait - see the policies section, where
    one does
  Below C++14 neither mutex exists, and neither does the policy that wraps it;
  the test says so and passes.
*/
bool
tests_selector_maps_the_shared_mutexes()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool shared_timed_maps =
        ( std::is_same<selected<std::shared_timed_mutex>::type,
                       any_condvar>::value &&
          std::is_same<condvar_of<shared_timed_lock_policy>::type,
                       any_condvar>::value );
#else
    // std::shared_timed_mutex is C++14; there is no policy to map
    D_CONSTEXPR bool shared_timed_maps = true;
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool shared_maps =
        ( std::is_same<selected<std::shared_mutex>::type,
                       any_condvar>::value &&
          std::is_same<condvar_of<shared_lock_policy>::type,
                       any_condvar>::value );
#else
    // std::shared_mutex is C++17; there is no policy to map
    D_CONSTEXPR bool shared_maps = true;
#endif

    return (shared_timed_maps && shared_maps);
}

/*
tests_selector_primary_accepts_any_type
  the primary template is UNCONSTRAINED - it will happily "select" a condvar for
  a type that is not a mutex at all.
  Tests the following:
  - condvar_selector<int>::type and condvar_selector<void*>::type are both
    condition_variable_any
  - the second template parameter (`typename = void`) is a SFINAE slot that no
    specialization uses, so nothing narrows the primary
  Pinned as current behaviour: a policy with a typo in its mutex_type does not
  fail here.  It fails much later, inside a wait, with an error about the
  argument to lock().  If the selector ever grows a constraint, this is the test
  that should change.
*/
bool
tests_selector_primary_accepts_any_type()
{
    D_CONSTEXPR bool an_int_selects_a_condvar =
        std::is_same<selected<int>::type, any_condvar>::value;

    D_CONSTEXPR bool a_pointer_selects_a_condvar =
        std::is_same<selected<void*>::type, any_condvar>::value;

    return ( an_int_selects_a_condvar &&
             a_pointer_selects_a_condvar );
}

/*
tests_every_policy_selects_its_condvar
  the mapping, read through portable_condvar, for every policy the build has.
  Tests the following:
  - null      -> no_op_condvar
  - exclusive -> condition_variable
  - timed     -> condition_variable_any
  - shared / shared_timed (where they exist) -> condition_variable_any
  - default_lock_policy -> whatever ITS mutex implies: condition_variable_any at
    C++17 and above (it is shared_lock_policy there), condition_variable below
    (it is exclusive_lock_policy)
*/
bool
tests_every_policy_selects_its_condvar()
{
    D_CONSTEXPR bool core_policies =
        ( std::is_same<condvar_of<null_lock_policy>::type,
                       null_condvar>::value      &&
          std::is_same<condvar_of<exclusive_lock_policy>::type,
                       real_condvar>::value      &&
          std::is_same<condvar_of<timed_lock_policy>::type,
                       any_condvar>::value );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool the_default_policy =
        std::is_same<condvar_of<default_lock_policy>::type,
                     any_condvar>::value;   // default is shared_lock_policy
#else
    D_CONSTEXPR bool the_default_policy =
        std::is_same<condvar_of<default_lock_policy>::type,
                     real_condvar>::value;  // default is exclusive_lock_policy
#endif

    static_assert(core_policies,
                  "each policy must select the condvar its mutex implies");

    return (core_policies && the_default_policy);
}

/*
tests_every_condvar_can_notify
  all three selected condvars can be notified, and notifying is noexcept.
  Tests the following:
  - notify_one and notify_all exist on condition_variable,
    condition_variable_any and no_op_condvar
  - and are noexcept on all three - which is what lets portable_condvar declare
    its own notify_one / notify_all noexcept and mean it
*/
bool
tests_every_condvar_can_notify()
{
    D_CONSTEXPR bool all_three_notify =
        ( has_notify<real_condvar>::value &&
          has_notify<any_condvar>::value  &&
          has_notify<null_condvar>::value );

    real_condvar real;
    any_condvar  any;
    null_condvar null;

    D_CONSTEXPR bool all_three_are_noexcept =
        ( noexcept(real.notify_one()) &&
          noexcept(real.notify_all()) &&
          noexcept(any.notify_one())  &&
          noexcept(any.notify_all())  &&
          noexcept(null.notify_one()) &&
          noexcept(null.notify_all()) );

    static_assert(all_three_notify && all_three_are_noexcept,
                  "every selected condvar must notify, and do it noexcept");

    return (all_three_notify && all_three_are_noexcept);
}

/*
tests_every_condvar_can_wait
  all three can wait, with and without a predicate.
  Tests the following:
  - wait(lock) and wait(lock, predicate) exist on all three, each with the lock
    type its policy actually uses
  - so portable_condvar's two wait() overloads instantiate for every policy
*/
bool
tests_every_condvar_can_wait()
{
    D_CONSTEXPR bool bare_wait =
        ( has_wait<real_condvar, real_lock>::value &&
          has_wait<any_condvar, any_lock>::value   &&
          has_wait<null_condvar, null_lock>::value );

    D_CONSTEXPR bool predicate_wait =
        ( has_wait_predicate<real_condvar, real_lock,
                             constant_predicate>::value &&
          has_wait_predicate<any_condvar, any_lock,
                             constant_predicate>::value &&
          has_wait_predicate<null_condvar, null_lock,
                             constant_predicate>::value );

    static_assert(bare_wait && predicate_wait,
                  "every selected condvar must offer both waits");

    return (bare_wait && predicate_wait);
}

/*
tests_every_condvar_can_wait_for
  all three can wait with a timeout.
  Tests the following:
  - wait_for(lock, duration) exists on all three
  - including the no-op, which "waits" by returning no_timeout at once - see the
    no-op section for what that costs
*/
bool
tests_every_condvar_can_wait_for()
{
    D_CONSTEXPR bool timed_wait =
        ( has_wait_for<real_condvar, real_lock>::value &&
          has_wait_for<any_condvar, any_lock>::value   &&
          has_wait_for<null_condvar, null_lock>::value );

    static_assert(timed_wait,
                  "every selected condvar must offer wait_for");

    return timed_wait;
}

/*
tests_only_the_real_condvars_have_wait_until
  the first latent hole in portable_condvar's surface.
  Tests the following:
  - condition_variable and condition_variable_any both have wait_until
  - internal::no_op_condvar does NOT
  - so portable_condvar<null_lock_policy>::wait_until(lock, deadline) is a
    COMPILE ERROR, and will stay one until someone writes the call
  portable_condvar declares wait_until unconditionally, but as a member template
  it is only instantiated on use.  The class therefore compiles for the null
  policy; only the call does not.  Code written against "the same source works
  locked or unlocked" will meet this the first time it needs a deadline.
  If no_op_condvar ever grows a wait_until, this test should be flipped - and
  the gap closed.
*/
bool
tests_only_the_real_condvars_have_wait_until()
{
    D_CONSTEXPR bool the_real_ones_have_it =
        ( has_wait_until<real_condvar, real_lock>::value &&
          has_wait_until<any_condvar, any_lock>::value );

    D_CONSTEXPR bool the_no_op_does_not =
        !has_wait_until<null_condvar, null_lock>::value;

    static_assert(the_real_ones_have_it,
                  "a real condvar must offer wait_until");

    return (the_real_ones_have_it && the_no_op_does_not);
}

/*
tests_only_condition_variable_any_takes_a_stop_token
  the second latent hole, and the sharper one.
  Tests the following:
  - std::condition_variable_any has wait(lock, stop_token, predicate)
  - std::condition_variable does NOT - the stop_token overload was only ever
    added to the `any` flavour
  - internal::no_op_condvar does not either
  - so portable_condvar's C++20 wait(lock, stoken, pred) compiles ONLY for
    policies whose mutex is not std::mutex: timed, shared, shared_timed.  Under
    exclusive_lock_policy - and under default_lock_policy below C++17, which IS
    exclusive_lock_policy - the call does not compile.
  Below C++20 there is no stop_token at all and nothing to say.
*/
bool
tests_only_condition_variable_any_takes_a_stop_token()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CONSTEXPR bool the_any_takes_one =
        has_stop_token_wait<any_condvar, any_lock,
                            constant_predicate>::value;

    D_CONSTEXPR bool the_real_one_does_not =
        !has_stop_token_wait<real_condvar, real_lock,
                             constant_predicate>::value;

    D_CONSTEXPR bool the_no_op_does_not =
        !has_stop_token_wait<null_condvar, null_lock,
                             constant_predicate>::value;

    static_assert(the_any_takes_one,
                  "condition_variable_any must take a stop_token");

    return ( the_any_takes_one     &&
             the_real_one_does_not &&
             the_no_op_does_not );
#else
    // std::stop_token is C++20; portable_condvar's overload is gated with it
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
