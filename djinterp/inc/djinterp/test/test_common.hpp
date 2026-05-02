/******************************************************************************
* djinterp [test]                                              test_common.hpp
*
*   DTest framework common definitions header.  Provides the foundational
* types, enumerations, and event infrastructure shared by all DTest modules:
*   - test type identification (test_type_id)
*   - test status classification (passed, failed, skipped, pending, error)
*   - test event identifier type and event data structure
*   - event handler function pointer type for runtime event dispatch
*   - portable constexpr support macros for compile-time evaluation
*
*   TEST TYPE IDENTIFICATION:
*   Every test_object carries a test_type_id - a signed 32-bit integer
* that identifies the kind of test it represents.  In isolation (no
* test_type registry), the id acts as a numeric rank: a child's id
* must be <= its parent's.  When a test_tree holds a test_type
* registry, matching ids resolve to their test_kind definition, which
* supplies rank, leaf/interior classification, and default options.
*
*   CONSTEXPR EVALUATION:
*   Test objects support dual-mode evaluation: constexpr (compile-time)
* and runtime.  The D_TEST_CONSTEXPR macro resolves to D_CONSTEXPR when
* the language standard permits relaxed constexpr (C++14+), enabling
* test logic to be verified at compile time.  Event dispatch is
* runtime-only.
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and djinterp.hpp
* for namespace macros and constexpr support.  Minimum requirement is
* C++11.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST TYPE IDENTIFICATION
* II.   STATUS CLASSIFICATION
* III.  EVENT SYSTEM
* IV.   CONSTEXPR SUPPORT MACROS
*
*
* path:      /inc/djinterp/test/test_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.14
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

// std
#include <cstddef>
#include <cstdint>
// djinterp
#include "../core/djinterp.hpp"

#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST TYPE IDENTIFICATION                             ///
///////////////////////////////////////////////////////////////////////////////

// test_type_id
//   type: signed 32-bit identifier for test object types.
// In isolation, the id doubles as a numeric rank for tree
// insertion (child id <= parent id).  When a test_type
// registry is present, the id resolves to a test_kind with
// explicit rank, leaf/interior classification, and default
// options.
//
// Negative values are reserved for framework-internal
// use.  User-defined types should use non-negative values.
typedef std::int32_t test_type_id;


///////////////////////////////////////////////////////////////////////////////
///                II.  STATUS CLASSIFICATION                               ///
///////////////////////////////////////////////////////////////////////////////

// test_status
//   enum: classification of a test object's evaluation state.
// A test object transitions from `pending` to `passed` or
// `failed` upon evaluation.  `skipped` indicates intentional
// non-evaluation.  `error` indicates that evaluation itself
// could not complete.
enum class test_status
{
    passed  = 0,
    failed  = 1,
    skipped = 2,
    pending = 3,
    error   = 4
};

///////////////////////////////////////////////////////////////////////////////
///                III. EVENT SYSTEM                                         ///
///////////////////////////////////////////////////////////////////////////////

// i.   event identifier
//////////////////////////////////////////

// test_event_id
//   type: lightweight numeric identifier for test lifecycle
// events.
typedef std::size_t test_event_id;


// ii.  event data
//////////////////////////////////////////

// test_event
//   struct: encapsulates the data associated with a single
// test event.  Carries the event identifier, the status at
// the time of the event, and an optional human-readable
// message.
struct test_event
{
    test_event_id event;
    test_status   status;
    const char*   message;

    D_CONSTEXPR test_event(
        test_event_id _event,
        test_status   _status,
        const char*   _message = nullptr
    ) D_NOEXCEPT
        : event(_event),
          status(_status),
          message(_message)
    {}
};


// iii. event handler
//////////////////////////////////////////

// fn_test_event_handler
//   typedef: function pointer type for the legacy
// callback-style event handler slot stored in a
// dtest_option_set under DTestOption::handler.  Kept as a
// plain function-pointer typedef (rather than switched to
// std::function) so the typedef can sit inside test_common.hpp
// without dragging <functional> into the dependency graph.
//
// NEW CODE SHOULD PREFER the event_handler / test_handler
// path from test_handler.hpp, which offers multi-subscriber
// dispatch, typed payloads, and propagation control.  This
// typedef remains for source compatibility with existing
// option-based wiring (see test_options.hpp).
typedef void (*fn_test_event_handler)(const test_event& _event);

///////////////////////////////////////////////////////////////////////////////
///                IV.  CONSTEXPR SUPPORT MACROS                            ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_CONSTEXPR
//   macro: resolves to D_CONSTEXPR on C++14 or later, where
// relaxed constexpr permits local variables, loops, and
// assignments within constexpr functions.  On C++11, resolves
// to nothing because single-return constexpr is too restrictive
// for most test evaluation bodies.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_TEST_CONSTEXPR            D_CONSTEXPR
#else
    #define D_TEST_CONSTEXPR
#endif

// D_TEST_STATIC_CONSTEXPR
//   macro: compound qualifier composing D_STATIC and
// D_TEST_CONSTEXPR.
#define D_TEST_STATIC_CONSTEXPR         D_STATIC D_TEST_CONSTEXPR


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMMON_