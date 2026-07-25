#include <string>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_parse_integration_atomic_success
  Verifies the atomic parser is the operational shape the formal definition
describes: given the residual, it yields a value and advances the offset by
exactly the input it consumed.
  Tests the following:
  - a leading digit is parsed to its numeric value,
  - the offset advances by exactly one element,
  - the residual is what follows the consumed digit,
  - a second parse continues from the new residual.
*/
bool
tests_parse_integration_atomic_success()
{
    const char* text = "42x";

    dp::parse_state<char> state(text, 3u);

    // the atomic parser consumes one digit and yields its value
    dp::parse_result<int> first = pa_parse_digit(state);

    D_PA_CHECK(first.ok());
    D_PA_CHECK(first.value() == 4);

    // exactly one element was consumed
    D_PA_CHECK(state.offset == 1u);
    D_PA_CHECK(pa_remaining_text(state) == std::string("2x"));

    // a second parse continues from the advanced residual
    dp::parse_result<int> second = pa_parse_digit(state);

    D_PA_CHECK(second.ok());
    D_PA_CHECK(second.value() == 2);
    D_PA_CHECK(state.offset == 2u);
    D_PA_CHECK(*state.current() == 'x');

    return true;
}

/*
tests_parse_integration_failure_carries_offset
  Verifies a failing parser reports the offset it stopped at and consumes
nothing, so the caller can locate the failure and retry from the same point.
  Tests the following:
  - a non-digit yields a Malformed failure,
  - the failure's offset is where the parser was looking,
  - the offset is not advanced by a failed parse,
  - the failure carries a descriptive message.
*/
bool
tests_parse_integration_failure_carries_offset()
{
    const char* text = "x9";

    dp::parse_state<char> state(text, 2u);

    // the leading character is not a digit, so the parse fails in place
    dp::parse_result<int> outcome = pa_parse_digit(state);

    D_PA_CHECK(!outcome.ok());
    D_PA_CHECK(outcome.error().status() == dp::DParseStatusMalformed);

    // the reported offset is exactly where the parser was positioned
    D_PA_CHECK(outcome.error().offset() == 0u);

    // and the residual is untouched: the parser consumed nothing
    D_PA_CHECK(state.offset == 0u);
    D_PA_CHECK(*state.current() == 'x');
    D_PA_CHECK(!outcome.error().message().empty());

    return true;
}

/*
tests_parse_integration_end_of_input
  Verifies parsing an exhausted stream fails with the dedicated end-of-input
status at the terminal offset, distinct from a malformed-input failure.
  Tests the following:
  - parsing at the end yields EndOfInput, not Malformed,
  - the failure's offset is the terminal offset,
  - the stream is unchanged,
  - reaching the end mid-parse produces the same failure.
*/
bool
tests_parse_integration_end_of_input()
{
    const char* text = "7";

    dp::parse_state<char> state(text, 1u);

    // consume the single available digit
    dp::parse_result<int> first = pa_parse_digit(state);

    D_PA_CHECK(first.ok());
    D_PA_CHECK(state.at_end());

    // the next parse hits end-of-input, a status distinct from malformed
    dp::parse_result<int> at_end = pa_parse_digit(state);

    D_PA_CHECK(!at_end.ok());
    D_PA_CHECK(at_end.error().status() == dp::DParseStatusEndOfInput);
    D_PA_CHECK(at_end.error().status() != dp::DParseStatusMalformed);

    // the failure points at the terminal offset and consumes nothing
    D_PA_CHECK(at_end.error().offset() == 1u);
    D_PA_CHECK(state.offset == 1u);

    // the same failure arises from a freshly-empty stream
    dp::parse_state<char> empty(text, 0u);
    dp::parse_result<int> immediate = pa_parse_digit(empty);

    D_PA_CHECK(!immediate.ok());
    D_PA_CHECK(immediate.error().status() == dp::DParseStatusEndOfInput);
    D_PA_CHECK(immediate.error().offset() == 0u);

    return true;
}

/*
tests_parse_integration_alt_restores_offset
  Verifies the match-or-restore semantics of the alternative combinator: a
failed branch rewinds the residual so the next alternative sees the input
the first one started from.
  Tests the following:
  - a failing branch under pa_attempt leaves the offset unmoved,
  - a subsequent alternative then succeeds from the original position,
  - the restore is exact even when the failing branch looked ahead,
  - the surviving value comes from the successful alternative.
*/
bool
tests_parse_integration_alt_restores_offset()
{
    const char* text = "5";

    dp::parse_state<char> state(text, 1u);

    // the first alternative expects a letter and fails; pa_attempt rewinds
    dp::parse_result<char> letter =
        pa_attempt(state,
                   [](dp::parse_state<char>& _s)
                   { return pa_parse_literal(_s, 'a'); });

    D_PA_CHECK(!letter.ok());

    // the residual is exactly where it began, so the next branch is fair
    D_PA_CHECK(state.offset == 0u);
    D_PA_CHECK(*state.current() == '5');

    // the second alternative parses a digit from the restored position
    dp::parse_result<int> digit = pa_parse_digit(state);

    D_PA_CHECK(digit.ok());
    D_PA_CHECK(digit.value() == 5);
    D_PA_CHECK(state.offset == 1u);

    return true;
}

