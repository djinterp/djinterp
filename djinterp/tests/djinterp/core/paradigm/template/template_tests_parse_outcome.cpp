/*******************************************************************************
* djinterp [core]                               template_tests_parse_outcome.cpp
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests_parse_outcome.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : template_tests_parse_outcome.cpp
//   Section IV.i-v: the parser sink.  Nothing in the schema changes for parsers -
// only the choice of sigma, which is pinned to the sum
//
//       sigma  =  (rho x tau) + E
//
// a result rho paired with the remaining source tau on success, or an error E.
// `parse_outcome` is that sink, `parse_success` / `parse_failure` build it,
// `is_parse_outcome` recognizes it, and `parser_system` is nothing more than a
// template_system with sigma pinned to it.
//
//   The storage is deliberately DIDACTIC rather than a true sum: all three
// components share the object and the inactive ones are value-initialized (which
// is why rho, tau and E must all be default-constructible).  That is an
// observable design choice, not an accident, so
// tests_parse_outcome_failure_value_initializes_inactive pins it explicitly - a
// failed outcome reports a value-initialized value() and remaining() rather than
// garbage or undefined behaviour.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "template_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_parse_success_construction
  Verifies the success factory.
  Tests the following:
  - the outcome carries the result and the remaining source
  - it reports success
  - its type is parse_outcome<clean_t<rho>, clean_t<tau>, E>
*/
bool
tests_parse_success_construction()
{
    auto outcome = parse_success<err_type>(42, carrier("rest"));

    static_assert(std::is_same<decltype(outcome), int_outcome>::value,
                  "the result and source types are decayed, E is explicit");

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 42);
    ok = ok && (outcome.remaining() == "rest");

    return ok;
}

/*
tests_parse_failure_construction
  Verifies the failure factory.
  Tests the following:
  - the outcome carries the error
  - it reports failure
  - its type is parse_outcome<rho, tau, clean_t<E>>
*/
bool
tests_parse_failure_construction()
{
    auto outcome = parse_failure<int, carrier>(err_type("boom"));

    static_assert(std::is_same<decltype(outcome), int_outcome>::value,
                  "rho and tau are explicit, E is decayed");

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "boom");

    return ok;
}

/*
tests_parse_outcome_is_ok_and_is_err
  Verifies the two arms are exclusive and exhaustive.
  Tests the following:
  - a success is ok and not err
  - a failure is err and not ok
  - is_err is exactly the negation of is_ok in both cases
*/
bool
tests_parse_outcome_is_ok_and_is_err()
{
    auto good = parse_success<err_type>(1, carrier("x"));
    auto bad  = parse_failure<int, carrier>(err_type("e"));

    bool ok = true;

    ok = ok && ( good.is_ok() && !good.is_err());
    ok = ok && (!bad.is_ok()  &&  bad.is_err());

    ok = ok && (good.is_err() == !good.is_ok());
    ok = ok && (bad.is_err()  == !bad.is_ok());

    return ok;
}

/*
tests_parse_outcome_operator_bool
  Verifies truthiness follows the ok arm.
  Tests the following:
  - a success is true, a failure is false
  - the conversion agrees with is_ok
*/
bool
tests_parse_outcome_operator_bool()
{
    auto good = parse_success<err_type>(1, carrier("x"));
    auto bad  = parse_failure<int, carrier>(err_type("e"));

    bool ok = true;

    ok = ok && (static_cast<bool>(good));
    ok = ok && (!static_cast<bool>(bad));
    ok = ok && (static_cast<bool>(good) == good.is_ok());
    ok = ok && (static_cast<bool>(bad) == bad.is_ok());

    // and it reads naturally in a condition, which is the point of having it
    if (good)
    {
        ok = ok && (good.value() == 1);
    }
    else
    {
        ok = false;
    }

    return ok;
}

/*
tests_parse_outcome_operator_bool_is_explicit
  Verifies the conversion is explicit, so an outcome does not silently decay into
  a bool where a value was meant.
  Tests the following:
  - parse_outcome is NOT implicitly convertible to bool
  - but it is explicitly convertible (contextual conversion still works)
*/
bool
tests_parse_outcome_operator_bool_is_explicit()
{
    static_assert(!std::is_convertible<int_outcome, bool>::value,
                  "operator bool is explicit");
    static_assert(std::is_constructible<bool, int_outcome>::value,
                  "but the explicit conversion is available");

    return ( !std::is_convertible<int_outcome, bool>::value &&
              std::is_constructible<bool, int_outcome>::value );
}

/*
tests_parse_outcome_value_and_remaining
  Verifies the success payload: the produced result rho and the leftover source
  tau that gets threaded to the next stage.
  Tests the following:
  - value() is the result, by const reference
  - remaining() is the leftover, by const reference
  - a parser that consumes one character leaves exactly the rest
*/
bool
tests_parse_outcome_value_and_remaining()
{
    static_assert(std::is_same<decltype(std::declval<const int_outcome&>().value()),
                               const int&>::value,
                  "value() yields a const reference to rho");
    static_assert(std::is_same<decltype(std::declval<const int_outcome&>().remaining()),
                               const carrier&>::value,
                  "remaining() yields a const reference to tau");

    auto outcome = digit_p()(carrier("7abc"));

    bool ok = true;

    ok = ok && (outcome.is_ok());
    ok = ok && (outcome.value() == 7);
    ok = ok && (outcome.remaining() == "abc");    // exactly what was not consumed

    return ok;
}

