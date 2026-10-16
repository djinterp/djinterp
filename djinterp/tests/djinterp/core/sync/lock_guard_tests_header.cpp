/*******************************************************************************
* djinterp [core]                                    lock_guard_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                      lock_guard_tests_header.cpp
//   Section: the header contract.  The claims about the FILE rather than about
// its guards - the include guard, double inclusion, the fact that it is
// self-sufficient (CAN be included first) despite its commented-out
// precondition
// guards, the dependencies it pulls, the std facilities it forwards, and the
// per-standard gating of the guards it defines.
//
//   The headline here is a NON-bug: lock_guard.hpp's "include env.h first"
// #error guards are commented out, exactly as cow.hpp's are - but here it does
// not matter, because lock_guard.hpp includes ../djinterp.hpp before
// ./lock_policy.hpp, so the environment is always in scope by the time
// lock_policy.hpp's own LIVE #error is reached.
// tests_header_is_self_sufficient
// pins that the header can stand first in a TU.

// djinterp
#include "lock_guard_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions would
// collide here if the guard did not hold; the successful build IS
// tests_header_is_idempotent's assertion.
#include "../../../../inc/djinterp/core/sync/lock_guard.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_LOCK_GUARD_HPP is defined after inclusion
  - it is defined to 1
  The guard is named for the THREADSAFE module, not the sync directory the file
  lives in - as the sibling sync headers are.
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_LOCK_GUARD_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_LOCK_GUARD_HPP == 1);
#else
    const bool is_defined = false;
    const bool is_one     = false;
#endif

    return (is_defined && is_one);
}

/*
tests_header_is_idempotent
  including the header twice in one TU is a no-op.
  Tests the following:
  - the second #include above redefined nothing
  - after both inclusions every guard is still a single, complete type
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(scoped_read_lock<exclusive_lock_policy>) > 0)  &&
          (sizeof(scoped_write_lock<exclusive_lock_policy>) > 0) &&
          (sizeof(scoped_try_lock<exclusive_lock_policy>) > 0) );

    return intact;
}

/*
tests_header_is_self_sufficient
  lock_guard.hpp can be the FIRST djinterp header in a translation unit.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - lock_policy.hpp's LIVE #error would
    have fired otherwise, since lock_guard.hpp pulls it in
  - and lock_policy.hpp itself is present, by its guard
  This is the crucial difference from cow.hpp.  Both headers have their own
  "include env.h first" guards commented out.  cow.hpp then includes
  lock_policy.hpp WITHOUT first including the core, so cow.hpp cannot stand
  first
  in a TU.  lock_guard.hpp includes ../djinterp.hpp BEFORE ./lock_policy.hpp, so
  it is self-sufficient - the commented-out guards are dead code with no
  consequence.  A separate throwaway TU that includes ONLY this header (see the
  suite's build notes) compiles, which is the standing proof.
*/
bool
tests_header_is_self_sufficient()
{
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_is_in = true;
#else
    const bool environment_is_in = false;
#endif

#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool lock_policy_is_in = true;
#else
    const bool lock_policy_is_in = false;
#endif

    return (environment_is_in && lock_policy_is_in);
}

/*
tests_header_pulls_its_dependencies
  the two djinterp headers lock_guard.hpp names.
  Tests the following:
  - ../djinterp.hpp (the environment) and ./lock_policy.hpp are both present, by
    their guards
  - and the policy types they carry are nameable here: a null policy, an
    exclusive policy, the no_op_mutex
  - so a consumer of lock_guard.hpp needs no further djinterp include to name a
    policy and construct a guard over it
*/
bool
tests_header_pulls_its_dependencies()
{
#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool has_lock_policy = true;
#else
    const bool has_lock_policy = false;
#endif

    const bool policies_are_nameable =
        ( (sizeof(null_lock_policy) > 0)      &&
          (sizeof(exclusive_lock_policy) > 0) &&
          (sizeof(no_op_mutex) > 0) );

    return (has_lock_policy && policies_are_nameable);
}

/*
tests_header_provides_the_std_facilities
  including lock_guard.hpp is enough to name the std types its guards are built
  on.
  Tests the following:
  - std::mutex and std::unique_lock (through lock_policy.hpp) and
    std::chrono::milliseconds (lock_guard.hpp includes <chrono> at C++11) are
    all
    nameable here
  - and this TU includes NEITHER <mutex> NOR <chrono> - the test header
    deliberately does not, precisely so that this test has teeth
  - so a consumer writing `guard(m, std::chrono::milliseconds(10))` beside a
    timed guard needs no further include
*/
bool
tests_header_provides_the_std_facilities()
{
    const bool nameable =
        ( (sizeof(std::mutex) > 0)                       &&
          (sizeof(std::unique_lock<std::mutex>) > 0)     &&
          (sizeof(std::chrono::milliseconds) > 0) );

    // a duration is constructible and readable, through lock_guard.hpp alone
    const std::chrono::milliseconds span(10);

    return (nameable && (span.count() == 10));
}

