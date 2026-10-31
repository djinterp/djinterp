/*******************************************************************************
* djinterp [test]                                        event_handler_tests.hpp
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
* path:      /tests/djinterp/cpp/event/event_handler_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

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
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/event/event_handler.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"   // spec mode: enriched authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// event_handler_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline
bool event_handler_check(
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

// DEvEmpty
//   fixture: a named event tag carrying an empty payload (arity 0).
D_EVENT_EMPTY(DEvEmpty);

// DEvUnary
//   fixture: a named event tag with a one-element payload.
D_EVENT(DEvUnary, int);

// DEvBinary
//   fixture: a named event tag with a two-element payload.
D_EVENT(DEvBinary, int, double);

// DEvTernary
//   fixture: a named event tag with a three-element payload.
D_EVENT(DEvTernary, int, double, char);

// DEvQuaternary
//   fixture: a named event tag with a four-element payload (variadic arity).
D_EVENT(DEvQuaternary, int, int, int, int);

// DEvPlain
//   fixture: a type that is not an event tag, used to confirm the handler
// concepts short-circuit on event identity without instantiating handler
// traits (which would fire event_traits' static_assert).
struct DEvPlain
{
};


// =========================================================================
//  shared handler fixtures (explicitly-typed function objects)
// =========================================================================

// DHVoidNullary
//   fixture: a void-returning handler for an empty event (always-pass).
struct DHVoidNullary
{
    void operator()() const
    {
    }
};

// DHVerdictNullary
//   fixture: a verdict-returning handler for an empty event.
struct DHVerdictNullary
{
    verdict operator()() const
    {
        return verdict::pass;
    }
};

// DHVoidUnary
//   fixture: a void-returning handler compatible with a unary event.
struct DHVoidUnary
{
    void operator()(
        int
    ) const
    {
    }
};

// DHPassUnary
//   fixture: a verdict-returning unary handler that always passes.
struct DHPassUnary
{
    verdict operator()(
        int
    ) const
    {
        return verdict::pass;
    }
};

// DHConsumeUnary
//   fixture: a verdict-returning unary handler that always consumes.
struct DHConsumeUnary
{
    verdict operator()(
        int
    ) const
    {
        return verdict::consume;
    }
};

// DHIntUnary
//   fixture: an invocable unary callable whose return type is neither void
// nor verdict, used to confirm such callables are not compatible handlers.
struct DHIntUnary
{
    int operator()(
        int
    ) const
    {
        return 0;
    }
};

// DHVoidBinary
//   fixture: a void-returning handler compatible with a binary event.
struct DHVoidBinary
{
    void operator()(
        int,
        double
    ) const
    {
    }
};

// DHPassBinary
//   fixture: a verdict-returning binary handler that always passes.
struct DHPassBinary
{
    verdict operator()(
        int,
        double
    ) const
    {
        return verdict::pass;
    }
};

// DHVoidTernary
//   fixture: a void-returning handler compatible with a ternary event.
struct DHVoidTernary
{
    void operator()(
        int,
        double,
        char
    ) const
    {
    }
};

// DHVoidQuaternary
//   fixture: a void-returning handler compatible with a four-arg event.
struct DHVoidQuaternary
{
    void operator()(
        int,
        int,
        int,
        int
    ) const
    {
    }
};

// DHNoexceptUnary
//   fixture: a noexcept verdict-returning unary handler (is_nothrow true).
struct DHNoexceptUnary
{
    verdict operator()(
        int
    ) const noexcept
    {
        return verdict::pass;
    }
};

// DHThrowingUnary
//   fixture: a potentially-throwing verdict-returning unary handler
// (is_nothrow false).
struct DHThrowingUnary
{
    verdict operator()(
        int
    ) const
    {
        return verdict::pass;
    }
};


// =========================================================================
//  value-capturing handlers (confirm payload reaches the handler)
// =========================================================================

// DCaptureInt
//   fixture: records the single int argument it receives.
struct DCaptureInt
{
    int* slot;

    void operator()(
        int _a
    ) const
    {
        if (slot)
        {
            *slot = _a;
        }
    }
};

// DCaptureSum
//   fixture: records the truncated sum of its two arguments.
struct DCaptureSum
{
    int* slot;

    void operator()(
        int    _a,
        double _b
    ) const
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

// DRecPass
//   fixture: increments an external counter and passes.
struct DRecPass
{
    int* n;

    verdict operator()(
        int
    ) const
    {
        if (n)
        {
            ++*n;
        }

        return verdict::pass;
    }
};

// DRecConsume
//   fixture: increments an external counter and consumes.
struct DRecConsume
{
    int* n;

    verdict operator()(
        int
    ) const
    {
        if (n)
        {
            ++*n;
        }

        return verdict::consume;
    }
};

// DRecVoid
//   fixture: increments an external counter and returns void (always-pass).
struct DRecVoid
{
    int* n;

    void operator()(
        int
    ) const
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
    verdict operator()(
        int& _x
    ) const
    {
        ++_x;

        return verdict::pass;
    }
};

// DExpectTwoRef
//   fixture: consumes iff the referenced int equals two; the downstream half
// of the seq lvalue-passthrough check.
struct DExpectTwoRef
{
    verdict operator()(
        int& _x
    ) const
    {
        return (_x == 2) ? verdict::consume : verdict::pass;
    }
};


// =========================================================================
//  move probe for seq's "arguments are never moved" contract
// =========================================================================

// DMoveProbe
//   fixture: counts how many times it is move-constructed, through the
// external counter it carries.
struct DMoveProbe
{
    int* moves;

    explicit DMoveProbe(int* _m)
        : moves(_m)
    {
    }

    DMoveProbe(
        const DMoveProbe&
    ) = default;

    DMoveProbe(DMoveProbe&& _o) noexcept
        : moves(_o.moves)
    {
        if (moves)
        {
            ++*moves;
        }
    }
};

// DMpPass
//   fixture: a handler taking the DMoveProbe by const reference; passes.
struct DMpPass
{
    verdict operator()(
        const DMoveProbe&
    ) const
    {
        return verdict::pass;
    }
};


// =========================================================================
//  test declarations
// =========================================================================

#endif  // !DTEST_SPEC_MODE (fixtures + check helper)

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


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Mirrors this suite as data - each unit test paired with its name and a
//  one-line descriptor - so run_module / run_suite can lower it into the
//  six-kind tree and drive the report.  References only the test_fn_ptr
//  declarations above; no fixtures required, which is what lets the
//  aggregate runner include every suite header in one TU.
// =========================================================================

namespace dt = ::djinterp::test;

inline
dt::module_spec event_handler_spec()
{
    return dt::module_spec{
        "event_handler",
        "The handler primitive: handler_id, verdict normalization and "
        "tuple-apply, handler compatibility traits, the seq/skip monoid, "
        "and the C++20 concept layer.",
        {
            dt::block_spec{
                "handler_id",
                "The opaque handler_id handle: ordering, validity, value semantics.",
                {
                    { "handler_id_relational",
                      "handler_id defines a strict, consistent ordering",
                      &tests_handler_id_relational },
                    { "handler_id_validity",
                      "valid() distinguishes bound ids from the null id",
                      &tests_handler_id_validity },
                    { "handler_id_null",
                      "the null handler_id is distinct from any bound id",
                      &tests_handler_id_null },
                    { "handler_id_value_semantics",
                      "handler_id copies and compares by value",
                      &tests_handler_id_value_semantics },
                }
            },
            dt::block_spec{
                "invocation",
                "Verdict normalization and payload-tuple application.",
                {
                    { "invoke_normalized_void",
                      "a void handler is normalized to verdict::pass",
                      &tests_invoke_normalized_void },
                    { "invoke_normalized_verdict",
                      "a verdict-returning handler's verdict is forwarded unchanged",
                      &tests_invoke_normalized_verdict },
                    { "apply_handler_arities",
                      "apply_handler unpacks payload tuples of arity zero through three",
                      &tests_apply_handler_arities },
                    { "apply_handler_void_normalization",
                      "apply_handler normalizes a void handler to pass",
                      &tests_apply_handler_void_normalization },
                    { "apply_handler_forwards_values",
                      "apply_handler delivers the payload values in order",
                      &tests_apply_handler_forwards_values },
                }
            },
            dt::block_spec{
                "monoid",
                "The sequencing monoid: skip (identity) and seq (combine).",
                {
                    { "skip_always_pass",
                      "skip is the always-pass step, the monoid's unit",
                      &tests_skip_always_pass },
                    { "seq_pass_pass",
                      "seq of two passing steps passes",
                      &tests_seq_pass_pass },
                    { "seq_left_zero",
                      "a consuming left step short-circuits seq (left zero)",
                      &tests_seq_left_zero },
                    { "seq_void_normalization",
                      "seq normalizes void steps to pass before combining",
                      &tests_seq_void_normalization },
                    { "seq_associativity",
                      "seq is associative",
                      &tests_seq_associativity },
                    { "seq_lvalue_passthrough",
                      "seq forwards an lvalue state through both steps",
                      &tests_seq_lvalue_passthrough },
                    { "seq_clean_type",
                      "seq's result type is cv/ref-stripped",
                      &tests_seq_clean_type },
                }
            },
            dt::block_spec{
                "traits",
                "handler_traits: invocability, return kind, compatibility, arity.",
                {
                    { "handler_traits_is_invocable",
                      "is_invocable detects a handler callable with the payload",
                      &tests_handler_traits_is_invocable },
                    { "handler_traits_return_type",
                      "return_type reports the handler's return type",
                      &tests_handler_traits_return_type },
                    { "handler_traits_returns_void_verdict",
                      "detects whether a handler returns void or verdict",
                      &tests_handler_traits_returns_void_verdict },
                    { "handler_traits_is_compatible",
                      "is_compatible gates handlers to void / verdict returns",
                      &tests_handler_traits_is_compatible },
                    { "handler_traits_is_nothrow",
                      "is_nothrow reflects a noexcept handler",
                      &tests_handler_traits_is_nothrow },
                    { "handler_traits_expected_arity",
                      "expected_arity matches the event payload arity",
                      &tests_handler_traits_expected_arity },
                    { "handler_traits_cvref",
                      "handler_traits strips cv/ref before inspecting",
                      &tests_handler_traits_cvref },
                }
            },
            dt::block_spec{
                "concepts",
                "The C++20 handler concept layer.",
                {
                    { "concept_is_handler",
                      "the is_handler concept holds for valid handlers",
                      &tests_concept_is_handler },
                    { "concept_handler_for",
                      "handler_for<E> matches handlers compatible with event E",
                      &tests_concept_handler_for },
                    { "concept_void_verdict_handler_for",
                      "separates void-returning from verdict-returning handlers",
                      &tests_concept_void_verdict_handler_for },
                    { "concept_nothrow_handlers",
                      "the nothrow-handler concept matches noexcept handlers",
                      &tests_concept_nothrow_handlers },
                    { "concept_handler_for_event_of_arity",
                      "handler_for composes with event_of_arity<N>",
                      &tests_concept_handler_for_event_of_arity },
                    { "concept_arity_aliases",
                      "the arity-named handler concepts match their arities",
                      &tests_concept_arity_aliases },
                    { "concept_non_event_safety",
                      "handler concepts are vacuously false for non-events",
                      &tests_concept_non_event_safety },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

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
