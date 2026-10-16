/*******************************************************************************
* djinterp [test]                                                  rcu_tests.hpp
*
*   Declarations and shared helpers for the rcu.hpp unit-test suite.
*
* WHAT THIS HEADER IS, AND WHAT THAT MEANS FOR TESTING: rcu.hpp provides
* epoch-based reclamation primitives - the building blocks for lock-free
* containers that use read-side critical sections and DEFERRED memory
* reclamation.  Readers mark a critical section by recording the current epoch;
* writers copy-modify-swap and retire the old copy, which is reclaimed only once
* every reader from the retiring epoch has exited.  So the suite tests both the
* small pieces in isolation and the reclamation PROTOCOL they combine into:
* epoch_counter          - a monotonic global epoch (advance / current);
* epoch_guard            - an RAII reader marker that records the epoch on
* construction and clears it on destruction; epoch_registry         - a fixed
* pool of per-thread epoch slots, with a lock-free (CAS) claim and a
* min-active-epoch scan; deferred_reclaimer<T>  - counter + registry + a retired
* list, the engine that decides when a retired node is safe to free;
* rcu_protected<T>       - a complete RCU value with read / update / modify.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - EVERYTHING here is non-copyable AND non-movable.  epoch_counter and the
*       registry slots hold atomics (which do not move); epoch_guard and the
*       registry own a reference / a raw array and declare their special members
*       accordingly.  A retired-node engine that could be copied would double
*       free, so this is correct - the suite checks the effect throughout.
*     - rcu_read_guard is the subtle one.  It declares its move constructor `=
*       default`, which READS as movable - but the default is DELETED, because
*       its epoch_guard member is not movable (a reference member plus a
*       declared destructor plus a deleted copy).  So the guard is neither
*       copyable nor movable.
*     - and that has a VERSION consequence, recorded rather than judged: because
*       rcu_protected::read_lock() returns the guard BY VALUE, and a value
*       return of a non-movable type needs C++17's guaranteed copy elision to be
*       well formed, read_lock() compiles only at C++17 and above.  The file's
*       banner says "C++11: all custom types available", but the reader ENTRY
*       POINT is in practice C++17+.  Everything else - the counter, guard,
*       registry, reclaimer, and rcu_protected's construction / update / modify
*       / queries - does work from C++11.
*     - advance() and retire() return / record the PREVIOUS epoch, not the new
*       one (fetch_add semantics); the reclamation boundary is `retire_epoch <
*       min_active_epoch` (strictly less), so a reader sitting in epoch E holds
*       back everything retired at epoch >= E, and force_reclaim() bypasses the
*       check entirely (shutdown only).
*
* VERSIONING: the suite floors at C++11 - rcu.hpp itself requires it (it needs
* <atomic>) and #errors out below.  read_lock()-dependent tests are additionally
* gated to C++17+, per the note above, and degrade to a stated no-op below it.
* The C++23 std::rcu delegation path (rcu_protected replaced by a
* std::rcu_domain wrapper when <rcu> exists) is not exercised: this toolchain
* has no <rcu>, so D_HAS_STD_RCU is 0 and the custom rcu_protected is what is
* under test - which the suite records.
*
*   PORTABILITY: rcu.hpp keeps a LIVE env-first #error and does NOT include the
* core itself, so it cannot be the first djinterp header in a TU (the honest
* variant, like lock_policy.hpp / hazard_pointer.hpp).  Every section file, and
* this header, includes djinterp/core/djinterp.hpp before rcu.hpp.
*
*
* path:      /tests/djinterp/core/sync/rcu_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.16
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TEST HELPERS
      -------------------
      a. timing + rendezvous utilities
      b. instrumented types (destruct_probe, rc_node)

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. II.  epoch_counter
      c.    III. epoch_guard
            d. IV.  epoch_registry
            e. V.   deferred_reclaimer
            f. VI.  rcu_protected
            g. concurrency (the reclamation protocol under threads)
*/

#ifndef DJINTERP_TESTS_RCU_
#define DJINTERP_TESTS_RCU_ 1

// std
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <type_traits>
// djinterp
//   rcu.hpp has a LIVE env-first #error and does not include the core itself,
// so the core must come FIRST - it cannot stand first in a TU.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/rcu.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "rcu_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TEST HELPERS                                  ///
///////////////////////////////////////////////////////////////////////////////

// a.   timing + rendezvous utilities
//////////////////////////////////////////

// test_clock
//   type: the suite's clock - steady, never the wall clock.
typedef std::chrono::steady_clock test_clock;

// k_deadline_ms
//   constant: how long a concurrency test waits for something that MUST happen
// before it calls the operation broken.  A liveness bound, not a performance
// one.
D_STATIC D_CONSTEXPR unsigned k_deadline_ms = 3000;

// k_agents
//   constant: how many threads a contention test starts.
D_STATIC D_CONSTEXPR int k_agents = 4;

// k_iterations
//   constant: how many operations each worker performs in a churn loop.
D_STATIC D_CONSTEXPR int k_iterations = 4000;

// sleep_ms
//   function: park this thread for _ms milliseconds.
D_INLINE void
sleep_ms(
    unsigned _ms
)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

    return;
}

// elapsed_ms
//   function: milliseconds since _start.
D_INLINE long long
elapsed_ms(
    const test_clock::time_point& _start
)
{
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            test_clock::now() - _start).count());
}