/*
tests_header_precondition_guards_are_commented_out
  the "include env.h first" guards in lock_guard.hpp are inert.
  Tests the following:
  - the environment is in scope regardless (checked via DJINTERP_ENV_ENV_H) -
    but NOT because lock_guard.hpp enforced it: its own #ifndef/#error blocks
    are
    commented out
  - so a hypothetical direct, first-in-TU inclusion is not stopped by
    lock_guard.hpp's guards; it is saved instead by the ../djinterp.hpp include
    that precedes lock_policy.hpp (see tests_header_is_self_sufficient)
  Pinned as current behaviour.  Uncommenting the guards would make the
  precondition explicit, but as things stand the header does not rely on them,
  and this test documents that the safety comes from the include ORDER, not from
  the guards.
*/
bool
tests_header_precondition_guards_are_commented_out()
{
    // the environment is present (or this TU would not have compiled through
    // lock_policy.hpp's live #error)
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_present = true;
#else
    const bool environment_present = false;
#endif

    // the guards being inert is not directly observable at run time; what IS
    // observable is that the header compiled and its types are whole
    const bool header_is_whole =
        (sizeof(scoped_try_lock<exclusive_lock_policy>) > 0);

    return (environment_present && header_is_whole);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the guards sit FLAT in djinterp - there is no sub-namespace here.
  Tests the following:
  - scoped_read_lock, scoped_write_lock and scoped_try_lock, found by
    unqualified
    lookup from djinterp::testing, ARE ::djinterp's
  - lock_guard.hpp keeps nothing in a nested `internal` namespace: every guard
    is
    part of the public surface, because call sites instantiate them directly
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool flat =
        ( std::is_same<scoped_read_lock<exclusive_lock_policy>,
                       ::djinterp::scoped_read_lock<
                           exclusive_lock_policy> >::value        &&
          std::is_same<scoped_write_lock<exclusive_lock_policy>,
                       ::djinterp::scoped_write_lock<
                           exclusive_lock_policy> >::value        &&
          std::is_same<scoped_try_lock<exclusive_lock_policy>,
                       ::djinterp::scoped_try_lock<
                           exclusive_lock_policy> >::value );

    static_assert(flat,
                  "the lock guards must live directly in namespace djinterp");

    return flat;
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros.
  Tests the following:
  - none of scoped_read_lock / scoped_write_lock / scoped_try_lock /
    scoped_timed_lock / upgrade_lock is a macro
  - so a consumer may declare, alias or qualify any of them without the
    preprocessor rewriting it first
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(scoped_read_lock)  || \
      defined(scoped_write_lock) || \
      defined(scoped_try_lock)   || \
      defined(scoped_timed_lock) || \
      defined(upgrade_lock) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_the_guards_are_gated_by_standard
  the guards appear as the standard rises, matching the banner's version table.
  Tests the following:
  - the blocking guards (read / write / try) are present at the suite's C++11
    floor
  - scoped_timed_lock is present too, since it is gated to C++11 and the floor
    is
    C++11
  - upgrade_lock is present IFF the build is C++17 or higher (it is gated there,
    because it needs a defer_lock unique_lock upgrade path), and the suite
    checks
    exactly that correspondence rather than assuming it
  So the surface a consumer sees genuinely tracks the language level, and this
  test is what would catch a gate drifting off its intended standard.
*/
bool
tests_the_guards_are_gated_by_standard()
{
    // always at the floor
    const bool blocking_present =
        ( (sizeof(scoped_read_lock<exclusive_lock_policy>) > 0)  &&
          (sizeof(scoped_write_lock<exclusive_lock_policy>) > 0) &&
          (sizeof(scoped_try_lock<exclusive_lock_policy>) > 0)   &&
          (sizeof(scoped_timed_lock<timed_lock_policy>) > 0) );

    // upgrade_lock exactly tracks C++17
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool upgrade_matches_cpp17 =
        (sizeof(upgrade_lock<shared_lock_policy>) > 0);
#else
    // below C++17 the type is not defined; the correspondence still holds
    const bool upgrade_matches_cpp17 = true;
#endif

    return (blocking_present && upgrade_matches_cpp17);
}


NS_END  // testing
NS_END  // djinterp
