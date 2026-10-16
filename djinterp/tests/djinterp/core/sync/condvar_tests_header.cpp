/*******************************************************************************
* djinterp [core]                                       condvar_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                          condvar_tests_header.cpp
//   Section: the header contract.  The claims about the FILE rather than about
// its types - the guard, double inclusion, the dependencies it takes and the
// order it takes them in, the std facilities it hands its includer, the macros
// it does (and does not) leave behind, namespace placement, and the C++11 gate
// that sections III and IV sit behind.
//
//   This TU includes NOTHING from djinterp but the test header, and nothing
// from std but what that header names.  Both "it provides" and "it leaves no
// trace" assertions below are only meaningful because of that.

// djinterp
#include "condvar_tests.hpp"
// ... and the header under test a SECOND time, directly.  Two definitions of
// portable_once and portable_condvar would collide here if the guard did not
// hold; the successful build IS tests_header_is_idempotent's assertion.
#include "../../../../inc/djinterp/core/sync/condvar.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_CONDVAR_HPP is defined after inclusion
  - it is defined to 1
  Note the guard is named for the THREADSAFE module, not the sync directory the
  file lives in; a consumer testing for it must spell it that way.
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_CONDVAR_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_CONDVAR_HPP == 1);
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
  - after both inclusions every entity is still a single, complete type
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(portable_once) > 0)                              &&
          (sizeof(portable_condvar<null_lock_policy>) > 0)         &&
          (sizeof(portable_condvar<exclusive_lock_policy>) > 0)    &&
          (sizeof(internal::no_op_condvar) > 0) );

    return intact;
}

/*
tests_header_inherits_its_guards_from_lock_policy
  the header's own precondition guards are COMMENTED OUT; the protection it
  still has is inherited.
  Tests the following:
  - condvar.hpp's `#ifndef __cplusplus` and `#ifndef DJINTERP_ENV_ENV_H`
    blocks are commented out at the top of the file, so it asserts neither
  - lock_policy.hpp, which it includes, asserts BOTH - and is live
  - condvar.hpp includes ../djinterp.hpp (hence env.h) BEFORE ./lock_policy.hpp,
    so that live precondition is satisfied by construction:
    DJINTERP_ENV_ENV_H
    is defined here, and this TU compiled at all
  - the ordering is the load-bearing part.  Swap those two includes and
    lock_policy.hpp's #error fires; this test is what says so out loud.
*/
bool
tests_header_inherits_its_guards_from_lock_policy()
{
#if defined(__cplusplus)
    const bool is_cpp = true;
#else
    const bool is_cpp = false;
#endif

    // lock_policy.hpp errors out without this; that the TU built proves
    // condvar.hpp brought it in first
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_first = true;
#else
    const bool environment_first = false;
#endif

    return (is_cpp && environment_first);
}

/*
tests_header_pulls_its_dependencies
  the two djinterp headers condvar.hpp names, and the C core beneath them.
  Tests the following:
  - djinterp.hpp's guard (DJINTERP_DJINTERP_HPP) and the C core's (DJINTERP_)
      are
    defined
  - lock_policy.hpp's guard (DJINTERP_SYNC_LOCK_POLICY_HPP) is defined - the
    header cannot be used without it, since portable_condvar is keyed on a
    policy's mutex_type
  - so a consumer needs to include condvar.hpp and nothing else to name a policy
*/
bool
tests_header_pulls_its_dependencies()
{
#if defined(DJINTERP_DJINTERP_HPP)
    const bool has_cpp_core = true;
#else
    const bool has_cpp_core = false;
#endif

#if defined(DJINTERP_)
    const bool has_c_core = true;
#else
    const bool has_c_core = false;
#endif

#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool has_lock_policy = true;
#else
    const bool has_lock_policy = false;
#endif

    // and the policies really are nameable from here
    const bool policies_are_nameable =
        ( (sizeof(null_lock_policy) > 0)      &&
          (sizeof(exclusive_lock_policy) > 0) &&
          (sizeof(timed_lock_policy) > 0) );

    return ( has_cpp_core        &&
             has_c_core          &&
             has_lock_policy     &&
             policies_are_nameable );
}

/*
tests_header_provides_the_std_facilities
  including condvar.hpp is enough to name the std types it is built on.
  Tests the following:
  - std::condition_variable, std::condition_variable_any, std::cv_status,
    std::mutex, std::timed_mutex, std::once_flag and std::unique_lock are all
    nameable here
  - and this TU includes NEITHER <condition_variable> NOR <mutex> (the test
    header deliberately does not, precisely so that this test has teeth): every
    one of those names arrived through condvar.hpp's own C++11 gate
  - so a consumer writing `std::unique_lock<std::mutex> lock(m);` beside a
    portable_condvar needs no further include
*/
bool
tests_header_provides_the_std_facilities()
{
    typedef std::unique_lock<std::mutex> exclusive_lock;

    const bool nameable =
        ( (sizeof(std::condition_variable) > 0)     &&
          (sizeof(std::condition_variable_any) > 0) &&
          (sizeof(std::mutex) > 0)                  &&
          (sizeof(std::timed_mutex) > 0)            &&
          (sizeof(std::once_flag) > 0)              &&
          (sizeof(exclusive_lock) > 0) );

    const bool cv_status_is_an_enum =
        std::is_enum<std::cv_status>::value;

    // <chrono> too - the timed waits take its durations
    const bool chrono_is_here =
        (std::chrono::milliseconds(2) > std::chrono::milliseconds(1));

    return ( nameable            &&
             cv_status_is_an_enum &&
             chrono_is_here );
}

