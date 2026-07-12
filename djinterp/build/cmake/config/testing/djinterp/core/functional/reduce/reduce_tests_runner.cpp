// djinterp [test]  reduce_tests_runner.cpp
//   Entry point for the reduce.hpp suite: registers every section's tests with
//   report_builder, one module per section.

// djinterp
#include "reduce_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- reduce.hpp Test Suite");
    rb.set_subtitle("functional : the drivers -- the iteration half of the "
                    "step/driver split");
    rb.set_author("teer");
    rb.set_description(
        "The header's claim is that one pure reducer, written once, is run by "
        "either driver across all three domains -- and that only the driver (loop "
        "vs. recursion) and the leaf differ. The suite puts that on trial rather "
        "than taking it on faith: count_all is a SINGLE reducer body handed "
        "unchanged to reduce_rt, to reduce_ct over a value_list, and to reduce_ct "
        "over a tuple's element types, and it counts correctly in all three. "
        "sum_with<Leaf> is one body with three leaves, which is the only thing "
        "that changes between the domains. A non-commutative reducer pins the "
        "LEFT fold three times over, once per domain. Where the drivers genuinely "
        "part company is the accumulator: reduce_rt assigns into it and returns "
        "_Acc, so its TYPE IS FIXED -- a reducer returning a long still comes back "
        "an int -- while both reduce_ct overloads recompute the type at every step, "
        "so it may EVOLVE into a growing value_list. The two type-domain entries "
        "differ too: the preferred type_c form walks element TYPES and folds a "
        "tuple of NON-default-constructible elements, which the convenience value "
        "form cannot even be called on. The whole module is gated to C++17, so the "
        "suite is gated to match -- and that it still COMPILES under C++11 and "
        "C++14, having included the header, is the check that the header is inert "
        "below its tier. Verified under C++11, C++14, C++17, and C++20.");

    rb.use_pdf("reduce_tests_report.pdf");

    // -- I. reduce_rt ------------------------------------------------------
    rb.module("I. reduce_rt (the runtime driver)",
              "the iterator-range form, the iterable form, and constexpr folding");
    rb.run("the iterator-range form",                    &tests_rt_range_form);
    rb.run("the iterable convenience form",              &tests_rt_iterable_form);
    rb.run("the two forms agree",                        &tests_rt_both_forms_agree);
    rb.run("an empty range returns the seed",            &tests_rt_empty_returns_the_seed);
    rb.run("it is a LEFT fold",                          &tests_rt_is_a_left_fold);
    rb.run("a single element",                           &tests_rt_single_element);
    rb.run("folds constexpr over a constexpr range",     &tests_rt_constexpr_over_a_constexpr_range);
    rb.run("the accumulator type is FIXED",              &tests_rt_accumulator_type_is_fixed);
    rb.run("it pulls a lazy, generated source",          &tests_rt_pulls_a_lazy_generated_source);
    rb.run("it finds begin/end by ADL",                  &tests_rt_finds_begin_end_by_adl);
    rb.run("each element is visited exactly once",       &tests_rt_visits_each_element_once);
    rb.run("the source is not consumed",                 &tests_rt_does_not_consume_its_source);

    // -- II. reduce_ct: the value domain -----------------------------------
    rb.module("II. reduce_ct (the value domain)",
              "value_list<auto...> -- the unified entry over value_list's own fold");
    rb.run("it folds an NTTP sequence",                  &tests_ct_value_list_folds);
    rb.run("an empty list returns the seed",             &tests_ct_value_list_empty_returns_the_seed);
    rb.run("it is a LEFT fold",                          &tests_ct_value_list_is_a_left_fold);
    rb.run("it delegates to value_list's fold",          &tests_ct_value_list_delegates_to_fold);
    rb.run("the accumulator type may EVOLVE",            &tests_ct_value_list_accumulator_may_evolve);
    rb.run("a heterogeneous pack",                       &tests_ct_value_list_heterogeneous);
    rb.run("a single element",                           &tests_ct_value_list_single_element);
    rb.run("each element is visited exactly once",       &tests_ct_value_list_visits_each_element_once);

    // -- III. reduce_ct: the type domain -----------------------------------
    rb.module("III. reduce_ct (the type domain)",
              "std::tuple -- the recursion walks element TYPES as type_c carriers");
    rb.run("the type_c entry (preferred)",               &tests_ct_tuple_type_c_entry);
    rb.run("the value entry (convenience)",              &tests_ct_tuple_value_entry);
    rb.run("the two entries agree",                      &tests_ct_tuple_both_entries_agree);
    rb.run("the empty tuple returns the seed",           &tests_ct_tuple_empty_returns_the_seed);
    rb.run("it is a LEFT fold over the types",           &tests_ct_tuple_is_a_left_fold);
    rb.run("it walks TYPES, not values (no tuple built)",&tests_ct_tuple_walks_types_not_values);
    rb.run("each type arrives as a type_c carrier",      &tests_ct_tuple_feeds_each_type_as_a_carrier);
    rb.run("duplicate types are separate positions",     &tests_ct_tuple_duplicate_types);
    rb.run("the accumulator type may EVOLVE",            &tests_ct_tuple_accumulator_may_evolve);
    rb.run("a single element",                           &tests_ct_tuple_single_element);
    rb.run("heterogeneous element types",                &tests_ct_tuple_heterogeneous_sizes);
    rb.run("each element is visited exactly once",       &tests_ct_tuple_visits_each_element_once);

    // -- IV. the step/driver split -----------------------------------------
    rb.module("IV. the step/driver split",
              "one reducer body, three domains -- and where the drivers part company");
    rb.run("ONE reducer body, THREE domains",            &tests_one_reducer_body_three_domains);
    rb.run("only the driver and the LEAF differ",        &tests_only_the_driver_and_the_leaf_differ);
    rb.run("rt FIXES the accumulator; ct lets it EVOLVE",&tests_the_drivers_differ_on_the_accumulator);
    rb.run("the drivers agree on the same data",         &tests_the_drivers_agree_on_the_same_data);
    rb.run("the drivers are unconstrained (any callable)",&tests_the_drivers_are_unconstrained);
    rb.run("the module is gated to its C++17 tier",      &tests_module_tier_gate);

    return rb.finish();
}
