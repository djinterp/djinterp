// djinterp [test]  functor_tests_runner.cpp
//   Entry point for the functor.hpp suite: registers every section's tests with
//   report_builder, one module per section.

// djinterp
#include "functor_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- functor.hpp Test Suite");
    rb.set_subtitle("functional : the Functor protocol and the generic "
                    "functorial map");
    rb.set_author("teer");
    rb.set_description(
        "The header makes two distinct claims about how a type becomes a functor, "
        "so the suite supplies one instance of each and plays them against one "
        "another. tmaybe and tbox are MONADS -- each receives a monad_traits "
        "specialization and nothing else -- so if they are functors at all, the "
        "blanket bridge (keyed on is_monad) is what made them so, and their map "
        "is monad_map exactly, in value and in type. lazy_view is NOT a monad: it "
        "carries an explicit specialization and is the context whose mapped type "
        "DEPENDS on the mapping function, so it names no rebind at all -- and the "
        "suite shows that both are nevertheless functors and both map, which is "
        "what 'rebind is not part of the core protocol' means. The functor laws "
        "(identity and composition) are checked on both roads, functor_map's dual "
        "domain is folded inside static_assert and run at runtime, and the "
        "undefined primary functor_traits is pinned with a completeness probe. "
        "Verified under C++17 and C++20.");

    rb.use_pdf("functor_tests_report.pdf");

    // -- I. the functor protocol -----------------------------------------
    rb.module("I. functor protocol",
              "functor_traits, is_functor (+ _v), and the absent rebind");
    rb.run("is_functor: both roads into the protocol",  &tests_is_functor_positive);
    rb.run("is_functor: non-functors rejected",         &tests_is_functor_negative);
    rb.run("std::decay applied before detection",       &tests_is_functor_decay);
    rb.run("detection requires the is_specialized marker", &tests_is_functor_requires_marker);
    rb.run("the primary is UNDEFINED (incomplete)",     &tests_functor_traits_primary_is_undefined);
    rb.run("functor_traits surface (marker/value_type/map)", &tests_functor_traits_surface);
    rb.run("is_functor_v agrees",                       &tests_is_functor_v_agrees);
    rb.run("rebind is NOT part of the core protocol",   &tests_rebind_is_not_core_protocol);

    // -- I.2 the monad bridge --------------------------------------------
    rb.module("I.2 the monad bridge",
              "every monad is a functor, keyed on is_monad, with zero wiring");
    rb.run("every monad is a functor",                  &tests_bridge_every_monad_is_a_functor);
    rb.run("keyed on is_monad (a one-way implication)", &tests_bridge_is_keyed_on_is_monad);
    rb.run("map is derived from monad_map exactly",     &tests_bridge_derives_map_from_monad_map);
    rb.run("value_type comes from the monad",           &tests_bridge_value_type_from_monad);
    rb.run("rebind comes from the monad",               &tests_bridge_rebind_from_monad);
    rb.run("a NEW monad costs zero functor wiring",     &tests_bridge_zero_wiring_for_a_new_monad);
    rb.run("no overlap with an explicit specialization",&tests_bridge_does_not_overlap_explicit);
    rb.run("the monad's context is preserved",          &tests_bridge_preserves_monad_context);

    // -- 0. structural traits + concepts ---------------------------------
    rb.module("0. structural traits + concepts",
              "functor_value_type, is_fmappable, Functor / fmappable_with");
    rb.run("functor_value_type: the inner type T",      &tests_functor_value_type);
    rb.run("the _t alias agrees",                       &tests_functor_value_type_t_alias);
    rb.run("the value-type helper soft-fails (SFINAE)", &tests_functor_value_type_helper_sfinae);
    rb.run("is_fmappable: mappable (functor, fn) pairs",&tests_is_fmappable_positive);
    rb.run("is_fmappable: finer-grained than is_functor",&tests_is_fmappable_negative);
    rb.run("is_fmappable does NOT require the marker",  &tests_is_fmappable_does_not_require_the_marker);
    rb.run("is_fmappable_v agrees",                     &tests_is_fmappable_v_agrees);
    rb.run("the Functor concept gates resolution",      &tests_functor_concept);
    rb.run("fmappable_with gates on the PAIR",          &tests_fmappable_with_concept);
    rb.run("the concept faces are gated to C++20",      &tests_concepts_gating);

    // -- II. functor_map -------------------------------------------------
    rb.module("II. functor_map (fmap)",
              "the one operation: the functor laws, the dual domain, genericity");
    rb.run("maps over a monad (the USAGE example)",     &tests_functor_map_over_a_monad);
    rb.run("the surrounding context is preserved",      &tests_functor_map_preserves_context);
    rb.run("maps over a view (lazy, composing)",        &tests_functor_map_over_a_view);
    rb.run("the inner type changes with the function",  &tests_functor_map_changes_inner_type);
    rb.run("law: identity",                             &tests_functor_map_law_identity);
    rb.run("law: composition",                          &tests_functor_map_law_composition);
    rb.run("the result type follows the instance's map",&tests_functor_map_result_type);
    rb.run("functor and function are forwarded",        &tests_functor_map_forwarding);
    rb.run("dual domain: folds constexpr, runs at runtime", &tests_functor_map_constexpr);
    rb.run("one call, generic over ANY functor",        &tests_functor_map_generic_over_any_functor);

    return rb.finish();
}
