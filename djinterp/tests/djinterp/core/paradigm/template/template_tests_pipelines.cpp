/*******************************************************************************
* djinterp [core]                                   template_tests_pipelines.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_pipelines.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_pipelines.cpp
//   Section III: stage pipelines.  Two conventions for building a transformation
// out of smaller pieces, and the difference between them is the whole point:
//
//     stages(f_1, f_2, ... f_n)         F = f_n . ... . f_1
//         only f_1 is binary - it alone consumes the (template, source) product.
//         The template is CONSUMED at stage 1 and is invisible thereafter.
//
//     reader_stages(f_1, ... f_n)       F_t = (f_n)_t . ... . (f_1)_t
//         every stage is binary (template, x) -> y.  The template is AMBIENT and
//         threaded into all of them.
//
//   tests_reader_stages_threads_the_template is the test that actually separates
// the two: it uses a second stage that can only produce the right answer if it
// still sees the template, which under `stages` it could not.
//
//   Note on argument deduction: stage_chain_helper::operator() deduces a SINGLE
// _Tau from both of its arguments, which is the homogeneity the algebra rests on
// (a template and a source are both inhabitants of tau).  The tests therefore
// pass explicit carrier values rather than string literals, whose array types
// would differ by length and fail to deduce.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_stages_binary_stage_only
  Verifies a chain with no unary stages at all: the tail is the empty
  unary_chain_helper, which is the identity.
  Tests the following:
  - stages(f_1) applies f_1 and returns its result unchanged
*/
bool
tests_stages_binary_stage_only()
{
    auto pipeline = stages(concat());

    const carrier t = "greet";
    const carrier a = "world";

    bool ok = true;

    ok = ok && (pipeline(t, a) == "greet:world");
    ok = ok && (pipeline(t, a) == concat()(t, a));   // the identity tail changes nothing

    return ok;
}

/*
tests_stages_one_unary_stage
  Verifies a single unary stage runs on the binary stage's output.
  Tests the following:
  - stages(f_1, f_2)(t, alpha) is f_2(f_1(t, alpha))
*/
bool
tests_stages_one_unary_stage()
{
    auto pipeline = stages(concat(), exclaim());

    const carrier t = "greet";
    const carrier a = "world";

    return (pipeline(t, a) == "greet:world!");
}

/*
tests_stages_many_unary_stages
  Verifies a longer chain, with the type changing from stage to stage - the tail
  is not required to stay in the carrier.
  Tests the following:
  - concat (string), then measure (size_t), then twice (size_t) composes
  - the chain's sink is the LAST stage's result type
*/
bool
tests_stages_many_unary_stages()
{
    auto pipeline = stages(concat(), exclaim(), measure(), twice());

    const carrier t = "ab";
    const carrier a = "cde";

    // "ab" ":" "cde" -> "ab:cde" (6) -> "ab:cde!" (7) -> 7 -> 14
    bool ok = (pipeline(t, a) == 14u);

    static_assert(std::is_same<transformation_sink_t<decltype(pipeline), carrier>,
                               std::size_t>::value,
                  "the chain's sink is the last stage's result type");

    return ok;
}

/*
tests_stages_composition_order
  Verifies the chain runs LEFT TO RIGHT: f_1 first, f_n last.  Two stages that do
  not commute make the order observable - if they were applied in the other order
  the answer would differ.
  Tests the following:
  - stages(f_1, exclaim, measure) counts the '!' that exclaim added, so exclaim
    ran BEFORE measure
  - the reversed spelling gives a different answer, so the ordering is real and
    not an accident of the fixtures
*/
bool
tests_stages_composition_order()
{
    auto exclaim_then_measure = stages(concat(), exclaim(), measure());

    const carrier t = "ab";
    const carrier a = "cd";

    // "ab:cd" is 5 chars; exclaim first -> "ab:cd!" -> 6
    bool ok = (exclaim_then_measure(t, a) == 6u);

    // and the other order is observably different: measure first -> 5, then a
    // unary stage on the size
    auto measure_then_twice = stages(concat(), measure(), twice());

    ok = ok && (measure_then_twice(t, a) == 10u);   // 5 * 2, NOT 6 * 2
    ok = ok && (measure_then_twice(t, a) != 12u);

    return ok;
}

/*
tests_stages_is_a_transformation
  Verifies the assembled chain is itself an F - that is the contract that makes it
  pluggable anywhere a transformation is wanted.
  Tests the following:
  - the chain satisfies is_transformation over the carrier
  - its sink is detectable
*/
bool
tests_stages_is_a_transformation()
{
    auto pipeline = stages(concat(), exclaim());

    using pipeline_t = decltype(pipeline);

    static_assert(is_transformation<pipeline_t, carrier>::value,
                  "a stage chain is a transformation");
    static_assert(!std::is_same<transformation_sink_t<pipeline_t, carrier>,
                                nonesuch>::value,
                  "so its sink is detectable");

    return is_transformation<pipeline_t, carrier>::value;
}

/*
tests_stages_plugs_into_template_system
  Verifies the chain really is usable as the F of a system, and that the
  factorization identity still holds when F is a pipeline.
  Tests the following:
  - a system over a stage chain applies it correctly
  - apply(t, alpha) == evaluate(instantiate(t), alpha) for that system
*/
bool
tests_stages_plugs_into_template_system()
{
    auto pipeline = stages(concat(), measure(), twice());

    auto sys = make_template_system<carrier>(pipeline);

    const carrier t = "ab";
    const carrier a = "cde";

    bool ok = true;

    ok = ok && (sys.apply(t, a) == 12u);                                // 6 * 2
    ok = ok && (sys.apply(t, a) == evaluate(sys.instantiate(t), a));    // identity holds
    ok = ok && (std::is_same<decltype(sys)::sink_type, std::size_t>::value);

    return ok;
}

