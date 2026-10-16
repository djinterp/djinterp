/*******************************************************************************
* djinterp [core]                                           rcu_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  rcu_tests_runner.cpp
//   Entry point for the rcu.hpp suite: registers every section's tests with
//   report_builder, one module per section.

// djinterp
#include "rcu_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- rcu.hpp Test Suite");
    rb.set_subtitle("sync : epoch-based reclamation -- an epoch counter, an RAII "
                    "reader guard, a slot registry, a deferred reclaimer, and a "
                    "complete rcu_protected value");
    rb.set_author("teer");
    rb.set_description(
        "rcu.hpp provides Read-Copy-Update primitives: the building blocks for "
        "lock-free containers that use read-side critical sections and DEFERRED "
        "memory reclamation. Readers mark a critical section by recording the "
        "current epoch; writers copy-modify-swap the data pointer and retire the "
        "old copy, which is reclaimed only once every reader from the retiring "
        "epoch has exited. The suite tests the pieces in isolation and the "
        "reclamation PROTOCOL they combine into, across seven sections: the "
        "header contract, the epoch_counter, the epoch_guard, the epoch_registry, "
        "the deferred_reclaimer, the rcu_protected value, and a concurrency "
        "section that puts real threads on the safety and liveness invariants. "
        "The reclamation-safety tests are DETERMINISTIC: a reader pinned in an "
        "epoch holds a node back, and releasing it lets the next scan collect it "
        "-- no threads needed to show the rule -- while the concurrency section "
        "adds genuine contention and is validated under ThreadSanitizer and "
        "AddressSanitizer. Several sharp edges are pinned. EVERYTHING here is "
        "non-copyable AND non-movable: the counter and the registry slots hold "
        "atomics (which do not move); epoch_guard owns a reference and the "
        "registry a raw slot array, with special members declared to match; a "
        "reclamation engine that could be copied would double-free. rcu_read_guard "
        "is the subtle one -- it declares its move constructor '= default', which "
        "reads as movable, but that default is DELETED because its epoch_guard "
        "member is non-movable, so the guard is neither copyable nor movable. "
        "That carries a VERSION consequence, recorded rather than judged: because "
        "read_lock() returns the guard BY VALUE, and returning a non-movable type "
        "by value is well-formed only under C++17's guaranteed copy elision, "
        "read_lock() compiles from C++17 onward but NOT at C++11/14 -- even though "
        "the file's banner lists all custom types as available at C++11. So the "
        "suite floors at C++11 (rcu.hpp needs <atomic> and #errors below it), runs "
        "the counter / guard / registry / reclaimer and rcu_protected's "
        "construction / update / modify / queries from C++11, and gates the "
        "read-side tests to C++17+, degrading them to a stated no-op below it. "
        "advance() and retire() return / record the PREVIOUS epoch (fetch_add "
        "semantics), and the reclamation boundary is 'retire_epoch < "
        "min_active_epoch' -- strictly less -- so a reader in epoch E holds back "
        "everything retired at epoch >= E, while force_reclaim() bypasses the "
        "check entirely (shutdown only). rcu_protected holds a SINGLE reader slot, "
        "so its read API is single-reader by construction; the concurrent safety "
        "test therefore builds the RCU pattern from the primitives with a "
        "per-thread slot each, not through rcu_protected. The C++23 std::rcu "
        "delegation path is not exercised: this toolchain ships no <rcu>, so "
        "D_HAS_STD_RCU is 0 and the custom rcu_protected is what is under test. "
        "Like lock_policy.hpp and hazard_pointer.hpp, rcu.hpp keeps a LIVE "
        "env-first #error and does not include the core itself, so it cannot be "
        "the first djinterp header in a TU -- every section includes the core "
        "before it. Verified under C++11, C++14, C++17 and C++20; "
        "ThreadSanitizer- and AddressSanitizer-clean.");

    rb.use_pdf("rcu_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "guard, env-first #error, dependencies, gating, and two version notes");
    rb.run("the include guard is defined, and is 1",       &tests_header_include_guard);
    rb.run("including it twice is a no-op",                &tests_header_is_idempotent);
    rb.run("it requires the environment first (not first)", &tests_header_requires_environment_first);
    rb.run("it pulls in its dependencies (strategy tags)", &tests_header_pulls_in_dependencies);
    rb.run("the entities live flat in djinterp",           &tests_entities_live_in_the_djinterp_namespace);
    rb.run("no stray macros (D_HAS_STD_RCU aside)",        &tests_header_defines_no_stray_macros);
    rb.run("the types are gated to C++11",                 &tests_header_is_gated_to_cpp11);
    rb.run("std::rcu delegation is off on this build",     &tests_std_rcu_delegation_is_off_on_this_build);
    rb.run("read_lock is, in practice, C++17+",            &tests_read_lock_availability_is_cpp17);

    // -- II. the epoch counter ---------------------------------------------
    rb.module("II. epoch_counter",
              "the monotonic global epoch; advance() returns the PREVIOUS value");
    rb.run("a fresh counter starts at epoch 0",            &tests_epoch_counter_starts_at_zero);
    rb.run("advance() returns the previous epoch",         &tests_epoch_counter_advance_returns_previous);
    rb.run("advance() increments the current epoch",       &tests_epoch_counter_advance_increments);
    rb.run("inactive is UINT64_MAX",                       &tests_epoch_counter_inactive_is_uint64_max);
    rb.run("inactive is a constexpr",                      &tests_epoch_counter_inactive_is_constexpr);
    rb.run("the counter is not copyable",                  &tests_epoch_counter_is_not_copyable);
    rb.run("the counter is not movable",                   &tests_epoch_counter_is_not_movable);
    rb.run("its operations are noexcept",                  &tests_epoch_counter_operations_are_noexcept);

    // -- III. the epoch guard ----------------------------------------------
    rb.module("III. epoch_guard",
              "the RAII reader marker; records on entry, resets to inactive on exit");
    rb.run("it records the epoch on construction",         &tests_epoch_guard_records_epoch_on_construction);
    rb.run("it resets to inactive on destruction",         &tests_epoch_guard_resets_to_inactive_on_destruction);
    rb.run("it captures the epoch as of construction",     &tests_epoch_guard_captures_current_epoch);
    rb.run("the guard is not copyable",                    &tests_epoch_guard_is_not_copyable);
    rb.run("the guard is NOT movable (root of C++17 note)", &tests_epoch_guard_is_not_movable);
    rb.run("construction is noexcept",                     &tests_epoch_guard_construction_is_noexcept);
    rb.run("two slots are independent",                    &tests_epoch_guard_two_slots_are_independent);

    // -- IV. the epoch registry --------------------------------------------
    rb.module("IV. epoch_registry",
              "the per-thread slot pool; lock-free claim, min-active-epoch scan");
    rb.run("the default capacity is 64",                   &tests_registry_default_capacity_is_64);
    rb.run("a custom capacity is honored",                 &tests_registry_custom_capacity);
    rb.run("register_thread returns a slot",               &tests_registry_register_returns_a_slot);
    rb.run("a fresh slot is inactive",                     &tests_registry_fresh_slot_is_inactive);
    rb.run("it registers until full, then nullptr",        &tests_registry_registers_until_full);
    rb.run("unregister frees the slot for reuse",          &tests_registry_unregister_frees_the_slot);
    rb.run("unregister(nullptr) is safe",                  &tests_registry_unregister_null_is_safe);
    rb.run("min_active_epoch is inactive with no readers", &tests_registry_min_active_epoch_with_no_readers);
    rb.run("min_active_epoch is the minimum",              &tests_registry_min_active_epoch_is_the_minimum);
    rb.run("an idle slot does not lower the minimum",      &tests_registry_idle_slot_does_not_lower_the_minimum);
    rb.run("capacity zero is handled",                     &tests_registry_capacity_zero_is_handled);
    rb.run("the registry is not copyable or movable",      &tests_registry_is_not_copyable_or_movable);

    // -- V. the deferred reclaimer -----------------------------------------
    rb.module("V. deferred_reclaimer",
              "the reclamation engine; retire / scan / force_reclaim, boundary logic");
    rb.run("a fresh reclaimer is empty at epoch 0",        &tests_reclaimer_starts_empty);
    rb.run("retire advances the epoch",                    &tests_reclaimer_retire_advances_the_epoch);
    rb.run("retire adds to the pending list",              &tests_reclaimer_retire_adds_to_pending);
    rb.run("scan reclaims with no readers",                &tests_reclaimer_scan_reclaims_with_no_readers);
    rb.run("an active reader blocks reclamation",          &tests_reclaimer_active_reader_blocks_reclamation);
    rb.run("it reclaims after the reader exits",           &tests_reclaimer_reclaims_after_reader_exits);
    rb.run("a custom deleter is used",                     &tests_reclaimer_custom_deleter_is_used);
    rb.run("the default deleter destroys the object",      &tests_reclaimer_default_deleter_destroys_the_object);
    rb.run("force_reclaim ignores active readers",         &tests_reclaimer_force_reclaim_ignores_readers);
    rb.run("a scan fires automatically at the threshold",  &tests_reclaimer_auto_scan_fires_at_threshold);
    rb.run("scan returns the reclaimed count",             &tests_reclaimer_scan_returns_the_count);
    rb.run("scan compacts and keeps survivors in order",   &tests_reclaimer_scan_compacts_and_keeps_survivors);
    rb.run("the reclaimer is not copyable or movable",     &tests_reclaimer_is_not_copyable_or_movable);

    // -- VI. the rcu_protected value ---------------------------------------
    rb.module("VI. rcu_protected",
              "the complete value; read [C++17+] / update / modify, single-reader");
    rb.run("it constructs from an lvalue",                 &tests_protected_constructs_from_an_lvalue);
    rb.run("it constructs from an rvalue",                 &tests_protected_constructs_from_an_rvalue);
    rb.run("update advances the epoch",                    &tests_protected_update_advances_the_epoch);
    rb.run("update retires the old value",                 &tests_protected_update_retires_the_old_value);
    rb.run("modify advances the epoch and retires",        &tests_protected_modify_advances_and_retires);
    rb.run("successive updates accumulate pending",        &tests_protected_updates_accumulate_pending);
    rb.run("rcu_protected is not copyable",                &tests_protected_is_not_copyable);
    rb.run("its type aliases identify the strategy",       &tests_protected_type_aliases_identify_the_strategy);
    rb.run("the read guard is not copyable or movable",    &tests_protected_read_guard_is_not_copyable_or_movable);
    rb.run("read returns the current value [C++17+]",      &tests_protected_read_returns_the_current_value);
    rb.run("updates are visible to readers [C++17+]",      &tests_protected_updates_are_visible_to_readers);
    rb.run("a modify is visible to readers [C++17+]",      &tests_protected_modify_is_visible_to_readers);

    // -- Concurrency -------------------------------------------------------
    rb.module("Concurrency",
              "the protocol under threads: distinct slots, no UAF, and progress");
    rb.run("concurrent registration hands out distinct slots", &tests_registry_concurrent_registration_is_distinct);
    rb.run("concurrent registration respects capacity",    &tests_registry_concurrent_registration_respects_capacity);
    rb.run("readers never observe reclaimed data",         &tests_concurrent_readers_never_observe_reclaimed_data);
    rb.run("retire and scan make progress under readers",  &tests_concurrent_retire_and_scan_make_progress);

    return rb.finish();
}
