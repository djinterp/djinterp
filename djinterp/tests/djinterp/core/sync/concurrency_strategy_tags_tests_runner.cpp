/*******************************************************************************
* djinterp [core]                     concurrency_strategy_tags_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  concurrency_strategy_tags_tests_runner.cpp
//   Entry point for the concurrency_strategy_tags.hpp suite: registers every
//   section's tests with report_builder, one module per section.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- concurrency_strategy_tags.hpp Test Suite");
    rb.set_subtitle("sync : the strategy tags and the concurrency-signature "
                    "vocabulary -- the two things a trait system reads");
    rb.set_author("teer");
    rb.set_description(
        "The header declares no functions and holds no state, so every claim it "
        "makes is a claim about TYPES -- and the suite tests it the way the trait "
        "system consumes it, not the way a container would. Section I is seven "
        "empty tags whose whole job is to be DISTINCT: the suite sweeps all 21 "
        "pairs for sameness, inheritance and convertibility, then proves each tag "
        "does the two jobs it exists for -- keying a specialization and selecting "
        "an overload. Section I's real interface is one line of the banner, the "
        "self-tagging fast path (using concurrency_strategy_tag = ...), and the "
        "suite re-implements that read locally and points it at types shaped like "
        "the primitives that self-tag today. Its sharpest tests are the negatives: "
        "a PRIVATE alias must not be seen (and must not hard-error), a probe at "
        "void or a function type must answer false rather than fail to compile, "
        "and an alias naming something that is not a tag must be detected AS an "
        "alias yet rejected as a strategy -- the fast path is structural, and no "
        "more than structural. Section II is the four signature coordinates: every "
        "one of the fifteen enumerators is pinned to its documented value, the two "
        "orders the header states in words are checked as orders, and the scoping "
        "is checked as scoping -- no conversion to int, to bool, or to a sibling "
        "coordinate, and no leak of `none` or `hazard` into namespace djinterp. "
        "Because a scoped enum's value range is its underlying type's, each enum "
        "is also cast out of range on purpose, to prove the value survives and "
        "falls through to the switch's default arm. Throughout, the suite includes "
        "the header under test AND NOTHING ELSE from djinterp: the layering claim "
        "-- that the vocabulary is usable beneath the trait layer, with no cycle "
        "back to it -- is only observable from a TU that could not have gotten the "
        "symbols any other way. Verified under C++11, C++14, C++17 and C++20.");

    rb.use_pdf("concurrency_strategy_tags_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, the single dependency, the namespace, the C++11 gate");
    rb.run("the include guard is defined, and is 1",  &tests_header_include_guard);
    rb.run("including it twice is a no-op",           &tests_header_is_idempotent);
    rb.run("it refuses anything but C++",             &tests_header_requires_cpp);
    rb.run("its ONE dependency is djinterp.hpp",      &tests_header_pulls_only_the_core);
    rb.run("no cycle back up to the trait layer",     &tests_header_has_no_trait_layer_cycle);
    rb.run("it publishes types, never macros",        &tests_header_defines_no_stray_macros);
    rb.run("everything sits FLAT in djinterp",        &tests_entities_live_in_the_djinterp_namespace);
    rb.run("section II is gated to C++11",            &tests_vocabulary_is_gated_to_cpp11);

    // -- I. the seven strategy tags ----------------------------------------
    rb.module("I. the seven strategy tags",
              "empty, trivial, standard-layout, one byte -- a name that costs nothing");
    rb.run("all seven are complete class types",      &tests_tag_types_are_complete);
    rb.run("a tag carries no state",                  &tests_tag_types_are_empty);
    rb.run("trivially constructed, copied, destroyed", &tests_tag_types_are_trivial);
    rb.run("standard layout",                         &tests_tag_types_are_standard_layout);
    rb.run("one byte, one-byte aligned",              &tests_tag_types_size_and_alignment);
    rb.run("default-constructible, and nothrow",      &tests_tag_types_are_default_constructible);
    rb.run("copyable and movable (trivially)",        &tests_tag_types_are_copyable_and_movable);
    rb.run("aggregates: tag{} is initialization",     &tests_tag_types_are_aggregates);
    rb.run("no vtable, not abstract, NOT final",      &tests_tag_types_are_not_polymorphic);
    rb.run("literal types: built at compile time",    &tests_tag_types_are_constexpr_friendly);
    rb.run("survive a by-value round trip",           &tests_tag_types_pass_by_value);
    rb.run("C++98 PODs, as the banner claims",        &tests_tag_types_are_cpp98_pods);

    // -- I. tag identity + composition -------------------------------------
    rb.module("I. tag identity + composition",
              "emptiness is what a tag costs; DISTINCTNESS is what it buys");
    rb.run("all 21 pairs are distinct types",         &tests_tags_are_pairwise_distinct);
    rb.run("...and each IS itself (not vacuous)",     &tests_tags_are_identical_to_themselves);
    rb.run("a flat set: no inheritance among them",   &tests_tags_are_unrelated_by_inheritance);
    rb.run("no tag converts to another",              &tests_tags_do_not_interconvert);
    rb.run("each selects its own dispatch arm",       &tests_tag_dispatch_selects_the_matching_overload);
    rb.run("a DERIVED tag dispatches as its base",    &tests_tag_dispatch_accepts_a_derived_tag);
    rb.run("each keys its own specialization",        &tests_tags_key_template_specializations);
    rb.run("exactly seven tags, no more, no fewer",   &tests_the_header_declares_exactly_seven_tags);
    rb.run("inheriting a tag costs nothing (EBO)",    &tests_tags_support_empty_base_optimization);
    rb.run("two tags compose -- into a NON-tag",      &tests_tags_compose_by_multiple_inheritance);
    rb.run("hybrid is a PEER, not a composite",       &tests_hybrid_is_a_peer_not_a_composite);
    rb.run("the tag predicate rejects everything else", &tests_the_tag_predicate_rejects_non_tags);

    // -- I. the self-tagging protocol --------------------------------------
    rb.module("I. the self-tagging fast path",
              "using concurrency_strategy_tag = <tag>; -- read directly, no probing");
    rb.run("a type that names the alias is seen",     &tests_self_tagging_is_detected);
    rb.run("the tag it named is recovered",           &tests_the_declared_tag_is_recovered);
    rb.run("all seven, end to end: read -> dispatch", &tests_every_tag_is_declarable);
    rb.run("a type with no alias is not seen",        &tests_untagged_types_are_not_detected);
    rb.run("SFINAE-safe: void, arrays, functions",    &tests_detection_is_sfinae_safe);
    rb.run("a PRIVATE alias is not visible",          &tests_a_private_alias_is_not_detected);
    rb.run("the C++98 typedef form is equivalent",    &tests_the_typedef_form_is_equivalent);
    rb.run("an INHERITED alias is found",             &tests_an_inherited_alias_is_detected);
    rb.run("a redeclared alias hides the base's",     &tests_a_shadowing_alias_wins);
    rb.run("a template can self-tag (and compute it)", &tests_a_template_can_self_tag);
    rb.run("the alias need not name a KNOWN tag",     &tests_the_alias_need_not_name_a_known_tag);
    rb.run("decay first: a reference is not a class", &tests_detection_after_decay);

    // -- II. concurrency_progress ------------------------------------------
    rb.module("II. concurrency_progress",
              "the progress grade: sequential < blocking < lock_free < wait_free");
    rb.run("the four grades are 0, 1, 2, 3",          &tests_progress_enumerator_values);
    rb.run("an enum -- not an integer",               &tests_progress_is_an_enum_not_an_integer);
    rb.run("SCOPED: no conversion in or out",         &tests_progress_is_scoped);
    rb.run("the underlying type is int",              &tests_progress_underlying_type);
    rb.run("it costs exactly one int",                &tests_progress_size_and_alignment);
    rb.run("a TOTAL ORDER (checked as an order)",     &tests_progress_is_a_total_order);
    rb.run("four values, pairwise unequal",           &tests_progress_equality);
    rb.run("round-trips through the underlying type", &tests_progress_round_trips_through_the_underlying_type);
    rb.run("a compile-time value (and an NTTP)",      &tests_progress_in_constant_expressions);
    rb.run("a switch reaches every grade",            &tests_progress_switch_covers_every_grade);
    rb.run("exactly four grades",                     &tests_progress_has_exactly_four_grades);
    rb.run("an out-of-range cast falls through",      &tests_progress_out_of_range_value_falls_through);

    // -- II. concurrency_arity ---------------------------------------------
    rb.module("II. concurrency_arity",
              "how many agents may act at once: swmr < mwmr");
    rb.run("swmr is 0, mwmr is 1",                    &tests_arity_enumerator_values);
    rb.run("SCOPED: no conversion in or out",         &tests_arity_is_scoped);
    rb.run("the underlying type is int",              &tests_arity_underlying_type);
    rb.run("it costs exactly one int",                &tests_arity_size_and_alignment);
    rb.run("a total order: swmr < mwmr",              &tests_arity_is_a_total_order);
    rb.run("two values do NOT make a bool",           &tests_arity_is_not_a_boolean);
    rb.run("round-trips through the underlying type", &tests_arity_round_trips_through_the_underlying_type);
    rb.run("a compile-time value (and an NTTP)",      &tests_arity_in_constant_expressions);
    rb.run("a switch reaches both arities",           &tests_arity_switch_covers_both_arities);
    rb.run("exactly two arities",                     &tests_arity_has_exactly_two_arities);
    rb.run("an out-of-range cast falls through",      &tests_arity_out_of_range_value_falls_through);

    // -- II. iteration_consistency -----------------------------------------
    rb.module("II. iteration_consistency",
              "overlap-semantics: inconsistent < weakly_consistent < snapshot");
    rb.run("the three semantics are 0, 1, 2",         &tests_iteration_enumerator_values);
    rb.run("SCOPED: no conversion in or out",         &tests_iteration_is_scoped);
    rb.run("the underlying type is int",              &tests_iteration_underlying_type);
    rb.run("the values order the semantics",          &tests_iteration_orders_the_overlap_semantics);
    rb.run("the DEGENERATE is the zero and the floor", &tests_iteration_the_degenerate_is_the_floor);
    rb.run("round-trips through the underlying type", &tests_iteration_round_trips_through_the_underlying_type);
    rb.run("a compile-time value (and an NTTP)",      &tests_iteration_in_constant_expressions);
    rb.run("a switch reaches every semantics",        &tests_iteration_switch_covers_every_semantics);
    rb.run("exactly three semantics",                 &tests_iteration_has_exactly_three_semantics);
    rb.run("an out-of-range cast falls through",      &tests_iteration_out_of_range_value_falls_through);

    // -- II. reclamation_obligation ----------------------------------------
    rb.module("II. reclamation_obligation",
              "what a container owes when a cell is freed while agents run");
    rb.run("the six obligations are 0 through 5",     &tests_reclamation_enumerator_values);
    rb.run("SCOPED: no conversion in or out",         &tests_reclamation_is_scoped);
    rb.run("the underlying type is int",              &tests_reclamation_underlying_type);
    rb.run("the values order the disciplines",        &tests_reclamation_orders_the_disciplines);
    rb.run("`none` is the zero, and the default",     &tests_reclamation_none_is_the_floor);
    rb.run("round-trips through the underlying type", &tests_reclamation_round_trips_through_the_underlying_type);
    rb.run("a compile-time value (and an NTTP)",      &tests_reclamation_in_constant_expressions);
    rb.run("a switch reaches every obligation",       &tests_reclamation_switch_covers_every_obligation);
    rb.run("exactly six obligations",                 &tests_reclamation_has_exactly_six_obligations);
    rb.run("an out-of-range cast falls through",      &tests_reclamation_out_of_range_value_falls_through);
    rb.run("scoping keeps `hazard` out of djinterp",  &tests_reclamation_enumerators_do_not_collide_with_the_tags);

    // -- II. the signature vocabulary --------------------------------------
    rb.module("II. the vocabulary as a whole",
              "four coordinates, one signature -- and no trait header in sight");
    rb.run("four coordinates, four distinct types",   &tests_the_four_enums_are_distinct_types);
    rb.run("none of them converts to another",        &tests_the_enums_do_not_interconvert);
    rb.run("four zeros, four different meanings",     &tests_coincident_values_are_not_coincident_types);
    rb.run("a coordinate is a name, not a number",    &tests_the_enums_are_not_arithmetic);
    rb.run("the four compose into a signature",       &tests_the_vocabulary_composes_into_a_signature);
    rb.run("a strategy -> signature map is writable", &tests_a_strategy_to_signature_mapping_is_expressible);
    rb.run("a signature is a constant expression",    &tests_the_signature_is_a_compile_time_value);
    rb.run("all fifteen enumerators are reachable",   &tests_every_enumerator_is_reachable);
    rb.run("the vocabulary needs no trait header",    &tests_the_vocabulary_needs_no_trait_header);

    return rb.finish();
}