/*
tests_parse_outcome_error
  Verifies the failure payload.
  Tests the following:
  - error() is the E, by const reference
  - a parser that cannot proceed reports its own diagnostic
*/
bool
tests_parse_outcome_error()
{
    static_assert(std::is_same<decltype(std::declval<const int_outcome&>().error()),
                               const err_type&>::value,
                  "error() yields a const reference to E");

    auto outcome = digit_p()(carrier("abc"));

    bool ok = true;

    ok = ok && (outcome.is_err());
    ok = ok && (outcome.error() == "expected digit");

    return ok;
}

/*
tests_parse_outcome_failure_value_initializes_inactive
  Verifies the didactic storage discipline the module documents: because all three
  components share the object, the INACTIVE ones are value-initialized rather than
  left indeterminate.  Reading value() or remaining() on a failed outcome is
  therefore well-defined (it is meaningless, but it is not undefined), and reading
  error() on a success is likewise.
  Tests the following:
  - a failed outcome's value() is a value-initialized rho and remaining() a
    value-initialized tau
  - a successful outcome's error() is a value-initialized E
*/
bool
tests_parse_outcome_failure_value_initializes_inactive()
{
    auto bad = parse_failure<int, carrier>(err_type("boom"));

    bool ok = true;

    ok = ok && (bad.is_err());
    ok = ok && (bad.value() == 0);            // value-initialized int
    ok = ok && (bad.remaining().empty());     // value-initialized string

    auto good = parse_success<err_type>(42, carrier("rest"));

    ok = ok && (good.is_ok());
    ok = ok && (good.error().empty());        // value-initialized E

    return ok;
}

/*
tests_parse_outcome_typedefs
  Verifies the sink advertises its three components.
  Tests the following:
  - result_type, source_type and error_type name rho, tau and E
  - two outcomes over the same tau and E but different rho differ only there,
    which is exactly the shape kleisli_then is specified for
*/
bool
tests_parse_outcome_typedefs()
{
    static_assert(std::is_same<int_outcome::result_type, int>::value,
                  "result_type is rho");
    static_assert(std::is_same<int_outcome::source_type, carrier>::value,
                  "source_type is tau");
    static_assert(std::is_same<int_outcome::error_type, err_type>::value,
                  "error_type is E");

    static_assert(std::is_same<char_outcome::result_type, char>::value,
                  "a second sink may differ in rho...");
    static_assert(std::is_same<char_outcome::source_type,
                               int_outcome::source_type>::value,
                  "...while sharing tau...");
    static_assert(std::is_same<char_outcome::error_type,
                               int_outcome::error_type>::value,
                  "...and E");

    return true;
}

/*
tests_parse_success_error_type_is_explicit
  Verifies E must be named on success.  On a successful parse no E value is
  present, so there is nothing to deduce it from - the module makes it an explicit
  template argument rather than guessing.
  Tests the following:
  - naming E selects the sink's error type
  - the same result and remaining under a different E produce a different sink
*/
bool
tests_parse_success_error_type_is_explicit()
{
    auto with_string_error = parse_success<err_type>(1, carrier("x"));
    auto with_int_error    = parse_success<int>(1, carrier("x"));

    static_assert(std::is_same<decltype(with_string_error)::error_type,
                               err_type>::value,
                  "E is whatever was named");
    static_assert(std::is_same<decltype(with_int_error)::error_type, int>::value,
                  "and naming a different E gives a different sink");
    static_assert(!std::is_same<decltype(with_string_error),
                                decltype(with_int_error)>::value,
                  "so the two sinks are distinct types");

    return with_string_error.is_ok() && with_int_error.is_ok();
}

/*
tests_parse_failure_result_and_source_are_explicit
  Verifies rho and tau must be named on failure, for the mirror-image reason: on a
  failed parse neither a result nor a leftover source exists to deduce them from.
  Tests the following:
  - naming rho and tau selects the sink
  - only E is deduced, from the error argument
*/
bool
tests_parse_failure_result_and_source_are_explicit()
{
    auto as_int  = parse_failure<int, carrier>(err_type("e"));
    auto as_char = parse_failure<char, carrier>(err_type("e"));

    static_assert(std::is_same<decltype(as_int), int_outcome>::value,
                  "rho and tau are named");
    static_assert(std::is_same<decltype(as_char), char_outcome>::value,
                  "naming a different rho gives a different sink");
    static_assert(std::is_same<decltype(as_int)::error_type, err_type>::value,
                  "E is deduced from the error argument (and decayed)");

    return as_int.is_err() && as_char.is_err();
}

