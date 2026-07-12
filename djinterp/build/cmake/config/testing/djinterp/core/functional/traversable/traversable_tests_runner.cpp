// djinterp [test]  traversable_tests_runner.cpp
//   Entry point for the traversable.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "traversable_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- traversable.hpp Test Suite");
    rb.set_subtitle("functional : traverse and sequence -- run an effect at every "
                    "element and invert the nesting");
    rb.set_author("teer");
    rb.set_description(
        "Traversal needs two things that must not be confused -- a STRUCTURE to "
        "walk and an EFFECT to thread -- so the suite supplies both and keeps them "
        "separable. tmaybe is both: as an effect it can FAIL, which makes an "
        "all-or-nothing traversal observable; as a structure it is a second "
        "traversable instance, so the generic traverse is shown to dispatch "
        "through traversable_traits rather than being wired to std::vector. tbox "
        "is an effect only -- a Functor AND an Applicative that is deliberately "
        "NOT a Traversable, which is the suite's sharpest negative: traversability "
        "is a separate obligation nothing else implies. The subtlest claim in the "
        "header gets a dedicated test: an EMPTY structure never calls f, so the "
        "effect F cannot come from a value -- it is recovered from the TYPE of f's "
        "result and injected with pure. A recording function proves f is invoked "
        "zero times while the result type is still the full F<T<B>>, and proves "
        "the walk is left-to-right. Both operations are checked against the law "
        "traverse(ta, f) == sequence(functor_map(ta, f)). Verified under C++17 and "
        "C++20.");

    rb.use_pdf("traversable_tests_report.pdf");

    // -- I. the traversable protocol --------------------------------------
    rb.module("I. traversable protocol",
              "traversable_traits (primary, undefined) and is_traversable");
    rb.run("is_traversable: both instances",            &tests_is_traversable_positive);
    rb.run("is_traversable: non-traversables rejected", &tests_is_traversable_negative);
    rb.run("traversability is a SEPARATE obligation",   &tests_is_traversable_is_a_separate_obligation);
    rb.run("std::decay applied before detection",       &tests_is_traversable_decay);
    rb.run("detection requires the is_specialized marker", &tests_is_traversable_requires_marker);
    rb.run("the primary is UNDEFINED (incomplete)",     &tests_traversable_traits_primary_is_undefined);
    rb.run("traits surface (marker / value_type / traverse)", &tests_traversable_traits_surface);
    rb.run("is_traversable_v agrees",                   &tests_is_traversable_v_agrees);

    // -- 0. structural traits + helpers -----------------------------------
    rb.module("0. structural traits + helpers",
              "traversable_value_type, the identity and append helpers, the concept");
    rb.run("traversable_value_type: the inner type A", &tests_traversable_value_type);
    rb.run("the _t alias agrees",                       &tests_traversable_value_type_t_alias);
    rb.run("the value-type helper soft-fails (SFINAE)", &tests_traversable_value_type_helper_sfinae);
    rb.run("it mirrors functor / foldable value types", &tests_traversable_value_type_mirrors_siblings);
    rb.run("the identity helper (sequence's function)", &tests_identity_helper);
    rb.run("the append helper grows the result vector", &tests_append_helper);
    rb.run("the append helper threads BY VALUE (pure)", &tests_append_helper_threads_by_value);
    rb.run("the Traversable concept gates resolution",  &tests_traversable_concept);
    rb.run("the concept face is gated to C++20",        &tests_traversable_concept_gating);

    // -- the std::vector instance -----------------------------------------
    rb.module("The std::vector instance",
              "walk left-to-right, combine with lift_a2 into F<std::vector<B>>");
    rb.run("all effects succeed -> the whole vector",   &tests_vector_traverse_all_succeed);
    rb.run("one failure sinks it (ALL-OR-NOTHING)",     &tests_vector_traverse_one_failure_sinks_it);
    rb.run("the empty vector uses pure (f uncalled)",   &tests_vector_traverse_empty_uses_pure);
    rb.run("the walk is LEFT-TO-RIGHT (observed)",      &tests_vector_traverse_is_left_to_right);
    rb.run("a single element",                          &tests_vector_traverse_single_element);
    rb.run("the element type may change (A -> B)",      &tests_vector_traverse_changes_inner_type);
    rb.run("the result type is F<std::vector<B>>",      &tests_vector_traverse_result_type);
    rb.run("generic in the effect F",                   &tests_vector_traverse_second_effect);
    rb.run("the source vector is left untouched",       &tests_vector_traverse_source_untouched);
    rb.run("companion to the vector foldable instance", &tests_vector_is_the_foldable_companion);

    // -- II. generic operations -------------------------------------------
    rb.module("II. traverse + sequence",
              "the one obligation, delegated -- and sequence derived from it");
    rb.run("traverse delegates to the instance",        &tests_traverse_delegates_to_the_instance);
    rb.run("a shape-preserving structure: F<T<B>>",     &tests_traverse_over_the_maybe_structure);
    rb.run("EMPTY: F recovered from f's TYPE, f uncalled", &tests_traverse_empty_recovers_the_effect_from_the_type);
    rb.run("a failing effect is hoisted out",           &tests_traverse_hoists_the_effect);
    rb.run("the result type is deduced, never named",   &tests_traverse_result_type_is_deduced);
    rb.run("the traversable is forwarded",              &tests_traverse_forwarding);
    rb.run("dual domain: folds constexpr, runs at runtime", &tests_traverse_constexpr);
    rb.run("sequence IS traverse with the identity",    &tests_sequence_is_traverse_with_identity);
    rb.run("sequence inverts the nesting T<F<A>> -> F<T<A>>", &tests_sequence_inverts_the_nesting);
    rb.run("sequence is ALL-OR-NOTHING",                &tests_sequence_is_all_or_nothing);
    rb.run("sequence on empty (emptiness is not failure)", &tests_sequence_empty);
    rb.run("law: traverse f == sequence . fmap f",      &tests_law_traverse_is_sequence_after_map);

    return rb.finish();
}
