/******************************************************************************
* djinterp [test]                                        event_common_tests.hpp
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
* path:      /tests/djinterp/core/event/event_common/event_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

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
#include <djinterp/core/event/event_common.hpp>


NS_DJINTERP
NS_TESTING


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
