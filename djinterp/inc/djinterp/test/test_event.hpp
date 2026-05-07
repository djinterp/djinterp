/******************************************************************************
* djinterp [test]                                                test_event.hpp
*
*   Centralized test-event module for the DTest framework.  Hosts the
* built-in lifecycle event tags (formerly in test_handler.hpp), the
* templated `test_event<_IntType>` payload struct used for value-tagged
* events, and small helpers that operate over both.
*
*   DESIGN:
*   This header is the single source of truth for "what events does the
* test framework dispatch?"  It does not own the dispatcher itself -
* `event_handler` (from the C++ event subsystem) is the dispatcher and
* `test_handler` is the session façade - but every event TAG that the
* test framework defines lives in this module's `events::` sub-namespace,
* alongside the templated payload struct used by value-tagged events.
*
*   TWO EVENT FAMILIES:
*   The framework has two distinct families of events:
*
*     1. LIFECYCLE EVENTS - declared via D_EVENT, fired automatically
*        by the test_handler walk.  Each carries a distinct payload
*        signature appropriate to that lifecycle moment (a
*        `const basic_test*`, a counter pair, etc.).  Listeners always
*        run when bound; there is no value gating.
*
*     2. VALUE-TAGGED EVENTS - declared in test_defaults.hpp via D_EVENT
*        with a `const test_event<_IntType>&` payload.  The carried
*        struct has a single integer value field of the requested
*        width, plus optional name and message strings.  Listeners may
*        be gated by value threshold (see default_test_handler in
*        test_defaults.hpp).
*
*   `test_event<_IntType>` lives in the `events::` sub-namespace to
* avoid colliding with the legacy non-templated `test::test_event`
* declared in test_common.hpp (kept for source compatibility with the
* old option-based callback wiring).  New code should always reach for
* `events::test_event<...>`.
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version gating goes through env.h and
* env_cpp_features.h.  The templated payload uses only `static_assert`
* and standard type traits available in C++11.
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY CHECKS
* II.   LIFECYCLE EVENT TAGS
* III.  VALUE-TAGGED EVENT PAYLOAD
* IV.   PAYLOAD HELPERS
*
*
* path:      /inc/djinterp/test/test_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_TEST_EVENT_
#define DJINTERP_TEST_EVENT_ 1


// =========================================================================
// I.   INCLUDES AND PORTABILITY CHECKS
// =========================================================================

#ifndef __cplusplus
    #error "test_event.hpp can only be used in C++ compilation mode"
#endif


// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
// djinterp  --  pull the environment header chain FIRST so that the
// feature-flag macros below are defined before we test them.
#include "../core/djinterp.hpp"
#include "../core/event/event.hpp"
#include "./test_common.hpp"
#include "./test_object.hpp"


// feature gates
//   the templated payload requires C++11's std::is_integral.  the
// D_EVENT-based tags require variadic templates, which are also a
// C++11 feature.  these checks mirror those in test_handler.hpp so
// either header may be included independently.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_event.hpp requires C++11 or higher"
#endif

#if !D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    #error "test_event.hpp requires variadic templates"
#endif


NS_DJINTERP
NS_TEST


// events
//   namespace: declarations for every test-framework event TAG
// (lifecycle and value-tagged) and the templated payload struct
// used by value-tagged events.  User-defined events live in the
// user's own namespace; placing the framework's events here
// avoids accidental collision and gives a single place to look
// when wiring listeners.
namespace events {


// =========================================================================
// II.  LIFECYCLE EVENT TAGS
// =========================================================================
//   These were formerly declared inline in test_handler.hpp.  They
// have been moved here as part of the centralization that gives
// test_event.hpp ownership of "what events does the framework
// define?"  The semantics are unchanged.

// on_session_start
//   event: fired once at the beginning of a run, before any
// node is visited.
D_EVENT_EMPTY(on_session_start);

// on_session_end
//   event: fired once at the end of a run; carries the
// accumulated pass and fail counters.
D_EVENT(on_session_end,
        std::size_t,    // _passed
        std::size_t);   // _failed

// on_module_start
//   event: fired when the walker enters an interior node
// identified as a module (rank-aware).
D_EVENT(on_module_start,
        const basic_test*);

// on_module_end
//   event: fired when the walker leaves a module node.
D_EVENT(on_module_end,
        const basic_test*);

// on_test_start
//   event: fired when the walker enters a leaf test node,
// before the node's status is observed.
D_EVENT(on_test_start,
        const basic_test*);

// on_test_end
//   event: fired when the walker leaves a leaf test node,
// after the corresponding status event has been dispatched.
D_EVENT(on_test_end,
        const basic_test*);

// on_test_passed
//   event: fired for a leaf test whose evaluation returned
// test_status::passed.
D_EVENT(on_test_passed,
        const basic_test*);

// on_test_failed
//   event: fired for a leaf test whose evaluation returned
// test_status::failed.
D_EVENT(on_test_failed,
        const basic_test*);

// on_test_skipped
//   event: fired for a leaf test whose evaluation was
// intentionally bypassed (test_status::skipped).
D_EVENT(on_test_skipped,
        const basic_test*);

// on_test_error
//   event: fired for a leaf test whose evaluation could not
// complete (test_status::error).  Carries an optional
// human-readable diagnostic.
D_EVENT(on_test_error,
        const basic_test*,
        const char*);

// on_status_change
//   event: fired whenever an observed status differs from
// the prior status.  Useful for transition-driven sinks
// (e.g. logging only the first failure of a run).
D_EVENT(on_status_change,
        const basic_test*,
        test_status,    // _from
        test_status);   // _to

// on_listener_threw
//   event: fired when a user listener escapes with an
// exception during dispatch.  Carries the event name and
// the exception's what() string.  Listeners for this event
// MUST NOT throw.
D_EVENT(on_listener_threw,
        const char*,    // _event_name
        const char*);   // _what


// =========================================================================
// III. VALUE-TAGGED EVENT PAYLOAD
// =========================================================================

// test_event
//   struct: value-tagged event payload, parameterised on the
// integer width of its `value` field.  Each `_IntType`
// instantiation produces a distinct type whose `value` member
// has the requested storage size and signedness.
//
// Usage:
//   the templated payload is paired with a D_EVENT-declared
// tag whose dispatch signature carries `const test_event<T>&`.
// See test_defaults.hpp for the default-shipped tag bindings
// and the threshold-filtered handler that consumes them.
//
//   constexpr-friendly: every member function is constexpr or
// D_TEST_CONSTEXPR (constexpr on C++14+).  The struct itself
// is trivially copyable, so passing it by value or by
// reference are both acceptable patterns.
template<typename _IntType>
struct test_event
{
    static_assert(std::is_integral<_IntType>::value,
                  "`_IntType` must be an integral type.");

    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using value_type = _IntType;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    //   produces a zero-valued event with no name or message.
    D_CONSTEXPR test_event() D_NOEXCEPT
        : value(static_cast<value_type>(0)),
          name(nullptr),
          message(nullptr)
    {}

    // from value only
    D_CONSTEXPR explicit test_event(
        value_type _value
    ) D_NOEXCEPT
        : value(_value),
          name(nullptr),
          message(nullptr)
    {}

    // from value and name
    D_CONSTEXPR test_event(
        value_type  _value,
        const char* _name
    ) D_NOEXCEPT
        : value(_value),
          name(_name),
          message(nullptr)
    {}

    // from value, name, and message
    D_CONSTEXPR test_event(
        value_type  _value,
        const char* _name,
        const char* _message
    ) D_NOEXCEPT
        : value(_value),
          name(_name),
          message(_message)
    {}

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    value_type  value;
    const char* name;
    const char* message;
};


// =========================================================================
// IV.  PAYLOAD HELPERS
// =========================================================================

// make_test_event
//   function: constructs a test_event<_IntType> with deduced
// integer type.  Convenience for fire-site code that wants to
// build a payload without naming the integer width twice.
//
// Usage:
//   handler.fire<events::on_test_event_32>(
//       events::make_test_event(static_cast<std::int32_t>(7),
//                               "name",
//                               "msg"));
template<typename _IntType>
D_CONSTEXPR_INLINE test_event<_IntType>
make_test_event(
    _IntType    _value,
    const char* _name    = nullptr,
    const char* _message = nullptr
) D_NOEXCEPT
{
    return test_event<_IntType>(_value, _name, _message);
}


}  // namespace events


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_EVENT_
