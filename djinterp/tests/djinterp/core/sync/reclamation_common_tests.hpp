/*******************************************************************************
* djinterp [test]                                   reclamation_common_tests.hpp
*
*   Declarations and shared helpers for the reclamation_common.hpp test suite.
*
*   WHAT THIS HEADER IS, AND WHAT THAT MEANS FOR TESTING:
*   reclamation_common.hpp is the retired-node engine that hazard_pointer.hpp
* and rcu.hpp were each maintaining a private copy of.  The whole point of the
* extraction is that those two schemes differ ONLY in the rule for calling a
* node safe, so the engine takes that rule as a predicate:
*     reclaim_list<T>   - the store: retire(), reclaim_if(pred),
*                         force_reclaim(), pending(), and an optional
*                         should_scan() threshold;
*     published_ptr<T>  - the single-writer / many-reader pointer whose
*                         publish() swaps in a new value and hands back the
*                         old one to retire.
*
*   THE SUITE THEREFORE TESTS BOTH CALL SITES, NOT AN ABSTRACTION:
*   the reclamation section drives reclaim_list with the RCU predicate
* (tag < safe_epoch) and again with the hazard predicate (pointer not in the
* protected set), and checks that each reproduces the behaviour its original
* engine had.  If the generalization were lossy, one of those two would fail -
* which is the only test that really matters for an extraction like this.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - reclamation runs each node's deleter EXACTLY once, and the survivors
*       keep their ORDER and their own deleters after compaction.  The
*       compaction is a two-cursor in-place move, so a survivor that ends up
*       holding another entry's deleter would be a silent double-free later;
*       the tests use per-node deleters and check the recovered order.
*     - reclaim_list does NOT free pending nodes in its destructor - it cannot
*       know they are unreachable.  The owner reclaims at teardown via
*       force_reclaim().  The suite relies on that (every test drains what it
*       retires) and states it, since a "leak" here is by design.
*     - the threshold is ADVISORY.  should_scan() only reports that a scan is
*       worthwhile; the list never scans itself, because only the owner holds
*       the predicate.  A threshold of 0 disables it entirely.
*     - reclaim_list is move-only and published_ptr is pinned: the list may be
*       relocated with its pending nodes intact, but the published pointer is
*       an address others load from and must never move.
*     - the deleters are std::function, so a caller may supply one that does
*       NOT delete (pooled storage); the engine must not assume ownership
*       semantics beyond calling what it was given.
*
*   VERSIONING: the suite floors at C++11, which is the header's own floor
* (it needs <atomic> and <functional>).  Nothing is version-gated above that.
*
*   PORTABILITY: reclamation_common.hpp includes ../djinterp.hpp itself (and
* sync_common.hpp), so it is self-sufficient and CAN stand first in a TU.
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TEST HELPERS
      -------------------
      a. instrumented payloads (live counting, deleter recording)
      b. the two scheme predicates, as the real call sites spell them

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.  reclaim_list - retiring and queries
      c.    I.  reclaim_list - reclamation (both schemes)
            d. II. published_ptr
*/

#ifndef DJINTERP_TESTS_RECLAMATION_COMMON_
#define DJINTERP_TESTS_RECLAMATION_COMMON_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
// djinterp
//   reclamation_common.hpp includes the core itself, so it may stand first.
#include "../../../../inc/djinterp/core/sync/reclamation_common.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "reclamation_common_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TEST HELPERS                                  ///
///////////////////////////////////////////////////////////////////////////////

// a.   instrumented payloads
//////////////////////////////////////////

// reclaim_probe_live
//   function: the shared live-instance counter for reclaim_probe, held in a
// function-local static so the header stays single-definition across the
// section TUs.
inline int&
reclaim_probe_live()
{
    static int counter = 0;

    return counter;
}

// reclaim_probe
//   type: a payload that counts its own live instances, so a test can prove
// the DEFAULT deleter really destroyed the object rather than just dropping
// the pointer.
struct reclaim_probe
{
    int value;

    explicit reclaim_probe(int _v = 0)
        : value(_v)
    {
        ++reclaim_probe_live();
    }

    reclaim_probe(const reclaim_probe& _other)
        : value(_other.value)
    {
        ++reclaim_probe_live();
    }

    ~reclaim_probe()
    {
        --reclaim_probe_live();
    }

    reclaim_probe& operator=(const reclaim_probe&) = delete;
};

// reclaim_log
//   type: records the values of the nodes a deleter freed, in the order the
// deleter ran.  Order is the interesting part: it proves the compaction kept
// the survivors in sequence and paired each with its own deleter.
struct reclaim_log
{
    std::vector<int> freed;

    // deleter_for
    //   returns a deleter that appends the node's value to this log and then
    // deletes it.  Each retired node gets its own, so a mismatched pairing
    // after compaction would show up as an out-of-order or wrong value.
    std::function<void(int*)>
    deleter_for()
    {
        reclaim_log* self = this;

        return [self](int* _p)
        {
            self->freed.push_back(*_p);
            delete _p;
        };
    }

    std::size_t
    count() const
    {
        return freed.size();
    }
};


// b.   the two scheme predicates, as the real call sites spell them
//////////////////////////////////////////

// epoch_is_safe
//   type: the RCU rule.  A node is reclaimable when the epoch it was retired
// in lies strictly BELOW the oldest epoch any reader still occupies, so a
// reader sitting in epoch E holds back everything retired at epoch >= E.
struct epoch_is_safe
{
    std::uint64_t safe_epoch;

    explicit epoch_is_safe(std::uint64_t _safe)
        : safe_epoch(_safe)
    {}

    bool
    operator()(const reclaim_list<int>::entry& _entry) const
    {
        return (_entry.tag < safe_epoch);
    }
};

// pointer_is_unprotected
//   type: the hazard-pointer rule.  A node is reclaimable when no active
// hazard record still points at it; the tag goes unused.
struct pointer_is_unprotected
{
    const std::vector<int*>* protected_ptrs;

    explicit pointer_is_unprotected(const std::vector<int*>* _protected)
        : protected_ptrs(_protected)
    {}

    bool
    operator()(const reclaim_list<int>::entry& _entry) const
    {
        for (std::size_t i = 0; i < protected_ptrs->size(); ++i)
        {
            if ((*protected_ptrs)[i] == _entry.ptr)
            {
                return false;
            }
        }

        return true;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_can_stand_first();
bool tests_header_pulls_in_sync_common();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_header_is_gated_to_cpp11();

// b.   I.  reclaim_list - retiring and queries
//////////////////////////////////////////
bool tests_list_starts_empty();
bool tests_list_retire_adds_to_pending();
bool tests_list_retire_defaults_the_tag_to_zero();
bool tests_list_retire_records_the_tag();
bool tests_list_retire_accepts_a_custom_deleter();
bool tests_list_default_deleter_destroys_the_object();
bool tests_list_threshold_is_off_by_default();
bool tests_list_threshold_reports_when_reached();
bool tests_list_threshold_zero_disables_the_report();
bool tests_list_is_move_only();
bool tests_list_move_carries_the_pending_nodes();
bool tests_list_entry_exposes_pointer_and_tag();

// c.   I.  reclaim_list - reclamation (both schemes)
//////////////////////////////////////////
bool tests_reclaim_frees_matching_nodes();
bool tests_reclaim_keeps_non_matching_nodes();
bool tests_reclaim_returns_the_count();
bool tests_reclaim_runs_each_deleter_exactly_once();
bool tests_reclaim_preserves_survivor_order();
bool tests_reclaim_compacts_non_contiguous_survivors();
bool tests_reclaim_on_an_empty_list_is_a_no_op();
bool tests_reclaim_can_free_everything();
bool tests_force_reclaim_frees_all_regardless();
bool tests_force_reclaim_returns_the_count();
bool tests_force_reclaim_on_an_empty_list_is_a_no_op();
bool tests_reclaim_tolerates_a_non_deleting_deleter();
bool tests_rcu_scheme_reproduces_epoch_reclamation();
bool tests_hazard_scheme_reproduces_protected_reclamation();
bool tests_both_schemes_share_one_engine();

// d.   II. published_ptr
//////////////////////////////////////////
bool tests_published_default_is_null();
bool tests_published_constructs_from_a_pointer();
bool tests_published_publish_returns_the_previous();
bool tests_published_load_sees_the_new_value();
bool tests_published_publish_accepts_null();
bool tests_published_compare_publish_succeeds_when_expected();
bool tests_published_compare_publish_fails_and_reports();
bool tests_published_is_pinned();
bool tests_published_costs_one_pointer();
bool tests_published_store_installs_without_returning();
bool tests_published_store_is_noexcept();
bool tests_published_pairs_with_the_reclaim_list();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_RECLAMATION_COMMON_
