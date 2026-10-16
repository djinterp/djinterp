/*******************************************************************************
* djinterp [core]                                   sync_common_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  sync_common_tests_runner.cpp
//   Entry point for the sync_common.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "sync_common_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- sync_common.hpp Test Suite");
    rb.set_subtitle("sync : shared foundations -- ownership bases, an adaptive "
                    "spin backoff, and a scope-exit guard");
    rb.set_author("teer");
    rb.set_description(
        "sync_common.hpp holds the foundations the rest of the sync module was "
        "hand-rolling: the ownership bases (noncopyable, nonmovable) and their "
        "in-class macro forms, an adaptive spin backoff, and a scope guard. "
        "Every one of them is about a CONTRACT rather than a computation, so "
        "most of the suite is trait- and effect-based across four sections: the "
        "header contract, the ownership bases and macros, the spin backoff, and "
        "the scope guard. The sharp edges are pinned deliberately. The bases and "
        "the macros must MATCH: noncopyable keeps move, and since a bare deleted "
        "copy constructor would SUPPRESS the implicit move, the D_NONCOPYABLE "
        "macro defaults the move members explicitly -- the suite holds base and "
        "macro against each other in both directions, because a silent "
        "divergence there would quietly pin types that meant to stay movable. "
        "The bases are usable only AS bases (their special members are "
        "protected, so neither is a standalone object) and they are empty, so "
        "inheriting one must not grow the derived type -- an empty-base-"
        "optimization size test guards that, since a policy meant to be pure "
        "documentation must not inflate objects that sit in hot data structures. "
        "backoff is a pure WAIT, never a synchronization primitive: it holds "
        "only a step counter, so it is deliberately copyable where the rest of "
        "the module is pinned, its operations are noexcept, and its escalation "
        "is BOUNDED -- past a small cap it yields rather than spinning longer, "
        "which the tests check with real deadlines because an unbounded spin "
        "would hang every retry loop in the module rather than merely fail. The "
        "reset test is written as a relative comparison (reset vs. un-reset vs. "
        "fresh) rather than a wall-clock bound, because absolute spin timings "
        "are far too fast to assert on and a reset that quietly did nothing "
        "would slip straight past a deadline. scope_guard must fire EXACTLY "
        "once: moving it transfers the duty rather than duplicating it (the "
        "moved-from guard is disarmed), dismiss() cancels it entirely, and "
        "nested guards unwind LIFO -- so every test counts invocations rather "
        "than merely observing that something happened. Verified under C++11, "
        "C++14, C++17 and C++20; AddressSanitizer-, LeakSanitizer- and "
        "UndefinedBehaviorSanitizer-clean.");

    rb.use_pdf("sync_common_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "guard, idempotency, standing first, namespace, and gating");
    rb.run("the include guard is defined, and is 1",     &tests_header_include_guard);
    rb.run("including it twice is a no-op",              &tests_header_is_idempotent);
    rb.run("it can stand first in a TU",                 &tests_header_can_stand_first);
    rb.run("the entities live flat in djinterp",         &tests_entities_live_in_the_djinterp_namespace);
    rb.run("its type vocabulary is types, not macros",   &tests_header_defines_no_stray_type_macros);
    rb.run("it publishes its two ownership macros",      &tests_header_publishes_its_ownership_macros);
    rb.run("the utilities are gated to C++11",           &tests_header_is_gated_to_cpp11);

    // -- I. ownership bases and macros -------------------------------------
    rb.module("I. ownership bases and macros",
              "noncopyable keeps move, nonmovable pins; the macros must agree");
    rb.run("noncopyable blocks copying",                 &tests_noncopyable_blocks_copying);
    rb.run("noncopyable PRESERVES moving",               &tests_noncopyable_preserves_moving);
    rb.run("nonmovable blocks copying",                  &tests_nonmovable_blocks_copying);
    rb.run("nonmovable blocks moving too",               &tests_nonmovable_blocks_moving);
    rb.run("D_NONCOPYABLE matches the base exactly",     &tests_macro_noncopyable_matches_the_base);
    rb.run("D_NONMOVABLE matches the base exactly",      &tests_macro_nonmovable_matches_the_base);
    rb.run("the bases are empty and cost nothing",       &tests_bases_are_empty_and_cost_nothing);
    rb.run("the bases are not standalone objects",       &tests_bases_are_not_standalone_objects);
    rb.run("derived types stay constructible",           &tests_derived_types_stay_constructible);
    rb.run("derived types keep their destructor",        &tests_derived_types_keep_their_destructor);

    // -- II. spin backoff ---------------------------------------------------
    rb.module("II. spin backoff",
              "cpu_relax and the bounded, adaptive wait built on it");
    rb.run("cpu_relax is callable and noexcept",         &tests_cpu_relax_is_callable_and_noexcept);
    rb.run("a fresh backoff starts at the shortest spin", &tests_backoff_starts_at_the_shortest_spin);
    rb.run("pause and reset are noexcept",               &tests_backoff_pause_is_noexcept);
    rb.run("the escalation is bounded (no hang)",        &tests_backoff_escalates_without_hanging);
    rb.run("reset returns to the cheapest step",         &tests_backoff_reset_returns_to_the_start);
    rb.run("a backoff is a copyable value",              &tests_backoff_is_copyable);
    rb.run("it costs only a counter",                    &tests_backoff_costs_only_a_counter);
    rb.run("it survives a very long spin loop",          &tests_backoff_survives_a_long_spin_loop);

    // -- III. scope guard ---------------------------------------------------
    rb.module("III. scope guard",
              "run exactly once on scope exit, unless dismissed or moved away");
    rb.run("the action runs on scope exit",              &tests_scope_guard_runs_on_scope_exit);
    rb.run("the action runs exactly once",               &tests_scope_guard_runs_exactly_once);
    rb.run("dismiss cancels the action",                 &tests_scope_guard_dismiss_cancels_the_action);
    rb.run("moving transfers the duty, not a copy",      &tests_scope_guard_moving_transfers_the_duty);
    rb.run("a guard is not copyable",                    &tests_scope_guard_is_not_copyable);
    rb.run("nested guards unwind in reverse order",      &tests_scope_guard_nests_in_reverse_order);
    rb.run("make_scope_guard deduces the callable",      &tests_make_scope_guard_deduces_the_callable);
    rb.run("it accepts any function object",             &tests_scope_guard_accepts_a_function_object);
    rb.run("dismiss is noexcept",                        &tests_scope_guard_dismiss_is_noexcept);

    return rb.finish();
}
