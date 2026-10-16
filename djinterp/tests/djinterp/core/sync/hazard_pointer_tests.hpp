/*******************************************************************************
* djinterp [test]                                       hazard_pointer_tests.hpp
*
*   Declarations for the hazard_pointer.hpp unit-test suite, plus the helper
* types the sections share.
*
*   WHAT A HAZARD POINTER IS FOR, AND WHAT THAT MEANS FOR TESTING:
*   A hazard pointer is a reader's public promise - "I am about to dereference
* this node; do not free it" - that a writer's reclamation scan must honour.
* The one property the whole file exists to provide is therefore a NEGATIVE
* one: a node that some active hazard record protects is NOT reclaimed by a
* concurrent scan.  That safety property is the spine of the suite
* (tests_scan_spares_a_protected_node, and its concurrent form
* tests_a_protected_node_survives_a_concurrent_scan), and almost everything else
* is either a piece of the protocol that leads up to it or an accounting fact
* that keeps it honest.
*
*   The header is a lock-free vocabulary, so most of it is observable
* single-threaded: acquire() hands out distinct records until the pool is empty
* and then returns nullptr; protect() publishes a pointer that is_protected()
* and collect_protected() can see; retire() queues a node and scan() frees
* exactly those queued nodes that no record protects, compacting the survivors.
* Those are counted and checked directly.  The genuinely concurrent claims - the
* atomic hand-off of a record, the protect/scan race - get their own section
* built from std::atomic and std::thread, with a rendezvous so the reader and
* the scanner actually overlap rather than lining up by luck.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - protect_and_validate() PROTECTS AND RETURNS A NULL POINTER when the
*       source holds null: it loads null, publishes null, re-reads null, and
*       reports success.  The caller gets nullptr back and must not read it as
*       "validated, go ahead" - it means "nothing to protect"
*       (tests_validate_on_a_null_source_returns_null).
*     - protect_and_validate(source, 0) - zero retries - never runs the loop,
*       so it returns nullptr WITHOUT publishing anything and WITHOUT clearing a
*       previously published protection.  A caller that passes 0 gets no
*       protection and no diagnostic (tests_validate_with_zero_retries_does_
*       nothing).
*     - a domain of capacity 0 allocates a zero-length record array (legal), so
*       acquire() always fails - and should_scan() is TRUE from the start,
*       because its threshold is 2*0 == 0 and the retired count is already >= 0
*       (tests_a_zero_capacity_domain_always_wants_to_scan).
*     - acquire() exhaustion is silent: the (capacity+1)-th caller gets nullptr,
*       and every guard type turns that into valid()==false plus protect/clear
*       that are safe no-ops.  A caller that does not check valid() gets no
*       protection and no error (tests_scoped_hazard_survives_exhaustion, and
*       kin).
*     - multi_hazard_domain::acquire_slot_group() claims the group by CAS on
*       its first slot ONLY, then plain-stores the rest active.  The group is
*       contiguous, and releasing it clears every slot
*       (tests_group_is_contiguous_and_fully_active).
*
*   PORTABILITY: the whole header is gated to C++11 (it needs <atomic>); below
* that it is empty.  Under C++23, IF <hazard_pointer> is available, the header
* delegates sections I to VIII to the standard and defines NONE of the custom
* types - so the suite compiles to almost nothing there.  Every section is
* therefore wrapped so that it degrades to a stated no-op when the custom types
* are absent; see the D_HAS_STD_HAZARD_POINTER discussion in
* tests_which_implementation_is_in_play.
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      -------
      a. availability gate (custom types vs std delegation)
      b. counted node payloads
      c.    deadlines and spin-waits (concurrency section)

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. II.   hazard_record
      c.    III.  hazard_domain - records
            d. III.  hazard_domain - scanning and retire count
            e. IV.   scoped_hazard
            f. V.    typed_hazard_ptr
            g. VI.   retired_list
            h. VII.  multi_hazard_domain
      i.    VIII. scoped_multi_hazard
            j. concurrency
*/

#ifndef DJINTERP_TESTS_HAZARD_POINTER_
#define DJINTERP_TESTS_HAZARD_POINTER_ 1

