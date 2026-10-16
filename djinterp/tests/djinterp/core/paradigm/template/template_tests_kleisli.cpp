/*******************************************************************************
* djinterp [core]                                     template_tests_kleisli.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_kleisli.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_kleisli.cpp
//   Section IV.vi-vii: kleisli sequencing.  Two ways to run one parser after
// another, both of which do the same two jobs - THREAD the remaining source
// forward, and SHORT-CIRCUIT on failure:
//
//     kleisli_then(P, Q)   -- the note's `Q <> P`.  Run P; on success feed P's
//                             leftover to Q and keep Q's result (P's result is
//                             discarded).  P and Q share tau and E; rho may
//                             differ.
//
//     kleisli_bind(P, k)   -- the note's `P >>= k`.  Run P; on success apply the
//                             continuation k to P's RESULT to choose the next
//                             parser, then run that on P's leftover.
//
//   The difference is what the second parser is allowed to depend on: under
// `then` it is fixed, under `bind` it is chosen from the first result.  The tests
// below make that observable rather than assuming it - the bind fixture folds P's
// digit into the value it produces, so the answer can only be right if the
// continuation genuinely saw it.
//
//   Both combinators must not merely REPORT failure but must actually skip the
// downstream work.  tests_kleisli_bind_short_circuits_on_first_failure pins that
// directly: the continuation's parser would produce a DIFFERENT error message, so
// seeing P's message proves k's parser never ran.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_kleisli_then_threads_the_remaining_source
  Verifies the leftover source is threaded from P into Q, which is what makes a
  sequence a sequence rather than two independent parses of the same input.
  Tests the following:
  - Q runs on what P did NOT consume
  - the final leftover is what BOTH consumed removed from the input
*/
bool
tests_kleisli_then_threads_the_remaining_source()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());

    // "7abc": P consumes '7' leaving "abc"; Q then consumes 'a' leaving "bc"
    auto outcome = sequenced(carrier("7abc"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 'a');       // Q saw "abc", not "7abc"
    ok = ok && (outcome.remaining() == "bc");  // both consumptions are reflected

    return ok;
}

/*
tests_kleisli_then_keeps_the_second_result
  Verifies `Q <> P` keeps Q's result and discards P's.
  Tests the following:
  - the outcome carries what Q produced
  - P's result is genuinely gone - the value is Q's even though P produced one
*/
bool
tests_kleisli_then_keeps_the_second_result()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());

    auto outcome = sequenced(carrier("7abc"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 'a');       // Q's result...
    ok = ok && (outcome.value() != '7');       // ...not P's

    return ok;
}

/*
tests_kleisli_then_result_type_is_the_second
  Verifies the outcome's TYPE is Q's, not P's - the two parsers here differ in rho
  (int vs char) precisely so this is observable.
  Tests the following:
  - the sequenced parser's outcome is Q's parse_outcome type
  - it shares tau and E with P, as the combinator requires
*/
bool
tests_kleisli_then_result_type_is_the_second()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());

    using outcome_t = decltype(sequenced(std::declval<const carrier&>()));

    static_assert(std::is_same<outcome_t, char_outcome>::value,
                  "the outcome is Q's, not P's");
    static_assert(std::is_same<outcome_t::result_type, char>::value,
                  "rho is Q's");
    static_assert(std::is_same<outcome_t::source_type,
                               int_outcome::source_type>::value,
                  "tau is shared");
    static_assert(std::is_same<outcome_t::error_type,
                               int_outcome::error_type>::value,
                  "E is shared");

    return true;
}

/*
tests_kleisli_then_short_circuits_on_first_failure
  Verifies P failing skips Q entirely and re-tags P's error as Q's outcome type.
  Tests the following:
  - the outcome is a failure
  - it carries P's error, so it was P that failed
  - the failure has Q's outcome type, so the error was re-tagged rather than the
    types being conflated
  - Q is not blamed: its own diagnostic is absent
*/
bool
tests_kleisli_then_short_circuits_on_first_failure()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());

    // "abc" has no leading digit, so P fails immediately
    auto outcome = sequenced(carrier("abc"));

    static_assert(std::is_same<decltype(outcome), char_outcome>::value,
                  "P's error is re-tagged into Q's outcome type");

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "expected digit");   // P's diagnostic
    ok = ok && (outcome.error() != "expected alpha");   // NOT Q's - Q never ran

    return ok;
}

