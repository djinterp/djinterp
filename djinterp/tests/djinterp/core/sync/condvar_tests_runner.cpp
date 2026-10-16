/*******************************************************************************
* djinterp [core]                                       condvar_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  condvar_tests_runner.cpp
//   Entry point for the condvar.hpp suite: registers every section's tests with
//   report_builder, one module per section.

// djinterp
#include "condvar_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- condvar.hpp Test Suite");
    rb.set_subtitle("sync : the yield, the core count, call-once, and the "
                    "policy-aware condition variable");
    rb.set_author("teer");
    rb.set_description(
        "condvar.hpp is four facilities and almost no code of its own, which is "
        "exactly why it needs testing: nearly every guarantee it offers is an "
        "INHERITED one, and an inherited guarantee is the kind people assume "
        "rather than read. The suite is therefore built around the things the "
        "header does not say. portable_once forwards to std::call_once, so it "
        "inherits three behaviours that surprise people and are pinned here as "
        "tests: the flag guards the FLAG and not the callable (a later call with "
        "a different callable does nothing at all); an exception does NOT consume "
        "the flag, so 'exactly once' means exactly one SUCCESSFUL completion and "
        "the next caller retries; and the arguments are FORWARDED, not "
        "decay-copied as std::thread's are, so a reference arrives as a reference. "
        "portable_condvar picks its condition variable from the policy's mutex "
        "type, and the three it can pick do NOT have the same surface -- so two "
        "of the wrapper's own methods are latent compile errors that depend on "
        "the policy you chose: wait_until under the null policy (no_op_condvar "
        "has none) and the C++20 stop_token wait under any std::mutex policy "
        "(std::condition_variable has none). A capability matrix pins both, and "
        "a third gap besides: the wrapper never forwards the standard's "
        "wait_until(lock, deadline, predicate). The no-op condvar gets a section "
        "of its own because what it does is not what its call sites read like: "
        "wait(lock, pred) returns at once and NEVER EVALUATES the predicate -- "
        "not once -- while wait_for(lock, dur, pred) calls it exactly once and "
        "returns it. The blocking tests are deterministic rather than "
        "probabilistic: a notification arriving while the predicate is false does "
        "not wake the waiter, a notification arriving with no waiter is LOST, and "
        "notify_one is only ever asserted to wake AT LEAST one -- a test that "
        "demanded exactly one would be asserting a race. Every waiter waits on a "
        "predicate carrying an abort arm, and every test ends in a rescue, so a "
        "broken wait is a reported FAILURE and never a hung runner. The harness "
        "is std::atomic and std::thread only: the framework's own latches and "
        "barriers are built ON portable_condvar, and a harness made from the "
        "thing under test could not have exposed it. Verified under C++11, C++14, "
        "C++17 and C++20.");

    rb.use_pdf("condvar_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, the include ORDER, what it hands you, what it leaves behind");
    rb.run("the include guard is defined, and is 1",   &tests_header_include_guard);
    rb.run("including it twice is a no-op",            &tests_header_is_idempotent);
    rb.run("its guards are lock_policy's, not its own", &tests_header_inherits_its_guards_from_lock_policy);
    rb.run("djinterp.hpp + lock_policy.hpp, in order", &tests_header_pulls_its_dependencies);
    rb.run("it supplies <mutex> and <condition_variable>", &tests_header_provides_the_std_facilities);
    rb.run("no stray macros (but WIN32_LEAN_AND_MEAN)", &tests_header_defines_no_stray_macros);
    rb.run("public in djinterp, details in internal",  &tests_entities_live_in_the_djinterp_namespace);
    rb.run("sections III and IV are gated to C++11",   &tests_sections_three_and_four_are_gated_to_cpp11);

    // -- I. the thread yield -----------------------------------------------
    rb.module("I. d_thread_yield",
              "the hint a spinloop gives the scheduler when spinning stops paying");
    rb.run("a nullary void function",                  &tests_yield_is_a_nullary_void_function);
    rb.run("callable, a thousand times over",          &tests_yield_is_callable);
    rb.run("it never throws",                          &tests_yield_does_not_throw);
    rb.run("...but is NOT declared noexcept",          &tests_yield_is_not_declared_noexcept);
    rb.run("safe from four threads at once",           &tests_yield_from_many_threads);
    rb.run("a yielding spin loop MAKES PROGRESS",      &tests_yield_in_a_spin_loop_makes_progress);
    rb.run("one definition, one address, all TUs",     &tests_yield_has_one_definition_across_tus);

    // -- II. hardware concurrency ------------------------------------------
    rb.module("II. hardware_concurrency",
              "how many agents the machine can really run -- and the zero it may answer");
    rb.run("a nullary unsigned function",              &tests_hardware_returns_unsigned);
    rb.run("the same answer every time",               &tests_hardware_is_deterministic);
    rb.run("ZERO is legal: the caller must clamp",     &tests_hardware_may_return_zero);
    rb.run("a non-zero answer is plausible",           &tests_hardware_is_plausible);
    rb.run("it IS std::thread's query, exactly",       &tests_hardware_matches_the_standard);
    rb.run("it never throws (and does not say so)",    &tests_hardware_does_not_throw);
    rb.run("four threads, one answer",                 &tests_hardware_is_callable_from_many_threads);
    rb.run("one definition, one address, all TUs",     &tests_hardware_has_one_definition_across_tus);

    // -- III. portable_once ------------------------------------------------
    rb.module("III. portable_once",
              "run this exactly once -- and what 'exactly once' turns out to mean");
    rb.run("default-constructible, and unfired",       &tests_once_is_default_constructible);
    rb.run("not copyable (the flag is the identity)",  &tests_once_is_not_copyable);
    rb.run("...and NOT MOVABLE either",                &tests_once_is_not_movable);
    rb.run("it holds one flag and nothing else",       &tests_once_holds_one_flag_and_nothing_else);
    rb.run("the callable runs",                        &tests_once_runs_the_callable);
    rb.run("five calls, one invocation",               &tests_once_runs_it_only_once);
    rb.run("a LATER callable is ignored entirely",     &tests_once_ignores_a_later_callable);
    rb.run("exactly once under contention",            &tests_once_runs_exactly_once_under_contention);
    rb.run("every caller sees the winner's writes",    &tests_once_publishes_the_result_to_every_caller);
    rb.run("arguments are forwarded",                  &tests_once_forwards_its_arguments);
    rb.run("...BY REFERENCE (no decay copy)",          &tests_once_forwards_by_reference);
    rb.run("lambda, function, functor, member fn",     &tests_once_accepts_any_callable);
    rb.run("an exception propagates out of call()",    &tests_once_propagates_an_exception);
    rb.run("...and does NOT consume the flag",         &tests_once_retries_after_an_exception);
    rb.run("two flags, two gates",                     &tests_once_instances_are_independent);

    // -- IV. the condvar selector ------------------------------------------
    rb.module("IV. the condvar selector",
              "which condition variable a policy's mutex gets -- and what it can then do");
    rb.run("std::mutex -> condition_variable",         &tests_selector_maps_std_mutex_to_condition_variable);
    rb.run("no_op_mutex -> the no-op condvar",         &tests_selector_maps_no_op_mutex_to_the_no_op_condvar);
    rb.run("everything else -> condition_variable_any", &tests_selector_falls_back_to_condition_variable_any);
    rb.run("the shared mutexes take the any",          &tests_selector_maps_the_shared_mutexes);
    rb.run("the primary accepts ANY type (unchecked)", &tests_selector_primary_accepts_any_type);
    rb.run("every policy selects its condvar",         &tests_every_policy_selects_its_condvar);
    rb.run("all three can notify (noexcept)",          &tests_every_condvar_can_notify);
    rb.run("all three can wait",                       &tests_every_condvar_can_wait);
    rb.run("all three can wait_for",                   &tests_every_condvar_can_wait_for);
    rb.run("only the REAL ones have wait_until",       &tests_only_the_real_condvars_have_wait_until);
    rb.run("only the ANY one takes a stop_token",      &tests_only_condition_variable_any_takes_a_stop_token);

    // -- IV. the no-op condvar ---------------------------------------------
    rb.module("IV. the no-op condvar",
              "the single-threaded condvar: free, and not what its call sites read like");
    rb.run("an empty class, one byte",                 &tests_no_op_condvar_is_empty);
    rb.run("notify is a noexcept no-op",               &tests_no_op_notify_is_a_noexcept_no_op);
    rb.run("wait() returns immediately",               &tests_no_op_wait_returns_immediately);
    rb.run("...and NEVER TOUCHES the lock",            &tests_no_op_wait_never_touches_the_lock);
    rb.run("wait(lock, pred) IGNORES the predicate",   &tests_no_op_wait_ignores_its_predicate);
    rb.run("wait_for CLAIMS it was notified",          &tests_no_op_wait_for_claims_no_timeout);
    rb.run("...and does not sleep at all",             &tests_no_op_wait_for_does_not_sleep);
    rb.run("but wait_for(pred) DOES ask, once",        &tests_no_op_wait_for_with_a_predicate_returns_it);
    rb.run("it is an internal detail",                 &tests_no_op_condvar_lives_in_internal);
    rb.run("the null policy's condvar is FREE",        &tests_the_null_policy_condvar_is_free);

    // -- IV. portable_condvar's surface ------------------------------------
    rb.module("IV. portable_condvar's surface",
              "what the wrapper IS, before anything blocks on it");
    rb.run("default-constructible under every policy", &tests_condvar_is_default_constructible);
    rb.run("not copyable",                             &tests_condvar_is_not_copyable);
    rb.run("...and NOT MOVABLE either",                &tests_condvar_is_not_movable);
    rb.run("condvar_type is public, and correct",      &tests_condvar_exposes_its_condvar_type);
    rb.run("notify_one / notify_all are noexcept",     &tests_condvar_notify_is_noexcept);
    rb.run("native() hands back the real condvar",     &tests_condvar_native_returns_the_underlying);
    rb.run("...and has a const overload",              &tests_condvar_native_has_a_const_overload);
    rb.run("it costs exactly what it wraps",           &tests_condvar_costs_exactly_its_condvar);
    rb.run("no vtable",                                &tests_condvar_has_no_vtable);
    rb.run("notifying nobody is harmless",             &tests_condvar_notify_without_waiters_is_harmless);
    rb.run("two condvars, two queues",                 &tests_condvar_instances_are_independent);

    // -- IV. wait + notify -------------------------------------------------
    rb.module("IV. wait + notify",
              "what actually happens when a thread blocks -- deterministically");
    rb.run("a bare wait blocks until notified",        &tests_wait_blocks_until_notified);
    rb.run("a blocked waiter RELEASES the mutex",      &tests_wait_releases_the_mutex_while_blocked);
    rb.run("...and takes it back before returning",    &tests_wait_reacquires_the_mutex_before_returning);
    rb.run("the predicate wait returns when it holds", &tests_wait_with_a_predicate_returns_when_it_holds);
    rb.run("an ALREADY-TRUE predicate never blocks",   &tests_wait_with_a_true_predicate_never_blocks);
    rb.run("a bare notify does NOT wake it",           &tests_wait_ignores_a_notification_while_the_predicate_is_false);
    rb.run("notify_one wakes the waiter",              &tests_notify_one_wakes_a_waiter);
    rb.run("notify_all wakes all four",                &tests_notify_all_wakes_every_waiter);
    rb.run("notify_one wakes AT LEAST one of four",    &tests_notify_one_wakes_at_least_one_of_many);
    rb.run("a notification with no waiter is LOST",    &tests_a_notification_with_no_waiter_is_lost);

    // -- IV. timed waits ---------------------------------------------------
    rb.module("IV. timed waits",
              "wait_for and wait_until: the deadline, and the three edges around it");
    rb.run("wait_for times out (and really waits)",    &tests_wait_for_times_out);
    rb.run("...and reports no_timeout when notified",  &tests_wait_for_returns_no_timeout_when_notified);
    rb.run("the predicate form: false on timeout",     &tests_wait_for_with_a_predicate_returns_false_on_timeout);
    rb.run("...and true when it comes to hold",        &tests_wait_for_with_a_predicate_returns_true_when_satisfied);
    rb.run("an already-true predicate returns at once", &tests_wait_for_with_a_true_predicate_returns_at_once);
    rb.run("a ZERO duration still checks, and expires", &tests_wait_for_with_a_zero_duration_times_out);
    rb.run("a bare notify does not end it early",      &tests_wait_for_keeps_waiting_while_the_predicate_is_false);
    rb.run("wait_until times out",                     &tests_wait_until_times_out);
    rb.run("a PAST deadline returns at once",          &tests_wait_until_with_a_past_deadline_returns_at_once);
    rb.run("...and no_timeout when notified",          &tests_wait_until_returns_no_timeout_when_notified);
    rb.run("the condition_variable_any path times out", &tests_wait_for_on_the_condition_variable_any_path);
    rb.run("wait_until(lock, tp, pred) is NOT forwarded", &tests_wait_until_has_no_predicate_overload);

    // -- IV. the lock policies ---------------------------------------------
    rb.module("IV. the lock policies",
              "every policy, with its own lock type -- and the C++20 stop_token");
    rb.run("the null policy cannot block",             &tests_the_null_policy_never_blocks);
    rb.run("exclusive: mutex + condition_variable",    &tests_the_exclusive_policy_handshake);
    rb.run("timed: timed_mutex + the any condvar",     &tests_the_timed_policy_handshake);
    rb.run("shared: a READER waits, a writer wakes it", &tests_the_shared_policy_wakes_a_reader);
    rb.run("shared_timed: the same, with a deadline",  &tests_the_shared_timed_policy_handshake);
    rb.run("the default policy, whatever it is today", &tests_the_default_policy_works);
    rb.run("every policy yields a usable condvar",     &tests_every_policy_yields_a_usable_condvar);
    rb.run("stop_token: cancelled -> the predicate",   &tests_the_stop_token_wait_returns_the_predicate);
    rb.run("stop_token: satisfied -> true",            &tests_the_stop_token_wait_returns_true_when_satisfied);
    rb.run("a stop ALREADY requested is not missed",   &tests_the_stop_token_wait_returns_at_once_if_already_stopped);

    return rb.finish();
}