/*
tests_is_parse_outcome
  Verifies the sink is recognizable as such.
  Tests the following:
  - every parse_outcome specialization is recognized
  - unrelated types are not
*/
bool
tests_is_parse_outcome()
{
    static_assert(is_parse_outcome<int_outcome>::value,
                  "a parse_outcome is recognized");
    static_assert(is_parse_outcome<char_outcome>::value,
                  "for any specialization");
    static_assert(!is_parse_outcome<int>::value,
                  "an unrelated type is not");
    static_assert(!is_parse_outcome<carrier>::value,
                  "nor is the carrier");
    static_assert(!is_parse_outcome<concat>::value,
                  "nor is a transformation");

    return ( is_parse_outcome<int_outcome>::value &&
            !is_parse_outcome<int>::value );
}

/*
tests_is_parse_outcome_v_agrees_with_trait
  Verifies the variable-template shorthand tracks the trait.
  Tests the following:
  - is_parse_outcome_v equals is_parse_outcome<...>::value, positive and negative
*/
bool
tests_is_parse_outcome_v_agrees_with_trait()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_parse_outcome_v<int_outcome> ==
                  is_parse_outcome<int_outcome>::value,
                  "_v tracks the trait (positive)");
    static_assert(is_parse_outcome_v<int> == is_parse_outcome<int>::value,
                  "_v tracks the trait (negative)");

    return (is_parse_outcome_v<int_outcome> && !is_parse_outcome_v<int>);
#else
    return true;
#endif
}

/*
tests_parser_system_pins_the_sink
  Verifies parser_system is exactly a template_system with sigma pinned to the
  parser shape - not a new kind of thing.
  Tests the following:
  - the alias names the corresponding template_system
  - its sink_type is parse_outcome<rho, tau, E>
  - the carrier and transformation are unchanged
*/
bool
tests_parser_system_pins_the_sink()
{
    using sys_t = parser_system<digit_grammar, carrier, int, err_type>;

    static_assert(std::is_same<sys_t,
                               template_system<digit_grammar, carrier,
                                               int_outcome>>::value,
                  "parser_system IS a template_system with sigma pinned");
    static_assert(std::is_same<sys_t::sink_type, int_outcome>::value,
                  "sigma is the parser sink (rho x tau) + E");
    static_assert(std::is_same<sys_t::carrier_type, carrier>::value,
                  "the carrier is unchanged");
    static_assert(std::is_same<sys_t::transformation_type, digit_grammar>::value,
                  "and so is F");

    return true;
}

/*
tests_parser_system_apply_and_instantiate
  Verifies a parser_system behaves as a system: the template is the grammar, the
  source is the input, and instantiate yields the instantiated parser P_t.
  Tests the following:
  - apply(grammar, input) runs the parse
  - instantiate(grammar) yields a P_t : tau -> parse_outcome
  - the factorization identity holds for parsers exactly as for any other F
  - a failing input produces a failing outcome
*/
bool
tests_parser_system_apply_and_instantiate()
{
    parser_system<digit_grammar, carrier, int, err_type> sys{ digit_grammar() };

    const carrier grammar = "digit";
    const carrier input   = "7abc";

    auto direct = sys.apply(grammar, input);

    bool ok = true;

    ok = ok && (direct.is_ok());
    ok = ok && (direct.value() == 7);
    ok = ok && (direct.remaining() == "abc");

    // P_t is an instantiated parser - a source-transformer
    auto parser = sys.instantiate(grammar);

    static_assert(is_source_transformer<decltype(parser), carrier>::value,
                  "an instantiated parser is a source-transformer");

    auto via_identity = evaluate(parser, input);

    ok = ok && (via_identity.is_ok());
    ok = ok && (via_identity.value() == direct.value());
    ok = ok && (via_identity.remaining() == direct.remaining());

    // failure travels through the system unchanged
    auto bad = sys.apply(grammar, carrier("abc"));

    ok = ok && (bad.is_err());
    ok = ok && (bad.error() == "expected digit");

    return ok;
}

/*
tests_parse_outcome_is_constexpr
  Verifies the sink and its factories fold in a constant expression - every member
  carries D_CONSTEXPR.  The components here are int, since std::string is not a
  literal type before C++20.
  Tests the following:
  - parse_success folds, and its accessors are readable at compile time
  - parse_failure folds, and reports its error at compile time
  - the inactive components of a failure are value-initialized at compile time
  - operator bool folds
*/
bool
tests_parse_outcome_is_constexpr()
{
    D_CONSTEXPR auto good = parse_success<int>(5, 9);   // rho=5, tau=9, E=int
    D_CONSTEXPR auto bad  = parse_failure<int, int>(-1);

    static_assert(good.is_ok(), "a success folds");
    static_assert(good.value() == 5, "and its result is readable");
    static_assert(good.remaining() == 9, "and its leftover is readable");

    static_assert(bad.is_err(), "a failure folds");
    static_assert(bad.error() == -1, "and its error is readable");
    static_assert(bad.value() == 0, "with the inactive arm value-initialized");
    static_assert(bad.remaining() == 0, "in both components");

    static_assert(static_cast<bool>(good), "operator bool folds (true)");
    static_assert(!static_cast<bool>(bad), "operator bool folds (false)");

    return (good.is_ok() && bad.is_err());
}


NS_END  // testing
NS_END  // djinterp