// spin_until
//   function: wait for a flag to go true, up to _ms.  Returns false if the
// deadline passed first.
D_INLINE bool
spin_until(
    const std::atomic<bool>& _flag,
    unsigned                 _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (!_flag.load())
    {
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}

// spin_until_count
//   function: as spin_until, for a counter reaching _target.
D_INLINE bool
spin_until_count(
    const std::atomic<int>& _counter,
    int                     _target,
    unsigned                _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (_counter.load() < _target)
    {
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}


// b.   instrumented types
//////////////////////////////////////////

// destruct_probe_live
//   function: the shared live-instance counter for destruct_probe, held in a
// function-local static so the header stays single-definition across the
// section TUs.
D_INLINE std::atomic<long>&
destruct_probe_live()
{
    static std::atomic<long> counter(0);

    return counter;
}

// destruct_probe
//   type: counts its own live instances, so a test can prove the reclaimer's
// DEFAULT deleter (`delete p`) actually runs the destructor - not just drops
// the pointer.
struct destruct_probe
{
    int value;

    explicit destruct_probe(int _v = 0)
        : value(_v)
    {
        destruct_probe_live().fetch_add(1);
    }

    destruct_probe(const destruct_probe& _other)
        : value(_other.value)
    {
        destruct_probe_live().fetch_add(1);
    }

    ~destruct_probe()
    {
        destruct_probe_live().fetch_sub(1);
    }

    destruct_probe& operator=(const destruct_probe&) = delete;
};

// k_node_magic
//   constant: a recognizable stamp a live rc_node carries, so a concurrent
// reader can tell a valid node from freed memory (backed by the sanitizers).
D_STATIC D_CONSTEXPR std::uint64_t k_node_magic =
    static_cast<std::uint64_t>(0xC0FFEEULL);

// rc_node
//   type: the payload the concurrency safety test publishes and retires - a
// magic stamp plus a value.  A reader dereferences it under an epoch_guard and
// checks the stamp; if the node had been reclaimed early, the sanitizer catches
// the freed read and the stamp check fails.
struct rc_node
{
    std::uint64_t magic;
    int           value;

    explicit rc_node(int _v)
        : magic(k_node_magic)
        , value(_v)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_requires_environment_first();
bool tests_header_pulls_in_dependencies();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_header_is_gated_to_cpp11();
bool tests_std_rcu_delegation_is_off_on_this_build();
bool tests_read_lock_availability_is_cpp17();

// b.   II.  epoch_counter
//////////////////////////////////////////
bool tests_epoch_counter_starts_at_zero();
bool tests_epoch_counter_advance_returns_previous();
bool tests_epoch_counter_advance_increments();
bool tests_epoch_counter_inactive_is_uint64_max();
bool tests_epoch_counter_inactive_is_constexpr();
bool tests_epoch_counter_is_not_copyable();
bool tests_epoch_counter_is_not_movable();
bool tests_epoch_counter_operations_are_noexcept();

// c.   III. epoch_guard
//////////////////////////////////////////
bool tests_epoch_guard_records_epoch_on_construction();
bool tests_epoch_guard_resets_to_inactive_on_destruction();
bool tests_epoch_guard_captures_current_epoch();
bool tests_epoch_guard_is_not_copyable();
bool tests_epoch_guard_is_not_movable();
bool tests_epoch_guard_construction_is_noexcept();
bool tests_epoch_guard_two_slots_are_independent();

// d.   IV.  epoch_registry
//////////////////////////////////////////
bool tests_registry_default_capacity_is_64();
bool tests_registry_custom_capacity();
bool tests_registry_register_returns_a_slot();
bool tests_registry_fresh_slot_is_inactive();
bool tests_registry_registers_until_full();
bool tests_registry_unregister_frees_the_slot();
bool tests_registry_unregister_null_is_safe();
bool tests_registry_min_active_epoch_with_no_readers();
bool tests_registry_min_active_epoch_is_the_minimum();
bool tests_registry_idle_slot_does_not_lower_the_minimum();
bool tests_registry_capacity_zero_is_handled();
bool tests_registry_is_not_copyable_or_movable();

// e.   V.   deferred_reclaimer
//////////////////////////////////////////
bool tests_reclaimer_starts_empty();
bool tests_reclaimer_retire_advances_the_epoch();
bool tests_reclaimer_retire_adds_to_pending();
bool tests_reclaimer_scan_reclaims_with_no_readers();
bool tests_reclaimer_active_reader_blocks_reclamation();
bool tests_reclaimer_reclaims_after_reader_exits();
bool tests_reclaimer_custom_deleter_is_used();
bool tests_reclaimer_default_deleter_destroys_the_object();
bool tests_reclaimer_force_reclaim_ignores_readers();
bool tests_reclaimer_auto_scan_fires_at_threshold();
bool tests_reclaimer_scan_returns_the_count();
bool tests_reclaimer_scan_compacts_and_keeps_survivors();
bool tests_reclaimer_is_not_copyable_or_movable();

// f.   VI.  rcu_protected
//////////////////////////////////////////
bool tests_protected_constructs_from_an_lvalue();
bool tests_protected_constructs_from_an_rvalue();
bool tests_protected_update_advances_the_epoch();
bool tests_protected_update_retires_the_old_value();
bool tests_protected_modify_advances_and_retires();
bool tests_protected_updates_accumulate_pending();
bool tests_protected_is_not_copyable();
bool tests_protected_type_aliases_identify_the_strategy();
bool tests_protected_read_guard_is_not_copyable_or_movable();
bool tests_protected_read_returns_the_current_value();
bool tests_protected_updates_are_visible_to_readers();
bool tests_protected_modify_is_visible_to_readers();

// g.   concurrency (the reclamation protocol under threads)
//////////////////////////////////////////
bool tests_registry_concurrent_registration_is_distinct();
bool tests_registry_concurrent_registration_respects_capacity();
bool tests_concurrent_readers_never_observe_reclaimed_data();
bool tests_concurrent_retire_and_scan_make_progress();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_RCU_
