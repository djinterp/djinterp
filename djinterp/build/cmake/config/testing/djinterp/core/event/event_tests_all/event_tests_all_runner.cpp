/******************************************************************************
* djinterp [test]                                          event_tests_runner.cpp
*
*   Aggregate runner for the entire event subsystem.  Builds a single report
* with one module per header-suite -- event_common, event_handler, event_table,
* event_registry, event_dispatcher -- and runs every test of each in the order
* its own runner uses.  One console summary and one PDF cover all five suites.
*
*   This runner intentionally includes no suite test header.  Each suite's
* header defines its fixtures (event tags, handler functors, a check helper)
* flat in djinterp::testing, and several of those names are shared across
* suites (e.g. ev_int, summing); pulling more than one header into this single
* translation unit would redefine them.  Instead the test entry points -- which
* are ordinary external-linkage functions, uniquely named across all suites --
* are forward-declared here and resolved at link time against the per-suite
* section objects.  (The suites' shared fixtures are token-identical, so the
* multi-object link is well-formed.)
*
*   To run a single suite in isolation, use that suite's own runner instead;
* this one is the union.
*
*
* path:      /build/cmake/config/testing/djinterp/core/event/event_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/test/output/test_report_runner.hpp>     // report_builder + report model
// NOTE: the five suite test headers are intentionally NOT included here.  Each
// defines its fixtures (ev_int, ev_empty, summing, ...) flat in
// djinterp::testing, so pulling more than one into this single TU redefines
// them.  The entry points are forward-declared below and resolved at link time
// against the per-suite section objects.


// ----------------------------------------------------------------------------
// forward declarations -- every event-suite test entry point, defined in the
// per-suite section translation units and linked in.  Grouped by suite, in the
// order each suite's runner reports them.
// ----------------------------------------------------------------------------
NS_DJINTERP
NS_TESTING

// ---- event_common ----
bool tests_verdict_enumerators();
bool tests_verdict_type_properties();
bool tests_consumed_values();
bool tests_consumed_consistency();
bool tests_has_payload_type();
bool tests_has_args_type();
bool tests_has_event_payload();
bool tests_event_payload_select();
bool tests_has_event_name();
bool tests_is_tuple();
bool tests_index_sequence();
bool tests_make_index_sequence();
bool tests_apply_impl_direct();
bool tests_apply_tuple_arities();
bool tests_apply_tuple_values();
bool tests_event_traits_payload_type();
bool tests_event_traits_args_alias();
bool tests_event_traits_arity();
bool tests_event_traits_has_name();
bool tests_event_traits_has_args();
bool tests_event_traits_legacy_and_both();
bool tests_d_event_payload();
bool tests_d_event_name();
bool tests_d_event_empty();
bool tests_d_event_arity_range();
bool tests_d_event_is_event();
bool tests_concept_is_event();
bool tests_concept_event_type();
bool tests_concept_non_event_type();
bool tests_concept_empty_event_type();
bool tests_concept_argument_event_type();
bool tests_concept_named_event_type();
bool tests_concept_unnamed_event_type();
bool tests_concept_event_of_arity();
bool tests_concept_nullary_unary();
bool tests_concept_binary_ternary();
bool tests_concept_variadic_event_type();

// ---- event_handler ----
bool tests_handler_id_relational();
bool tests_handler_id_validity();
bool tests_handler_id_null();
bool tests_handler_id_value_semantics();
bool tests_invoke_normalized_void();
bool tests_invoke_normalized_verdict();
bool tests_apply_handler_arities();
bool tests_apply_handler_void_normalization();
bool tests_apply_handler_forwards_values();
bool tests_handler_traits_is_invocable();
bool tests_handler_traits_return_type();
bool tests_handler_traits_returns_void_verdict();
bool tests_handler_traits_is_compatible();
bool tests_handler_traits_is_nothrow();
bool tests_handler_traits_expected_arity();
bool tests_handler_traits_cvref();
bool tests_skip_always_pass();
bool tests_seq_pass_pass();
bool tests_seq_left_zero();
bool tests_seq_void_normalization();
bool tests_seq_associativity();
bool tests_seq_lvalue_passthrough();
bool tests_seq_clean_type();
bool tests_concept_is_handler();
bool tests_concept_handler_for();
bool tests_concept_void_verdict_handler_for();
bool tests_concept_nothrow_handlers();
bool tests_concept_handler_for_event_of_arity();
bool tests_concept_arity_aliases();
bool tests_concept_non_event_safety();

// ---- event_table ----
bool tests_type_key_stable_unique();
bool tests_handler_entry_fields();
bool tests_table_default_empty();
bool tests_insert_ids_increasing();
bool tests_insert_counts_enabled_contains();
bool tests_remove_basic();
bool tests_remove_disabled_bookkeeping();
bool tests_id_non_reuse();
bool tests_enable_disable_flips_and_counts();
bool tests_enable_disable_idempotent_and_missing();
bool tests_is_enabled_states();
bool tests_contains_states();
bool tests_entries_for_missing_null();
bool tests_entries_for_present_contents();
bool tests_has_entries_for();
bool tests_count_for();
bool tests_type_key_count();
bool tests_for_each_entry();
bool tests_for_each_entry_for();
bool tests_merge_into_empty();
bool tests_merge_order_and_fresh_ids();
bool tests_merge_preserves_enabled();
bool tests_merge_identity_and_src_untouched();
bool tests_clear();
bool tests_clear_for();
bool tests_clear_for_missing_noop();
bool tests_clear_for_mixed_enabled();
bool tests_stats_struct_aggregate();
bool tests_stats_empty_table();
bool tests_stats_populated();
bool tests_stats_after_clear();
bool tests_detection_core_mutation();
bool tests_detection_const_query();
bool tests_detection_extended();
bool tests_traits_facade_real();
bool tests_traits_facade_structural();
bool tests_traits_facade_negative();
bool tests_traits_facade_cvref();
bool tests_concept_is_event_table_type();
bool tests_concept_readable_and_non();
bool tests_concept_clearable_counting();
bool tests_concept_optional_features();
bool tests_concept_extended();
bool tests_concept_derived_false_on_non_table();

// ---- event_registry ----
bool tests_dispatch_result_fields_and_consumed();
bool tests_run_result_aggregate();
bool tests_fused_step_empty();
bool tests_fused_step_fold_order_and_size();
bool tests_fused_step_short_circuit();
bool tests_fused_step_operator_and_run_one();
bool tests_bind_returns_valid_id_and_registers();
bool tests_bind_distinct_ids();
bool tests_dispatch_no_handlers();
bool tests_dispatch_single_pass();
bool tests_dispatch_single_consume();
bool tests_dispatch_order_and_count();
bool tests_dispatch_consume_short_circuit();
bool tests_dispatch_disabled_masked();
bool tests_dispatch_payload_delivery();
bool tests_dispatch_void_handler_and_nullary();
bool tests_dispatch_multi_event_isolation_and_cvref();
bool tests_run_empty_trace();
bool tests_run_counts_pass();
bool tests_run_consumed_count();
bool tests_run_no_handlers();
bool tests_run_short_circuit_accumulation();
bool tests_compile_empty();
bool tests_compile_size_and_fold();
bool tests_compile_masks_disabled();
bool tests_compile_snapshot_independent();
bool tests_unbind();
bool tests_enable_disable();
bool tests_is_enabled_contains();
bool tests_merge();
bool tests_typed_queries();
bool tests_aggregate_queries();
bool tests_table_access();

// ---- event_dispatcher ----
bool tests_queue_enqueue_pending_empty_clear();
bool tests_queue_process_order_and_count();
bool tests_queue_process_partial_and_remainder();
bool tests_queue_process_max_clamped_and_all();
bool tests_queue_process_empty();
bool tests_queue_delivery_time_binding();
bool tests_queue_payload_captured_by_value();
bool tests_queue_reentrant_enqueue_deferred();
bool tests_queue_heterogeneous();
bool tests_dispatcher_bind_and_management();
bool tests_dispatcher_fire_immediate();
bool tests_dispatcher_run();
bool tests_dispatcher_compile();
bool tests_dispatcher_merge();
bool tests_dispatcher_typed_and_aggregate_queries();
bool tests_dispatcher_component_access();
bool tests_dispatcher_queue_pending();
bool tests_dispatcher_process_dispatches();
bool tests_dispatcher_queue_then_bind_then_process();
bool tests_dispatcher_process_partial_and_all();
bool tests_dispatcher_fire_vs_queue();
bool tests_drive_result_fields();
bool tests_drive_empty_trace();
bool tests_drive_counts_and_consumed();
bool tests_drive_empty_step();
bool tests_drive_coherence_with_run();
bool tests_traits_real_dispatcher();
bool tests_traits_negative();
bool tests_traits_duck_typed_structural();
bool tests_traits_clean_t_normalization();
bool tests_traits_typed_bind_fire_queue();
bool tests_concepts_core();
bool tests_concepts_typed_capability();
bool tests_concepts_composite();

NS_END  // testing
NS_END  // djinterp


// D_EV_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_EV_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("djinterp event subsystem unit tests");

    // one PDF for the whole subsystem; omit for a console-only run.
    rb.use_pdf("event_tests.pdf");

    // I. event_common
    rb.module("event_common",
              "Event foundations: verdict set, tag detection, traits, "
              "declaration macros, and the C++20 concept layer");
    D_EV_RUN(tests_verdict_enumerators);
    D_EV_RUN(tests_verdict_type_properties);
    D_EV_RUN(tests_consumed_values);
    D_EV_RUN(tests_consumed_consistency);
    D_EV_RUN(tests_has_payload_type);
    D_EV_RUN(tests_has_args_type);
    D_EV_RUN(tests_has_event_payload);
    D_EV_RUN(tests_event_payload_select);
    D_EV_RUN(tests_has_event_name);
    D_EV_RUN(tests_is_tuple);
    D_EV_RUN(tests_index_sequence);
    D_EV_RUN(tests_make_index_sequence);
    D_EV_RUN(tests_apply_impl_direct);
    D_EV_RUN(tests_apply_tuple_arities);
    D_EV_RUN(tests_apply_tuple_values);
    D_EV_RUN(tests_event_traits_payload_type);
    D_EV_RUN(tests_event_traits_args_alias);
    D_EV_RUN(tests_event_traits_arity);
    D_EV_RUN(tests_event_traits_has_name);
    D_EV_RUN(tests_event_traits_has_args);
    D_EV_RUN(tests_event_traits_legacy_and_both);
    D_EV_RUN(tests_d_event_payload);
    D_EV_RUN(tests_d_event_name);
    D_EV_RUN(tests_d_event_empty);
    D_EV_RUN(tests_d_event_arity_range);
    D_EV_RUN(tests_d_event_is_event);
    D_EV_RUN(tests_concept_is_event);
    D_EV_RUN(tests_concept_event_type);
    D_EV_RUN(tests_concept_non_event_type);
    D_EV_RUN(tests_concept_empty_event_type);
    D_EV_RUN(tests_concept_argument_event_type);
    D_EV_RUN(tests_concept_named_event_type);
    D_EV_RUN(tests_concept_unnamed_event_type);
    D_EV_RUN(tests_concept_event_of_arity);
    D_EV_RUN(tests_concept_nullary_unary);
    D_EV_RUN(tests_concept_binary_ternary);
    D_EV_RUN(tests_concept_variadic_event_type);

    // II. event_handler
    rb.module("event_handler",
              "The handler primitive: handler_id, verdict normalization and "
              "tuple-apply, handler compatibility traits, the seq/skip monoid, "
              "and the C++20 concept layer");
    D_EV_RUN(tests_handler_id_relational);
    D_EV_RUN(tests_handler_id_validity);
    D_EV_RUN(tests_handler_id_null);
    D_EV_RUN(tests_handler_id_value_semantics);
    D_EV_RUN(tests_invoke_normalized_void);
    D_EV_RUN(tests_invoke_normalized_verdict);
    D_EV_RUN(tests_apply_handler_arities);
    D_EV_RUN(tests_apply_handler_void_normalization);
    D_EV_RUN(tests_apply_handler_forwards_values);
    D_EV_RUN(tests_handler_traits_is_invocable);
    D_EV_RUN(tests_handler_traits_return_type);
    D_EV_RUN(tests_handler_traits_returns_void_verdict);
    D_EV_RUN(tests_handler_traits_is_compatible);
    D_EV_RUN(tests_handler_traits_is_nothrow);
    D_EV_RUN(tests_handler_traits_expected_arity);
    D_EV_RUN(tests_handler_traits_cvref);
    D_EV_RUN(tests_skip_always_pass);
    D_EV_RUN(tests_seq_pass_pass);
    D_EV_RUN(tests_seq_left_zero);
    D_EV_RUN(tests_seq_void_normalization);
    D_EV_RUN(tests_seq_associativity);
    D_EV_RUN(tests_seq_lvalue_passthrough);
    D_EV_RUN(tests_seq_clean_type);
    D_EV_RUN(tests_concept_is_handler);
    D_EV_RUN(tests_concept_handler_for);
    D_EV_RUN(tests_concept_void_verdict_handler_for);
    D_EV_RUN(tests_concept_nothrow_handlers);
    D_EV_RUN(tests_concept_handler_for_event_of_arity);
    D_EV_RUN(tests_concept_arity_aliases);
    D_EV_RUN(tests_concept_non_event_safety);

    // III. event_table
    rb.module("event_table",
              "Type-erased handler storage: keys, entries, mask, iteration, "
              "merge, statistics, structural traits, and concepts");
    D_EV_RUN(tests_type_key_stable_unique);
    D_EV_RUN(tests_handler_entry_fields);
    D_EV_RUN(tests_table_default_empty);
    D_EV_RUN(tests_insert_ids_increasing);
    D_EV_RUN(tests_insert_counts_enabled_contains);
    D_EV_RUN(tests_remove_basic);
    D_EV_RUN(tests_remove_disabled_bookkeeping);
    D_EV_RUN(tests_id_non_reuse);
    D_EV_RUN(tests_enable_disable_flips_and_counts);
    D_EV_RUN(tests_enable_disable_idempotent_and_missing);
    D_EV_RUN(tests_is_enabled_states);
    D_EV_RUN(tests_contains_states);
    D_EV_RUN(tests_entries_for_missing_null);
    D_EV_RUN(tests_entries_for_present_contents);
    D_EV_RUN(tests_has_entries_for);
    D_EV_RUN(tests_count_for);
    D_EV_RUN(tests_type_key_count);
    D_EV_RUN(tests_for_each_entry);
    D_EV_RUN(tests_for_each_entry_for);
    D_EV_RUN(tests_merge_into_empty);
    D_EV_RUN(tests_merge_order_and_fresh_ids);
    D_EV_RUN(tests_merge_preserves_enabled);
    D_EV_RUN(tests_merge_identity_and_src_untouched);
    D_EV_RUN(tests_clear);
    D_EV_RUN(tests_clear_for);
    D_EV_RUN(tests_clear_for_missing_noop);
    D_EV_RUN(tests_clear_for_mixed_enabled);
    D_EV_RUN(tests_stats_struct_aggregate);
    D_EV_RUN(tests_stats_empty_table);
    D_EV_RUN(tests_stats_populated);
    D_EV_RUN(tests_stats_after_clear);
    D_EV_RUN(tests_detection_core_mutation);
    D_EV_RUN(tests_detection_const_query);
    D_EV_RUN(tests_detection_extended);
    D_EV_RUN(tests_traits_facade_real);
    D_EV_RUN(tests_traits_facade_structural);
    D_EV_RUN(tests_traits_facade_negative);
    D_EV_RUN(tests_traits_facade_cvref);
    D_EV_RUN(tests_concept_is_event_table_type);
    D_EV_RUN(tests_concept_readable_and_non);
    D_EV_RUN(tests_concept_clearable_counting);
    D_EV_RUN(tests_concept_optional_features);
    D_EV_RUN(tests_concept_extended);
    D_EV_RUN(tests_concept_derived_false_on_non_table);

    // IV. event_registry
    rb.module("event_registry",
              "Typed subscription layer: bind, dispatch and run folds, the "
              "fused compile path, merge, management, and queries");
    D_EV_RUN(tests_dispatch_result_fields_and_consumed);
    D_EV_RUN(tests_run_result_aggregate);
    D_EV_RUN(tests_fused_step_empty);
    D_EV_RUN(tests_fused_step_fold_order_and_size);
    D_EV_RUN(tests_fused_step_short_circuit);
    D_EV_RUN(tests_fused_step_operator_and_run_one);
    D_EV_RUN(tests_bind_returns_valid_id_and_registers);
    D_EV_RUN(tests_bind_distinct_ids);
    D_EV_RUN(tests_dispatch_no_handlers);
    D_EV_RUN(tests_dispatch_single_pass);
    D_EV_RUN(tests_dispatch_single_consume);
    D_EV_RUN(tests_dispatch_order_and_count);
    D_EV_RUN(tests_dispatch_consume_short_circuit);
    D_EV_RUN(tests_dispatch_disabled_masked);
    D_EV_RUN(tests_dispatch_payload_delivery);
    D_EV_RUN(tests_dispatch_void_handler_and_nullary);
    D_EV_RUN(tests_dispatch_multi_event_isolation_and_cvref);
    D_EV_RUN(tests_run_empty_trace);
    D_EV_RUN(tests_run_counts_pass);
    D_EV_RUN(tests_run_consumed_count);
    D_EV_RUN(tests_run_no_handlers);
    D_EV_RUN(tests_run_short_circuit_accumulation);
    D_EV_RUN(tests_compile_empty);
    D_EV_RUN(tests_compile_size_and_fold);
    D_EV_RUN(tests_compile_masks_disabled);
    D_EV_RUN(tests_compile_snapshot_independent);
    D_EV_RUN(tests_unbind);
    D_EV_RUN(tests_enable_disable);
    D_EV_RUN(tests_is_enabled_contains);
    D_EV_RUN(tests_merge);
    D_EV_RUN(tests_typed_queries);
    D_EV_RUN(tests_aggregate_queries);
    D_EV_RUN(tests_table_access);

    // V. event_dispatcher
    rb.module("event_dispatcher",
              "Event front end: the deferred event_queue, the dispatcher "
              "facade (immediate and deferred), the fused drive, and the "
              "dispatcher traits and concepts");
    D_EV_RUN(tests_queue_enqueue_pending_empty_clear);
    D_EV_RUN(tests_queue_process_order_and_count);
    D_EV_RUN(tests_queue_process_partial_and_remainder);
    D_EV_RUN(tests_queue_process_max_clamped_and_all);
    D_EV_RUN(tests_queue_process_empty);
    D_EV_RUN(tests_queue_delivery_time_binding);
    D_EV_RUN(tests_queue_payload_captured_by_value);
    D_EV_RUN(tests_queue_reentrant_enqueue_deferred);
    D_EV_RUN(tests_queue_heterogeneous);
    D_EV_RUN(tests_dispatcher_bind_and_management);
    D_EV_RUN(tests_dispatcher_fire_immediate);
    D_EV_RUN(tests_dispatcher_run);
    D_EV_RUN(tests_dispatcher_compile);
    D_EV_RUN(tests_dispatcher_merge);
    D_EV_RUN(tests_dispatcher_typed_and_aggregate_queries);
    D_EV_RUN(tests_dispatcher_component_access);
    D_EV_RUN(tests_dispatcher_queue_pending);
    D_EV_RUN(tests_dispatcher_process_dispatches);
    D_EV_RUN(tests_dispatcher_queue_then_bind_then_process);
    D_EV_RUN(tests_dispatcher_process_partial_and_all);
    D_EV_RUN(tests_dispatcher_fire_vs_queue);
    D_EV_RUN(tests_drive_result_fields);
    D_EV_RUN(tests_drive_empty_trace);
    D_EV_RUN(tests_drive_counts_and_consumed);
    D_EV_RUN(tests_drive_empty_step);
    D_EV_RUN(tests_drive_coherence_with_run);
    D_EV_RUN(tests_traits_real_dispatcher);
    D_EV_RUN(tests_traits_negative);
    D_EV_RUN(tests_traits_duck_typed_structural);
    D_EV_RUN(tests_traits_clean_t_normalization);
    D_EV_RUN(tests_traits_typed_bind_fire_queue);
    D_EV_RUN(tests_concepts_core);
    D_EV_RUN(tests_concepts_typed_capability);
    D_EV_RUN(tests_concepts_composite);

    return rb.finish();
}

#undef D_EV_RUN