/*
tests_header_defines_no_stray_macros
  the header publishes types and functions, not macros - with one deliberate,
  platform-specific exception.
  Tests the following:
  - none of d_thread_yield / hardware_concurrency / portable_once /
    portable_condvar / no_op_condvar / condvar_selector is a macro
  - WIN32_LEAN_AND_MEAN: on Windows the header DOES define it (to trim
    <windows.h> before pulling it in), and that is a TU-wide side effect a
    consumer inherits.  Off Windows, the header defines no macro at all.
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(d_thread_yield)       || \
      defined(hardware_concurrency) || \
      defined(portable_once)        || \
      defined(portable_condvar)     || \
      defined(no_op_condvar)        || \
      defined(condvar_selector) )
    const bool names_are_not_macros = false;
#else
    const bool names_are_not_macros = true;
#endif

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    // the one macro the header is entitled to leave behind
    #if defined(WIN32_LEAN_AND_MEAN)
        const bool win32_side_effect_as_documented = true;
    #else
        const bool win32_side_effect_as_documented = false;
    #endif
#else
    // ...and it leaves nothing behind anywhere else
    #if defined(WIN32_LEAN_AND_MEAN)
        const bool win32_side_effect_as_documented = false;
    #else
        const bool win32_side_effect_as_documented = true;
    #endif
#endif

    return ( names_are_not_macros &&
             win32_side_effect_as_documented );
}

/*
tests_entities_live_in_the_djinterp_namespace
  the public entities sit FLAT in djinterp; the two implementation details sit
  in djinterp::internal.
  Tests the following:
  - d_thread_yield and hardware_concurrency, found by unqualified lookup from
    djinterp::testing, ARE ::djinterp's (same address, same type)
  - portable_once and portable_condvar likewise
  - no_op_condvar and condvar_selector are ::djinterp::internal's - hidden, as
    an implementation detail should be, but nameable by a test
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    // taken through pointers, so the comparison is a comparison and not a
    // tautology the compiler folds away before it has said anything
    typedef void     (*yield_fn)();
    typedef unsigned (*hardware_fn)();

    const yield_fn    yield_here      = &d_thread_yield;
    const yield_fn    yield_qualified = &::djinterp::d_thread_yield;
    const hardware_fn hardware_here   = &hardware_concurrency;
    const hardware_fn hardware_qual   = &::djinterp::hardware_concurrency;

    const bool functions_are_flat =
        ( (yield_here    == yield_qualified) &&
          (hardware_here == hardware_qual) );

    D_CONSTEXPR bool types_are_flat =
        ( std::is_same<portable_once, ::djinterp::portable_once>::value &&
          std::is_same<portable_condvar<null_lock_policy>,
                       ::djinterp::portable_condvar<
                           ::djinterp::null_lock_policy>>::value );

    D_CONSTEXPR bool details_are_internal =
        ( std::is_same<internal::no_op_condvar,
                       ::djinterp::internal::no_op_condvar>::value &&
          std::is_same<
              internal::condvar_selector<std::mutex>::type,
              ::djinterp::internal::condvar_selector<std::mutex>::type>::value
              );

    static_assert(types_are_flat,
                  "portable_once / portable_condvar must live in djinterp");
    static_assert(details_are_internal,
                  "the condvar selector must live in djinterp::internal");

    return ( functions_are_flat  &&
             types_are_flat      &&
             details_are_internal );
}

/*
tests_sections_three_and_four_are_gated_to_cpp11
  sections I and II are C++98-shaped; III (portable_once) and IV
  (portable_condvar) sit behind D_ENV_LANG_IS_CPP11_OR_HIGHER.
  Tests the following:
  - the gate is on for this build (the suite's floor is C++11, as the
    framework's is)
  - with the gate on, portable_once and portable_condvar are complete types
  - the two free functions of sections I and II exist EITHER WAY - they are the
    part of the header a C++98 TU still gets, and they are declared outside the
    gate
*/
bool
tests_sections_three_and_four_are_gated_to_cpp11()
{
    const bool gate_is_on = (D_ENV_LANG_IS_CPP11_OR_HIGHER) ? true : false;

    const bool gated_sections_are_present =
        ( (sizeof(portable_once) > 0) &&
          (sizeof(portable_condvar<exclusive_lock_policy>) > 0) );

    // sections I and II live OUTSIDE the gate: declared, and callable, with
    // the return types they carry at every standard
    D_CONSTEXPR bool ungated_sections_are_present =
        ( std::is_same<decltype(d_thread_yield()), void>::value &&
          std::is_same<decltype(hardware_concurrency()), unsigned>::value );

    return ( gate_is_on                  &&
             gated_sections_are_present  &&
             ungated_sections_are_present );
}


NS_END  // testing
NS_END  // djinterp
