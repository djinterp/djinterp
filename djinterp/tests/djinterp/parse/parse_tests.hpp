/******************************************************************************
* djinterp [parse/test]                                        parse_tests.hpp
*
*   The two-faced suite header for parse.hpp (the DTest DTEST_SPEC_MODE
* idiom).  parse.hpp carries the support types that face into a parser:
* the parseable / parse_traits minor-major mapping, the parse_status code
* space, the value-semantic parse_error, the parse_result<T> refinement of
* functional::result<T, parse_error>, and the parse_state<E> surface stream.
*
*   This header presents two faces, selected by DTEST_SPEC_MODE:
*     - normal mode (the section TUs): pulls in parse.hpp and exposes the
*       fixtures plus the D_PA_CHECK macro the bodies need.
*     - spec  mode (the runner): pulls in djinterp/test/test_defaults.hpp
*       and exposes parse_spec(), the module -> block -> test tree.
*
*   The bool tests_*() declarations are visible in BOTH faces: the section
* TUs define them, the spec provider references them by address.
*
*
* BUILD PREREQUISITE
* ==================
*   As shipped, parse.hpp does not compile against result.hpp.  parse.hpp
* spells the base of parse_result and the tagged-constructor helpers as
*
*       functional::result<_ValueType, parse_error>
*       functional::internal::ok_tag  /  functional::internal::err_tag
*
* but result.hpp opens NS_DJINTERP only -- it declares result, ok_tag, and
* err_tag in ::djinterp and ::djinterp::internal.  There is no `functional`
* namespace anywhere in the tree, and no NS_FUNCTIONAL macro is defined in
* djinterp.hpp (the identifier appears in that header's table of contents
* but is never #defined).  `functional` in result.hpp's preamble names the
* DIRECTORY the header lives in (/inc/djinterp/core/functional/), not a
* namespace.  The compiler reports:
*
*       parse.hpp:296: error: 'functional' has not been declared
*
*   Two one-line repairs are equivalent, and this suite is written against
* either.  Preferred, because it touches parse.hpp only:
*
*       // in parse.hpp, immediately after NS_DJINTERP
*       namespace functional = ::djinterp;
*
* or, equivalently, drop the `functional::` qualifier from the four sites
* in parse.hpp (lines 296, 299, 309-310, 318-319, 327-328, 336-337).
*
*   Both leave result's free surface (ok, err, or_value_with, map_err_with,
* unwrap_with, to_maybe, operator|) in ::djinterp.  This suite reaches that
* surface through the single alias `pa_fn` below, so if result.hpp is ever
* genuinely relocated into a djinterp::functional namespace, the suite is
* repaired by editing that one line.
*
*
* PINNED BEHAVIOUR
* ================
*   Three findings are asserted as they stand today.  Each is named in its
* test descriptor, and each assertion is expected to INVERT if the module
* is changed:
*
*   1. parse_result<T> does NOT inherit result's tagged constructors, so it
*      is not constructible from (ok_tag, T) or (err_tag, parse_error).
*      Consequence: result::and_then and result::or_else build their
*      short-circuit branch with exactly those constructors, so a callable
*      returning parse_result<U> makes them ILL-FORMED.  Callables must
*      return the base result<U, parse_error>.  This is a hard error in the
*      function body, not a substitution failure, so it cannot be probed by
*      SFINAE -- the suite pins the root cause (the missing constructors)
*      via is_constructible instead, and exercises the base-returning form.
*
*   2. is_result<parse_result<T>>::value is FALSE.  is_result matches the
*      result<T, E> specialization exactly and does not see through a
*      derived class, so parse_result does not answer to result.hpp's own
*      detection vocabulary even though it IS-A result.
*
*   3. parse_state::advance adds before it clamps, so the addition can wrap.
*      From offset 5 over a length-10 input, advance(SIZE_MAX) yields offset
*      4 -- the stream moves BACKWARDS.  A clamp written as
*      `if (_count > (length - offset)) { offset = length; }` would not.
*
*
* LANGUAGE FLOOR
* ==============
*   C++20, matching djinterp_add_test_executable (the helper builds C++20 /
* C17).  Nothing in the suite requires concepts; the bodies are C++11-clean
* apart from what result.hpp itself demands of the dialect.
*
*
* path:      /tests/djinterp/parse/parse_tests.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_PARSE_TESTS_
#define DJINTERP_TESTS_PARSE_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

// -- (part 1) mode-gated includes -------------------------------------------
//   djinterp core is always first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "parse.hpp"                        // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"  // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

#ifdef DTEST_SPEC_MODE
// dt
//   namespace alias: names the entities under test (djinterp::test).
//   Deviation from the guide skeleton, which declares this alias
// unconditionally: djinterp::test is introduced by test_defaults.hpp, which
// only the spec face includes.  In normal mode the namespace does not exist
// yet, so an unconditional alias would not compile.  The alias is used only
// by the spec provider, so gating it costs nothing.
namespace dt = ::djinterp::test;
#endif


// pa_check_report
//   function: prints a failing check and returns the condition unchanged.
// Self-contained (printf only), so it lives OUTSIDE the fixture guard --
// it depends on nothing from the header under test.
inline bool
pa_check_report(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d\n           %s\n",
                    _file,
                    _line,
                    _expression);
    }

    return _condition;
}

// D_PA_CHECK
//   macro: evaluate a condition once; on failure report it and return false
// from the enclosing test.  Variadic so a top-level comma inside a trait
// expression (std::is_same<A, B>::value) passes through whole.  The two
// letters PA are unique to this suite so co-compiled suites never collide.
#define D_PA_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::pa_check_report(                            \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

// dp
//   namespace alias: the subsystem under test (djinterp::parse).
namespace dp = ::djinterp::parse;

// pa_fn
//   namespace alias: where result.hpp's free surface actually lives -- the
// factories (ok / err), the pipeline combinator factories (or_value_with /
// map_err_with / unwrap_with), operator|, and to_maybe.  See the BUILD
// PREREQUISITE note above: today that is ::djinterp.  If result.hpp is ever
// relocated into a real djinterp::functional namespace, this one line is
// the whole of the repair.
namespace pa_fn = ::djinterp;


// ---------------------------------------------------------------------------
//  trait probes -- the shapes is_parseable must and must not detect
// ---------------------------------------------------------------------------

// pa_probe_pair
//   struct: exposes both nested aliases; is_parseable must accept it.
struct pa_probe_pair
{
    using minor = char;
    using major = std::string;
};

// pa_probe_minor_only
//   struct: exposes `minor` alone; the detection idiom requires BOTH, so
// is_parseable must reject it.
struct pa_probe_minor_only
{
    using minor = char;
};

// pa_probe_major_only
//   struct: exposes `major` alone; is_parseable must reject it.
struct pa_probe_major_only
{
    using major = std::string;
};

// pa_probe_plain
//   struct: exposes neither alias; is_parseable must reject it.
struct pa_probe_plain
{
    int field;
};

// pa_probe_private
//   struct: declares both aliases but keeps them private, so they are not
// nameable from outside; is_parseable must reject it.  Pins that detection
// respects access control rather than mere presence.
class pa_probe_private
{
private:
    using minor = char;
    using major = std::string;
};

// pa_probe_derived
//   struct: inherits its aliases from parseable rather than declaring them;
// is_parseable must accept it, because a nested-name lookup finds inherited
// members.
struct pa_probe_derived : dp::parseable<unsigned char, std::string>
{};

// pa_probe_same
//   struct: minor and major are the SAME type -- a degenerate but legal
// parseable domain (a stream of aggregates parsed into an aggregate).
struct pa_probe_same
{
    using minor = int;
    using major = int;
};


// ---------------------------------------------------------------------------
//  stream element fixtures -- parse_state is agnostic to its element type
// ---------------------------------------------------------------------------

// pa_token
//   struct: a non-character stream element, so parse_state can be shown to
// carry token sequences as readily as character streams.
struct pa_token
{
    int kind;
    int value;
};

// operator== (pa_token)
//   function: field-wise equality, so token streams can be compared.
inline bool
operator==(
    const pa_token& _a,
    const pa_token& _b
)
{
    return ( (_a.kind  == _b.kind) &&
             (_a.value == _b.value) );
}


// ---------------------------------------------------------------------------
//  payload fixture -- proves the value branch is moved, not copied
// ---------------------------------------------------------------------------

// pa_move_probe
//   struct: counts the copies and moves it has undergone, so parse_result's
// value constructors can be shown to forward the value category they were
// handed.
struct pa_move_probe
{
    int value;
    int copies;
    int moves;

    pa_move_probe()
        : value (0),
          copies(0),
          moves (0)
    {}

    explicit pa_move_probe(
        int _value
    )
        : value (_value),
          copies(0),
          moves (0)
    {}

    pa_move_probe(
        const pa_move_probe& _other
    )
        : value (_other.value),
          copies(_other.copies + 1),
          moves (_other.moves)
    {}

    pa_move_probe(
        pa_move_probe&& _other
    ) noexcept
        : value (_other.value),
          copies(_other.copies),
          moves (_other.moves + 1)
    {}

    pa_move_probe&
    operator=(
        const pa_move_probe& _other
    )
    {
        if (this != &_other)
        {
            value  = _other.value;
            copies = _other.copies + 1;
            moves  = _other.moves;
        }

        return *this;
    }

    pa_move_probe&
    operator=(
        pa_move_probe&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            value  = _other.value;
            copies = _other.copies;
            moves  = _other.moves + 1;
        }

        return *this;
    }
};

// operator== (pa_move_probe)
//   function: equality on the carried value only; the bookkeeping counters
// are deliberately excluded so two probes with the same value compare equal.
inline bool
operator==(
    const pa_move_probe& _a,
    const pa_move_probe& _b
)
{
    return (_a.value == _b.value);
}


// ---------------------------------------------------------------------------
//  a hand-written parser toolkit -- the integration block's subject
// ---------------------------------------------------------------------------
//   These are the smallest parsers that exercise the formal shape parse.hpp
// documents: a function taking parse_state<char>& (the residual Sigma*,
// threaded by reference) and returning parse_result<T> (the maybe/result
// arm).  Success advances `offset`; failure leaves the caller to decide
// whether to restore it, which is what pa_attempt does.

// pa_is_digit
//   function: locale-independent ASCII digit test, so the parsers below do
// not depend on <cctype> and its locale-sensitive behaviour.
inline bool
pa_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

// pa_parse_digit
//   function: the atomic parser -- consumes one ASCII digit and yields its
// numeric value.  Fails with DParseStatusEndOfInput at exhaustion and with
// DParseStatusMalformed on a non-digit, each carrying the offset at which
// the failure occurred.
inline dp::parse_result<int>
pa_parse_digit(
    dp::parse_state<char>& _state
)
{
    const char* c;

    if (_state.at_end())
    {
        return dp::parse_result<int>::make_error(
            dp::DParseStatusEndOfInput,
            _state.offset,
            "expected a digit, found end of input");
    }

    c = _state.current();

    if (!pa_is_digit(*c))
    {
        return dp::parse_result<int>::make_error(
            dp::DParseStatusMalformed,
            _state.offset,
            "expected a digit");
    }

    _state.advance();

    return dp::parse_result<int>(static_cast<int>(*c - '0'));
}

// pa_parse_literal
//   function: consumes one specific character.  Used to show that a failing
// parser reports the offset it stopped at without consuming input.
inline dp::parse_result<char>
pa_parse_literal(
    dp::parse_state<char>& _state,
    char                   _expected
)
{
    if (_state.at_end())
    {
        return dp::parse_result<char>::make_error(
            dp::DParseStatusEndOfInput,
            _state.offset,
            "expected a literal, found end of input");
    }

    if (*_state.current() != _expected)
    {
        return dp::parse_result<char>::make_error(
            dp::DParseStatusMalformed,
            _state.offset,
            "unexpected character");
    }

    _state.advance();

    return dp::parse_result<char>(_expected);
}

// pa_parse_number
//   function: consumes one or more digits into a bounded accumulator.  The
// bound is deliberately small (9999) so the DParseStatusOverflow arm is
// reachable from a short input.
inline dp::parse_result<int>
pa_parse_number(
    dp::parse_state<char>& _state
)
{
    std::size_t start;
    int         accumulator;

    start       = _state.offset;
    accumulator = 0;

    // a number is at least one digit; reject an empty run outright
    if ( (_state.at_end()) ||
         (!pa_is_digit(*_state.current())) )
    {
        return dp::parse_result<int>::make_error(
            dp::DParseStatusMalformed,
            start,
            "expected at least one digit");
    }

    // fold the digit run, guarding the accumulator against overflow
    while ( (!_state.at_end()) &&
            (pa_is_digit(*_state.current())) )
    {
        accumulator = (accumulator * 10) +
                      static_cast<int>(*_state.current() - '0');

        if (accumulator > 9999)
        {
            return dp::parse_result<int>::make_error(
                dp::DParseStatusOverflow,
                _state.offset,
                "numeric literal exceeds the accumulator bound");
        }

        _state.advance();
    }

    return dp::parse_result<int>(accumulator);
}

// pa_attempt
//   function: the Alternative `alt` semantics parse.hpp documents -- save
// the offset, run the parser, and restore the offset if it failed, so a
// failed branch never leaks consumed input to the next one.  Returns the
// parser's result unchanged.
template<typename _Parser>
inline auto
pa_attempt(
    dp::parse_state<char>& _state,
    _Parser                _parser
)
-> decltype(_parser(_state))
{
    std::size_t saved;

    saved = _state.offset;

    decltype(_parser(_state)) outcome = _parser(_state);

    // match-or-restore: a failed branch rewinds the residual
    if (!outcome.ok())
    {
        _state.offset = saved;
    }

    return outcome;
}

// pa_remaining_text
//   function: the unconsumed residual as a string, so a test can state what
// is left of Sigma* after a parse rather than inspecting offsets by hand.
inline std::string
pa_remaining_text(
    const dp::parse_state<char>& _state
)
{
    if (_state.at_end())
    {
        return std::string();
    }

    return std::string(_state.data + _state.offset,
                       _state.remaining());
}


// pa_result_error_payload_collides
//   trait: true when parse_result<_Value> would carry two identical
// constructor signatures -- which is exactly when _Value is parse_error,
// since the value constructors take (const _Value& / _Value&&) and the
// error constructors take (const parse_error& / parse_error&&).  Computed
// WITHOUT instantiating parse_result<_Value>: for the colliding case that
// class is ill-formed (a redeclared constructor), so any probe that named
// it would be a hard error, not a false result.  The predicate reduces to
// "is _Value the same type as parse_error", which is the precise condition
// for the collision.  Isolated here as a trait so the assertion reads as a
// pinned type-level fact.
template<typename _Value>
struct pa_result_error_payload_collides
    : std::is_same<
          typename std::decay<_Value>::type,
          ::djinterp::parse::parse_error>::type
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes -------------------------

// I.   PARSEABLE / PARSE_TRAITS        (parse_tests_parseable.cpp)
bool tests_parse_parseable_aliases();
bool tests_parse_is_parseable_primary();
bool tests_parse_is_parseable_detection();
bool tests_parse_is_parseable_access_and_inheritance();
bool tests_parse_is_parseable_explicit_specialisations();
bool tests_parse_is_parseable_no_cv_ref_decay();
bool tests_parse_is_parseable_trait_shape();
bool tests_parse_traits_specialisations();
bool tests_parse_traits_primary_declared();

// II.  PARSE_STATUS                    (parse_tests_status.cpp)
bool tests_parse_status_underlying_type();
bool tests_parse_status_constant_values();
bool tests_parse_status_constants_distinct();
bool tests_parse_status_standard_range();
bool tests_parse_status_user_range();
bool tests_parse_status_extremes_round_trip();
bool tests_parse_status_classification();

// III. PARSE_ERROR                     (parse_tests_error.cpp)
bool tests_parse_error_default_construction();
bool tests_parse_error_string_construction();
bool tests_parse_error_default_message();
bool tests_parse_error_cstring_construction();
bool tests_parse_error_cstring_null_message();
bool tests_parse_error_message_is_owning();
bool tests_parse_error_copy_semantics();
bool tests_parse_error_equality_fields();
bool tests_parse_error_equality_axioms();
bool tests_parse_error_inequality();
bool tests_parse_error_extreme_fields();

// IV.  PARSE_RESULT                    (parse_tests_result.cpp)
bool tests_parse_result_value_construction();
bool tests_parse_result_error_construction();
bool tests_parse_result_base_construction();
bool tests_parse_result_type_surface();
bool tests_parse_result_ok_shadowing();
bool tests_parse_result_value_accessors();
bool tests_parse_result_factories();
bool tests_parse_result_copy_and_move();
bool tests_parse_result_move_forwards_value_category();
bool tests_parse_result_equality();
bool tests_parse_result_value_or_and_unwrap();
bool tests_parse_result_inherited_map();
bool tests_parse_result_inherited_map_err();
bool tests_parse_result_inherited_and_then();
bool tests_parse_result_inherited_or_else();
bool tests_parse_result_inherited_match();
bool tests_parse_result_operator_bool();
bool tests_parse_result_pipeline_combinators();
bool tests_parse_result_tagged_constructors_absent();
bool tests_parse_result_non_trivial_payload();

// V.   PARSE_STATE                     (parse_tests_state.cpp)
bool tests_parse_state_element_alias();
bool tests_parse_state_default_construction();
bool tests_parse_state_explicit_construction();
bool tests_parse_state_construction_does_not_clamp();
bool tests_parse_state_remaining();
bool tests_parse_state_at_end();
bool tests_parse_state_current();
bool tests_parse_state_advance_default_and_zero();
bool tests_parse_state_advance_clamps();
bool tests_parse_state_advance_overflow_wraps();
bool tests_parse_state_empty_and_null_input();
bool tests_parse_state_save_and_restore();
bool tests_parse_state_copy_independence();
bool tests_parse_state_non_character_elements();
bool tests_parse_state_full_traversal();

// VI.  INTEGRATION                     (parse_tests_integration.cpp)
bool tests_parse_integration_atomic_success();
bool tests_parse_integration_failure_carries_offset();
bool tests_parse_integration_end_of_input();
bool tests_parse_integration_alt_restores_offset();
bool tests_parse_integration_alt_keeps_offset_on_success();
bool tests_parse_integration_sequence_consumes_residual();
bool tests_parse_integration_overflow_status();
bool tests_parse_integration_monadic_chain();
bool tests_parse_integration_error_propagates_untouched();
bool tests_parse_integration_traits_drive_the_stream();


// -- (part 3) the spec provider -- spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// parse_spec
//   provider: the one authoritative description of this suite -- a module
// carrying one block per section translation unit, every node named and
// described.  run_module walks it, runs each predicate once, and renders
// the report from these strings.
inline dt::module_spec
parse_spec()
{
    return dt::module_spec{
        "parse",
        "Common primitives of the parsing subframework: the parseable / "
        "parse_traits minor-major mapping, the parse_status code space, "
        "the value-semantic parse_error, the parse_result<T> refinement of "
        "result<T, parse_error>, and the parse_state<E> surface stream that "
        "carries the formal residual.",
        {
            dt::block_spec{
                "parseable",
                "The minor (token) to major (aggregate) mapping: the "
                "parseable carrier, the is_parseable detection idiom and "
                "its three explicit specialisations, and the parse_traits "
                "lookup.",
                {
                    { "tests_parse_parseable_aliases",
                      "parseable<M, J> exposes exactly the nested aliases "
                      "minor = M and major = J, for distinct, identical, "
                      "and qualified type arguments.",
                      &tests_parse_parseable_aliases },
                    { "tests_parse_is_parseable_primary",
                      "The primary template reports false for every type "
                      "that carries no nested aliases: scalars, void, "
                      "pointers, arrays, functions, and std::vector.",
                      &tests_parse_is_parseable_primary },
                    { "tests_parse_is_parseable_detection",
                      "The SFINAE specialisation requires BOTH aliases -- "
                      "minor alone and major alone are each rejected, and "
                      "the pair is accepted.",
                      &tests_parse_is_parseable_detection },
                    { "tests_parse_is_parseable_access_and_inheritance",
                      "Detection respects access control (private aliases "
                      "are not detected) and sees inherited aliases, so a "
                      "type derived from parseable is parseable.",
                      &tests_parse_is_parseable_access_and_inheritance },
                    { "tests_parse_is_parseable_explicit_specialisations",
                      "The three explicit specialisations report true for "
                      "std::string, const char*, and char*, none of which "
                      "carries nested aliases.",
                      &tests_parse_is_parseable_explicit_specialisations },
                    { "tests_parse_is_parseable_no_cv_ref_decay",
                      "PINNED: is_parseable does not decay its argument, "
                      "so const std::string, std::string&, and char* const "
                      "are all reported false.",
                      &tests_parse_is_parseable_no_cv_ref_decay },
                    { "tests_parse_is_parseable_trait_shape",
                      "is_parseable presents the std::integral_constant "
                      "bool-trait shape: value, value_type, type, and the "
                      "conversion and call operators.",
                      &tests_parse_is_parseable_trait_shape },
                    { "tests_parse_traits_specialisations",
                      "parse_traits maps each of std::string, const char*, "
                      "and char* to minor = char and the major matching the "
                      "key, deriving from the corresponding parseable.",
                      &tests_parse_traits_specialisations },
                    { "tests_parse_traits_primary_declared",
                      "The parse_traits primary template is declared but "
                      "not defined, so an unmapped key names an incomplete "
                      "type that is still usable as a pointee.",
                      &tests_parse_traits_primary_declared },
                }
            },
            dt::block_spec{
                "status",
                "The integral outcome classifier: the parse_status type "
                "itself, the six standard codes, their disjointness, and "
                "the user-defined code range above DParseStatusUserBase.",
                {
                    { "tests_parse_status_underlying_type",
                      "parse_status is exactly std::int32_t -- signed, four "
                      "bytes, and an integral type rather than an enum.",
                      &tests_parse_status_underlying_type },
                    { "tests_parse_status_constant_values",
                      "The six standard codes carry their documented "
                      "values: Success 0, Failure 1, EndOfInput 2, "
                      "Overflow 3, Malformed 4, UserBase 64.",
                      &tests_parse_status_constant_values },
                    { "tests_parse_status_constants_distinct",
                      "Every pair of standard codes is distinct, so no two "
                      "outcomes can be confused for one another.",
                      &tests_parse_status_constants_distinct },
                    { "tests_parse_status_standard_range",
                      "Success is the only zero code, every other standard "
                      "code is non-zero, and all of them sit strictly below "
                      "DParseStatusUserBase.",
                      &tests_parse_status_standard_range },
                    { "tests_parse_status_user_range",
                      "Derived codes at and above DParseStatusUserBase "
                      "collide with no standard code and round-trip "
                      "through parse_error unchanged.",
                      &tests_parse_status_user_range },
                    { "tests_parse_status_extremes_round_trip",
                      "INT32_MIN and INT32_MAX survive storage in a "
                      "parse_error and a parse_result, confirming the code "
                      "space is the whole of int32_t.",
                      &tests_parse_status_extremes_round_trip },
                    { "tests_parse_status_classification",
                      "A classifier written over the codes routes each "
                      "standard status and an unknown status to the arm "
                      "the code space implies.",
                      &tests_parse_status_classification },
                }
            },
            dt::block_spec{
                "error",
                "The value-semantic failure descriptor: its constructors "
                "(including the null const char* guard), its accessors, its "
                "owning message, and the equality that parse_result needs.",
                {
                    { "tests_parse_error_default_construction",
                      "A default parse_error is DParseStatusFailure at "
                      "offset 0 with an empty message.",
                      &tests_parse_error_default_construction },
                    { "tests_parse_error_string_construction",
                      "The std::string constructor stores status, offset, "
                      "and message verbatim, including embedded NULs.",
                      &tests_parse_error_string_construction },
                    { "tests_parse_error_default_message",
                      "Omitting the message argument yields an empty "
                      "message while status and offset are preserved.",
                      &tests_parse_error_default_message },
                    { "tests_parse_error_cstring_construction",
                      "A string literal selects the const char* overload "
                      "and is copied into the owning message.",
                      &tests_parse_error_cstring_construction },
                    { "tests_parse_error_cstring_null_message",
                      "A null const char* is guarded and becomes the empty "
                      "message rather than undefined behaviour.",
                      &tests_parse_error_cstring_null_message },
                    { "tests_parse_error_message_is_owning",
                      "The message is an owning std::string: mutating or "
                      "discarding the source buffer leaves it unchanged, "
                      "which is the copy-without-lifetime-caveats promise.",
                      &tests_parse_error_message_is_owning },
                    { "tests_parse_error_copy_semantics",
                      "Copy construction, copy assignment, and self-"
                      "assignment all produce an independent equal error.",
                      &tests_parse_error_copy_semantics },
                    { "tests_parse_error_equality_fields",
                      "Two errors are equal only when status, offset, and "
                      "message all match; each field differing alone breaks "
                      "equality.",
                      &tests_parse_error_equality_fields },
                    { "tests_parse_error_equality_axioms",
                      "Equality is reflexive, symmetric, and transitive "
                      "across a sample of errors.",
                      &tests_parse_error_equality_axioms },
                    { "tests_parse_error_inequality",
                      "operator!= is the exact negation of operator== over "
                      "the full comparison matrix.",
                      &tests_parse_error_inequality },
                    { "tests_parse_error_extreme_fields",
                      "SIZE_MAX offsets, INT32 extremes, long messages, and "
                      "empty-versus-absent messages are all carried and "
                      "compared correctly.",
                      &tests_parse_error_extreme_fields },
                }
            },
            dt::block_spec{
                "result",
                "The parse_result<T> refinement of result<T, parse_error>: "
                "its compact legacy face, the inherited monadic surface, "
                "and the boundaries of the refinement.",
                {
                    { "tests_parse_result_value_construction",
                      "Implicit construction from a value yields a success "
                      "carrying that value, for both lvalue and rvalue "
                      "sources.",
                      &tests_parse_result_value_construction },
                    { "tests_parse_result_error_construction",
                      "Implicit construction from a parse_error yields a "
                      "failure carrying that error unchanged.",
                      &tests_parse_result_error_construction },
                    { "tests_parse_result_base_construction",
                      "A base result<T, parse_error> converts to a "
                      "parse_result on both branches, by copy and by move.",
                      &tests_parse_result_base_construction },
                    { "tests_parse_result_type_surface",
                      "PINNED: parse_result publishes value_type and "
                      "error_type and derives publicly from result, yet "
                      "is_result<parse_result<T>> is FALSE and the type has "
                      "no default constructor.",
                      &tests_parse_result_type_surface },
                    { "tests_parse_result_ok_shadowing",
                      "The shadowing ok() returns bool (not maybe<T>), "
                      "agrees with the inherited is_ok/is_err, and differs "
                      "in type from the base's ok().",
                      &tests_parse_result_ok_shadowing },
                    { "tests_parse_result_value_accessors",
                      "value() is readable through a const reference and "
                      "mutable through a non-const one; error() exposes the "
                      "descriptor on the failure branch.",
                      &tests_parse_result_value_accessors },
                    { "tests_parse_result_factories",
                      "make_ok and both make_error overloads build the same "
                      "results the constructors do, including the default "
                      "and null message forms.",
                      &tests_parse_result_factories },
                    { "tests_parse_result_copy_and_move",
                      "Copy and move construction and assignment preserve "
                      "the branch, including assignment that switches "
                      "branch and self-assignment.",
                      &tests_parse_result_copy_and_move },
                    { "tests_parse_result_move_forwards_value_category",
                      "The rvalue value constructor moves its payload while "
                      "the lvalue one copies it, so the value category "
                      "reaches the stored object.",
                      &tests_parse_result_move_forwards_value_category },
                    { "tests_parse_result_equality",
                      "The inherited equality compares branch first, then "
                      "value or error, and is reached through derived-to-"
                      "base deduction.",
                      &tests_parse_result_equality },
                    { "tests_parse_result_value_or_and_unwrap",
                      "value_or substitutes only on the failure branch, and "
                      "unwrap returns the value or throws "
                      "std::runtime_error carrying the given message.",
                      &tests_parse_result_value_or_and_unwrap },
                    { "tests_parse_result_inherited_map",
                      "PINNED: map transforms the success side and "
                      "propagates the error untouched, returning the BASE "
                      "result<U, parse_error> rather than parse_result<U>.",
                      &tests_parse_result_inherited_map },
                    { "tests_parse_result_inherited_map_err",
                      "map_err transforms the error side only and leaves a "
                      "success value untouched.",
                      &tests_parse_result_inherited_map_err },
                    { "tests_parse_result_inherited_and_then",
                      "and_then chains a base-returning callable on the "
                      "success branch and short-circuits on failure without "
                      "invoking it.",
                      &tests_parse_result_inherited_and_then },
                    { "tests_parse_result_inherited_or_else",
                      "or_else recovers from the failure branch and leaves "
                      "a success untouched without invoking the recovery "
                      "callable.",
                      &tests_parse_result_inherited_or_else },
                    { "tests_parse_result_inherited_match",
                      "match dispatches to the ok arm or the err arm and "
                      "never evaluates the other.",
                      &tests_parse_result_inherited_match },
                    { "tests_parse_result_operator_bool",
                      "The explicit operator bool tracks ok() and does not "
                      "permit implicit conversion to an arithmetic type.",
                      &tests_parse_result_operator_bool },
                    { "tests_parse_result_pipeline_combinators",
                      "operator| reaches parse_result through derived-to-"
                      "base deduction for or_value_with, map_err_with, and "
                      "unwrap_with.",
                      &tests_parse_result_pipeline_combinators },
                    { "tests_parse_result_tagged_constructors_absent",
                      "PINNED: parse_result does not inherit the tagged "
                      "constructors, which is why and_then and or_else "
                      "require base-returning callables.",
                      &tests_parse_result_tagged_constructors_absent },
                    { "tests_parse_result_non_trivial_payload",
                      "A std::string payload is constructed, copied, and "
                      "reassigned across branches without corrupting the "
                      "discriminated storage; PINNED: parse_result<T> is "
                      "uninstantiable when T is parse_error, as the value "
                      "and error constructors then collide.",
                      &tests_parse_result_non_trivial_payload },
                }
            },
            dt::block_spec{
                "state",
                "The surface stream Sigma*: its construction, its derived "
                "queries (remaining / at_end / current), the clamping "
                "advance, and the save-and-restore the alt combinator "
                "relies on.",
                {
                    { "tests_parse_state_element_alias",
                      "element_type names the stream's element for "
                      "character, byte, integral, and aggregate streams.",
                      &tests_parse_state_element_alias },
                    { "tests_parse_state_default_construction",
                      "A default parse_state is a null, empty, exhausted "
                      "stream: no data, zero length, zero offset.",
                      &tests_parse_state_default_construction },
                    { "tests_parse_state_explicit_construction",
                      "The explicit constructor stores data, length, and "
                      "offset, defaulting the offset to zero.",
                      &tests_parse_state_explicit_construction },
                    { "tests_parse_state_construction_does_not_clamp",
                      "PINNED: an initial offset beyond length is stored "
                      "verbatim; the derived queries stay defensive and "
                      "report exhaustion rather than reading out of range.",
                      &tests_parse_state_construction_does_not_clamp },
                    { "tests_parse_state_remaining",
                      "remaining() counts down to zero across the stream "
                      "and saturates at zero once the offset reaches or "
                      "passes the length.",
                      &tests_parse_state_remaining },
                    { "tests_parse_state_at_end",
                      "at_end() is false strictly before the length and "
                      "true at and beyond it, so the boundary is exact.",
                      &tests_parse_state_at_end },
                    { "tests_parse_state_current",
                      "current() points at data + offset while input "
                      "remains and is null at and beyond the end.",
                      &tests_parse_state_current },
                    { "tests_parse_state_advance_default_and_zero",
                      "advance() defaults to a single element and "
                      "advance(0) is a no-op.",
                      &tests_parse_state_advance_default_and_zero },
                    { "tests_parse_state_advance_clamps",
                      "Advancing past the end clamps the offset to exactly "
                      "the length, from inside the stream and from the end.",
                      &tests_parse_state_advance_clamps },
                    { "tests_parse_state_advance_overflow_wraps",
                      "PINNED: advance adds before it clamps, so a count "
                      "near SIZE_MAX wraps and moves the offset BACKWARDS "
                      "instead of clamping to the end.",
                      &tests_parse_state_advance_overflow_wraps },
                    { "tests_parse_state_empty_and_null_input",
                      "A zero-length stream over real storage and a fully "
                      "null stream are both immediately exhausted with a "
                      "null current().",
                      &tests_parse_state_empty_and_null_input },
                    { "tests_parse_state_save_and_restore",
                      "Saving and restoring offset rewinds consumed input "
                      "exactly, which is the match-or-restore contract alt "
                      "depends on.",
                      &tests_parse_state_save_and_restore },
                    { "tests_parse_state_copy_independence",
                      "A copied state is an independent snapshot: advancing "
                      "either one leaves the other's offset untouched.",
                      &tests_parse_state_copy_independence },
                    { "tests_parse_state_non_character_elements",
                      "Byte buffers and token sequences instantiate "
                      "parse_state identically to character streams.",
                      &tests_parse_state_non_character_elements },
                    { "tests_parse_state_full_traversal",
                      "Walking a stream with current() and advance() visits "
                      "every element exactly once and reconstructs it.",
                      &tests_parse_state_full_traversal },
                }
            },
            dt::block_spec{
                "integration",
                "The pieces composed as the formal definition intends: "
                "parsers of shape parse_state& -> parse_result<T>, their "
                "residual bookkeeping, the match-or-restore alternative, "
                "and the monadic surface over the outcome.",
                {
                    { "tests_parse_integration_atomic_success",
                      "A successful atomic parse yields the parsed value "
                      "and advances the residual by exactly one element.",
                      &tests_parse_integration_atomic_success },
                    { "tests_parse_integration_failure_carries_offset",
                      "A failing parse reports the offset at which it "
                      "stopped and consumes nothing.",
                      &tests_parse_integration_failure_carries_offset },
                    { "tests_parse_integration_end_of_input",
                      "Parsing an exhausted stream fails with "
                      "DParseStatusEndOfInput at the terminal offset.",
                      &tests_parse_integration_end_of_input },
                    { "tests_parse_integration_alt_restores_offset",
                      "A failed branch under pa_attempt rewinds the offset, "
                      "so the next alternative sees the original residual.",
                      &tests_parse_integration_alt_restores_offset },
                    { "tests_parse_integration_alt_keeps_offset_on_success",
                      "A successful branch under pa_attempt keeps every "
                      "element it consumed.",
                      &tests_parse_integration_alt_keeps_offset_on_success },
                    { "tests_parse_integration_sequence_consumes_residual",
                      "A sequence of parsers consumes the input left to "
                      "right until the residual is empty.",
                      &tests_parse_integration_sequence_consumes_residual },
                    { "tests_parse_integration_overflow_status",
                      "An accumulator that exceeds its bound fails with "
                      "DParseStatusOverflow at the offending offset.",
                      &tests_parse_integration_overflow_status },
                    { "tests_parse_integration_monadic_chain",
                      "A parse outcome threads through map and and_then on "
                      "the base carrier to produce a derived value.",
                      &tests_parse_integration_monadic_chain },
                    { "tests_parse_integration_error_propagates_untouched",
                      "A parse_error survives a whole map / and_then chain "
                      "with its status, offset, and message intact.",
                      &tests_parse_integration_error_propagates_untouched },
                    { "tests_parse_integration_traits_drive_the_stream",
                      "parse_traits<std::string>::minor selects the "
                      "parse_state element type, so the minor-major mapping "
                      "actually drives a parser.",
                      &tests_parse_integration_traits_drive_the_stream },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_PARSE_TESTS_
