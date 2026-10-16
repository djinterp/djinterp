/*******************************************************************************
* djinterp [test]                                          lock_policy_tests.hpp
*
*   Declarations for the lock_policy.hpp unit-test suite.
*
*   WHAT THIS HEADER IS, AND WHAT THAT MEANS FOR TESTING:
*   lock_policy.hpp is compile-time vocabulary.  It defines a level enum, a
* family of policy structs that each name a mutex type and its read/write lock
* types and carry a few boolean descriptors, a zero-cost no_op_mutex/no_op_guard
* pair for the null policy, and a metaprogramming SELECTOR that maps a level to
* the right policy - falling back to the best available policy when the
* requested one needs a newer standard than the current build.  There is almost
* no run-time behaviour to exercise (only no_op_mutex's no-op operations), so
* the suite is overwhelmingly static: it asserts on type identities and
* compile-time constants with static_assert, and returns the same fact at run
* time so the runner can tally it.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - the enum is MISSPELLED.  Its name is thread_safety_leavel (sic), and the
*       policies' `level` members are typed by that misspelled name.  A
* correctly
*       spelled alias, thread_safety_level, is provided and names the very same
*       type - so downstream code can spell it right, but the primary name is
* the
*       typo (tests_the_correctly_spelled_alias_names_the_same_enum).
*     - atomic_only maps to null_lock_policy.  Two DISTINCT levels - none and
*       atomic_only - select the SAME policy, because lock-free-atomic code
* needs
*       no mutex (tests_selector_atomic_only_also_maps_to_null).
*     - the selector FALLS BACK by standard.  select_lock_policy<shared> is
*       shared_lock_policy only at C++17; below that it degrades to
*       exclusive_lock_policy.  select_lock_policy<shared_timed> is
*       shared_timed_lock_policy from C++14; below that it degrades to
*       timed_lock_policy.  The suite checks the mapping THAT ACTUALLY HOLDS for
*       the build's standard, rather than assuming the C++17 answer
*       (tests_selector_shared_maps_correctly_for_this_standard, and its timed
*       twin).
*     - no_op_mutex::try_lock() ALWAYS returns true - the null policy gives no
*       real exclusion (tests_no_op_mutex_try_lock_always_succeeds).
*     - no_op_guard is NON-MOVABLE: it deletes its copy operations and declares
* a
*       destructor, so no implicit move is generated.  This is the root cause of
*       the lock_guard suite's finding that scoped_read_lock<null_lock_policy>
* is
*       non-movable (tests_no_op_guard_is_not_movable).
*
*   A LIVE PRECONDITION: unlike cow.hpp and lock_guard.hpp, lock_policy.hpp's
* "include env.h first" guard is a LIVE #error, and the header does NOT include
* the environment itself - so it genuinely cannot be the first djinterp header
* in
* a TU.  The suite's own header includes the core before it, which is the
* documented way to use the file (tests_header_requires_the_environment_first).
*
*   PORTABILITY: the suite floors at C++11 (the runner and framework need it),
* matching the sibling suites.  lock_policy.hpp reaches lower - null_lock_policy
* and a struct-based level enum exist in C++98 - but that path is out of this
* harness's scope.  Within the suite, every mention of shared_lock_policy is
* gated to C++17 and every mention of shared_timed_lock_policy to C++14, because
* below those standards those names DO NOT EXIST (naming one is a hard error,
* not
* a soft failure) - which is exactly the trap the selector's fallbacks exist to
* avoid, and which this suite is careful never to spring.
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.    thread_safety_leavel (the level enum)
      c.    II.   null policy (no_op_mutex, no_op_guard, null_lock_policy)
            d. III.  exclusive_lock_policy
            e. IV.   timed_lock_policy
            f. V.    shared_lock_policy
            g. VI.   shared_timed_lock_policy
            h. VII.  the policy selector
      i.    VIII. default_lock_policy
*/

#ifndef DJINTERP_TESTS_LOCK_POLICY_
#define DJINTERP_TESTS_LOCK_POLICY_ 1

