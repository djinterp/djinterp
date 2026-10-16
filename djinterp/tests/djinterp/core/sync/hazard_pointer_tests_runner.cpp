/*******************************************************************************
* djinterp [core]                                hazard_pointer_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  hazard_pointer_tests_runner.cpp
//   Entry point for the hazard_pointer.hpp suite: registers every section's
//   tests with report_builder, one module per section.

// djinterp
#include "hazard_pointer_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- hazard_pointer.hpp Test Suite");
    rb.set_subtitle("sync : hazard-pointer memory reclamation -- records, "
                    "domains, retired lists, and the RAII guards over them");
    rb.set_author("teer");
    rb.set_description(
        "A hazard pointer is a reader's public promise -- 'I am about to "
        "dereference this node; do not free it' -- that a writer's reclamation "
        "scan must honour. The one property the whole file exists to provide is "
        "therefore a NEGATIVE one, and it is the spine of this suite: a node "
        "that some active hazard record protects is NOT reclaimed by a scan, no "
        "matter that it has been retired (tests_scan_spares_a_protected_node, "
        "and its two-thread form tests_a_protected_node_survives_a_concurrent_"
        "scan). Break that and every lock-free structure built on the header is "
        "a use-after-free generator. Reclamation is invisible from a value, so "
        "the suite instruments the payload: a counted_node raises a live counter "
        "when built and a deletion counter when freed, and the tests assert on "
        "those counts -- a leak shows up as a non-zero live count, a double free "
        "as a live count gone negative. The suite pins several sharp edges the "
        "header does not spell out. protect_and_validate() on a NULL source "
        "returns null via SUCCESS (it loads, publishes, and re-reads null, all "
        "matching), so the caller must read that null as 'nothing to protect', "
        "not 'validation failed'. With zero retries it runs the loop zero times "
        "and returns null without publishing anything, while still clearing any "
        "prior protection. A capacity-0 domain allocates a zero-length record "
        "array, so acquire() always fails AND should_scan() is true from the "
        "start (its threshold is 2*0 == 0). Acquire exhaustion is silent -- the "
        "(capacity+1)-th caller gets null, and every guard turns that into "
        "valid()==false with protect/clear as safe no-ops. The header's own "
        "'include env.h first' precondition is a LIVE #error, honestly "
        "diagnosed, so the test header includes the core first. Under C++23 with "
        "<hazard_pointer> present the header delegates to the standard and "
        "defines none of the custom types, so every section degrades to a stated "
        "no-op there. The concurrency section drives real threads through a "
        "deterministic rendezvous -- readers that genuinely overlap a scanner, "
        "never a race that happens to pass -- and is ThreadSanitizer-clean. "
        "Verified under C++11, C++14, C++17 and C++20.");

    rb.use_pdf("hazard_pointer_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, the live env precondition, the C++11 gate, the C++23 fork");
    rb.run("the include guard is defined, and is 1",     &tests_header_include_guard);
    rb.run("including it twice is a no-op",              &tests_header_is_idempotent);
    rb.run("it requires env.h first (a LIVE #error)",    &tests_header_requires_the_environment_first);
    rb.run("its one djinterp dependency is in",          &tests_header_pulls_its_dependency);
    rb.run("it supplies <atomic>, <cstdint>, <vector>",  &tests_header_provides_the_std_facilities);
    rb.run("the whole header is gated to C++11",         &tests_header_is_gated_to_cpp11);
    rb.run("custom types vs std delegation, coherently", &tests_which_implementation_is_in_play);
    rb.run("the types live flat in djinterp",            &tests_entities_live_in_the_djinterp_namespace);
    rb.run("no stray macros (one intended)",             &tests_header_defines_no_stray_macros);
    rb.run("the std delegation aliases exist when picked", &tests_the_std_delegation_aliases_exist_when_selected);

    // -- II. hazard_record -------------------------------------------------
    rb.module("II. hazard_record",
              "one atomic slot for one protected pointer");
    rb.run("a fresh record is inactive and null",        &tests_record_default_is_inactive_and_null);
    rb.run("it is exactly two atomics",                  &tests_record_holds_two_atomics);
    rb.run("the protected slot is a void*",              &tests_record_pointer_is_a_void_pointer);
    rb.run("a record is not copyable",                   &tests_record_is_not_copyable);

    // -- III. hazard_domain -- records -------------------------------------
    rb.module("III. hazard_domain -- records",
              "claiming and returning slots; the strategy aliases");
    rb.run("it reports its capacity (default 64)",       &tests_domain_reports_its_capacity);
    rb.run("a domain is not copyable",                   &tests_domain_is_not_copyable);
    rb.run("it declares the hazard strategy tag",        &tests_domain_declares_the_hazard_strategy_tag);
    rb.run("it self-marks with hazard_domain_type",      &tests_domain_marks_itself_with_hazard_domain_type);
    rb.run("acquire hands out a record",                 &tests_acquire_hands_out_a_record);
    rb.run("acquire hands out DISTINCT records",         &tests_acquire_hands_out_distinct_records);
    rb.run("acquire fills the pool then returns null",   &tests_acquire_fills_the_pool_then_returns_null);
    rb.run("release returns a record to the pool",       &tests_release_returns_a_record_to_the_pool);
    rb.run("release clears the protected pointer",       &tests_release_clears_the_protected_pointer);
    rb.run("release of null is a no-op",                 &tests_release_of_null_is_a_no_op);
    rb.run("a reacquired record carries no stale pin",  &tests_a_reacquired_record_is_clean);
    rb.run("a zero-capacity domain acquires nothing",    &tests_a_zero_capacity_domain_acquires_nothing);

    // -- III. hazard_domain -- scanning ------------------------------------
    rb.module("III. hazard_domain -- scanning and the retire counter",
              "is_protected / collect_protected, and the should_scan heuristic");
    rb.run("is_protected sees a published pointer",      &tests_is_protected_sees_a_published_pointer);
    rb.run("is_protected is false when unprotected",     &tests_is_protected_is_false_for_an_unprotected_pointer);
    rb.run("is_protected of null is false",              &tests_is_protected_of_null_is_false);
    rb.run("is_protected ignores an INACTIVE record",    &tests_is_protected_ignores_an_inactive_record);
    rb.run("collect gathers the protected pointers",     &tests_collect_gathers_the_protected_pointers);
    rb.run("collect clears its output first",            &tests_collect_clears_its_output_first);
    rb.run("collect skips null and inactive slots",      &tests_collect_skips_null_and_inactive_slots);
    rb.run("the retire counter starts at zero",          &tests_retired_count_starts_at_zero);
    rb.run("increment and decrement retired",            &tests_increment_and_decrement_retired);
    rb.run("decrement takes a batch count",              &tests_decrement_retired_takes_a_count);
    rb.run("should_scan crosses at 2x capacity",         &tests_should_scan_crosses_at_twice_capacity);
    rb.run("a zero-cap domain ALWAYS wants to scan",     &tests_a_zero_capacity_domain_always_wants_to_scan);

    // -- IV. scoped_hazard -------------------------------------------------
    rb.module("IV. scoped_hazard",
              "RAII around a record; and the exhaustion no-op path");
    rb.run("it acquires on construction",                &tests_scoped_acquires_on_construction);
    rb.run("it releases on destruction",                 &tests_scoped_releases_on_destruction);
    rb.run("protect publishes",                          &tests_scoped_protect_publishes);
    rb.run("clear unpublishes (keeps the record)",       &tests_scoped_clear_unpublishes);
    rb.run("a guard is not copyable",                    &tests_scoped_is_not_copyable);
    rb.run("it survives pool exhaustion (invalid)",      &tests_scoped_hazard_survives_exhaustion);
    rb.run("protect while invalid is a safe no-op",      &tests_scoped_protect_while_invalid_is_a_no_op);

    // -- V. typed_hazard_ptr -----------------------------------------------
    rb.module("V. typed_hazard_ptr",
              "the protect-then-verify loop, with its null-source and zero-retry edges");
    rb.run("it acquires a record",                       &tests_typed_acquires_a_record);
    rb.run("it is not copyable",                         &tests_typed_is_not_copyable);
    rb.run("validate on a stable source succeeds",       &tests_validate_on_a_stable_source_succeeds);
    rb.run("validate PUBLISHES the validated pointer",   &tests_validate_publishes_the_validated_pointer);
    rb.run("validate on a NULL source returns null",     &tests_validate_on_a_null_source_returns_null);
    rb.run("validate with ZERO retries does nothing",    &tests_validate_with_zero_retries_does_nothing);
    rb.run("validate clears protection when it gives up", &tests_validate_clears_protection_when_it_gives_up);
    rb.run("protect publishes directly",                 &tests_typed_protect_publishes_directly);
    rb.run("clear unpublishes",                          &tests_typed_clear_unpublishes);
    rb.run("on an exhausted domain it is invalid",       &tests_typed_on_an_exhausted_domain_is_invalid);

    // -- VI. retired_list --------------------------------------------------
    rb.module("VI. retired_list",
              "retire and scan -- where the safety property lands");
    rb.run("a fresh list is empty",                      &tests_retired_list_starts_empty);
    rb.run("retire queues a node (does not free it)",    &tests_retire_queues_a_node);
    rb.run("scan reclaims an unprotected node",          &tests_scan_reclaims_an_unprotected_node);
    rb.run("scan SPARES a protected node (THE property)", &tests_scan_spares_a_protected_node);
    rb.run("scan compacts the survivors by identity",    &tests_scan_compacts_the_survivors);
    rb.run("scan returns the reclaimed count",           &tests_scan_returns_the_reclaimed_count);
    rb.run("scan honours a custom deleter",              &tests_scan_with_a_custom_deleter);
    rb.run("the default retire uses delete",             &tests_default_retire_deletes_the_node);
    rb.run("scan_by_epoch reclaims older entries",       &tests_scan_by_epoch_reclaims_older_entries);
    rb.run("scan_by_epoch boundary is exclusive",        &tests_scan_by_epoch_keeps_newer_entries);
    rb.run("clear runs every deleter unconditionally",   &tests_clear_runs_every_deleter);
    rb.run("pending and empty track the queue",          &tests_pending_and_empty_track_the_queue);
    rb.run("scan on an empty list is a no-op",           &tests_scan_on_an_empty_list_is_a_no_op);

    // -- VII. multi_hazard_domain ------------------------------------------
    rb.module("VII. multi_hazard_domain",
              "N contiguous slots per thread, claimed as a group");
    rb.run("it reports its geometry (groups x slots)",   &tests_multi_reports_its_geometry);
    rb.run("it declares the hazard strategy tag",        &tests_multi_declares_the_hazard_strategy_tag);
    rb.run("it self-marks with hazard_domain_type",      &tests_multi_marks_itself_with_hazard_domain_type);
    rb.run("it is not copyable",                         &tests_multi_is_not_copyable);
    rb.run("acquire_slot_group hands out a group",       &tests_acquire_group_hands_out_a_group);
    rb.run("a group is contiguous and fully active",     &tests_group_is_contiguous_and_fully_active);
    rb.run("group acquire fills then returns null",      &tests_acquire_group_fills_then_returns_null);
    rb.run("release_slot_group clears every slot",       &tests_release_group_clears_every_slot);
    rb.run("is_protected scans every slot",              &tests_multi_is_protected_scans_every_slot);
    rb.run("collect gathers across groups",              &tests_multi_collect_gathers_across_groups);
    rb.run("should_scan crosses at 2x TOTAL slots",      &tests_multi_should_scan_crosses_at_twice_total_slots);
    rb.run("slots_per_thread is the template argument",  &tests_multi_slots_per_thread_is_the_template_argument);

    // -- VIII. scoped_multi_hazard -----------------------------------------
    rb.module("VIII. scoped_multi_hazard",
              "RAII around a slot group; per-slot protect/clear");
    rb.run("it acquires a group",                        &tests_scoped_multi_acquires_a_group);
    rb.run("it releases on destruction",                 &tests_scoped_multi_releases_on_destruction);
    rb.run("protect publishes into a slot",              &tests_scoped_multi_protects_a_slot);
    rb.run("clear(slot) drops one, keeps the others",    &tests_scoped_multi_clears_a_slot);
    rb.run("clear_all drops every slot",                 &tests_scoped_multi_clear_all);
    rb.run("a guard is not copyable",                    &tests_scoped_multi_is_not_copyable);
    rb.run("an out-of-range slot is a safe no-op",       &tests_scoped_multi_out_of_range_slot_is_a_no_op);
    rb.run("it survives group-pool exhaustion",          &tests_scoped_multi_survives_exhaustion);

    // -- Concurrency -------------------------------------------------------
    rb.module("Concurrency",
              "the safety property under real threads -- deterministic, TSan-clean");
    rb.run("concurrent acquire hands out distinct records", &tests_concurrent_acquire_hands_out_distinct_records);
    rb.run("a protected node survives a concurrent scan",   &tests_a_protected_node_survives_a_concurrent_scan);
    rb.run("validation is sound under a churning source",   &tests_validation_under_a_churning_source_is_sound);
    rb.run("concurrent protect and scan leak nothing",      &tests_concurrent_protect_and_scan_leak_nothing);
    rb.run("group acquisition is atomic under contention",  &tests_group_acquisition_is_atomic_under_contention);

    return rb.finish();
}
