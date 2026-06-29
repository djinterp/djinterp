// djinterp
#include "../../../../../../../../../tests/djinterp/core/event/event_table/event_table_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_ET_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_ET_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("event_table.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("event_table_tests.pdf");

    rb.module("event_table",
              "Type-erased handler storage: keys, entries, mask, iteration, "
              "merge, statistics, structural traits, and concepts");

    // I.   internal storage types (type_key, handler_entry)
    D_ET_RUN(tests_type_key_stable_unique);
    D_ET_RUN(tests_handler_entry_fields);

    // III. core: insert / remove / mask / lookup
    D_ET_RUN(tests_table_default_empty);
    D_ET_RUN(tests_insert_ids_increasing);
    D_ET_RUN(tests_insert_counts_enabled_contains);
    D_ET_RUN(tests_remove_basic);
    D_ET_RUN(tests_remove_disabled_bookkeeping);
    D_ET_RUN(tests_id_non_reuse);
    D_ET_RUN(tests_enable_disable_flips_and_counts);
    D_ET_RUN(tests_enable_disable_idempotent_and_missing);
    D_ET_RUN(tests_is_enabled_states);
    D_ET_RUN(tests_contains_states);

    // III. bucket access + iteration
    D_ET_RUN(tests_entries_for_missing_null);
    D_ET_RUN(tests_entries_for_present_contents);
    D_ET_RUN(tests_has_entries_for);
    D_ET_RUN(tests_count_for);
    D_ET_RUN(tests_type_key_count);
    D_ET_RUN(tests_for_each_entry);
    D_ET_RUN(tests_for_each_entry_for);

    // III. merge + clear + clear_for
    D_ET_RUN(tests_merge_into_empty);
    D_ET_RUN(tests_merge_order_and_fresh_ids);
    D_ET_RUN(tests_merge_preserves_enabled);
    D_ET_RUN(tests_merge_identity_and_src_untouched);
    D_ET_RUN(tests_clear);
    D_ET_RUN(tests_clear_for);
    D_ET_RUN(tests_clear_for_missing_noop);
    D_ET_RUN(tests_clear_for_mixed_enabled);

    // II + III. event_table_stats + get_stats
    D_ET_RUN(tests_stats_struct_aggregate);
    D_ET_RUN(tests_stats_empty_table);
    D_ET_RUN(tests_stats_populated);
    D_ET_RUN(tests_stats_after_clear);

    // IV + V. structural detection traits + event_table_traits facade
    D_ET_RUN(tests_detection_core_mutation);
    D_ET_RUN(tests_detection_const_query);
    D_ET_RUN(tests_detection_extended);
    D_ET_RUN(tests_traits_facade_real);
    D_ET_RUN(tests_traits_facade_structural);
    D_ET_RUN(tests_traits_facade_negative);
    D_ET_RUN(tests_traits_facade_cvref);

    // VI.  concept constraints (C++20+)
    D_ET_RUN(tests_concept_is_event_table_type);
    D_ET_RUN(tests_concept_readable_and_non);
    D_ET_RUN(tests_concept_clearable_counting);
    D_ET_RUN(tests_concept_optional_features);
    D_ET_RUN(tests_concept_extended);
    D_ET_RUN(tests_concept_derived_false_on_non_table);

    return rb.finish();
}

#undef D_ET_RUN