/*
tests_parse_integration_alt_keeps_offset_on_success
  Verifies the other half of the alternative contract: a successful branch
under pa_attempt keeps every element it consumed, so success commits the
input.
  Tests the following:
  - a succeeding branch under pa_attempt advances normally,
  - the consumed input is not rewound,
  - the value is the branch's value,
  - a following parse continues from the committed position.
*/
bool
tests_parse_integration_alt_keeps_offset_on_success()
{
    const char* text = "a1";

    dp::parse_state<char> state(text, 2u);

    // the branch succeeds, so pa_attempt must NOT rewind
    dp::parse_result<char> letter =
        pa_attempt(state,
                   [](dp::parse_state<char>& _s)
                   { return pa_parse_literal(_s, 'a'); });

    D_PA_CHECK(letter.ok());
    D_PA_CHECK(letter.value() == 'a');

    // the consumed element stays consumed
    D_PA_CHECK(state.offset == 1u);
    D_PA_CHECK(*state.current() == '1');

    // and parsing continues from the committed residual
    dp::parse_result<int> digit = pa_parse_digit(state);

    D_PA_CHECK(digit.ok());
    D_PA_CHECK(digit.value() == 1);
    D_PA_CHECK(state.at_end());

    return true;
}

/*
tests_parse_integration_sequence_consumes_residual
  Verifies a sequence of parsers consumes the input left to right until the
residual is empty, which is the shape of a grammar production.
  Tests the following:
  - each parser in turn consumes its part of the input,
  - the residual shrinks monotonically,
  - the sequence ends exactly at end-of-input,
  - each intermediate value is the one that parser produced.
*/
bool
tests_parse_integration_sequence_consumes_residual()
{
    const char* text = "1a2";

    dp::parse_state<char> state(text, 3u);

    // digit, then literal, then digit -- a fixed three-symbol production
    dp::parse_result<int> one = pa_parse_digit(state);

    D_PA_CHECK(one.ok());
    D_PA_CHECK(one.value() == 1);
    D_PA_CHECK(state.remaining() == 2u);

    dp::parse_result<char> mid = pa_parse_literal(state, 'a');

    D_PA_CHECK(mid.ok());
    D_PA_CHECK(mid.value() == 'a');
    D_PA_CHECK(state.remaining() == 1u);

    dp::parse_result<int> two = pa_parse_digit(state);

    D_PA_CHECK(two.ok());
    D_PA_CHECK(two.value() == 2);

    // the production consumed the entire residual
    D_PA_CHECK(state.at_end());
    D_PA_CHECK(state.offset == 3u);
    D_PA_CHECK(pa_remaining_text(state).empty());

    return true;
}

/*
tests_parse_integration_overflow_status
  Verifies a parser that guards an accumulator reports the dedicated
overflow status at the offending offset, exercising a user-facing status
code end to end.
  Tests the following:
  - a number within the bound parses to its value,
  - a number exceeding the bound fails with Overflow,
  - the failure offset points into the overflowing run,
  - a number of exactly the boundary width still succeeds.
*/
bool
tests_parse_integration_overflow_status()
{
    // a bounded number well within range parses cleanly
    dp::parse_state<char> small_state("123", 3u);
    dp::parse_result<int> small = pa_parse_number(small_state);

    D_PA_CHECK(small.ok());
    D_PA_CHECK(small.value() == 123);
    D_PA_CHECK(small_state.at_end());

    // a number that grows past the accumulator bound fails with Overflow
    dp::parse_state<char> big_state("99999", 5u);
    dp::parse_result<int> big = pa_parse_number(big_state);

    D_PA_CHECK(!big.ok());
    D_PA_CHECK(big.error().status() == dp::DParseStatusOverflow);

    // the failure is reported partway through the digit run, where the
    // accumulator first exceeded the bound
    D_PA_CHECK(big.error().offset() > 0u);
    D_PA_CHECK(big.error().offset() <= 5u);

    // the largest in-range value (exactly the bound) is accepted
    dp::parse_state<char> edge_state("9999", 4u);
    dp::parse_result<int> edge = pa_parse_number(edge_state);

    D_PA_CHECK(edge.ok());
    D_PA_CHECK(edge.value() == 9999);

    return true;
}

