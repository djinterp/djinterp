/*******************************************************************************
* djinterp [test]                                         event_common_tests.hpp
*
*   Declarations for the unit-test suite covering event_common.hpp.  Each free
* function exercises one entity of the event foundations header and returns
* true if every check inside it passed, false otherwise.  Tests are grouped
* into translation units by the section of event_common.hpp they cover:
*
*   - event_common_tests_verdict.cpp      -> II.  VERDICT (the set P)
*   - event_common_tests_detection.cpp    -> III. EVENT TAG DETECTION
*   - event_common_tests_index_apply.cpp  -> I.   INDEX SEQUENCE + tuple-apply
*   - event_common_tests_traits.cpp       -> IV.  EVENT TRAITS
*   - event_common_tests_macros.cpp       -> V.   DECLARATION MACROS
*   - event_common_tests_concepts.cpp     -> VI.  CONCEPT CONSTRAINTS (C++20+)
*
*   The lone shared check helper, event_common_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean so the calling test can fold it into a running result.  The
* D_EC_CHECK macro is the intended call site: it captures the expression text,
* file, and line.  A small set of event-tag fixtures shared by several
* sections, plus the seq_size helper used by the index-sequence tests, live
* here as well.
*
*   NOTE: the entities under test live in djinterp (and djinterp::internal);
* the tests themselves live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/core/event/event_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_EVENT_COMMON_TESTS_
#define DJINTERP_EVENT_COMMON_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#ifndef DTEST_SPEC_MODE
#include <djinterp/core/event/event_common.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: enriched authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// event_common_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
event_common_check(
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
//  shared fixtures (event tags exercised by several sections)
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

// ev_legacy
//   fixture: an event tag declaring its payload under the legacy `args_type`
// spelling only, with no name() member.
struct ev_legacy
{
    using args_type = std::tuple<int, char>;
};

// ev_unnamed
//   fixture: a canonical event tag (payload_type) with no name() member.
struct ev_unnamed
{
    using payload_type = std::tuple<float>;
};

// ev_both
//   fixture: an event tag declaring BOTH spellings, used to confirm the
// canonical `payload_type` is preferred over the legacy `args_type`.
struct ev_both
{
    using payload_type = std::tuple<int>;
    using args_type    = std::tuple<double, double>;
};

// ev_badname
//   fixture: a canonical event tag whose name() returns the wrong type (not
// const char*), used to confirm has_event_name discriminates on return type.
struct ev_badname
{
    using payload_type = std::tuple<>;

    static int name()
    {
        return 0;
    }
};

// ev_plain
//   fixture: a type that is not an event tag (no payload under either
// spelling).
struct ev_plain
{
};


// =========================================================================
//  index-sequence helper
// =========================================================================

// seq_size
//   helper: counts the indices carried by an index_sequence.  Specialized on
// the module's own index_sequence so it matches both the C++11 polyfill and
// the C++14+ std alias.
template<typename _Seq>
struct seq_size;

template<std::size_t... _I>
struct seq_size< ::djinterp::internal::index_sequence<_I...> >
{
    static const std::size_t value = sizeof...(_I);
};


// =========================================================================
//  test declarations
// =========================================================================

#endif  // !DTEST_SPEC_MODE (fixtures + check helper)

// II.  VERDICT (the set P)
bool tests_verdict_enumerators();
bool tests_verdict_type_properties();
bool tests_consumed_values();
bool tests_consumed_consistency();

// III. EVENT TAG DETECTION
bool tests_has_payload_type();
bool tests_has_args_type();
bool tests_has_event_payload();
bool tests_event_payload_select();
bool tests_has_event_name();
bool tests_is_tuple();

// I.   INDEX SEQUENCE POLYFILL + tuple-apply utilities
bool tests_index_sequence();
bool tests_make_index_sequence();
bool tests_apply_impl_direct();
bool tests_apply_tuple_arities();
bool tests_apply_tuple_values();

// IV.  EVENT TRAITS
bool tests_event_traits_payload_type();
bool tests_event_traits_args_alias();
bool tests_event_traits_arity();
bool tests_event_traits_has_name();
bool tests_event_traits_has_args();
bool tests_event_traits_legacy_and_both();

// V.   DECLARATION MACROS
bool tests_d_event_payload();
bool tests_d_event_name();
bool tests_d_event_empty();
bool tests_d_event_arity_range();
bool tests_d_event_is_event();

// VI.  CONCEPT CONSTRAINTS (C++20+; vacuous pass where concepts are absent)
bool tests_concept_is_event();
bool tests_concept_event_type();
bool tests_concept_non_event_type();
bool tests_concept_empty_event_type();
bool tests_concept_argument_event_type();
bool tests_concept_named_event_type();
bool tests_concept_unnamed_event_type();
bool tests_concept_event_of_arity();
bool tests_concept_nullary_unary();
bool tests_concept_binary_ternary();
bool tests_concept_variadic_event_type();


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

inline dt::module_spec
event_common_spec()
{
    return dt::module_spec{
        "event_common",
        "Event foundations: the verdict set, tag detection, the "
        "index-sequence and tuple-apply utilities, event traits, the "
        "declaration macros, and the C++20 concept layer.",
        {
            dt::block_spec{
                "verdict",
                "The verdict set P: the scoped enum and consumed().",
                {
                    { "verdict_enumerators",
                      "pass and consume hold underlying 0/1 and compare unequal",
                      &tests_verdict_enumerators },
                    { "verdict_type_properties",
                      "verdict is a scoped enum with an int underlying type",
                      &tests_verdict_type_properties },
                    { "consumed_values",
                      "consumed() is true for consume, false for pass",
                      &tests_consumed_values },
                    { "consumed_consistency",
                      "consumed(v) agrees with (v == consume) at every point",
                      &tests_consumed_consistency },
                }
            },
            dt::block_spec{
                "detection",
                "Event-tag detection: payload spellings, names, is_tuple.",
                {
                    { "has_payload_type",
                      "detects the canonical payload_type spelling, cv/ref-stripped",
                      &tests_has_payload_type },
                    { "has_args_type",
                      "detects the legacy args_type spelling",
                      &tests_has_args_type },
                    { "has_event_payload",
                      "accepts a tag under either payload spelling",
                      &tests_has_event_payload },
                    { "event_payload_select",
                      "prefers payload_type over args_type when both are present",
                      &tests_event_payload_select },
                    { "has_event_name",
                      "requires name() to return const char*, rejects wrong types",
                      &tests_has_event_name },
                    { "is_tuple",
                      "recognises std::tuple specializations, rejects non-tuples",
                      &tests_is_tuple },
                }
            },
            dt::block_spec{
                "index_apply",
                "The index_sequence polyfill and the tuple-apply utilities.",
                {
                    { "index_sequence",
                      "carries exactly the indices written into it",
                      &tests_index_sequence },
                    { "make_index_sequence",
                      "make_index_sequence<N> builds exactly N indices",
                      &tests_make_index_sequence },
                    { "apply_impl_direct",
                      "apply_impl invokes a callable with index-selected elements",
                      &tests_apply_impl_direct },
                    { "apply_tuple_arities",
                      "apply forwards tuples of arities zero through four",
                      &tests_apply_tuple_arities },
                    { "apply_tuple_values",
                      "apply preserves element values through the call",
                      &tests_apply_tuple_values },
                }
            },
            dt::block_spec{
                "traits",
                "event_traits: payload, arity, has_name / has_args.",
                {
                    { "event_traits_payload_type",
                      "payload_type is the declared payload tuple",
                      &tests_event_traits_payload_type },
                    { "event_traits_args_alias",
                      "args_type is a legacy alias equal to payload_type",
                      &tests_event_traits_args_alias },
                    { "event_traits_arity",
                      "arity counts the payload elements",
                      &tests_event_traits_arity },
                    { "event_traits_has_name",
                      "has_name reflects the presence of a valid name()",
                      &tests_event_traits_has_name },
                    { "event_traits_has_args",
                      "has_args reflects the presence of a payload",
                      &tests_event_traits_has_args },
                    { "event_traits_legacy_and_both",
                      "resolves legacy-only and dual-spelling tags to their payload",
                      &tests_event_traits_legacy_and_both },
                }
            },
            dt::block_spec{
                "macros",
                "The D_EVENT / D_EVENT_EMPTY declaration macros.",
                {
                    { "d_event_payload",
                      "D_EVENT declares a tag whose payload_type is the given tuple",
                      &tests_d_event_payload },
                    { "d_event_name",
                      "D_EVENT gives the tag a name() returning its identifier",
                      &tests_d_event_name },
                    { "d_event_empty",
                      "D_EVENT_EMPTY declares a well-formed arity-zero tag",
                      &tests_d_event_empty },
                    { "d_event_arity_range",
                      "macro-declared tags report the correct arity across the range",
                      &tests_d_event_arity_range },
                    { "d_event_is_event",
                      "macro-declared tags satisfy the detection traits",
                      &tests_d_event_is_event },
                }
            },
            dt::block_spec{
                "concepts",
                "The C++20 concept layer (vacuous pass where concepts are absent).",
                {
                    { "concept_is_event",
                      "is_event holds for tags and fails for non-events",
                      &tests_concept_is_event },
                    { "concept_event_type",
                      "event_type strips qualifiers before applying is_event",
                      &tests_concept_event_type },
                    { "concept_non_event_type",
                      "non_event_type is the negation of event_type",
                      &tests_concept_non_event_type },
                    { "concept_empty_event_type",
                      "empty_event_type matches only arity-zero tags",
                      &tests_concept_empty_event_type },
                    { "concept_argument_event_type",
                      "argument_event_type matches only tags carrying a payload",
                      &tests_concept_argument_event_type },
                    { "concept_named_event_type",
                      "named_event_type matches only tags exposing a name()",
                      &tests_concept_named_event_type },
                    { "concept_unnamed_event_type",
                      "unnamed_event_type matches event tags without a name()",
                      &tests_concept_unnamed_event_type },
                    { "concept_event_of_arity",
                      "event_of_arity<N> matches tags of exactly arity N",
                      &tests_concept_event_of_arity },
                    { "concept_nullary_unary",
                      "the nullary and unary arity concepts match their arities",
                      &tests_concept_nullary_unary },
                    { "concept_binary_ternary",
                      "the binary and ternary arity concepts match their arities",
                      &tests_concept_binary_ternary },
                    { "concept_variadic_event_type",
                      "variadic_event_type matches tags of arity four or more",
                      &tests_concept_variadic_event_type },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_EC_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through event_common_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g.
// std::is_same<A, B>::value) need no defensive parentheses.  Yields the
// boolean result for accumulation at the call site.
#define D_EC_CHECK(...)                                                       \
    ::djinterp::testing::event_common_check(                                  \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_EVENT_COMMON_TESTS_
