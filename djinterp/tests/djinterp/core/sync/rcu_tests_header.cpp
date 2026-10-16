/*******************************************************************************
* djinterp [core]                                           rcu_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       rcu_tests_header.cpp
// Section: the header contract.  The claims about the FILE - the include guard,
// double inclusion, the LIVE env-first #error (rcu.hpp cannot stand first, the
// honest variant), the dependencies it pulls in, the namespace its entities
// live in, the C++11 gating, and the two version facts worth recording: that
// the C++23 std::rcu delegation is OFF on this toolchain (so the custom
// rcu_protected is what is tested), and that rcu_protected::read_lock() is in
// practice a C++17+ entry point even though the banner lists everything at
// C++11.

// djinterp
#include "rcu_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions of the
// classes would collide here if the include guard did not hold; the successful
// build IS tests_header_is_idempotent's assertion.  (The core is already in
// from rcu_tests.hpp, so rcu.hpp's env-first #error is satisfied on this line
// too.)
#include "../../../../inc/djinterp/core/sync/rcu.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_RCU_HPP is defined after inclusion
  - it is defined to 1
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_RCU_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_RCU_HPP == 1);
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
  - after both inclusions the types are still single, whole types
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(epoch_counter) > 0)             &&
          (sizeof(epoch_registry) > 0)            &&
          (sizeof(deferred_reclaimer<int>) > 0)   &&
          (sizeof(rcu_protected<int>) > 0) );

    return intact;
}

/*
tests_header_requires_environment_first
  rcu.hpp cannot be the first djinterp header in a TU.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - which it must be, because rcu.hpp
    keeps a LIVE `#error "rcu.hpp requires env.h to be included first"` and does
    NOT include the core itself, so the core had to come first for this TU to
    compile at all
  This is the honest variant, like lock_policy.hpp and hazard_pointer.hpp (and
  the opposite of the self-sufficient lock_guard / lock_policy_c).  A separate
  probe TU (not shown) confirms that a bare `#include "rcu.hpp"` with no prior
  core include fails with that #error; here we confirm the symbol whose presence
  means the required ordering was honored.
*/
bool
tests_header_requires_environment_first()
{
#if defined(DJINTERP_ENV_ENV_H)
    return true;
#else
    return false;
#endif
}

/*
tests_header_pulls_in_dependencies
  rcu.hpp brings in the headers it is layered on.
  Tests the following:
  - concurrency_strategy_tags.hpp is in: rcu_strategy_tag is nameable, and
    rcu_protected's concurrency_strategy_tag alias IS that tag
  - so the strategy-tag vocabulary the containers key on really did come along
    with rcu.hpp (it includes ./concurrency_strategy_tags.hpp and ./atomic.hpp)
*/
bool
tests_header_pulls_in_dependencies()
{
    D_CONSTEXPR bool tag_ok =
        std::is_same<
            rcu_protected<int>::concurrency_strategy_tag,
            rcu_strategy_tag>::value;

    static_assert(tag_ok,
                  "rcu_protected must be tagged with rcu_strategy_tag");

    return tag_ok;
}

/*
tests_entities_live_in_the_djinterp_namespace
  the RCU types sit directly in djinterp.
  Tests the following:
  - epoch_counter, epoch_registry, deferred_reclaimer<int> and
    rcu_protected<int>, found by unqualified lookup from djinterp::testing, ARE
    ::djinterp's
  These are the public surface consumers name directly.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool public_surface =
        ( std::is_same<epoch_counter, ::djinterp::epoch_counter>::value   &&
          std::is_same<epoch_registry, ::djinterp::epoch_registry>::value &&
          std::is_same<deferred_reclaimer<int>,
                       ::djinterp::deferred_reclaimer<int> >::value       &&
          std::is_same<rcu_protected<int>,
                       ::djinterp::rcu_protected<int> >::value );

    static_assert(public_surface,
                  "the RCU types must live directly in namespace djinterp");

    return public_surface;
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros - for its public vocabulary.
  Tests the following:
  - none of epoch_counter / epoch_guard / epoch_registry / deferred_reclaimer /
    rcu_protected is a macro
  - (D_HAS_STD_RCU, by contrast, IS a macro the header defines on purpose, as a
    standard-library-availability switch; it is not a public type and is checked
    separately, not here)
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(epoch_counter)       || \
      defined(epoch_guard)         || \
      defined(epoch_registry)      || \
      defined(deferred_reclaimer)  || \
      defined(rcu_protected) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_header_is_gated_to_cpp11
  the RCU types appear only at C++11 and above.
  Tests the following:
  - at the suite's C++11 floor (and up) the types are all present and complete
  - this matches the banner's "C++98/03: unavailable (requires <atomic>)" line:
    the whole body sits inside `#if D_ENV_LANG_IS_CPP11_OR_HIGHER`
  Since the suite floors at C++11, the gate is always satisfied here; the test
  records the correspondence.
*/
bool
tests_header_is_gated_to_cpp11()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const bool present =
        ( (sizeof(epoch_counter) > 0)   &&
          (sizeof(epoch_guard) > 0)     &&
          (sizeof(epoch_registry) > 0) );

    return present;
#else
    return true;
#endif
}

/*
tests_std_rcu_delegation_is_off_on_this_build
  the C++23 std::rcu delegation path is not active here.
  Tests the following:
  - D_HAS_STD_RCU is 0, so the header's own rcu_protected (the `#if
    !D_HAS_STD_RCU` branch) is the one in play, not a std::rcu_domain wrapper
  The banner notes that at C++23 rcu_protected delegates to std::rcu_domain WHEN
  AVAILABLE; this toolchain ships no <rcu>, so the custom implementation is what
  the rest of the suite exercises.  Recorded so the intent of the tests is
  clear: they cover the fallback implementation, which is the one that will
  actually be compiled on any pre-<rcu> toolchain.
*/
bool
tests_std_rcu_delegation_is_off_on_this_build()
{
#if defined(D_HAS_STD_RCU)
    const bool delegation_off = (D_HAS_STD_RCU == 0);
#else
    const bool delegation_off = false;   // the header always defines it
#endif

    return delegation_off;
}

/*
tests_read_lock_availability_is_cpp17
  the reader entry point is, in practice, C++17+ - a banner-vs-reality note.
  Tests the following:
  - rcu_read_guard is neither copy- nor move-constructible (its move ctor is
    declared `= default` but is DELETED, because its epoch_guard member cannot
    be moved) - true at every standard
  - because read_lock() returns that guard BY VALUE, and returning a non-movable
    type by value is well-formed only under C++17's guaranteed copy elision,
    read_lock() compiles from C++17 onward but NOT at C++11/14 - even though the
    file's banner lists all custom types as available at C++11
  So this pins the ROOT CAUSE (a non-movable guard) and, where the standard
  allows it, that the entry point is live: the C++17+ read-side tests in the
  rcu_protected section actually call read_lock(), and their mere compilation at
  C++17+ is the other half of this fact.
*/
bool
tests_read_lock_availability_is_cpp17()
{
    typedef rcu_protected<int>::rcu_read_guard guard_type;

    D_CONSTEXPR bool guard_is_pinned =
        ( !std::is_copy_constructible<guard_type>::value &&
          !std::is_move_constructible<guard_type>::value );

    static_assert(guard_is_pinned,
                  "rcu_read_guard must be neither copyable nor movable");

    // At C++17+ the entry point is additionally known to compile (the read-side
    // tests exercise it); below C++17 only the root-cause trait above holds.
    return guard_is_pinned;
}


NS_END  // testing
NS_END  // djinterp