/*
tests_parse_integration_monadic_chain
  Verifies a parse outcome threads through the inherited monadic surface on
the base carrier to produce a derived value, so parsing composes with pure
transformation.
  Tests the following:
  - a successful parse maps to a transformed value,
  - and_then chains a further computation on the base carrier,
  - the composed result carries the final value,
  - the whole chain runs only because the parse succeeded.
*/
bool
tests_parse_integration_monadic_chain()
{
    const char* text = "8";

    dp::parse_state<char> state(text, 1u);

    // parse a digit, then map and bind on the base carrier the parse
    // returns; the callables use the base result, per the pinned limitation
    pa_fn::result<int, dp::parse_error> chained =
        pa_parse_digit(state)
            .map([](const int& _v) { return _v + 1; })
            .and_then([](const int& _v)
                      {
                          return pa_fn::ok<int, dp::parse_error>(_v * 10);
                      });

    D_PA_CHECK(chained.is_ok());
    D_PA_CHECK(chained.value() == 90);

    // the digit really was consumed as part of the chain
    D_PA_CHECK(state.at_end());

    // a match at the end of the chain reads the final value
    std::string rendered = chained.match(
        [](const int& _v) { return std::string("n=") + std::to_string(_v); },
        [](const dp::parse_error&) { return std::string("failed"); });

    D_PA_CHECK(rendered == std::string("n=90"));

    return true;
}

/*
tests_parse_integration_error_propagates_untouched
  Verifies a parse failure survives an entire map / and_then chain with its
status, offset, and message intact, so the original diagnostic reaches the
caller no matter how the outcome is post-processed.
  Tests the following:
  - a failed parse short-circuits every stage,
  - none of the transforming callables runs,
  - the propagated error equals the original in all three fields,
  - the final outcome is still a failure.
*/
bool
tests_parse_integration_error_propagates_untouched()
{
    const char* text = "z";

    dp::parse_state<char> state(text, 1u);

    int map_calls  = 0;
    int bind_calls = 0;

    // the parse fails at the first character; the chain must not run its
    // callables and must carry the failure straight through
    pa_fn::result<int, dp::parse_error> outcome =
        pa_parse_digit(state)
            .map([&map_calls](const int& _v) { ++map_calls; return _v; })
            .and_then([&bind_calls](const int& _v)
                      {
                          ++bind_calls;
                          return pa_fn::ok<int, dp::parse_error>(_v);
                      });

    D_PA_CHECK(outcome.is_err());
    D_PA_CHECK(map_calls  == 0);
    D_PA_CHECK(bind_calls == 0);

    // the diagnostic is exactly the one the parser produced
    D_PA_CHECK(outcome.error().status() == dp::DParseStatusMalformed);
    D_PA_CHECK(outcome.error().offset() == 0u);
    D_PA_CHECK(!outcome.error().message().empty());

    // and the input was not consumed by the failed parse
    D_PA_CHECK(state.offset == 0u);

    return true;
}

/*
tests_parse_integration_traits_drive_the_stream
  Verifies the minor/major mapping is not merely descriptive: parse_traits
selects the very element type the parse_state and its parser are built on,
tying the trait layer to a running parse.
  Tests the following:
  - parse_traits<std::string>::minor is the element type of the stream,
  - a parser over that element type consumes a std::string's characters,
  - the major type names what a full parse would build,
  - the parsed characters reconstruct the source string.
*/
bool
tests_parse_integration_traits_drive_the_stream()
{
    // the trait chooses the element type the whole parse is built on
    typedef dp::parse_traits<std::string>::minor minor_type;
    typedef dp::parse_traits<std::string>::major major_type;

    D_PA_CHECK((std::is_same<minor_type, char>::value));
    D_PA_CHECK((std::is_same<major_type, std::string>::value));

    // a stream whose element type came from the trait
    const std::string source("246");

    dp::parse_state<minor_type> state(source.data(), source.size());

    // fold the digits the same way the major type would be built up
    major_type   rebuilt;
    int          sum;

    sum = 0;

    while (!state.at_end())
    {
        dp::parse_result<int> digit = pa_parse_digit(state);

        D_PA_CHECK(digit.ok());

        rebuilt.push_back(
            static_cast<char>('0' + digit.value()));
        sum += digit.value();
    }

    // the parse consumed the whole string via the trait-selected element
    D_PA_CHECK(state.at_end());
    D_PA_CHECK(rebuilt == source);
    D_PA_CHECK(sum == 12);

    return true;
}

NS_END  // testing
NS_END  // djinterp