// std
// std  (NOT <atomic>, <cstdint> or <vector>: hazard_pointer.hpp's own gated
//       include chain supplies those, and
//       tests_header_provides_the_std_facilities is only an honest test of that
//       from a TU that could not have gotten them elsewhere)
#include <chrono>
#include <cstddef>
#include <thread>
#include <type_traits>
// djinterp
//   The core comes first: hazard_pointer.hpp opens with a LIVE guard -
//     #ifndef DJINTERP_ENV_ENV_H
//         #error "hazard_pointer.hpp requires env.h to be included first"
//   - so it cannot be the first djinterp header in a TU.  Unlike cow.hpp's
// silent version of this, the guard here is honest: it diagnoses the mistake
// cleanly.  tests_header_requires_the_environment_first records it.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/hazard_pointer.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "hazard_pointer_tests.hpp requires C++11 or higher"
#endif


// D_HAS_CUSTOM_HAZARD
//   Whether the CUSTOM hazard types (sections II-VIII) are defined.  They are,
// except under C++23 with <hazard_pointer> present, where the header delegates
// to the standard and defines only hazard_pointer_domain / hazard_pointer<T>.
// Every section-body test is written to degrade to a stated no-op when this is
// 0, so the suite stays green whichever implementation the standard selects.
#if D_HAS_STD_HAZARD_POINTER
    #define D_HAS_CUSTOM_HAZARD 0
#else
    #define D_HAS_CUSTOM_HAZARD 1
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// a.   counted node payloads
//////////////////////////////////////////
//   Reclamation is the whole point, and reclamation is invisible from a value:
// the bytes a freed node held may still read back correctly.  So the suite
// COUNTS instead - how many nodes are live right now, and how many the retired
// list has freed - through function-local-static atomics that every section TU
// shares.  A leak shows up as a non-zero live count at the end of a test; a
// double free shows up as a live count that went negative.

// probe_live
//   function: how many counted_node objects exist right now.  Back to zero at
// the end of a clean test.
D_INLINE std::atomic<long>&
probe_live()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_deletions
//   function: how many counted_node objects the reclamation path has destroyed.
// This is the number scan() and clear() are claims about.
D_INLINE std::atomic<long>&
probe_deletions()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_reset
//   function: zero both counters.  Called at the top of any test that counts.
D_INLINE void
probe_reset()
{
    probe_live().store(0);
    probe_deletions().store(0);

    return;
}

// counted_node
//   type: the payload the retired-list tests reclaim.  It exists only to be
// counted: every construction raises the live count, every destruction raises
// the deletion count and lowers the live one.  A node is "reclaimed" when its
// deleter runs, which is exactly when this destructor runs.
struct counted_node
{
    int value;

    explicit counted_node(
        int _value = 0
    )
        : value(_value)
    {
        probe_live().fetch_add(1);
    }

    counted_node(const counted_node&)            = delete;
    counted_node& operator=(const counted_node&) = delete;

    ~counted_node()
    {
        probe_deletions().fetch_add(1);
        probe_live().fetch_sub(1);
    }
};


