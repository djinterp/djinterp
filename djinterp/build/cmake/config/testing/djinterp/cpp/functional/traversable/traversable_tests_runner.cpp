/******************************************************************************
* djinterp [test]                                traversable_tests_runner.cpp
*
*   Runner (the main() entry point) for the traversable.hpp unit suite.  Per the
* DTest authoring guide a runner is thin: main() only chooses a few options and
* hands the spec to run_module, whose exit code is the whole of main.
*
*   WHERE THE SPEC COMES FROM.  traversable_tests.hpp declares its tests_*
* predicates flat (visible to every consumer) but -- unlike the cofree suite --
* ships NO per-section block_spec provider and NO inline traversable_spec()
* provider (it predates the DTEST_SPEC_MODE two-faced-header idiom, and does not
* pull in test_defaults.hpp).  The module_spec is therefore assembled here, from
* those declarations, in build_traversable_spec() below; the blocks mirror the
* header's section groups, and each row's descriptor is lifted from the test
* body's own comment.  main() stays thin -- the spec is data in a helper.
*
*   (If traversable_tests.hpp is later migrated to carry an inline
* traversable_spec() guarded by DTEST_SPEC_MODE, delete build_traversable_spec()
* and call tt::traversable_spec() directly.)
*
*   Every tests_* function is DEFINED unconditionally in its section TU (the
* C++14 variable-template and C++20 concept checks are #if-gated INSIDE the
* bodies, not around them), so the spec below lists them all without feature
* guards.
*
* path:      /build/cmake/config/testing/djinterp/cpp/functional/traversable/traversable_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.20
******************************************************************************/

// std
#include <string>
// djinterp -- the framework surface (module_spec, run_module, the option set)
//   full-from-inc, and the suite header (bare, via the CMakeLists INCLUDES) for
//   the tests_* declarations.  The suite header does not itself pull in
//   test_defaults.hpp, so the runner includes it explicitly.
#include "djinterp/cpp/modules/test/test_defaults.hpp"
#include "traversable_tests.hpp"


// TU-local shorthands: dt names the framework surface (option set, module_spec,
// run_module); tt names the suite's tests_* predicates.
namespace dt = ::djinterp::test;
namespace tt = ::djinterp::testing;


