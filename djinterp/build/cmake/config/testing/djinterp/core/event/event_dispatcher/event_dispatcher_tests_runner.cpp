// djinterp
#include "../../../../../../../../../tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_ED_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_ED_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("event_dispatcher.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("event_dispatcher_tests.pdf");

    rb.module("event_dispatcher",
              "Event front end: the deferred event_queue, the dispatcher "
              "facade (immediate and deferred), the fused drive, and the "
              "dispatcher traits and concepts");

    // I/II. event_queue
    D_ED_RUN(tests_queue_enqueue_pending_empty_clear);
    D_ED_RUN(tests_queue_process_order_and_count);
    D_ED_RUN(tests_queue_process_partial_and_remainder);
    D_ED_RUN(tests_queue_process_max_clamped_and_all);
    D_ED_RUN(tests_queue_process_empty);
    D_ED_RUN(tests_queue_delivery_time_binding);
    D_ED_RUN(tests_queue_payload_captured_by_value);
    D_ED_RUN(tests_queue_reentrant_enqueue_deferred);
    D_ED_RUN(tests_queue_heterogeneous);

    // III. event_dispatcher facade -- immediate surface
    D_ED_RUN(tests_dispatcher_bind_and_management);
    D_ED_RUN(tests_dispatcher_fire_immediate);
    D_ED_RUN(tests_dispatcher_run);
    D_ED_RUN(tests_dispatcher_compile);
    D_ED_RUN(tests_dispatcher_merge);
    D_ED_RUN(tests_dispatcher_typed_and_aggregate_queries);
    D_ED_RUN(tests_dispatcher_component_access);

    // III. event_dispatcher facade -- deferred surface + integration
    D_ED_RUN(tests_dispatcher_queue_pending);
    D_ED_RUN(tests_dispatcher_process_dispatches);
    D_ED_RUN(tests_dispatcher_queue_then_bind_then_process);
    D_ED_RUN(tests_dispatcher_process_partial_and_all);
    D_ED_RUN(tests_dispatcher_fire_vs_queue);

    // IV.  fused drive
    D_ED_RUN(tests_drive_result_fields);
    D_ED_RUN(tests_drive_empty_trace);
    D_ED_RUN(tests_drive_counts_and_consumed);
    D_ED_RUN(tests_drive_empty_step);
    D_ED_RUN(tests_drive_coherence_with_run);

    // V/VI. event_dispatcher_traits + typed detection
    D_ED_RUN(tests_traits_real_dispatcher);
    D_ED_RUN(tests_traits_negative);
    D_ED_RUN(tests_traits_duck_typed_structural);
    D_ED_RUN(tests_traits_clean_t_normalization);
    D_ED_RUN(tests_traits_typed_bind_fire_queue);

    // VII. concepts (C++20+)
    D_ED_RUN(tests_concepts_core);
    D_ED_RUN(tests_concepts_typed_capability);
    D_ED_RUN(tests_concepts_composite);

    return rb.finish();
}

#undef D_ED_RUN