// b.   deadlines and spin-waits (concurrency section)
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
//   constant: how many threads a concurrency test starts.
D_STATIC D_CONSTEXPR int k_agents = 4;

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
// deadline passed first - which is how a stalled thread becomes a FAILED test
// rather than a hung runner.  It sleeps between polls: a hot spin on a
// single-core box can starve the very thread it waits for.
D_INLINE bool
spin_until(
    const std::atomic<bool>& _flag,
    unsigned                 _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (!_flag.load())
    {
        // check if the deadline has passed
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
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_requires_the_environment_first();
bool tests_header_pulls_its_dependency();
bool tests_header_provides_the_std_facilities();
bool tests_header_is_gated_to_cpp11();
bool tests_which_implementation_is_in_play();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_the_std_delegation_aliases_exist_when_selected();

// b.   II.  hazard_record
//////////////////////////////////////////
bool tests_record_default_is_inactive_and_null();
bool tests_record_holds_two_atomics();
bool tests_record_pointer_is_a_void_pointer();
bool tests_record_is_not_copyable();

// c.   III. hazard_domain - records
//////////////////////////////////////////
bool tests_domain_reports_its_capacity();
bool tests_domain_is_not_copyable();
bool tests_domain_declares_the_hazard_strategy_tag();
bool tests_domain_marks_itself_with_hazard_domain_type();
bool tests_acquire_hands_out_a_record();
bool tests_acquire_hands_out_distinct_records();
bool tests_acquire_fills_the_pool_then_returns_null();
bool tests_release_returns_a_record_to_the_pool();
bool tests_release_clears_the_protected_pointer();
bool tests_release_of_null_is_a_no_op();
bool tests_a_reacquired_record_is_clean();
bool tests_a_zero_capacity_domain_acquires_nothing();

// d.   III. hazard_domain - scanning and retire count
//////////////////////////////////////////
bool tests_is_protected_sees_a_published_pointer();
bool tests_is_protected_is_false_for_an_unprotected_pointer();
bool tests_is_protected_of_null_is_false();
bool tests_is_protected_ignores_an_inactive_record();
bool tests_collect_gathers_the_protected_pointers();
bool tests_collect_clears_its_output_first();
bool tests_collect_skips_null_and_inactive_slots();
bool tests_retired_count_starts_at_zero();
bool tests_increment_and_decrement_retired();
bool tests_decrement_retired_takes_a_count();
bool tests_should_scan_crosses_at_twice_capacity();
bool tests_a_zero_capacity_domain_always_wants_to_scan();

// e.   IV.  scoped_hazard
//////////////////////////////////////////
bool tests_scoped_acquires_on_construction();
bool tests_scoped_releases_on_destruction();
bool tests_scoped_protect_publishes();
bool tests_scoped_clear_unpublishes();
bool tests_scoped_is_not_copyable();
bool tests_scoped_hazard_survives_exhaustion();
bool tests_scoped_protect_while_invalid_is_a_no_op();

// f.   V.   typed_hazard_ptr
//////////////////////////////////////////
bool tests_typed_acquires_a_record();
bool tests_typed_is_not_copyable();
bool tests_validate_on_a_stable_source_succeeds();
bool tests_validate_publishes_the_validated_pointer();
bool tests_validate_on_a_null_source_returns_null();
bool tests_validate_with_zero_retries_does_nothing();
bool tests_validate_clears_protection_when_it_gives_up();
bool tests_typed_protect_publishes_directly();
bool tests_typed_clear_unpublishes();
bool tests_typed_on_an_exhausted_domain_is_invalid();

// g.   VI.  retired_list
//////////////////////////////////////////
bool tests_retired_list_starts_empty();
bool tests_retire_queues_a_node();
bool tests_scan_reclaims_an_unprotected_node();
bool tests_scan_spares_a_protected_node();
bool tests_scan_compacts_the_survivors();
bool tests_scan_returns_the_reclaimed_count();
bool tests_scan_with_a_custom_deleter();
bool tests_default_retire_deletes_the_node();
bool tests_scan_by_epoch_reclaims_older_entries();
bool tests_scan_by_epoch_keeps_newer_entries();
bool tests_clear_runs_every_deleter();
bool tests_pending_and_empty_track_the_queue();
bool tests_scan_on_an_empty_list_is_a_no_op();

// h.   VII. multi_hazard_domain
//////////////////////////////////////////
bool tests_multi_reports_its_geometry();
bool tests_multi_declares_the_hazard_strategy_tag();
bool tests_multi_marks_itself_with_hazard_domain_type();
bool tests_multi_is_not_copyable();
bool tests_acquire_group_hands_out_a_group();
bool tests_group_is_contiguous_and_fully_active();
bool tests_acquire_group_fills_then_returns_null();
bool tests_release_group_clears_every_slot();
bool tests_multi_is_protected_scans_every_slot();
bool tests_multi_collect_gathers_across_groups();
bool tests_multi_should_scan_crosses_at_twice_total_slots();
bool tests_multi_slots_per_thread_is_the_template_argument();

// i.   VIII. scoped_multi_hazard
//////////////////////////////////////////
bool tests_scoped_multi_acquires_a_group();
bool tests_scoped_multi_releases_on_destruction();
bool tests_scoped_multi_protects_a_slot();
bool tests_scoped_multi_clears_a_slot();
bool tests_scoped_multi_clear_all();
bool tests_scoped_multi_is_not_copyable();
bool tests_scoped_multi_out_of_range_slot_is_a_no_op();
bool tests_scoped_multi_survives_exhaustion();

// j.   concurrency
//////////////////////////////////////////
bool tests_concurrent_acquire_hands_out_distinct_records();
bool tests_a_protected_node_survives_a_concurrent_scan();
bool tests_validation_under_a_churning_source_is_sound();
bool tests_concurrent_protect_and_scan_leak_nothing();
bool tests_group_acquisition_is_atomic_under_contention();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_HAZARD_POINTER_
