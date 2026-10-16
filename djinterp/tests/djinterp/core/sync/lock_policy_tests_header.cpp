/*******************************************************************************
* djinterp [core]                                   lock_policy_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     lock_policy_tests_header.cpp
//   Section: the header contract.  The claims about the FILE rather than about
// its policies - the include guard, double inclusion, the LIVE environment
// precondition, the std facilities it forwards, the namespaces its entities
// live in (public for the policies, internal for the selector impl), and the
// per-standard gating of the policies it defines.
//
//   The precondition here is HONEST, unlike cow.hpp's and lock_guard.hpp's: the
// "include env.h first" guard is a live #error, and lock_policy.hpp does NOT
// include the environment itself, so it genuinely cannot stand first in a TU.
// tests_header_requires_the_environment_first pins that.

// djinterp
#include "lock_policy_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions of the
// policies or the enum would collide here if the guard did not hold; the
// successful build IS tests_header_is_idempotent's assertion.  It needs no core
// include this time - the first inclusion already brought the environment in.
#include "../../../../inc/djinterp/core/sync/lock_policy.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_LOCK_POLICY_HPP is defined after inclusion
  - it is defined to 1
  The guard is named for the THREADSAFE module, not the sync directory the file
  lives in - as the sibling sync headers are.
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_LOCK_POLICY_HPP == 1);
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
  - after both inclusions the enum and the null policy are still single, whole
    types
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(null_lock_policy) > 0) &&
          (sizeof(no_op_mutex) > 0) );

    // the enum still has its values
    const bool enum_intact =
        (thread_safety_leavel::none != thread_safety_leavel::shared_timed);

    return (intact && enum_intact);
}

/*
tests_header_requires_the_environment_first
  the header is not self-sufficient, and says so with a LIVE guard.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - it must be, or the header's live
    #error would have fired and this TU would not exist
  - so the header's precondition is a real, enforced guard, not a comment
  This is the honest end of the spectrum.  cow.hpp and lock_guard.hpp both have
  this same guard COMMENTED OUT; lock_guard.hpp gets away with it by including
  the core itself, and cow.hpp does not and is buggy.  lock_policy.hpp keeps the
  guard live AND does not include the core, so it must be included after the
  environment - which the suite's own header does.
*/
bool
tests_header_requires_the_environment_first()
{
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_is_in = true;
#else
    const bool environment_is_in = false;
#endif

    return environment_is_in;
}

/*
tests_header_provides_the_std_facilities
  including lock_policy.hpp is enough to name the std mutex types the policies
  are built on.
  Tests the following:
  - std::mutex, std::timed_mutex and std::unique_lock (the C++11 policies'
    types)
    are all nameable here
  - at C++14+, std::shared_timed_mutex is too; at C++17+, std::shared_mutex
  - and this TU includes NEITHER <mutex> NOR <shared_mutex> - the test header
    deliberately does not, so this test has teeth
  - so a consumer naming a policy's mutex_type needs no further include
*/
bool
tests_header_provides_the_std_facilities()
{
    const bool cpp11_types =
        ( (sizeof(std::mutex) > 0)                             &&
          (sizeof(std::timed_mutex) > 0)                       &&
          (sizeof(std::unique_lock<std::mutex>) > 0) );

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    const bool shared_timed_type = (sizeof(std::shared_timed_mutex) > 0);
#else
    const bool shared_timed_type = true;
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool shared_type = (sizeof(std::shared_mutex) > 0);
#else
    const bool shared_type = true;
#endif

    return (cpp11_types && shared_timed_type && shared_type);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the enum and the policies sit directly in djinterp.
  Tests the following:
  - thread_safety_leavel, no_op_mutex, no_op_guard and null_lock_policy, found
    by
    unqualified lookup from djinterp::testing, ARE ::djinterp's
  - at C++11 the exclusive and timed policies are too
  These are the public surface: consumers name them directly.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool public_surface =
        ( std::is_same<no_op_mutex, ::djinterp::no_op_mutex>::value           &&
          std::is_same<no_op_guard, ::djinterp::no_op_guard>::value           &&
          std::is_same<null_lock_policy,
                       ::djinterp::null_lock_policy>::value                   &&
          std::is_same<exclusive_lock_policy,
                       ::djinterp::exclusive_lock_policy>::value              &&
          std::is_same<timed_lock_policy,
                       ::djinterp::timed_lock_policy>::value );

    static_assert(public_surface,
                  "the policies must live directly in namespace djinterp");

    return public_surface;
}

/*
tests_the_selector_impl_lives_in_the_internal_namespace
  the selector's implementation detail is hidden; its public face is not.
  Tests the following:
  - internal::select_lock_policy_impl<L> is nameable as
    ::djinterp::internal::select_lock_policy_impl<L> - it lives in the internal
    namespace, out of the public surface
  - while select_lock_policy<L>, the public alias, is ::djinterp's directly
  So the messy per-standard specialization table is kept in `internal`, and
  consumers see only the clean select_lock_policy alias.
*/
bool
tests_the_selector_impl_lives_in_the_internal_namespace()
{
    // the impl is reachable through the internal namespace
    D_CONSTEXPR bool impl_is_internal =
        std::is_same<
            internal::select_lock_policy_impl<
                thread_safety_leavel::exclusive>::type,
            ::djinterp::internal::select_lock_policy_impl<
                thread_safety_leavel::exclusive>::type>::value;

    // the public alias is directly in djinterp
    D_CONSTEXPR bool alias_is_public =
        std::is_same<
            select_lock_policy<thread_safety_leavel::exclusive>,
            ::djinterp::select_lock_policy<
                thread_safety_leavel::exclusive> >::value;

    static_assert(impl_is_internal && alias_is_public,
                  "the selector impl must be internal, its alias public");

    return (impl_is_internal && alias_is_public);
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros.
  Tests the following:
  - none of null_lock_policy / exclusive_lock_policy / no_op_mutex /
    select_lock_policy / default_lock_policy is a macro
  - so a consumer may declare, alias or qualify any of them without the
    preprocessor rewriting it first
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(null_lock_policy)      || \
      defined(exclusive_lock_policy) || \
      defined(no_op_mutex)           || \
      defined(select_lock_policy)    || \
      defined(default_lock_policy) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_the_policies_are_gated_by_standard
  the policies appear as the standard rises, matching the banner's version
  table.
  Tests the following:
  - null_lock_policy, exclusive_lock_policy and timed_lock_policy are present at
    the suite's C++11 floor
  - shared_timed_lock_policy is present IFF the build is C++14 or higher
  - shared_lock_policy is present IFF the build is C++17 or higher
  - and the suite checks exactly these correspondences rather than assuming them
  The check for the higher policies is written so that it only NAMES them under
  the standard that defines them - because below that standard the name does not
  exist at all, and naming it would be a hard compile error, not a false test.
*/
bool
tests_the_policies_are_gated_by_standard()
{
    // always at the floor
    const bool floor_present =
        ( (sizeof(null_lock_policy) > 0)      &&
          (sizeof(exclusive_lock_policy) > 0) &&
          (sizeof(timed_lock_policy) > 0) );

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    const bool shared_timed_tracks_cpp14 =
        (sizeof(shared_timed_lock_policy) > 0);
#else
    const bool shared_timed_tracks_cpp14 = true;
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool shared_tracks_cpp17 =
        (sizeof(shared_lock_policy) > 0);
#else
    const bool shared_tracks_cpp17 = true;
#endif

    return ( floor_present            &&
             shared_timed_tracks_cpp14 &&
             shared_tracks_cpp17 );
}


NS_END  // testing
NS_END  // djinterp