/*
tests_stages_decays_its_arguments
  Verifies the stages are stored decayed (clean_t), so the chain owns them.
  Tests the following:
  - passing lvalues and rvalues produces the same chain type
  - a chain built from temporaries still works after they are gone
*/
bool
tests_stages_decays_its_arguments()
{
    concat  first;
    exclaim second;

    auto from_lvalues = stages(first, second);
    auto from_rvalues = stages(concat(), exclaim());

    static_assert(std::is_same<decltype(from_lvalues),
                               decltype(from_rvalues)>::value,
                  "lvalue and rvalue stages decay to the same chain");
    static_assert(std::is_same<decltype(from_rvalues),
                               internal::stage_chain_helper<concat, exclaim>>::value,
                  "and that type is stage_chain_helper<clean_t<...>...>");

    const carrier t = "greet";
    const carrier a = "world";

    return (from_rvalues(t, a) == "greet:world!") &&
           (from_lvalues(t, a) == from_rvalues(t, a));
}

/*
tests_reader_stages_single_stage
  Verifies the one-stage Reader chain, which is just the stage itself.
  Tests the following:
  - reader_stages(f)(t, alpha) is f(t, alpha)
*/
bool
tests_reader_stages_single_stage()
{
    auto pipeline = reader_stages(wrap());

    const carrier t = "T";
    const carrier a = "A";

    return (pipeline(t, a) == "T<A>");
}

/*
tests_reader_stages_two_stages
  Verifies the two-stage Reader chain.
  Tests the following:
  - reader_stages(f_1, f_2)(t, alpha) is f_2(t, f_1(t, alpha))
*/
bool
tests_reader_stages_two_stages()
{
    auto pipeline = reader_stages(wrap(), suffix_template());

    const carrier t = "T";
    const carrier a = "A";

    // f_1: "T" + "<A>"  -> "T<A>"
    // f_2: "T<A>" + "/" + "T" -> "T<A>/T"      <- f_2 still sees the template
    return (pipeline(t, a) == "T<A>/T");
}

/*
tests_reader_stages_threads_the_template
  Verifies the DEFINING property of the Reader convention, and the one thing that
  distinguishes it from `stages`: the template is ambient, so EVERY stage sees it
  - not just the first.  suffix_template appends the template to its input, which
  it could only do if the template were still available at stage 2; under
  `stages`, where the template is consumed at stage 1, that information is gone.
  Tests the following:
  - the second stage's output contains the template
  - changing the template changes the second stage's contribution, proving the
    stage is reading it rather than having closed over a constant
  - a three-stage chain threads the template all the way to the end
*/
bool
tests_reader_stages_threads_the_template()
{
    auto pipeline = reader_stages(wrap(), suffix_template());

    bool ok = true;

    // stage 2 appends the template, so the template must have reached it
    ok = ok && (pipeline(carrier("T"), carrier("A")) == "T<A>/T");
    ok = ok && (pipeline(carrier("Z"), carrier("A")) == "Z<A>/Z");   // it tracks t

    // three stages: the template must still be visible at the last one
    auto deep = reader_stages(wrap(), wrap(), suffix_template());

    // f_1: "T<A>" ; f_2: "T<T<A>>" ; f_3: "T<T<A>>/T"
    ok = ok && (deep(carrier("T"), carrier("A")) == "T<T<A>>/T");

    return ok;
}

/*
tests_reader_stages_is_a_transformation
  Verifies the Reader chain is an F as well, so both conventions produce something
  pluggable as the F of a system.
  Tests the following:
  - the chain satisfies is_transformation over the carrier
  - it plugs into a template_system and the identity holds
*/
bool
tests_reader_stages_is_a_transformation()
{
    auto pipeline = reader_stages(wrap(), suffix_template());

    using pipeline_t = decltype(pipeline);

    static_assert(is_transformation<pipeline_t, carrier>::value,
                  "a Reader chain is a transformation");

    auto sys = make_template_system<carrier>(pipeline);

    const carrier t = "T";
    const carrier a = "A";

    bool ok = true;

    ok = ok && (sys.apply(t, a) == "T<A>/T");
    ok = ok && (sys.apply(t, a) == evaluate(sys.instantiate(t), a));

    return ok;
}

/*
tests_pipelines_are_constexpr
  Verifies both chain kinds fold in a constant expression - every stage helper
  carries D_CONSTEXPR.  The carrier is int here, since std::string is not a
  literal type before C++20.
  Tests the following:
  - a stages() chain with an empty tail folds
  - a stages() chain with several unary stages folds
  - a reader_stages() chain folds, with the template threaded through
*/
bool
tests_pipelines_are_constexpr()
{
    // identity tail
    static_assert(stages(mul())(6, 7) == 42, "an empty unary tail folds");

    // (6 * 7) + 1 = 43, doubled = 86
    D_CONSTEXPR auto pipeline = stages(mul(), add_one(), doubler());

    static_assert(pipeline(6, 7) == 86, "a stage chain folds");

    // reader: f_1(t=10, x=1) = 11 ; f_2(t=10, x=11) = 21
    D_CONSTEXPR auto reader = reader_stages(reader_add(), reader_add());

    static_assert(reader(10, 1) == 21,
                  "a Reader chain folds, with t threaded to both stages");

    return (pipeline(6, 7) == 86) && (reader(10, 1) == 21);
}


NS_END  // testing
NS_END  // djinterp
