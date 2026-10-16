/*******************************************************************************
* djinterp [core]                                 lock_policy_c_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                   lock_policy_c_tests_header.cpp
// Section: the header contract.  The claims about the FILE - the include guard,
// double inclusion, self-sufficiency (it includes the core and the base
// lock_policy itself, so it CAN stand first, unlike lock_policy.hpp), the
// platform threading facilities it forwards, the namespace its entities live
// in, and the per-standard gating.
//
// Two facts here are worth stating up front.  First, unlike lock_policy.hpp -
// which has a LIVE env-first #error and does NOT include the core - this header
// DOES include ../djinterp.hpp before ./lock_policy.hpp, so lock_policy.hpp's
// guard is satisfied and this file is self-sufficient (the lock_guard pattern).
// Second, the file's versioning banner says c_shared is a C++17 addition, but
// the code gates it only by platform; the suite records that c_shared is
// present at its C++11 floor regardless.

// djinterp
#include "lock_policy_c_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions of the
// policies or the guard classes would collide here if the include guard did not
// hold; the successful build IS tests_header_is_idempotent's assertion.
#include "../../../../inc/djinterp/core/sync/lock_policy_c.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_LOCK_POLICY_C_HPP is defined after inclusion
  - it is defined to 1
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_LOCK_POLICY_C_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_LOCK_POLICY_C_HPP == 1);
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
  - after both inclusions the policies are still single, whole types
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(c_exclusive) > 0) &&
          (sizeof(c_shared) > 0)    &&
          (sizeof(c_recursive) > 0) );

    return intact;
}

/*
tests_header_is_self_sufficient
  the header can be the first djinterp header in a TU.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - lock_policy_c.hpp includes
    ../djinterp.hpp itself
  - and the base thread_safety_level is available - it includes
    ./lock_policy.hpp itself, AFTER the core, so lock_policy.hpp's live
    env-first #error is satisfied rather than tripped
  This is the lock_guard pattern, and the opposite of lock_policy.hpp, which
  cannot stand first.  A separate probe TU (not shown) confirms a bare
  #include of lock_policy_c.hpp with no prior include compiles; here we confirm
  the two symbols that inclusion must have brought in.
*/
bool
tests_header_is_self_sufficient()
{
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_in = true;
#else
    const bool environment_in = false;
#endif

    // the base lock_policy came in too (its enum/alias is nameable)
    const bool base_policy_in =
        (sizeof(select_lock_policy<thread_safety_level::exclusive>) > 0);

    return (environment_in && base_policy_in);
}

/*
tests_header_pulls_in_the_base_lock_policy
  the C policies are layered on the base header's vocabulary.
  Tests the following:
  - thread_safety_level (the base enum's alias) is nameable, and the C policies'
    `level` members are of that type
  - so lock_policy_c.hpp genuinely depends on and includes lock_policy.hpp,
    reusing its level enum rather than defining a parallel one
  The C policies deliberately use thread_safety_level (the CORRECTLY spelled
  alias), not the misspelled primary name - so this also confirms the alias is
  what downstream code reaches for.
*/
bool
tests_header_pulls_in_the_base_lock_policy()
{
    D_CONSTEXPR bool level_is_base_enum =
        std::is_same<
            decltype(c_exclusive::level), const thread_safety_level>::value;

    return level_is_base_enum;
}

/*
tests_header_provides_the_platform_threading_facilities
  including the header is enough to have the platform's locking primitives.
  Tests the following:
  - exactly one platform path is active: on POSIX D_HAS_PTHREADS is 1, on
    Windows the Win32 path is taken instead
  - and the policies' mutex_type is a complete type either way, so the platform
    primitive really was pulled in
  A consumer naming a C policy needs no further platform include.
*/
bool
tests_header_provides_the_platform_threading_facilities()
{
#if defined(D_HAS_PTHREADS) && D_HAS_PTHREADS
    const bool platform_selected = true;
#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const bool platform_selected = true;
#else
    const bool platform_selected = false;
#endif

    const bool mutex_types_complete =
        ( (sizeof(c_exclusive::mutex_type) > 0) &&
          (sizeof(c_shared::mutex_type) > 0)    &&
          (sizeof(c_recursive::mutex_type) > 0) );

    return (platform_selected && mutex_types_complete);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the policies and guards sit directly in djinterp.
  Tests the following:
  - c_exclusive, c_shared, c_recursive, and the c_lock_guard template, found by
    unqualified lookup from djinterp::testing, ARE ::djinterp's
  These are the public surface consumers name directly.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool public_surface =
        ( std::is_same<c_exclusive, ::djinterp::c_exclusive>::value &&
          std::is_same<c_shared, ::djinterp::c_shared>::value       &&
          std::is_same<c_recursive, ::djinterp::c_recursive>::value &&
          std::is_same<c_lock_guard<c_exclusive::mutex_type>,
                       ::djinterp::c_lock_guard<
                           c_exclusive::mutex_type> >::value );

    static_assert(public_surface,
                  "the C policies must live directly in namespace djinterp");

    return public_surface;
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros - for its policies and guards.
  Tests the following:
  - none of c_exclusive / c_shared / c_recursive / c_lock_guard / c_spinlock is
    a macro
  - (D_HAS_PTHREADS, by contrast, IS a macro the header defines on purpose, as a
    platform switch; it is not part of the public type surface and is not
    checked here)
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(c_exclusive)  || \
      defined(c_shared)     || \
      defined(c_recursive)  || \
      defined(c_lock_guard) || \
      defined(c_spinlock) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_the_spinlock_is_gated_to_cpp11
  the spinlock appears only at C++11 and above.
  Tests the following:
  - at the suite's C++11 floor (and up), c_spinlock and spinlock_mutex are
    present
  - this matches the banner's "C++11: + c_spinlock" line: the spinlock needs
    std::atomic_flag, a C++11 type
  Since the suite floors at C++11, the gate is always satisfied here; the test
  records the correspondence.  The name is only referenced inside the C++11
  guard, because below C++11 it would not exist.
*/
bool
tests_the_spinlock_is_gated_to_cpp11()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const bool present =
        ( (sizeof(c_spinlock) > 0) &&
          (sizeof(spinlock_mutex) > 0) );

    return present;
#else
    return true;
#endif
}

/*
tests_c_shared_is_available_at_this_standard
  the banner-vs-code discrepancy, recorded: c_shared is present below C++17.
  Tests the following:
  - c_shared is a complete type at the suite's floor, C++11, and its level is
    thread_safety_level::shared
  - the versioning banner lists c_shared under "C++17: + c_shared on all
    platforms", but the CODE gates it only by PLATFORM (its pthread_rwlock /
    SRWLOCK backing needs no particular standard), so on this platform it exists
    well before C++17
  Pinned so the mismatch is on record: code that relies on c_shared at C++11/14
  will in fact find it, contrary to what the banner promises.
*/
bool
tests_c_shared_is_available_at_this_standard()
{
    const bool present = (sizeof(c_shared) > 0);

    const bool level_ok =
        (c_shared::level == thread_safety_level::shared);

    return (present && level_ok);
}


NS_END  // testing
NS_END  // djinterp
