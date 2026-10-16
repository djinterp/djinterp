/*******************************************************************************
* djinterp [core]                                  lock_policy_tests_default.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_default.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    lock_policy_tests_default.cpp
//   Section VIII: default_lock_policy - the project's default choice.
//
//   The header picks a project-wide default that tracks the standard: shared
// locking when it is available (C++17+), exclusive otherwise (C++11-16), and
// null pre-C++11.  The suite floors at C++11, so it checks the two upper cases
// -
// and, more durably than naming a specific policy, it pins the INVARIANT that
// ties the default to the rest of the file: the default is whatever the
// selector
// returns for the default's own level.  That relationship holds on every
// standard and would survive a future re-choice of the default.

// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_default_policy_for_this_standard
  the default is the best policy the standard allows.
  Tests the following:
  - at C++17 and above, default_lock_policy is shared_lock_policy
  - at C++11 through C++14, it is exclusive_lock_policy (shared locking is not
    yet available)
  - the suite asserts whichever is correct for the build
  The higher policy name is mentioned only inside the C++17 guard, since below
  C++17 it does not exist.
*/
bool
tests_default_policy_for_this_standard()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool is_shared =
        std::is_same<default_lock_policy, shared_lock_policy>::value;

    static_assert(is_shared,
                  "at C++17 the default policy is shared_lock_policy");

    return is_shared;
#else
    D_CONSTEXPR bool is_exclusive =
        std::is_same<default_lock_policy, exclusive_lock_policy>::value;

    static_assert(is_exclusive,
                  "below C++17 the default policy is exclusive_lock_policy");

    return is_exclusive;
#endif
}

/*
tests_default_is_a_real_policy
  whatever the default is, it is a genuine thread-safe policy.
  Tests the following:
  - default_lock_policy is thread-safe (is_threadsafe is true)
  - it carries a level and the standard lock-type aliases (mutex_type,
    read_lock_type, write_lock_type all name types)
  So the default is always a usable policy, never the null one at the suite's
  supported standards - the project default assumes real locking from C++11 up.
*/
bool
tests_default_is_a_real_policy()
{
    const bool threadsafe = default_lock_policy::is_threadsafe;

    const bool has_the_aliases =
        ( (sizeof(default_lock_policy::mutex_type) > 0)      &&
          (sizeof(default_lock_policy::read_lock_type) > 0)  &&
          (sizeof(default_lock_policy::write_lock_type) > 0) );

    // and it is not the null policy at these standards
    D_CONSTEXPR bool not_null =
        !std::is_same<default_lock_policy, null_lock_policy>::value;

    return (threadsafe && has_the_aliases && not_null);
}

/*
tests_default_matches_the_selector
  the invariant: the default equals the selector's answer for its own level.
  Tests the following:
  - select_lock_policy<default_lock_policy::level> is default_lock_policy itself
  - so the default is internally consistent with the selector: choosing the
    default by name and choosing it by its level give the same policy
  This is the durable check.  It does not care WHICH policy the default is -
  only
  that the two ways of naming it agree - so it holds on every standard and would
  still hold if the project changed its default, as long as the selector and the
  default stay in step.
*/
bool
tests_default_matches_the_selector()
{
    D_CONSTEXPR bool consistent =
        std::is_same<
            select_lock_policy<default_lock_policy::level>,
            default_lock_policy>::value;

    static_assert(consistent,
                  "the default must equal select_lock_policy<its own level>");

    return consistent;
}


NS_END  // testing
NS_END  // djinterp
