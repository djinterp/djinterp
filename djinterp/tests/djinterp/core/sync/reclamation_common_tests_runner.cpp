/*******************************************************************************
* djinterp [core]                            reclamation_common_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  reclamation_common_tests_runner.cpp
//   Entry point for the reclamation_common.hpp suite: registers every
//   section's tests with report_builder, one module per section.

// djinterp
#include "reclamation_common_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- reclamation_common.hpp Test Suite");
    rb.set_subtitle("sync : the shared retired-node engine -- one store, two "
                    "reclamation schemes");
    rb.set_author("teer");
    rb.set_description(
        "reclamation_common.hpp is the retired-node engine that hazard_pointer."
        "hpp and rcu.hpp were each maintaining a private copy of. The whole "
        "point of the extraction is that those two schemes differ ONLY in the "
        "rule for calling a node safe, so the engine takes that rule as a "
        "predicate: reclaim_list<T> is the store (retire, reclaim_if, "
        "force_reclaim, pending, and an advisory should_scan threshold), and "
        "published_ptr<T> is the single-writer / many-reader pointer whose "
        "publish swaps in a replacement and hands back the old value to retire. "
        "The suite therefore tests BOTH CALL SITES rather than an abstraction: "
        "the reclamation section drives one reclaim_list with the RCU rule (tag "
        "< safe epoch, checking the strictly-less boundary a reader pinned in an "
        "epoch depends on) and again with the hazard rule (pointer not currently "
        "announced, where the tag goes unused), and confirms each reproduces the "
        "behaviour its original engine had -- if the generalization were lossy, "
        "one of those two would fail, which is the only test that really matters "
        "for an extraction like this. A third test scans a single list with one "
        "rule and then the other, since one engine answering two questions is "
        "the claim the whole refactor rests on. The sharp edges are pinned "
        "deliberately. Reclamation runs each node's deleter EXACTLY once and the "
        "survivors keep their ORDER and their own deleters through compaction: "
        "that compaction is an in-place two-cursor move, so a survivor left "
        "holding another entry's deleter would be a silent double-free later, "
        "and the tests give every node its own deleter and check the recovered "
        "order rather than merely the count -- including a non-contiguous case, "
        "since a prefix-only test would pass even with a broken compaction. "
        "reclaim_list deliberately does NOT free pending nodes in its "
        "destructor (it cannot know they are unreachable; the owner drains via "
        "force_reclaim at teardown), so every test drains what it retires, which "
        "is also how the real call sites behave. The threshold is ADVISORY -- "
        "the list never scans itself because only the owner holds the predicate "
        "-- and a threshold of 0 disables it entirely, which must not read as "
        "'every node qualifies'. The list is move-only (two lists holding the "
        "same raw pointers would double-free) while published_ptr is PINNED, "
        "since readers load from its address. Deleters are std::function, so a "
        "caller may supply one that does not delete at all (pooled storage) and "
        "the engine must add no delete of its own. Verified under C++11, C++14, "
        "C++17 and C++20; AddressSanitizer-, LeakSanitizer- and "
        "UndefinedBehaviorSanitizer-clean.");

    rb.use_pdf("reclamation_common_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "guard, idempotency, standing first, its dependency, and gating");
    rb.run("the include guard is defined, and is 1",       &tests_header_include_guard);
    rb.run("including it twice is a no-op",                &tests_header_is_idempotent);
    rb.run("it can stand first in a TU",                   &tests_header_can_stand_first);
    rb.run("it pulls in sync_common.hpp itself",           &tests_header_pulls_in_sync_common);
    rb.run("the entities live flat in djinterp",           &tests_entities_live_in_the_djinterp_namespace);
    rb.run("it publishes types, not macros",               &tests_header_defines_no_stray_macros);
    rb.run("the engine is gated to C++11",                 &tests_header_is_gated_to_cpp11);

    // -- I. reclaim_list: retiring and queries ------------------------------
    rb.module("I. reclaim_list -- retiring and queries",
              "what retire records, what pending reports, and the advisory "
              "scan threshold");
    rb.run("a fresh list holds nothing",                   &tests_list_starts_empty);
    rb.run("retiring records a node without freeing it",   &tests_list_retire_adds_to_pending);
    rb.run("the tag defaults to 0",                        &tests_list_retire_defaults_the_tag_to_zero);
    rb.run("a supplied tag reaches the predicate",         &tests_list_retire_records_the_tag);
    rb.run("a custom deleter is stored and used",          &tests_list_retire_accepts_a_custom_deleter);
    rb.run("the default deleter destroys the object",      &tests_list_default_deleter_destroys_the_object);
    rb.run("the threshold is off by default",              &tests_list_threshold_is_off_by_default);
    rb.run("an armed threshold reports when reached",      &tests_list_threshold_reports_when_reached);
    rb.run("a threshold of 0 disables the report",         &tests_list_threshold_zero_disables_the_report);
    rb.run("the list is move-only",                        &tests_list_is_move_only);
    rb.run("moving carries the pending nodes",             &tests_list_move_carries_the_pending_nodes);
    rb.run("an entry exposes both pointer and tag",        &tests_list_entry_exposes_pointer_and_tag);

    // -- I. reclaim_list: reclamation ---------------------------------------
    rb.module("I. reclaim_list -- reclamation (both schemes)",
              "the predicate decides; compaction preserves order; RCU and "
              "hazard both ride the same engine");
    rb.run("an approving predicate frees the nodes",       &tests_reclaim_frees_matching_nodes);
    rb.run("a refusing predicate keeps them",              &tests_reclaim_keeps_non_matching_nodes);
    rb.run("reclaim_if returns how many it freed",         &tests_reclaim_returns_the_count);
    rb.run("each deleter runs exactly once",               &tests_reclaim_runs_each_deleter_exactly_once);
    rb.run("compaction preserves survivor order",          &tests_reclaim_preserves_survivor_order);
    rb.run("compaction handles non-contiguous gaps",       &tests_reclaim_compacts_non_contiguous_survivors);
    rb.run("scanning an empty list is a no-op",            &tests_reclaim_on_an_empty_list_is_a_no_op);
    rb.run("a full sweep empties the list",                &tests_reclaim_can_free_everything);
    rb.run("force_reclaim frees all regardless",           &tests_force_reclaim_frees_all_regardless);
    rb.run("force_reclaim returns the count",              &tests_force_reclaim_returns_the_count);
    rb.run("force_reclaim on an empty list is a no-op",    &tests_force_reclaim_on_an_empty_list_is_a_no_op);
    rb.run("a non-deleting deleter is honoured as given",  &tests_reclaim_tolerates_a_non_deleting_deleter);
    rb.run("RCU: epoch reclamation is reproduced",         &tests_rcu_scheme_reproduces_epoch_reclamation);
    rb.run("HAZARD: protected reclamation is reproduced",  &tests_hazard_scheme_reproduces_protected_reclamation);
    rb.run("both schemes really share one engine",         &tests_both_schemes_share_one_engine);

    // -- II. published_ptr ---------------------------------------------------
    rb.module("II. published_ptr",
              "the publish half of the cycle: swap in, hand the old value back "
              "to retire");
    rb.run("a default-constructed pointer is null",        &tests_published_default_is_null);
    rb.run("it can be constructed over a node",            &tests_published_constructs_from_a_pointer);
    rb.run("publish returns the previous value",           &tests_published_publish_returns_the_previous);
    rb.run("readers see the newly published value",        &tests_published_load_sees_the_new_value);
    rb.run("publishing null is legitimate",                &tests_published_publish_accepts_null);
    rb.run("compare_publish installs when expected",       &tests_published_compare_publish_succeeds_when_expected);
    rb.run("a losing CAS reports what it found",           &tests_published_compare_publish_fails_and_reports);
    rb.run("a published pointer is pinned",                &tests_published_is_pinned);
    rb.run("it costs exactly one atomic pointer",          &tests_published_costs_one_pointer);
    rb.run("store installs for initialization only",       &tests_published_store_installs_without_returning);
    rb.run("store is noexcept",                            &tests_published_store_is_noexcept);
    rb.run("it pairs with the retired list",               &tests_published_pairs_with_the_reclaim_list);

    return rb.finish();
}
