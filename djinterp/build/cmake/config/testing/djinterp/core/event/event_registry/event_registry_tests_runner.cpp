// djinterp
#include "../../../../../../../../../tests/djinterp/core/event/event_registry/event_registry_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_ER_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_ER_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("event_registry.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("event_registry_tests.pdf");

    rb.module("event_registry",
              "Typed subscription layer: bind, dispatch and run folds, the "
              "fused compile path, merge, management, and queries");

    // I.   dispatch_result + run_result
    D_ER_RUN(tests_dispatch_result_fields_and_consumed);
    D_ER_RUN(tests_run_result_aggregate);

    // II.  fused_step (the staging proposition)
    D_ER_RUN(tests_fused_step_empty);
    D_ER_RUN(tests_fused_step_fold_order_and_size);
    D_ER_RUN(tests_fused_step_short_circuit);
    D_ER_RUN(tests_fused_step_operator_and_run_one);

    // III. bind + dispatch (the inner fold)
    D_ER_RUN(tests_bind_returns_valid_id_and_registers);
    D_ER_RUN(tests_bind_distinct_ids);
    D_ER_RUN(tests_dispatch_no_handlers);
    D_ER_RUN(tests_dispatch_single_pass);
    D_ER_RUN(tests_dispatch_single_consume);
    D_ER_RUN(tests_dispatch_order_and_count);
    D_ER_RUN(tests_dispatch_consume_short_circuit);
    D_ER_RUN(tests_dispatch_disabled_masked);
    D_ER_RUN(tests_dispatch_payload_delivery);
    D_ER_RUN(tests_dispatch_void_handler_and_nullary);
    D_ER_RUN(tests_dispatch_multi_event_isolation_and_cvref);

    // III. run (the outer fold)
    D_ER_RUN(tests_run_empty_trace);
    D_ER_RUN(tests_run_counts_pass);
    D_ER_RUN(tests_run_consumed_count);
    D_ER_RUN(tests_run_no_handlers);
    D_ER_RUN(tests_run_short_circuit_accumulation);

    // III. compile (the fused path)
    D_ER_RUN(tests_compile_empty);
    D_ER_RUN(tests_compile_size_and_fold);
    D_ER_RUN(tests_compile_masks_disabled);
    D_ER_RUN(tests_compile_snapshot_independent);

    // III. management + merge + typed/aggregate queries + table access
    D_ER_RUN(tests_unbind);
    D_ER_RUN(tests_enable_disable);
    D_ER_RUN(tests_is_enabled_contains);
    D_ER_RUN(tests_merge);
    D_ER_RUN(tests_typed_queries);
    D_ER_RUN(tests_aggregate_queries);
    D_ER_RUN(tests_table_access);

    return rb.finish();
}

#undef D_ER_RUN