/*
tests_kleisli_then_propagates_second_failure
  Verifies Q's failure surfaces when P succeeded.
  Tests the following:
  - the outcome is a failure carrying Q's diagnostic
  - so P's success did not mask it
*/
bool
tests_kleisli_then_propagates_second_failure()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());

    // "77c": P consumes '7' leaving "7c"; Q wants a letter and finds '7'
    auto outcome = sequenced(carrier("77c"));

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "expected alpha");   // Q's diagnostic

    return ok;
}

/*
tests_kleisli_then_nests
  Verifies a sequenced parser is itself a parser, so sequencing composes.
  Tests the following:
  - then(then(P, Q), Q) runs all three in order, threading the source throughout
  - a failure anywhere in the chain short-circuits the rest
*/
bool
tests_kleisli_then_nests()
{
    auto three = kleisli_then(kleisli_then(digit_p(), alpha_p()), alpha_p());

    // "7ab!": P takes '7', Q takes 'a', Q again takes 'b', leaving "!"
    auto outcome = three(carrier("7ab!"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 'b');       // the LAST parser's result
    ok = ok && (outcome.remaining() == "!");

    // a failure in the middle stops the chain
    auto broken = three(carrier("79b!"));      // the second stage wants a letter

    ok = ok && (broken.is_err());
    ok = ok && (broken.error() == "expected alpha");

    return ok;
}

/*
tests_kleisli_bind_passes_the_result_to_the_continuation
  Verifies `P >>= k` applies k to P's RESULT - the property that distinguishes bind
  from then.  The continuation here folds P's digit into the value its parser
  produces, so the final answer is only correct if k genuinely received it.
  Tests the following:
  - "42xyz" yields 42, which requires k to have seen the 4 that P produced
  - a different first digit changes the answer accordingly, so k is reading the
    result rather than having closed over a constant
*/
bool
tests_kleisli_bind_passes_the_result_to_the_continuation()
{
    auto bound = kleisli_bind(digit_p(), fold_continuation());

    // P produces 4; k builds a parser carrying 4; that parser reads '2' -> 4*10+2
    auto outcome = bound(carrier("42xyz"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 42);        // 42, not 2 -- k saw P's result

    // change P's result and the answer tracks it
    auto other = bound(carrier("71xyz"));

    ok = ok && (other.is_ok());
    ok = ok && (other.value() == 71);

    return ok;
}

/*
tests_kleisli_bind_threads_the_remaining_source
  Verifies the continuation's parser runs on P's leftover, not on the original
  input.
  Tests the following:
  - the second digit consumed is the one AFTER the first
  - the final leftover reflects both consumptions
*/
bool
tests_kleisli_bind_threads_the_remaining_source()
{
    auto bound = kleisli_bind(digit_p(), fold_continuation());

    auto outcome = bound(carrier("42xyz"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 42);          // the '2' came from position 1
    ok = ok && (outcome.remaining() == "xyz");   // both digits are gone

    return ok;
}

/*
tests_kleisli_bind_short_circuits_on_first_failure
  Verifies P failing means the continuation NEVER RUNS - not merely that the
  outcome reports a failure.  The continuation's parser has its own distinct
  diagnostic, so seeing P's diagnostic instead proves k's parser was never invoked.
  Tests the following:
  - the outcome is a failure carrying P's diagnostic
  - it does NOT carry the continuation's parser's diagnostic, which is what would
    surface had k run on an empty or unconsumed source
*/
bool
tests_kleisli_bind_short_circuits_on_first_failure()
{
    auto bound = kleisli_bind(digit_p(), fold_continuation());

    // "xyz" has no leading digit: P fails, so k must never be applied
    auto outcome = bound(carrier("xyz"));

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "expected digit");          // P's diagnostic
    ok = ok && (outcome.error() != "expected second digit");   // k's parser never ran

    return ok;
}

/*
tests_kleisli_bind_propagates_continuation_failure
  Verifies the continuation's parser failing surfaces as the outcome's failure.
  Tests the following:
  - P succeeds but the continuation's parser cannot proceed
  - the outcome carries the continuation's parser's diagnostic, not P's
*/
bool
tests_kleisli_bind_propagates_continuation_failure()
{
    auto bound = kleisli_bind(digit_p(), fold_continuation());

    // "4xy": P takes '4'; the continuation's parser wants a second digit and
    // finds 'x'
    auto outcome = bound(carrier("4xy"));

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "expected second digit");   // k's parser failed
    ok = ok && (outcome.error() != "expected digit");          // P did not

    return ok;
}

/*
tests_kleisli_helpers_are_source_transformers
  Verifies a sequenced parser is an F_t in the module's own terms - which is what
  lets it be nested, evaluated, or handed to a template_system like any other
  source-transformer.
  Tests the following:
  - both combinators' results satisfy is_source_transformer over the carrier
  - their sinks are detectable and are parse_outcomes
  - a sequenced parser can be driven through evaluate
*/
bool
tests_kleisli_helpers_are_source_transformers()
{
    auto sequenced = kleisli_then(digit_p(), alpha_p());
    auto bound     = kleisli_bind(digit_p(), fold_continuation());

    using then_t = decltype(sequenced);
    using bind_t = decltype(bound);

    static_assert(is_source_transformer<then_t, carrier>::value,
                  "a `then` chain is a source-transformer");
    static_assert(is_source_transformer<bind_t, carrier>::value,
                  "a `bind` chain is a source-transformer");

    static_assert(is_parse_outcome<
                      source_transformer_sink_t<then_t, carrier>>::value,
                  "and its sink is a parse_outcome");
    static_assert(is_parse_outcome<
                      source_transformer_sink_t<bind_t, carrier>>::value,
                  "for both");

    // and they really are drivable through ev
    auto via_evaluate = evaluate(sequenced, carrier("7abc"));

    bool ok = true;

    ok = ok && (via_evaluate.is_ok());
    ok = ok && (via_evaluate.value() == 'a');

    return ok;
}

/*
tests_kleisli_decays_its_arguments
  Verifies P, Q and k are stored decayed (clean_t), so the combinators own them.
  Tests the following:
  - passing lvalues and rvalues produces the same helper type
  - that type is the internal helper over the decayed arguments
  - a chain built from temporaries still works after they are gone
*/
bool
tests_kleisli_decays_its_arguments()
{
    digit_p           p;
    alpha_p           q;
    fold_continuation k;

    auto then_from_lvalues = kleisli_then(p, q);
    auto then_from_rvalues = kleisli_then(digit_p(), alpha_p());

    static_assert(std::is_same<decltype(then_from_lvalues),
                               decltype(then_from_rvalues)>::value,
                  "lvalue and rvalue parsers decay to the same helper");
    static_assert(std::is_same<decltype(then_from_rvalues),
                               internal::kleisli_then_helper<digit_p, alpha_p>>::value,
                  "and that type is kleisli_then_helper<clean_t<P>, clean_t<Q>>");

    auto bind_from_lvalues = kleisli_bind(p, k);
    auto bind_from_rvalues = kleisli_bind(digit_p(), fold_continuation());

    static_assert(std::is_same<decltype(bind_from_lvalues),
                               decltype(bind_from_rvalues)>::value,
                  "the same holds for bind");
    static_assert(std::is_same<decltype(bind_from_rvalues),
                               internal::kleisli_bind_helper<digit_p,
                                                             fold_continuation>>::value,
                  "kleisli_bind_helper<clean_t<P>, clean_t<K>>");

    // the temporaries are gone; the chains hold their own copies
    bool ok = true;

    ok = ok && (then_from_rvalues(carrier("7abc")).value() == 'a');
    ok = ok && (bind_from_rvalues(carrier("42xyz")).value() == 42);
    ok = ok && (then_from_lvalues(carrier("7abc")).value() ==
                then_from_rvalues(carrier("7abc")).value());

    return ok;
}


NS_END  // testing
NS_END  // djinterp
