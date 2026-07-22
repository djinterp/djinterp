/******************************************************************************
* djinterp [test]                                               test_event.hpp
*
*   The DTest event alphabet -- the set of event TAGS the test framework
* dispatches over, declared against the formal event subsystem (event.hpp).
* In that subsystem an event is not a class instance but a TYPE: a tag with a
* nested `payload_type` (a std::tuple of value domains) and a `name()`.  The
* type IS the registration, so there is no enrolment step and the alphabet is
* open -- new summands are added simply by declaring new tags.
*
*   This header supplies two things:
*     1. the BUILT-IN lifecycle tags, in the `djinterp::test::events`
*        sub-namespace, that test_handler.hpp and test_builder.hpp fire as
*        they walk a tree (session, module, test, status, and diagnostic
*        events); and
*     2. the D_TEST_EVENT family of macros, so a user can declare a CUSTOM
*        test event -- one carrying the same node payload as the built-ins --
*        in a single line, and bind / fire it through exactly the same
*        handler API.
*
*   RELATION TO test_common.hpp:
*   test_common.hpp carries a small, self-contained `test_event` STRUCT (an
* {id, status, message} record) plus `test_event_id`; that is the framework's
* lightweight, event-subsystem-free observation record and is unrelated to the
* tags here.  This header instead names the formal event ALPHABET; the two
* coexist (there is no `test_event` type declared in this file).
*
*   PAYLOAD CONVENTION:
*   Every node-scoped tag carries a single `const basic_test*` -- the visited
* node, from which a handler reads status, type id, and metadata (name /
* message).  A pointer payload is why this header only FORWARD-DECLARES
* basic_test rather than including test_object.hpp: it deliberately sits below
* test_object.hpp in the include chain (both test_handler.hpp and
* test_builder.hpp include this header first), keeping the alphabet a light
* leaf of the dependency graph.  test_object.hpp completes the type.
*
*   HANDLERS RETURN A VERDICT:
*   In the refactored event subsystem a handler is any callable accepting the
* payload and returning `void` (always-pass) or a `verdict`; it does not
* mutate a context.  Compatibility is checked at bind time.  These tags carry
* no opinion about that -- a tag is only an identity and a payload shape.
*
*   USAGE -- binding a built-in lifecycle listener:
*     handler.on<on_test_failed>(
*         [](const basic_test* _n) D_NOEXCEPT
*         {
*             std::fprintf(stderr, "FAIL: %s\n",
*                          _n->metadata().get("name"));
*         });
*
*   USAGE -- declaring and firing a custom test event:
*     namespace myproj { D_TEST_EVENT(on_flaky_retry); }   // node payload
*     handler.on<myproj::on_flaky_retry>(
*         [](const basic_test*) { ... });
*     handler.fire<myproj::on_flaky_retry>(&node);
*
*   PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26.  The tags are plain types; the
* event_traits / concept machinery they plug into degrades from concepts
* (C++20+) to static_assert (C++11) inside event.hpp with no change here.
*
*
* TABLE OF CONTENTS
* =================
* I.    NODE FORWARD DECLARATIONS
* II.   BUILT-IN EVENT ALPHABET     (djinterp::test::events)
* III.  CUSTOM EVENT DECLARATION    (D_TEST_EVENT family)
*
*
* path:      /inc/djinterp/test/test_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_TEST_EVENT_
#define DJINTERP_TEST_EVENT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <tuple>
// djinterp
#include "../core/djinterp.hpp"      // NS_*, D_* qualifiers
#include "../core/event/event_common.hpp"   // verdict, event_traits, D_EVENT[_EMPTY]
#include "./test_common.hpp"         // test_status (status-change payload),
                                     // test_metadata alias (basic_test below)


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   NODE FORWARD DECLARATIONS
// =========================================================================

// test_metadata
//   the default per-node metadata container (basic_metadata<>) is NOT
// forward-declared here: it arrives as an alias from test_common.hpp (included
// above), which is where basic_metadata and its default template arguments are
// declared.  Named below only to spell the default argument of the basic_test
// alias, for which the incomplete basic_metadata<> suffices (pointer payload).

// test_object
//   forward declaration: the node template, completed in test_object.hpp.
// Declared WITHOUT default arguments (test_object.hpp specifies them); the
// node payloads below name only a pointer to a specialization, for which an
// incomplete type suffices.  The parameter list mirrors the primary template
// in test_object.hpp.
template<typename    _StatusType,
         typename    _MetadataContainer,
         typename... _Options>
struct test_object;

// basic_test
//   type: the default node instantiation (test_object<>), the element every
// lifecycle tag carries by `const basic_test*`.  Spelled with explicit
// arguments so it resolves before test_object.hpp's defaults are visible;
// test_object.hpp re-aliases the same type, which is a legal redeclaration.
using basic_test = test_object<std::uint8_t, test_metadata>;


// =========================================================================
// II.  BUILT-IN EVENT ALPHABET
// =========================================================================
//
//   The lifecycle tags fired by the framework's walkers (test_handler.hpp,
// test_builder.hpp).  Each is declared with D_EVENT / D_EVENT_EMPTY from
// event.hpp, so it carries a `payload_type` tuple and a `name()`; the payload
// shapes below match the framework's fire sites exactly.  Listeners bind to a
// tag with `handler.on<TAG>(callable)` and the framework dispatches
// with the corresponding payload.

// ---- session lifecycle ----

// on_session_start
//   the run is beginning; counters have just been reset.  Empty payload.
D_EVENT_EMPTY(on_session_start);

// on_session_end
//   the run has finished.  Payload: (passed_count, failed_count).
D_EVENT(on_session_end, std::size_t, std::size_t);


// ---- module (interior node) lifecycle ----

// on_module_start
//   an interior node (module / block / test group) is being entered.
// Payload: the node.
D_EVENT(on_module_start, const basic_test*);

// on_module_end
//   an interior node has been fully visited.  Payload: the node.
D_EVENT(on_module_end, const basic_test*);


// ---- test (leaf node) lifecycle ----

// on_test_start
//   a leaf (assertion / test function) is being entered, before its
// status-specific event.  Payload: the node.
D_EVENT(on_test_start, const basic_test*);

// on_test_end
//   a leaf has been fully visited, after its status-specific event.
// Payload: the node.
D_EVENT(on_test_end, const basic_test*);


// ---- terminal status (leaf node) ----

// on_test_passed
//   a leaf resolved to test_status::passed.  Payload: the node.
D_EVENT(on_test_passed, const basic_test*);

// on_test_failed
//   a leaf resolved to test_status::failed.  Payload: the node.
D_EVENT(on_test_failed, const basic_test*);

// on_test_skipped
//   a leaf resolved to test_status::skipped.  Payload: the node.
D_EVENT(on_test_skipped, const basic_test*);

// on_test_error
//   a leaf resolved to test_status::error.  Payload: (node, message),
// where the message is an optional diagnostic string (may be null).
D_EVENT(on_test_error, const basic_test*, const char*);

// on_status_change
//   a leaf's status changed during evaluation (e.g. a deferred leaf was
// run).  Payload: (node, before, after).
D_EVENT(on_status_change, const basic_test*, test_status, test_status);


// ---- diagnostics ----

// on_listener_threw
//   a bound listener escaped with an exception during dispatch; the
// framework caught it and re-reported it here rather than aborting the
// run.  Payload: (event_name, what), both C-strings.  A handler for this
// event must not itself throw.
D_EVENT(on_listener_threw, const char*, const char*);



NS_END  // test
NS_END  // djinterp


// =========================================================================
// III. CUSTOM EVENT DECLARATION
// =========================================================================
//
//   Future users declare custom test events with the macros below (or, for a
// fully arbitrary payload, with D_EVENT / D_EVENT_EMPTY from event.hpp
// directly).  The type IS the registration: no enrolment call is needed, and
// the new tag binds and fires through the same handler API as the built-ins.
//
//   These macros are namespace-agnostic -- they declare a tag in the current
// scope and refer to the node type by its fully-qualified name -- so a custom
// event may live in the user's own namespace, or in djinterp::test::events
// alongside the built-ins.  Each expands to a struct, so a trailing semicolon
// is supplied by the caller.

// D_TEST_EVENT
//   declares a custom test event carrying the visited node, matching the
// built-in lifecycle shape.  Payload: (const basic_test*).
// Usage:
//   D_TEST_EVENT(on_flaky_retry);
#define D_TEST_EVENT(_name)                                                   \
    D_EVENT(_name, const ::djinterp::test::basic_test*)

// D_TEST_EVENT_MSG
//   declares a custom test event carrying the node plus a C-string message,
// matching the on_test_error shape.  Payload: (const basic_test*, const char*).
// Usage:
//   D_TEST_EVENT_MSG(on_soft_warning);
#define D_TEST_EVENT_MSG(_name)                                               \
    D_EVENT(_name, const ::djinterp::test::basic_test*, const char*)

// D_TEST_EVENT_X
//   declares a custom test event carrying the node followed by caller-
//   specified extra payload domains.  Payload: (const basic_test*, ...).
// Usage:
//   D_TEST_EVENT_X(on_retry, int /*attempt*/);
#define D_TEST_EVENT_X(_name, ...)                                            \
    D_EVENT(_name, const ::djinterp::test::basic_test*, __VA_ARGS__)


#endif  // DJINTERP_TEST_EVENT_