namespace
{
    // build_traversable_spec
    //   assembles the module_spec for the traversable suite: one block per
    // section of traversable_tests.hpp, each row naming a tests_* predicate and
    // what it verifies.  Kept here (rather than in the header) only because the
    // suite header ships no provider of its own.
    dt::module_spec build_traversable_spec()
    {
        return dt::module_spec{
            "traversable",
            "traversable.hpp -- the capstone of the functional layer: walk a "
            "structure T left-to-right, run an applicative effect f : A -> F<B> "
            "at each element, and collect the results back into the same shape "
            "(T<A> -> F<T'<B>>).  Covers the traversable_traits protocol and "
            "is_traversable, the structural traits / internal helpers / C++20 "
            "concept, the one shipped std::vector instance, and the generic "
            "traverse / sequence operations.",
            {
                // -- I.  traversable protocol -------------------------------
                dt::block_spec{
                    "I. traversable protocol",
                    "traversable_traits (the undefined primary) and "
                    "is_traversable",
                    {
                        dt::test_spec{
                            "tests_is_traversable_positive",
                            "std::vector and the maybe structure are recognised "
                            "as traversables",
                            &tt::tests_is_traversable_positive },
                        dt::test_spec{
                            "tests_is_traversable_negative",
                            "unrelated structs, scalars, and effect functions "
                            "are refused",
                            &tt::tests_is_traversable_negative },
                        dt::test_spec{
                            "tests_is_traversable_is_a_separate_obligation",
                            "tbox is a functor + applicative + monad yet still "
                            "not a traversable",
                            &tt::tests_is_traversable_is_a_separate_obligation },
                        dt::test_spec{
                            "tests_is_traversable_decay",
                            "const / ref / rvalue / volatile spellings all decay "
                            "to the traversable",
                            &tt::tests_is_traversable_decay },
                        dt::test_spec{
                            "tests_is_traversable_requires_marker",
                            "detection keys on the is_specialized marker, not on "
                            "a mere traverse",
                            &tt::tests_is_traversable_requires_marker },
                        dt::test_spec{
                            "tests_traversable_traits_primary_is_undefined",
                            "the primary template is undefined, so a "
                            "non-traversable reads as incomplete",
                            &tt::tests_traversable_traits_primary_is_undefined },
                        dt::test_spec{
                            "tests_traversable_traits_surface",
                            "a specialization exposes the marker, value_type, and "
                            "traverse",
                            &tt::tests_traversable_traits_surface },
                        dt::test_spec{
                            "tests_is_traversable_v_agrees",
                            "the _v shorthand equals the trait's value",
                            &tt::tests_is_traversable_v_agrees },
                    }
                },

                // -- 0'. structural traits & concepts -----------------------
                dt::block_spec{
                    "0'. structural traits & concepts",
                    "traversable_value_type, the internal identity / append "
                    "helpers, the C++20 Traversable concept",
                    {
                        dt::test_spec{
                            "tests_traversable_value_type",
                            "the inner value type A of a traversable T<A>",
                            &tt::tests_traversable_value_type },
                        dt::test_spec{
                            "tests_traversable_value_type_t_alias",
                            "the _t alias equals the trait's ::type",
                            &tt::tests_traversable_value_type_t_alias },
                        dt::test_spec{
                            "tests_traversable_value_type_helper_sfinae",
                            "the internal helper soft-fails (no ::type) for a "
                            "non-traversable",
                            &tt::tests_traversable_value_type_helper_sfinae },
                        dt::test_spec{
                            "tests_traversable_value_type_mirrors_siblings",
                            "the traversable inner type matches functor_value_type",
                            &tt::tests_traversable_value_type_mirrors_siblings },
                        dt::test_spec{
                            "tests_identity_helper",
                            "the identity helper returns its argument unchanged "
                            "(the seam sequence is built on)",
                            &tt::tests_identity_helper },
                        dt::test_spec{
                            "tests_append_helper",
                            "the append reducer appends at the end, preserving "
                            "order",
                            &tt::tests_append_helper },
                        dt::test_spec{
                            "tests_append_helper_threads_by_value",
                            "the accumulator is threaded by value, never mutated",
                            &tt::tests_append_helper_threads_by_value },
                        dt::test_spec{
                            "tests_traversable_concept",
                            "the Traversable concept mirrors the trait and gates "
                            "overload resolution",
                            &tt::tests_traversable_concept },
                        dt::test_spec{
                            "tests_traversable_concept_gating",
                            "the concept face is gated to C++20; the trait is the "
                            "always-present floor",
                            &tt::tests_traversable_concept_gating },
                    }
                },

                // -- the std::vector instance -------------------------------
                dt::block_spec{
                    "the std::vector instance",
                    "the one shipped instance: walk left-to-right, run "
                    "f : A -> F<B>, combine with lift_a2 into F<vector<B>>",
                    {
                        dt::test_spec{
                            "tests_vector_traverse_all_succeed",
                            "every element succeeds; results materialise into the "
                            "vector inside the effect",
                            &tt::tests_vector_traverse_all_succeed },
                        dt::test_spec{
                            "tests_vector_traverse_one_failure_sinks_it",
                            "all-or-nothing: a single failing element sinks the "
                            "traversal",
                            &tt::tests_vector_traverse_one_failure_sinks_it },
                        dt::test_spec{
                            "tests_vector_traverse_empty_uses_pure",
                            "an empty vector injects the empty shape with pure; f "
                            "is never called",
                            &tt::tests_vector_traverse_empty_uses_pure },
                        dt::test_spec{
                            "tests_vector_traverse_is_left_to_right",
                            "the walk is left-to-right, one call per element in "
                            "source order",
                            &tt::tests_vector_traverse_is_left_to_right },
                        dt::test_spec{
                            "tests_vector_traverse_single_element",
                            "the one-element case: one effect combined with the "
                            "pure seed",
                            &tt::tests_vector_traverse_single_element },
                        dt::test_spec{
                            "tests_vector_traverse_changes_inner_type",
                            "f : A -> F<B> may change the element type through the "
                            "walk",
                            &tt::tests_vector_traverse_changes_inner_type },
                        dt::test_spec{
                            "tests_vector_traverse_result_type",
                            "the result type is F<std::vector<B>>, assembled via "
                            "monad_rebind",
                            &tt::tests_vector_traverse_result_type },
                        dt::test_spec{
                            "tests_vector_traverse_second_effect",
                            "the instance is generic in the effect F it threads",
                            &tt::tests_vector_traverse_second_effect },
                        dt::test_spec{
                            "tests_vector_traverse_source_untouched",
                            "the source vector is walked, not consumed",
                            &tt::tests_vector_traverse_source_untouched },
                        dt::test_spec{
                            "tests_vector_is_the_foldable_companion",
                            "std::vector is both a foldable and a traversable over "
                            "the same element type",
                            &tt::tests_vector_is_the_foldable_companion },
                    }
                },

                // -- II. generic operations ---------------------------------
                dt::block_spec{
                    "II. generic operations",
                    "traverse (delegated to the instance) and sequence (traverse "
                    "with the identity helper)",
                    {
                        dt::test_spec{
                            "tests_traverse_delegates_to_the_instance",
                            "traverse delegates to traversable_traits<T>, "
                            "dispatching per structure",
                            &tt::tests_traverse_delegates_to_the_instance },
                        dt::test_spec{
                            "tests_traverse_over_the_maybe_structure",
                            "for a shape-preserving structure the result is "
                            "F<T<B>>",
                            &tt::tests_traverse_over_the_maybe_structure },
                        dt::test_spec{
                            "tests_traverse_empty_recovers_the_effect_from_the_type",
                            "on an empty structure F is recovered from f's result "
                            "type and injected with pure",
                            &tt::tests_traverse_empty_recovers_the_effect_from_the_type },
                        dt::test_spec{
                            "tests_traverse_hoists_the_effect",
                            "a failure inside the structure is hoisted out to the "
                            "effect",
                            &tt::tests_traverse_hoists_the_effect },
                        dt::test_spec{
                            "tests_traverse_result_type_is_deduced",
                            "the result type is whatever the instance produces; "
                            "the protocol never names it",
                            &tt::tests_traverse_result_type_is_deduced },
                        dt::test_spec{
                            "tests_traverse_forwarding",
                            "traverse takes the traversable by forwarding "
                            "reference",
                            &tt::tests_traverse_forwarding },
                        dt::test_spec{
                            "tests_traverse_constexpr",
                            "traverse and sequence fold in a constant expression "
                            "(and at runtime)",
                            &tt::tests_traverse_constexpr },
                        dt::test_spec{
                            "tests_sequence_is_traverse_with_identity",
                            "sequence is traverse with the library's identity "
                            "helper",
                            &tt::tests_sequence_is_traverse_with_identity },
                        dt::test_spec{
                            "tests_sequence_inverts_the_nesting",
                            "sequence turns T<F<A>> inside out into F<T<A>>",
                            &tt::tests_sequence_inverts_the_nesting },
                        dt::test_spec{
                            "tests_sequence_is_all_or_nothing",
                            "sequencing a vector of maybes yields a value only if "
                            "every element was one",
                            &tt::tests_sequence_is_all_or_nothing },
                        dt::test_spec{
                            "tests_sequence_empty",
                            "the empty cases: emptiness is not failure",
                            &tt::tests_sequence_empty },
                        dt::test_spec{
                            "tests_law_traverse_is_sequence_after_map",
                            "traverse(ta, f) == sequence(functor_map(ta, f))",
                            &tt::tests_law_traverse_is_sequence_after_map },
                    }
                },
            }
        };
    }
}


int main()
{
    // options: render a PDF beside the always-on console report.  Leave document
    // at its txt default (or drop these two lines) for a console-only run.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("traversable_tests.pdf"));

    return dt::run_module(
        build_traversable_spec(),
        "traversable.hpp unit tests",
        "walk a structure, thread an applicative effect, collect back the shape: "
        "traverse and sequence",
        opts);
}
