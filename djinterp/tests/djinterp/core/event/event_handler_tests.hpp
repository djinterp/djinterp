/******************************************************************************
* djinterp [test]                                       event_handler_tests.hpp
*
*   Declarations for the unit-test suite covering event_handler.hpp.  Each free
* function exercises one entity of the handler primitive header and returns
* true if every check inside it passed, false otherwise.  Tests are grouped
* into translation units by the section of event_handler.hpp they cover:
*
*   - event_handler_tests_handler_id.cpp  -> I.   HANDLER IDENTIFICATION
*   - event_handler_tests_invocation.cpp  -> II.  VERDICT NORMALIZATION + apply
*   - event_handler_tests_traits.cpp      -> III/IV. COMPATIBILITY + TRAITS
*   - event_handler_tests_monoid.cpp      -> V.   THE HANDLER MONOID (seq, skip)
*   - event_handler_tests_concepts.cpp    -> VI.  CONCEPT CONSTRAINTS (C++20+)
*
*   The lone shared check helper, event_handler_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean so the calling test can fold it into a running result.  The
* D_EH_CHECK macro is the intended call site: it captures the expression text,
* file, and line.  The event-tag fixtures and the handler (function-object)
* fixtures shared across sections live here as well.
*
*   The handler fixtures are explicitly-typed function objects (never generic
* lambdas) so the suite stays C++11-clean.  The stateful recorders hold a
* pointer to an external counter so they survive seq's by-value capture.
*
*   NOTE: the entities under test live in djinterp (and djinterp::internal);
* the tests themselves live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_EVENT_HANDLER_TESTS_
#define DJINTERP_EVENT_HANDLER_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include <djinterp/core/event/event_handler.hpp>


NS_DJINTERP
NS_TESTING


// =========================================================================
//  shared check helper
// =========================================================================

// event_handler_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
event_handler_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  shared event-tag fixtures
// =========================================================================

// ev_empty
//   fixture: a named event tag carrying an empty payload (arity 0).
D_EVENT_EMPTY(ev_empty);

// ev_unary
//   fixture: a named event tag with a one-element payload.
D_EVENT(ev_unary, int);

// ev_binary
//   fixture: a named event tag with a two-element payload.
D_EVENT(ev_binary, int, double);

// ev_ternary
//   fixture: a named event tag with a three-element payload.
D_EVENT(ev_ternary, int, double, char);

// ev_quaternary
//   fixture: a named event tag with a four-element payload (variadic arity).
D_EVENT(ev_quaternary, int, int, int, int);

// ev_plain
//   fixture: a type that is not an event tag, used to confirm the handler
// concepts short-circuit on event identity without instantiating handler
// traits (which would fire event_traits' static_assert).
struct ev_plain
{
};


// =========================================================================
//  shared handler fixtures (explicitly-typed function objects)
// =========================================================================

// h_void_nullary
//   fixture: a void-returning handler for an empty event (always-pass).
struct h_void_nullary
{
    void operator()() const
    {
    }
};

// h_verdict_nullary
//   fixture: a verdict-returning handler for an empty event.
struct h_verdict_nullary
{
    verdict operator()() const
    {
        return verdict::pass;
    }
};

// h_void_unary
//   fixture: a void-returning handler compatible with a unary event.
struct h_void_unary
{
    void operator()(int) const
    {
    }
};

// h_pass_unary
//   fixture: a verdict-returning unary handler that always passes.
struct h_pass_unary
{
    verdict operator()(int) const
    {
        return verdict::pass;
    }
};

// h_consume_unary
//   fixture: a verdict-returning unary handler that always consumes.
struct h_consume_unary
{
    verdict operator()(int) const
    {
        return verdict::consume;
    }
};

// h_int_unary
//   fixture: an invocable unary callable whose return type is neither void
// nor verdict, used to confirm such callables are not compatible handlers.
struct h_int_unary
{
    int operator()(int) const
    {
        return 0;
    }
};

// h_void_binary
//   fixture: a void-returning handler compatible with a binary event.
struct h_void_binary
{
    void operator()(int, double) const
    {
    }
};

// h_pass_binary
//   fixture: a verdict-returning binary handler that always passes.
struct h_pass_binary
{
    verdict operator()(int, double) const
    {
        return verdict::pass;
    }
};

// h_void_ternary
//   fixture: a void-returning handler compatible with a ternary event.
struct h_void_ternary
{
    void operator()(int, double, char) const
    {
    }
};

// h_void_quaternary
//   fixture: a void-returning handler compatible with a four-arg event.
struct h_void_quaternary
{
    void operator()(int, int, int, int) const
    {
    }
};

// h_noexcept_unary
//   fixture: a noexcept verdict-returning unary handler (is_nothrow true).
struct h_noexcept_unary
{
    verdict operator()(int) const noexcept
    {
        return verdict::pass;
    }
};

// h_throwing_unary
//   fixture: a potentially-throwing verdict-returning unary handler
// (is_nothrow false).
struct h_throwing_unary
{
    verdict operator()(int) const
    {
        return verdict::pass;
    }
};


// =========================================================================
//  value-capturing handlers (confirm payload reaches the handler)
// =========================================================================

// capture_int
//   fixture: records the single int argument it receives.
struct capture_int
{
    int* slot;

    void operator()(int _a) const
    {
        if (slot)
        {
            *slot = _a;
        }
    }
};

// capture_sum
//   fixture: records the truncated sum of its two arguments.
struct capture_sum
{
    int* slot;

    void operator()(int _a, double _b) const
    {
        if (slot)
        {
            *slot = _a + static_cast<int>(_b);
        }
    }
};


// =========================================================================
//  stateful recorders for the monoid (survive seq's by-value capture)
// =========================================================================

// rec_pass
//   fixture: increments an external counter and passes.
struct rec_pass
{
    int* n;

    verdict operator()(int) const
    {
        if (n)
        {
            ++*n;
        }

        return verdict::pass;
    }
};

// rec_consume
//   fixture: increments an external counter and consumes.
struct rec_consume
{
    int* n;

    verdict operator()(int) const
    {
        if (n)
        {
            ++*n;
        }

        return verdict::consume;
    }
};

// rec_void
//   fixture: increments an external counter and returns void (always-pass).
struct rec_void
{
    int* n;

    void operator()(int) const
    {
        if (n)
        {
            ++*n;
        }
    }
};

// add_one_ref
//   fixture: increments an int passed by reference and passes; used to prove
// seq routes the same lvalue through both stages.
struct add_one_ref
{
    verdict operator()(int& _x) const
    {
        ++_x;

        return verdict::pass;
    }
};

// expect_two_ref
//   fixture: consumes iff the referenced int equals two; the downstream half
// of the seq lvalue-passthrough check.
struct expect_two_ref
{
    verdict operator()(int& _x) const
    {
        return (_x == 2) ? verdict::consume : verdict::pass;
    }
};


// =========================================================================
//  move probe for seq's "arguments are never moved" contract
// =========================================================================

// move_probe
//   fixture: counts how many times it is move-constructed, through the
// external counter it carries.
struct move_probe
{
    int* moves;

    explicit move_probe(int* _m)
        : moves(_m)
    {
    }

    move_probe(const move_probe&) = default;

    move_probe(move_probe&& _o) noexcept
        : moves(_o.moves)
    {
        if (moves)
        {
            ++*moves;
        }
    }
};

// mp_pass
//   fixture: a handler taking the move_probe by const reference; passes.
struct mp_pass
{
    verdict operator()(const move_probe&) const
    {
        return verdict::pass;
    }
};


// =========================================================================
//  test declarations
// =========================================================================

// I.   HANDLER IDENTIFICATION
bool tests_handler_id_relational();
bool tests_handler_id_validity();
bool tests_handler_id_null();
bool tests_handler_id_value_semantics();

// II.  VERDICT NORMALIZATION + tuple-apply
bool tests_invoke_normalized_void();
bool tests_invoke_normalized_verdict();
bool tests_apply_handler_arities();
bool tests_apply_handler_void_normalization();
bool tests_apply_handler_forwards_values();

// III/IV. HANDLER COMPATIBILITY DETECTION + TRAITS
bool tests_handler_traits_is_invocable();
bool tests_handler_traits_return_type();
bool tests_handler_traits_returns_void_verdict();
bool tests_handler_traits_is_compatible();
bool tests_handler_traits_is_nothrow();
bool tests_handler_traits_expected_arity();
bool tests_handler_traits_cvref();

// V.   THE HANDLER MONOID (seq, skip)
bool tests_skip_always_pass();
bool tests_seq_pass_pass();
bool tests_seq_left_zero();
bool tests_seq_void_normalization();
bool tests_seq_associativity();
bool tests_seq_lvalue_passthrough();
bool tests_seq_clean_type();

// VI.  CONCEPT CONSTRAINTS (C++20+; vacuous pass where concepts are absent)
bool tests_concept_is_handler();
bool tests_concept_handler_for();
bool tests_concept_void_verdict_handler_for();
bool tests_concept_nothrow_handlers();
bool tests_concept_handler_for_event_of_arity();
bool tests_concept_arity_aliases();
bool tests_concept_non_event_safety();


NS_END  // testing
NS_END  // djinterp


// D_EH_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through event_handler_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g.
// handler_traits<A, B>::value) need no defensive parentheses.  Yields the
// boolean result for accumulation at the call site.
#define D_EH_CHECK(...)                                                       \
    ::djinterp::testing::event_handler_check(                                 \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_EVENT_HANDLER_TESTS_
