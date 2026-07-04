/******************************************************************************
* djinterp [test]                                             test_handler.hpp
*
*   DTest framework session root.  Composes a typed `event_dispatcher` (from
* the C++ event subsystem) with per-session result counters and a tree
* walker, exposing a single facade through which every test lifecycle
* observation flows.  Built-in lifecycle events and user-defined custom
* events use the same registration and dispatch mechanism, so output
* sinks, instrumentation, and ad-hoc assertions all bind to the dispatcher
* the same way.
*
*   DESIGN:
*   test_handler owns an `event_dispatcher` and a small set of session
* counters (passed / failed / skipped / errors / pending / total).  It
* does NOT own a tree or options set; trees are passed by reference
* into `run(...)`, options are looked up externally.  The handler MAY
* hold a non-owning pointer to a `test_printer` - when set via
* `set_printer(...)`, the handler installs a bundle of listeners on
* its own `event_dispatcher` that drive the printer; when cleared, the
* bundle is unbound.  The printer itself is a listener through and
* through; this header just provides a one-line wiring API.
*
*   NODES ARE PRE-EVALUATED:
*   As of the test_object refactor, a node carries no deferred-callable
* handle: its result / status are authoritative, written by whatever
* evaluated the test (a builder, an inline check, a subtree factory)
* BEFORE the walk.  The handler therefore READS each node's status and
* fires the matching lifecycle events; it does not reach into a side
* table to produce that status.  The earlier set_callable_table(...)
* wiring is gone - deferred work is composed on the authoring side and
* its verdict written onto the node.
*
*   BUILT-IN LIFECYCLE EVENTS:
*   The built-in event TAGS are declared in test_event.hpp under the
* `djinterp::test::` sub-namespace.  This file includes that
* header and references the tags by their qualified names.
*
*   CUSTOM EVENTS:
*   Users declare custom events anywhere with `D_EVENT(name, ...)` or
* `D_EVENT_EMPTY(name)` from event.hpp.  No registration step is
* required: the type IS the registration.  The handler binds and fires
* custom events through exactly the same `on<E>()` and `fire<E>()`
* methods used for built-ins.
*
*   HANDLERS RETURN A VERDICT:
*   In the refactored event subsystem a handler is a callable taking the
* event's payload and returning `void` (always-pass) or a `verdict`; it
* no longer mutates an event_context.  The dispatcher's bind() verifies
* that contract at compile time.  fire<E>() returns the enriched
* `dispatch_result` (count + final verdict); the count-returning facade
* methods below surface only its `invoked` field.
*
*   USAGE EXAMPLE - printing a custom-event warning:
*     D_EVENT_EMPTY(on_unreachable_path);
*
*     test_handler handler;
*     handler.on<on_unreachable_path>(
*         []() D_NOEXCEPT
*         {
*             std::fputs("\xE2\x80\xBC THIS SHOULD NOT HAPPEN\n", stderr);
*         });
*
*     // anywhere a test should never reach:
*     handler.fire<on_unreachable_path>();
*
*   PRINTER WIRING:
*     test_printer printer( ... );
*     test_handler handler;
*     handler.set_printer(&printer);   // installs lifecycle bundle
*     handler.run(my_tree);            // walk + dispatch
*     handler.clear_printer();         // unbinds (also done in dtor)
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
* handler constraints are activated when D_ENV_CPP_FEATURE_LANG_CONCEPTS
* is non-zero; on older toolchains the same calls compile through the
* event subsystem's static_assert path.
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY CHECKS
* II.   FORWARD DECLARATIONS
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
// I.   INCLUDES AND PORTABILITY CHECKS
// =========================================================================
//#ifndef __cplusplus
//    #error "test_handler.hpp can only be used in C++ compilation mode"
//#endif


// std
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/event/event_handler.hpp"
#include "../core/event/event_dispatcher.hpp"
#include "./test_common.hpp"
#include "./test_event.hpp"
#include "./test_object.hpp"


// feature gates
//   validated after the framework includes above.  these would
// previously fire as cascading #errors if the user included
// test_handler.hpp before djinterp.hpp; they are intentionally
// placed after the include chain so the contract is "include this
// header and it wires itself up."
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "test_handler.hpp requires C++11 or higher"
//#endif
//
//#if !D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
//    #error "test_handler.hpp requires variadic templates"
//#endif
//
//#if !D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
//    #error "test_handler.hpp requires rvalue references"
//#endif
//
//#if !D_ENV_CPP_FEATURE_LANG_LAMBDAS
//    #error "test_handler.hpp requires lambda expressions"
//#endif


NS_DJINTERP
NS_TEST


// =========================================================================
// II.  FORWARD DECLARATIONS
// =========================================================================

// test_printer
//   forward declaration: full definition lives in test_printer.hpp.
// We hold only a non-owning pointer to a test_printer; forward
// declaring it here avoids pulling the printer's transitive
// include set into every translation unit that uses test_handler.
class test_printer;


// =========================================================================
// III. SESSION RESULTS
// =========================================================================

// session_verdict
//   enum: three-way outcome classification used by
// session_result::verdict().  Distinguishes "tests
// remain unimplemented" (pending) from "tests actually
// failed" (failed) so that report writers can emit a
// truthful status line for unfinished suites.
enum class session_verdict
{
    // No leaves were observed.  Suites with zero leaves
    // are treated as empty rather than passing.
    empty,

    // Every observed leaf resolved to passed; no
    // failures, errors, or pending.
    passed,

    // No failures or errors, but at least one leaf is
    // pending (assertion not yet implemented).  Distinct
    // from `failed` because nothing actually broke; the
    // suite is incomplete, not wrong.
    pending,

    // At least one leaf failed or errored.  Errors are
    // bucketed here rather than in a separate state
    // because for verdict purposes both mean "the suite
    // does not pass."
    failed
};


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
    //
    //   NOTE: the predicate intentionally does NOT treat
    // pending as a pass.  An unfinished suite reports
    // false here so that callers using the boolean form
    // (e.g. for an exit code) cannot accidentally ship
    // green CI on a half-ported suite.  Callers that
    // want three-way reasoning (pass / pending / fail)
    // should use verdict() instead.
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

    // verdict
    //   returns the three-way classification.  See
    // session_verdict for the semantics of each state.
    //
    //   Decision order: failed/errors dominate (a real
    // failure outranks any number of pending), pending
    // is reported when nothing actually failed but work
    // remains, passed is reserved for the case where
    // every leaf resolved to passed AND at least one
    // leaf was observed.
    D_CONSTEXPR session_verdict verdict() const D_NOEXCEPT
    {
        return ( (total == 0)
            ?  session_verdict::empty
            :  ( ( (failed > 0) || (errors > 0) )
                 ?  session_verdict::failed
                 :  ( (pending > 0)
                      ?  session_verdict::pending
                      :  session_verdict::passed ) ) );
    }
};


// =========================================================================
// IV.  TEST HANDLER
// =========================================================================

// test_handler
//   class: session root for the DTest framework.  Owns one
// event_dispatcher and the per-session counters; provides a thin
// typed facade for binding listeners, firing built-in or custom
// events, walking a tree of test_object-protocol elements, and
// - when a printer is attached via set_printer - installing a
// listener bundle that drives the printer from event dispatches.
class test_handler
{
public:
    test_handler()
        : m_events(),
          m_result(),
          m_sink(),
          m_printer(nullptr),
          m_printer_listener_ids()
    {}

    // destructor unbinds any printer bundle still attached.  The
    // event_dispatcher's own destruction would tear listeners down
    // anyway, but explicit unbinding keeps the bookkeeping
    // observable in the destruction order.
    //
    //   Virtual because test_handler is designed for subclassing
    // (default_test_handler in test_defaults.hpp extends the
    // listener bundle), so polymorphic deletion through a base
    // pointer must invoke the derived destructor.
    virtual ~test_handler()
    {
        clear_printer();
    }

    // ---- listener registration ----

    // on
    //   binds a callable as a listener for _Event on this
    // handler's event_dispatcher.  Returns the handler_id for
    // later enable / disable / unbind.
    template<typename _Event,
             typename _Callable>
    handler_id on(
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
        handler_id _id
    )
    {
        return m_events.unbind(_id);
    }

    // enable
    //   re-enables a previously disabled listener.
    bool enable(
        handler_id _id
    )
    {
        return m_events.enable(_id);
    }

    // disable
    //   suspends a listener without removing it.  Disabled
    // listeners are skipped during dispatch but remain bound.
    bool disable(
        handler_id _id
    )
    {
        return m_events.disable(_id);
    }

    // ---- dispatch ----

    // fire
    //   dispatches _Event immediately to every enabled
    // listener.  Returns the number of listeners invoked
    // (the `invoked` field of the dispatcher's enriched
    // dispatch_result).
    template<typename _Event,
             typename... _Args>
    std::size_t fire(
        _Args&&... _args
    )
    {
        return m_events.fire<_Event>(
            std::forward<_Args>(_args)...).invoked;
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
        return m_events.has_handlers_for<_Event>();
    }

    // ---- printer wiring ----
    //
    //   The printer is a listener through and through.  set_printer
    // does the wiring on the user's behalf: it stores a non-owning
    // pointer to the printer and binds a bundle of listeners on
    // this handler's event_dispatcher whose bodies forward to the
    // printer's lifecycle methods.  clear_printer() unbinds the
    // bundle.  The handler does NOT take ownership of the printer.
    //
    //   Subclasses (e.g. default_test_handler in test_defaults.hpp)
    // may extend the bundle with additional listeners - typically
    // for value-tagged events with threshold filtering - by
    // overriding install_printer_listeners() / uninstall_printer_listeners().

    // set_printer
    //   attaches a printer.  If a printer was previously attached,
    // its listener bundle is removed before the new bundle is
    // installed.  Pass nullptr to detach without re-attaching;
    // equivalent to clear_printer().
    void set_printer(
        test_printer* _printer
    )
    {
        clear_printer();

        if (_printer == nullptr)
        {
            return;
        }

        m_printer = _printer;
        install_printer_listeners();

        return;
    }

    // clear_printer
    //   unbinds the printer bundle and forgets the printer
    // pointer.  Idempotent.
    void clear_printer() D_NOEXCEPT
    {
        if (m_printer == nullptr)
        {
            return;
        }

        uninstall_printer_listeners();
        m_printer = nullptr;

        return;
    }

    // printer
    //   returns the currently attached printer, or nullptr if
    // none is attached.
    D_CONSTEXPR test_printer*
    printer() const D_NOEXCEPT
    {
        return m_printer;
    }

    // ---- session lifecycle ----

    // start_session
    //   resets counters and fires on_session_start.
    // Idempotent across nested calls; the counter reset is
    // the meaningful state change.
    void start_session()
    {
        m_result = session_result();
        m_events.fire<on_session_start>();

        return;
    }

    // end_session
    //   fires on_session_end with the current pass /
    // fail counters.  Counters are NOT cleared; call
    // reset_counters() explicitly if a clean slate is wanted.
    void end_session()
    {
        m_events.fire<on_session_end>(
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
    // type_id() accessors).  Each node's status is taken as
    // authoritative; the walk fires events and tallies, it
    // does not evaluate.
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
    //   returns a reference to the underlying event_dispatcher
    // for advanced use (direct table access, batch listener
    // operations, custom queue introspection).
    event_dispatcher& events() D_NOEXCEPT
    {
        return m_events;
    }

    const event_dispatcher& events() const D_NOEXCEPT
    {
        return m_events;
    }

    // ---- assertion sink ----

    // sink
    //   returns the per-session assertion sink (mutable).  This is
    // the same vector that record_assertion appends to via
    // push_assertion; callers can enumerate or clear it as needed.
    std::vector<basic_test>& sink() D_NOEXCEPT
    {
        return m_sink;
    }

    // sink
    //   const overload - read-only enumeration of recorded assertions.
    const std::vector<basic_test>& sink() const D_NOEXCEPT
    {
        return m_sink;
    }

    // push_assertion
    //   appends a basic_test node to the internal sink.  Called by
    // record_assertion / record_status / run_unit_test in
    // test_defaults.hpp; safe to call directly from custom recorders
    // that want to drop a leaf into the report without touching the
    // counter side of the handler.
    void push_assertion(
        const basic_test& _node
    )
    {
        m_sink.push_back(_node);
        return;
    }

    // clear_sink
    //   wipes every recorded assertion.  Counters in m_result are
    // unaffected; call reset_counters() to zero those.
    void clear_sink() D_NOEXCEPT
    {
        m_sink.clear();
        return;
    }

protected:
    // install_printer_listeners
    //   binds the listener bundle that drives the printer from
    // lifecycle events.  Called from set_printer() after
    // m_printer has been stored.
    //
    //   The base class definition is a no-op: this header keeps
    // test_printer forward-declared (not included) to stay light,
    // so it cannot synthesize listener bodies that call into the
    // printer's methods directly.  Subclasses that include
    // test_printer.hpp - e.g. default_test_handler in
    // test_defaults.hpp - override this method to install the
    // lifecycle bundle and any additional value-tagged listeners.
    //   Each bound handler_id MUST be appended to
    // m_printer_listener_ids so that uninstall_printer_listeners()
    // can unbind it on teardown or printer replacement.
    virtual void install_printer_listeners()
    {
        return;
    }

    // uninstall_printer_listeners
    //   unbinds every handler_id in m_printer_listener_ids
    // and clears the vector.  Subclasses that override
    // install_printer_listeners() do NOT need to override
    // this method - the base implementation handles all
    // handler_ids regardless of which subclass added them,
    // because every binding goes through the same vector.
    void uninstall_printer_listeners() D_NOEXCEPT
    {
        for (handler_id id : m_printer_listener_ids)
        {
            m_events.unbind(id);
        }

        m_printer_listener_ids.clear();

        return;
    }

private:
    // visit_node
    //   dispatches the appropriate lifecycle events for a
    // single node and updates the counters.  Interior nodes
    // produce module_start / module_end pairs; leaf nodes
    // produce test_start / status-event / test_end triples.
    //
    //   The node's status is authoritative: it was written by
    // whatever evaluated the test before the walk.  The handler
    // reads it, fires the matching status-specific event, and
    // tallies it; it performs no evaluation of its own.  Walks
    // therefore observe a const node.
    void visit_node(
        const basic_test& _node
    )
    {
        const basic_test* obj_ptr = &_node;
        bool              is_leaf = node_is_leaf(_node);

        // dispatch interior-vs-leaf entry event
        if (is_leaf)
        {
            fire_if_listened<on_test_start>(obj_ptr);
        }
        else
        {
            fire_if_listened<on_module_start>(obj_ptr);
        }

        // dispatch the status-specific event for leaf nodes
        if (is_leaf)
        {
            dispatch_status_event(obj_ptr,
                                  to_test_status(_node.status()));
            increment_for(to_test_status(_node.status()));

            fire_if_listened<on_test_end>(obj_ptr);
        }
        else
        {
            fire_if_listened<on_module_end>(obj_ptr);
        }

        return;
    }

    // to_test_status
    //   helper: maps the basic_test's numeric status_type
    // (uint8_t by default) onto the framework-wide test_status
    // enum used by the counters and status events.
    static test_status to_test_status(
        typename basic_test::status_type _raw
    ) D_NOEXCEPT
    {
        switch (_raw)
        {
            case basic_test::status_passed:  return test_status::passed;
            case basic_test::status_failed:  return test_status::failed;
            case basic_test::status_skipped: return test_status::skipped;
            case basic_test::status_pending: return test_status::pending;
            case basic_test::status_error:   return test_status::error;
            default:                         return test_status::pending;
        }
    }

    // dispatch_status_event
    //   maps a test_status value to the corresponding
    // status-specific event and fires it (gated by
    // listener presence to keep the no-listener path free).
    void dispatch_status_event(
        const basic_test* _obj,
        test_status       _status
    )
    {
        switch (_status)
        {
            case test_status::passed:
                fire_if_listened<on_test_passed>(_obj);
                break;

            case test_status::failed:
                fire_if_listened<on_test_failed>(_obj);
                break;

            case test_status::skipped:
                fire_if_listened<on_test_skipped>(_obj);
                break;

            case test_status::error:
                fire_if_listened<on_test_error>(
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
        if (m_events.has_handlers_for<_Event>())
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
    //   classification: returns true iff _node is a leaf (an
    // assertion or a test-function wrapper) per the kind
    // taxonomy declared in test_defaults.hpp.  Interior kinds
    // (test, test_block, module) return false.
    //
    //   The leaf kinds are the two lowest-rank kinds:
    //     D_TEST_KIND_ASSERT  = 0  (rank 0, leaf)
    //     D_TEST_KIND_TEST_FN = 1  (rank 1, leaf)
    //   Higher rank values are interior nodes (test, block,
    // module).  This rank-based predicate avoids a registry
    // lookup at every visit and keeps the walk dependency-free
    // - test_handler.hpp does not need to include
    // test_defaults.hpp for the kind enumeration.
    //
    //   Custom test_handler subclasses or alternative walkers
    // that use a richer kind taxonomy may override this method
    // (it is virtual) to plug in a kind-set-aware predicate
    // (e.g. resolving through is_leaf(kinds, id) from
    // test_kind.hpp).
    virtual bool node_is_leaf(
        const basic_test& _node
    ) const D_NOEXCEPT
    {
        return ( _node.type_id() <= test_type_id{1} );
    }

    event_dispatcher        m_events;
    session_result          m_result;
    std::vector<basic_test> m_sink;

protected:
    // m_printer is protected (not private) so subclasses that
    // extend the listener bundle can read the pointer when
    // building their listener bodies without going through a
    // virtual accessor on the hot path.
    test_printer*           m_printer;
    std::vector<handler_id> m_printer_listener_ids;
};


// =========================================================================
// V.   CONVENIENCE MACROS
// =========================================================================

// D_TEST_ON
//   macro: shorthand for binding a listener.  Mirrors
// `_handler.on<_Event>(_lambda)` while keeping call sites tight.
#define D_TEST_ON(_handler, _Event, _lambda)                                  \
    (_handler).template on<_Event>(_lambda)

// D_TEST_FIRE
//   macro: shorthand for immediate dispatch.  Mirrors
// `_handler.fire<_Event>(...)`.
#define D_TEST_FIRE(_handler, _Event, ...)                                    \
    (_handler).template fire<_Event>(__VA_ARGS__)

// D_TEST_QUEUE
//   macro: shorthand for deferred dispatch.  Mirrors
// `_handler.queue<_Event>(...)`.
#define D_TEST_QUEUE(_handler, _Event, ...)                                   \
    (_handler).template queue<_Event>(__VA_ARGS__)


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_HANDLER_
