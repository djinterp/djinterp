/******************************************************************************
* djinterp [test]                                             test_handler.hpp
*
*   DTest framework session root.  Composes a typed `event_handler` (from
* the C++ event subsystem) with per-session result counters and a tree
* walker, exposing a single facade through which every test lifecycle
* observation flows.  Built-in lifecycle events and user-defined custom
* events use the same registration and dispatch mechanism, so output
* sinks, instrumentation, and ad-hoc assertions all bind to the handler
* the same way.
*
*   DESIGN:
*   test_handler owns an `event_handler` and a small set of session
* counters (passed / failed / skipped / errors / pending / total).  It
* does NOT own a tree, options set, or printer; trees are passed by
* reference into `run(...)`, options are looked up externally, and
* printers attach themselves as event listeners.  This keeps the
* handler focused on a single responsibility — driving the event flow
* — and avoids pulling the entire test stack into a single header.
*
*   BUILT-IN LIFECYCLE EVENTS:
*   The `djinterp::test::events` sub-namespace declares the events the
* handler fires automatically as it walks a tree:
*     on_session_start
*     on_session_end       (passed_count, failed_count)
*     on_module_start      (const test_object*)
*     on_module_end        (const test_object*)
*     on_test_start        (const test_object*)
*     on_test_end          (const test_object*)
*     on_test_passed       (const test_object*)
*     on_test_failed       (const test_object*)
*     on_test_skipped      (const test_object*)
*     on_test_error        (const test_object*, const char* message)
*     on_status_change     (const test_object*, test_status, test_status)
*     on_listener_threw    (const char* event_name, const char* what)
*
*   The walker checks `events().has_listeners_for<E>()` before
* constructing each event, so when nothing is bound the cost reduces
* to a single `unordered_map::find` returning `end()` — no allocation,
* no payload construction, no virtual call.
*
*   CUSTOM EVENTS:
*   Users declare custom events anywhere with `D_EVENT(name, ...)` or
* `D_EVENT_EMPTY(name)` from event_traits.hpp.  No registration step
* is required: the type IS the registration.  The handler binds and
* fires custom events through exactly the same `on<E>()` and `fire<E>()`
* methods used for built-ins.
*
*   USAGE EXAMPLE — printing a custom-event warning:
*     D_EVENT_EMPTY(on_unreachable_path);
*
*     test_handler handler;
*     handler.on<on_unreachable_path>(
*         [](event_context& _ctx) D_NOEXCEPT
*         {
*             std::fputs("\xE2\x80\xBC THIS SHOULD NOT HAPPEN\n", stderr);
*         });
*
*     // anywhere a test should never reach:
*     handler.fire<on_unreachable_path>();
*
*   CONVENIENCE MACROS:
*     D_TEST_ON(_handler, _Event, _lambda)        binds a listener
*     D_TEST_FIRE(_handler, _Event, ...)          immediate dispatch
*     D_TEST_QUEUE(_handler, _Event, ...)         deferred dispatch
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version gating goes through the env.h
* and env_cpp_features.h interface (D_ENV_LANG_IS_CPP11_OR_HIGHER,
* D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES, etc.).  Concept-based
* listener constraints are activated when D_ENV_CPP_FEATURE_LANG_CONCEPTS
* is non-zero; on older toolchains the same calls compile through the
* event_handler's static_assert path.
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY CHECKS
* II.   BUILT-IN LIFECYCLE EVENTS
* III.  SESSION RESULTS
* IV.   TEST HANDLER
* V.    CONVENIENCE MACROS
*
*
* path:      /inc/djinterp/test/test_handler.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_TEST_HANDLER_
#define DJINTERP_TEST_HANDLER_ 1


// =========================================================================
// I.   PORTABILITY CHECKS
// =========================================================================

// require the C++ framework header
#ifndef DJINTERP_CPP_
    #error "test_handler.hpp requires djinterp.hpp to be included first"
#endif

#ifndef __cplusplus
    #error "test_handler.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_handler.hpp requires C++11 or higher"
#endif

#if !D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    #error "test_handler.hpp requires variadic templates"
#endif

#if !D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #error "test_handler.hpp requires rvalue references"
#endif

#if !D_ENV_CPP_FEATURE_LANG_LAMBDAS
    #error "test_handler.hpp requires lambda expressions"
#endif


// std
#include <cstddef>
#include <cstdio>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../event/event.hpp"
#include "./test_common.hpp"
#include "./test_object.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// II.  BUILT-IN LIFECYCLE EVENTS
// =========================================================================

// events
//   namespace: declarations for the lifecycle events fired
// automatically by test_handler::run(...).  User-defined events
// live in the user's own namespace; placing the built-ins here
// avoids accidental collision.
namespace events {

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
            const test_object*);

    // on_module_end
    //   event: fired when the walker leaves a module node.
    D_EVENT(on_module_end,
            const test_object*);

    // on_test_start
    //   event: fired when the walker enters a leaf test node,
    // before the node's status is observed.
    D_EVENT(on_test_start,
            const test_object*);

    // on_test_end
    //   event: fired when the walker leaves a leaf test node,
    // after the corresponding status event has been dispatched.
    D_EVENT(on_test_end,
            const test_object*);

    // on_test_passed
    //   event: fired for a leaf test whose evaluation returned
    // test_status::passed.
    D_EVENT(on_test_passed,
            const test_object*);

    // on_test_failed
    //   event: fired for a leaf test whose evaluation returned
    // test_status::failed.
    D_EVENT(on_test_failed,
            const test_object*);

    // on_test_skipped
    //   event: fired for a leaf test whose evaluation was
    // intentionally bypassed (test_status::skipped).
    D_EVENT(on_test_skipped,
            const test_object*);

    // on_test_error
    //   event: fired for a leaf test whose evaluation could not
    // complete (test_status::error).  Carries an optional
    // human-readable diagnostic.
    D_EVENT(on_test_error,
            const test_object*,
            const char*);

    // on_status_change
    //   event: fired whenever an observed status differs from
    // the prior status.  Useful for transition-driven sinks
    // (e.g. logging only the first failure of a run).
    D_EVENT(on_status_change,
            const test_object*,
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

}  // namespace events


// =========================================================================
// III. SESSION RESULTS
// =========================================================================

// session_result
//   struct: snapshot of the per-session counter aggregates
// maintained by test_handler.  Returned from
// test_handler::result() and passed to consumers that prefer
// pull-based reporting over event subscription.
struct session_result
{
    std::size_t passed;
    std::size_t failed;
    std::size_t skipped;
    std::size_t errors;
    std::size_t pending;
    std::size_t total;

    D_CONSTEXPR session_result() D_NOEXCEPT
        : passed(0),
          failed(0),
          skipped(0),
          errors(0),
          pending(0),
          total(0)
    {}

    // all_passed
    //   returns true if total > 0 and every observed node
    // resolved to test_status::passed.
    D_CONSTEXPR bool all_passed() const D_NOEXCEPT
    {
        return ( (total > 0)    &&
                 (passed == total) );
    }

    // any_failed
    //   returns true if at least one node resolved to
    // test_status::failed or test_status::error.
    D_CONSTEXPR bool any_failed() const D_NOEXCEPT
    {
        return ( (failed > 0) ||
                 (errors > 0) );
    }
};


// =========================================================================
// IV.  TEST HANDLER
// =========================================================================

// test_handler
//   class: session root for the DTest framework.  Owns one
// event_handler and the per-session counters; provides a thin
// typed facade for binding listeners, firing built-in or custom
// events, and walking a tree of test_object-protocol elements.
class test_handler
{
public:
    test_handler()
        : m_events(),
          m_result()
    {}

    // ---- listener registration ----

    // on
    //   binds a callable as a listener for _Event on this
    // handler's event_handler.  Returns the listener_id for
    // later enable / disable / unbind.
    template<typename _Event,
             typename _Callable>
    listener_id on(
        _Callable&& _fn
    )
    {
        return m_events.bind<_Event>(
            std::forward<_Callable>(_fn));
    }

    // off
    //   removes the listener identified by _id.  Returns
    // true if the listener was found and removed.
    bool off(
        listener_id _id
    )
    {
        return m_events.unbind(_id);
    }

    // enable
    //   re-enables a previously disabled listener.
    bool enable(
        listener_id _id
    )
    {
        return m_events.enable(_id);
    }

    // disable
    //   suspends a listener without removing it.  Disabled
    // listeners are skipped during dispatch but remain bound.
    bool disable(
        listener_id _id
    )
    {
        return m_events.disable(_id);
    }

    // ---- dispatch ----

    // fire
    //   dispatches _Event immediately to every enabled
    // listener.  Returns the number of listeners invoked.
    template<typename _Event,
             typename... _Args>
    std::size_t fire(
        _Args&&... _args
    )
    {
        return m_events.fire<_Event>(
            std::forward<_Args>(_args)...);
    }

    // queue
    //   enqueues _Event for later processing.  Arguments are
    // captured by value at enqueue time.
    template<typename _Event,
             typename... _Args>
    void queue(
        _Args&&... _args
    )
    {
        m_events.queue<_Event>(
            std::forward<_Args>(_args)...);

        return;
    }

    // process
    //   dispatches up to _max_events queued events.  Returns
    // the count actually dispatched.
    std::size_t process(
        std::size_t _max_events
    )
    {
        return m_events.process(_max_events);
    }

    // process_all
    //   dispatches every queued event.  Returns the count
    // dispatched.
    std::size_t process_all()
    {
        return m_events.process_all();
    }

    // ---- listener-presence query ----

    // has_listeners_for
    //   returns true if at least one listener is bound for
    // _Event.  Cheap O(1) probe; intended for short-circuiting
    // expensive payload construction at fire sites.
    template<typename _Event>
    bool has_listeners_for() const
    {
        return m_events.has_listeners_for<_Event>();
    }

    // ---- session lifecycle ----

    // start_session
    //   resets counters and fires events::on_session_start.
    // Idempotent across nested calls; the counter reset is
    // the meaningful state change.
    void start_session()
    {
        m_result = session_result();
        m_events.fire<events::on_session_start>();

        return;
    }

    // end_session
    //   fires events::on_session_end with the current pass /
    // fail counters.  Counters are NOT cleared; call
    // reset_counters() explicitly if a clean slate is wanted.
    void end_session()
    {
        m_events.fire<events::on_session_end>(
            m_result.passed,
            m_result.failed);

        return;
    }

    // ---- tree walk ----

    // run
    //   walks an iterable of test_object-protocol elements,
    // firing the appropriate lifecycle events for each node
    // and updating the session counters.  Brackets the walk
    // with start_session / end_session.
    //
    // _Iterable must expose begin() / end() yielding values
    // that satisfy the test_object protocol (status() and
    // is_leaf() / type_id() accessors).
    template<typename _Iterable>
    void run(
        _Iterable& _nodes
    )
    {
        start_session();

        for (auto& node : _nodes)
        {
            visit_node(node);
        }

        end_session();

        return;
    }

    // ---- counters ----

    // record
    //   updates the counters for an externally-observed
    // status change without firing any event.  Useful for
    // adapters bridging non-event evaluation paths into the
    // handler's bookkeeping.
    void record(
        test_status _status
    ) D_NOEXCEPT
    {
        increment_for(_status);

        return;
    }

    // reset_counters
    //   zeroes every per-session counter.  Does not affect
    // bound listeners or queued events.
    void reset_counters() D_NOEXCEPT
    {
        m_result = session_result();

        return;
    }

    // result
    //   returns the current session_result snapshot by value.
    D_CONSTEXPR session_result result() const D_NOEXCEPT
    {
        return m_result;
    }

    // passed
    //   returns the running passed count.
    D_CONSTEXPR std::size_t passed() const D_NOEXCEPT
    {
        return m_result.passed;
    }

    // failed
    //   returns the running failed count.
    D_CONSTEXPR std::size_t failed() const D_NOEXCEPT
    {
        return m_result.failed;
    }

    // skipped
    //   returns the running skipped count.
    D_CONSTEXPR std::size_t skipped() const D_NOEXCEPT
    {
        return m_result.skipped;
    }

    // errors
    //   returns the running error count.
    D_CONSTEXPR std::size_t errors() const D_NOEXCEPT
    {
        return m_result.errors;
    }

    // pending
    //   returns the running pending count.
    D_CONSTEXPR std::size_t pending() const D_NOEXCEPT
    {
        return m_result.pending;
    }

    // total
    //   returns the total nodes observed across all status
    // categories.
    D_CONSTEXPR std::size_t total() const D_NOEXCEPT
    {
        return m_result.total;
    }

    // ---- component access ----

    // events
    //   returns a reference to the underlying event_handler
    // for advanced use (direct table access, batch listener
    // operations, custom queue introspection).
    event_handler& events() D_NOEXCEPT
    {
        return m_events;
    }

    const event_handler& events() const D_NOEXCEPT
    {
        return m_events;
    }

private:
    // visit_node
    //   dispatches the appropriate lifecycle events for a
    // single node and updates the counters.  Interior nodes
    // produce module_start / module_end pairs; leaf nodes
    // produce test_start / status-event / test_end triples.
    template<typename _Object>
    void visit_node(
        const _Object& _node
    )
    {
        const test_object* obj_ptr;
        bool               is_leaf;

        obj_ptr = static_cast<const test_object*>(&_node);
        is_leaf = node_is_leaf(_node);

        // dispatch interior-vs-leaf entry event
        if (is_leaf)
        {
            fire_if_listened<events::on_test_start>(obj_ptr);
        }
        else
        {
            fire_if_listened<events::on_module_start>(obj_ptr);
        }

        // dispatch the status-specific event for leaf nodes
        if (is_leaf)
        {
            dispatch_status_event(obj_ptr, _node.status());
            increment_for(_node.status());

            fire_if_listened<events::on_test_end>(obj_ptr);
        }
        else
        {
            fire_if_listened<events::on_module_end>(obj_ptr);
        }

        return;
    }

    // dispatch_status_event
    //   maps a test_status value to the corresponding
    // status-specific event and fires it (gated by
    // listener presence to keep the no-listener path free).
    void dispatch_status_event(
        const test_object* _obj,
        test_status        _status
    )
    {
        switch (_status)
        {
            case test_status::passed:
                fire_if_listened<events::on_test_passed>(_obj);
                break;

            case test_status::failed:
                fire_if_listened<events::on_test_failed>(_obj);
                break;

            case test_status::skipped:
                fire_if_listened<events::on_test_skipped>(_obj);
                break;

            case test_status::error:
                fire_if_listened<events::on_test_error>(
                    _obj,
                    static_cast<const char*>(nullptr));
                break;

            case test_status::pending:
                // no status event; pending is a non-terminal
                // observation
                break;
        }

        return;
    }

    // fire_if_listened
    //   short-circuits the no-listener path: skips payload
    // construction and the dispatch call entirely when no
    // listeners are bound for _Event.
    template<typename _Event,
             typename... _Args>
    void fire_if_listened(
        _Args&&... _args
    )
    {
        if (m_events.has_listeners_for<_Event>())
        {
            m_events.fire<_Event>(
                std::forward<_Args>(_args)...);
        }

        return;
    }

    // increment_for
    //   updates the appropriate counter for an observed
    // status, plus the running total.
    void increment_for(
        test_status _status
    ) D_NOEXCEPT
    {
        switch (_status)
        {
            case test_status::passed:
                ++m_result.passed;
                break;

            case test_status::failed:
                ++m_result.failed;
                break;

            case test_status::skipped:
                ++m_result.skipped;
                break;

            case test_status::error:
                ++m_result.errors;
                break;

            case test_status::pending:
                ++m_result.pending;
                break;
        }

        ++m_result.total;

        return;
    }

    // node_is_leaf
    //   protocol probe: returns true if the node exposes a
    // truthy `is_leaf()`; defaults to true when the protocol
    // member is absent so that flat iterables behave as
    // leaf-only collections.
    template<typename _Object>
    static bool node_is_leaf(
        const _Object& _node
    )
    {
        return node_is_leaf_impl(_node, 0);
    }

    template<typename _Object>
    static auto node_is_leaf_impl(
        const _Object& _node,
        int /*_overload*/
    ) -> decltype(_node.is_leaf())
    {
        return _node.is_leaf();
    }

    template<typename _Object>
    static bool node_is_leaf_impl(
        const _Object& /*_node*/,
        ...
    )
    {
        return true;
    }


    event_handler  m_events;
    session_result m_result;
};


// =========================================================================
// V.   CONVENIENCE MACROS
// =========================================================================

// D_TEST_ON
//   macro: shorthand for binding a listener.  Mirrors
// `_handler.on<_Event>(_lambda)` while keeping call sites tight.
#define D_TEST_ON(_handler, _Event, _lambda)                       \
    (_handler).template on<_Event>(_lambda)

// D_TEST_FIRE
//   macro: shorthand for immediate dispatch.  Mirrors
// `_handler.fire<_Event>(...)`.
#define D_TEST_FIRE(_handler, _Event, ...)                         \
    (_handler).template fire<_Event>(__VA_ARGS__)

// D_TEST_QUEUE
//   macro: shorthand for deferred dispatch.  Mirrors
// `_handler.queue<_Event>(...)`.
#define D_TEST_QUEUE(_handler, _Event, ...)                        \
    (_handler).template queue<_Event>(__VA_ARGS__)


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_HANDLER_