// std
// std  (NOT <mutex> or <shared_mutex>: lock_policy.hpp's own gated include
//       chain supplies those, and tests_header_provides_the_std_facilities is
//       only an honest test of that from a TU that could not have gotten them
//       elsewhere)
#include <cstddef>
#include <type_traits>
// djinterp
//   The core comes first: lock_policy.hpp opens with a LIVE guard -
//     #ifndef DJINTERP_ENV_ENV_H
//         #error "lock_policy.hpp requires env.h to be included first"
//   - and does NOT include the environment itself, so it cannot be the first
// djinterp header in a TU.  tests_header_requires_the_environment_first records
// that the guard is real and satisfied here.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/lock_policy.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "lock_policy_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////
//   The header under test is pure vocabulary, so there are no shared helper
// types to declare here - every test is self-contained, and the ones that touch
// run-time behaviour (no_op_mutex's operations) construct what they need
// locally.

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_requires_the_environment_first();
bool tests_header_provides_the_std_facilities();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_the_selector_impl_lives_in_the_internal_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_the_policies_are_gated_by_standard();

// b.   I.   thread_safety_leavel (the level enum)
//////////////////////////////////////////
bool tests_level_has_the_six_named_values();
bool tests_level_ordering_is_a_strict_hierarchy();
bool tests_level_is_a_scoped_enum_over_int();
bool tests_the_correctly_spelled_alias_names_the_same_enum();
bool tests_level_is_usable_in_a_constant_expression();

// c.   II.  null policy
//////////////////////////////////////////
bool tests_no_op_mutex_operations_are_callable();
bool tests_no_op_mutex_try_lock_always_succeeds();
bool tests_no_op_guard_is_constructible_from_the_mutex();
bool tests_no_op_guard_is_not_copyable();
bool tests_no_op_guard_is_not_movable();
bool tests_null_policy_aliases();
bool tests_null_policy_read_and_write_are_the_same_guard();
bool tests_null_policy_descriptors_are_all_false();
bool tests_null_policy_level_is_none();

// d.   III. exclusive_lock_policy
//////////////////////////////////////////
bool tests_exclusive_aliases();
bool tests_exclusive_read_and_write_are_identical();
bool tests_exclusive_descriptors();
bool tests_exclusive_level_is_exclusive();

// e.   IV.  timed_lock_policy
//////////////////////////////////////////
bool tests_timed_aliases();
bool tests_timed_read_and_write_are_identical();
bool tests_timed_descriptors();
bool tests_timed_level_is_timed();

// f.   V.   shared_lock_policy (C++17+)
//////////////////////////////////////////
bool tests_shared_aliases();
bool tests_shared_read_and_write_differ();
bool tests_shared_descriptors();
bool tests_shared_level_is_shared();
bool tests_shared_read_lock_is_a_shared_lock();

// g.   VI.  shared_timed_lock_policy (C++14+)
//////////////////////////////////////////
bool tests_shared_timed_aliases();
bool tests_shared_timed_read_and_write_differ();
bool tests_shared_timed_descriptors();
bool tests_shared_timed_level_is_shared_timed();
bool tests_shared_timed_is_the_most_capable();

// h.   VII. the policy selector
//////////////////////////////////////////
bool tests_selector_none_maps_to_null();
bool tests_selector_atomic_only_also_maps_to_null();
bool tests_selector_exclusive_maps_to_exclusive();
bool tests_selector_timed_maps_to_timed();
bool tests_selector_shared_maps_correctly_for_this_standard();
bool tests_selector_shared_timed_maps_correctly_for_this_standard();
bool tests_select_lock_policy_is_the_public_alias_over_the_impl();
bool tests_selector_resolves_every_level();

// i.   VIII. default_lock_policy
//////////////////////////////////////////
bool tests_default_policy_for_this_standard();
bool tests_default_is_a_real_policy();
bool tests_default_matches_the_selector();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_LOCK_POLICY_
