/*******************************************************************************
* djinterp [test]                                     event_dispatcher_tests.hpp
*
*   Declarations for the unit-test suite covering event_dispatcher.hpp.  Each
* free function exercises one entity of the event front end and returns true
* if every check inside it passed.  Tests are grouped into translation units
* by the section of event_dispatcher.hpp they cover:
*
*   - event_dispatcher_tests_queue.cpp              -> I/II. event_queue
*   - event_dispatcher_tests_dispatcher_dispatch.cpp-> III.  facade (immediate)
*   - event_dispatcher_tests_dispatcher_defer.cpp   -> III.  facade (deferred)
*   - event_dispatcher_tests_drive.cpp              -> IV.   fused drive
*   - event_dispatcher_tests_traits.cpp             -> V/VI. dispatcher traits
*   - event_dispatcher_tests_concepts.cpp           -> VII.  concepts (C++20+)
*
*   The lone shared check helper, event_dispatcher_check, reports a failing
* check (with its stringized expression and source location) and forwards the
* boolean.  The D_ED_CHECK macro is the intended call site.
*
*   IMPORTANT (include order): event_dispatcher.hpp enforces that the djinterp
* framework header is included first (it #errors otherwise -- the guard fires
* in preprocessing before its own #include of the framework header can help),
* so this test header includes djinterp/core/djinterp.hpp before it.  The
* section TUs include only this header and inherit that ordering.
*
*   Fixtures.  Event tags model payloads of arity one (ev_int), two (ev_two),
* and zero (ev_none).  Handlers take their payload value domains by value (the
* only shape handler_traits accepts): summing / two_sum prove delivery,
* consumer halts the fold, order_rec witnesses dispatch order, and reenqueue
* re-enqueues into a queue it holds a pointer to (to witness that re-entrant
* enqueues during process() are deferred).  duck is a structural duck-typed
* dispatcher (it has the ten non-template facade operations but is not an
* event_dispatcher) used to show event_dispatcher_traits is structural, not
* nominal; not_disp satisfies none of the interface.
*
*   NOTE: the entities under test live in djinterp (and djinterp::internal);
* the tests themselves live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/core/event/event_dispatcher_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_EVENT_DISPATCHER_TESTS_
#define DJINTERP_EVENT_DISPATCHER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <vector>
// djinterp
// djinterp  -- framework header FIRST (event_dispatcher.hpp requires it), then
// the header under test.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/core/event/event_dispatcher.hpp>
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

// event_dispatcher_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
event_dispatcher_check(
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
//  shared fixtures -- event tags (payload arities 1, 2, 0)
// =========================================================================

D_EVENT(ev_int, int);          // payload_type std::tuple<int>
D_EVENT(ev_two, int, int);     // payload_type std::tuple<int, int>
D_EVENT_EMPTY(ev_none);        // payload_type std::tuple<>


// =========================================================================
//  shared fixtures -- handlers (compatible: by value)
// =========================================================================

// summing
//   fixture: adds its delivered ev_int payload value into a shared sink.
struct summing
{
    int* sink;

    verdict operator()(int _x) const
    {
        if (sink)
        {
            *sink += _x;
        }

        return verdict::pass;
    }
};

// consumer
//   fixture: counts its invocations and consumes (halts the fold).
struct consumer
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

// two_sum
//   fixture: adds the sum of its two delivered ev_two payload values into a
// shared sink.
struct two_sum
{
    int* sink;

    verdict operator()(int _x, int _y) const
    {
        if (sink)
        {
            *sink += (_x + _y);
        }

        return verdict::pass;
    }
};

// order_rec
//   fixture: records its id into a shared log to witness dispatch order.
struct order_rec
{
    std::vector<int>* log;
    int               id;

    verdict operator()(int) const
    {
        if (log)
        {
            log->push_back(id);
        }

        return verdict::pass;
    }
};

// reenqueue
//   fixture: a handler that re-enqueues an ev_int occurrence into a queue it
// holds a pointer to.  Used to witness that occurrences enqueued re-entrantly
// during process() land after the current batch and are deferred.
struct reenqueue
{
    event_queue* q;

    verdict operator()(int) const
    {
        if (q)
        {
            q->enqueue<ev_int>(0);
        }

        return verdict::pass;
    }
};


// =========================================================================
//  shared fixtures -- structural dispatcher stand-ins (for traits/concepts)
// =========================================================================

// duck
//   fixture: a structural duck-typed dispatcher exposing the ten non-template
// facade operations with the exact signatures event_dispatcher_traits detects,
// yet unrelated to event_dispatcher.  Demonstrates the traits are structural.
struct duck
{
    bool unbind(handler_id)              { return true; }
    bool enable(handler_id)              { return true; }
    bool disable(handler_id)             { return true; }
    bool is_enabled(handler_id) const    { return true; }
    bool contains(handler_id) const      { return true; }
    std::size_t handler_count() const    { return 0; }
    std::size_t enabled_count() const    { return 0; }
    std::size_t pending_events() const   { return 0; }
    std::size_t process(std::size_t)     { return 0; }
    std::size_t process_all()            { return 0; }
};

// not_disp
//   fixture: a plain type satisfying none of the dispatcher interface.
struct not_disp
{
};


// =========================================================================
//  test declarations
// =========================================================================

#endif  // !DTEST_SPEC_MODE (fixtures + check helper)

// I/II. event_queue
bool tests_queue_enqueue_pending_empty_clear();
bool tests_queue_process_order_and_count();
bool tests_queue_process_partial_and_remainder();
bool tests_queue_process_max_clamped_and_all();
bool tests_queue_process_empty();
bool tests_queue_delivery_time_binding();
bool tests_queue_payload_captured_by_value();
bool tests_queue_reentrant_enqueue_deferred();
bool tests_queue_heterogeneous();

// III. event_dispatcher facade -- immediate surface
bool tests_dispatcher_bind_and_management();
bool tests_dispatcher_fire_immediate();
bool tests_dispatcher_run();
bool tests_dispatcher_compile();
bool tests_dispatcher_merge();
bool tests_dispatcher_typed_and_aggregate_queries();
bool tests_dispatcher_component_access();

// III. event_dispatcher facade -- deferred surface + queue/registry integration
bool tests_dispatcher_queue_pending();
bool tests_dispatcher_process_dispatches();
bool tests_dispatcher_queue_then_bind_then_process();
bool tests_dispatcher_process_partial_and_all();
bool tests_dispatcher_fire_vs_queue();

// IV.  fused drive
bool tests_drive_result_fields();
bool tests_drive_empty_trace();
bool tests_drive_counts_and_consumed();
bool tests_drive_empty_step();
bool tests_drive_coherence_with_run();

// V/VI. event_dispatcher_traits + typed detection
bool tests_traits_real_dispatcher();
bool tests_traits_negative();
bool tests_traits_duck_typed_structural();
bool tests_traits_clean_t_normalization();
bool tests_traits_typed_bind_fire_queue();

// VII. concepts (C++20+)
bool tests_concepts_core();
bool tests_concepts_typed_capability();
bool tests_concepts_composite();


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
event_dispatcher_spec()
{
    return dt::module_spec{
        "event_dispatcher",
        "Event front end: the deferred event_queue, the dispatcher "
        "facade (immediate and deferred), the fused drive, and the "
        "dispatcher traits and concepts.",
        {
            dt::block_spec{
                "queue",
                "The deferred event_queue: enqueue, process, delivery-time binding.",
                {
                    { "queue_enqueue_pending_empty_clear",
                      "enqueue makes the queue non-empty; clear empties it",
                      &tests_queue_enqueue_pending_empty_clear },
                    { "queue_process_order_and_count",
                      "process delivers queued occurrences in order",
                      &tests_queue_process_order_and_count },
                    { "queue_process_partial_and_remainder",
                      "processing a prefix leaves the remainder queued",
                      &tests_queue_process_partial_and_remainder },
                    { "queue_process_max_clamped_and_all",
                      "process clamps its max and can drain all",
                      &tests_queue_process_max_clamped_and_all },
                    { "queue_process_empty",
                      "processing an empty queue does nothing",
                      &tests_queue_process_empty },
                    { "queue_delivery_time_binding",
                      "the queue binds handlers at delivery time",
                      &tests_queue_delivery_time_binding },
                    { "queue_payload_captured_by_value",
                      "enqueue captures the payload by value",
                      &tests_queue_payload_captured_by_value },
                    { "queue_reentrant_enqueue_deferred",
                      "re-entrant enqueues are deferred to a later process",
                      &tests_queue_reentrant_enqueue_deferred },
                    { "queue_heterogeneous",
                      "the queue holds occurrences of differing events",
                      &tests_queue_heterogeneous },
                }
            },
            dt::block_spec{
                "dispatcher_dispatch",
                "The dispatcher facade: the immediate surface.",
                {
                    { "dispatcher_bind_and_management",
                      "the dispatcher binds handlers and manages them",
                      &tests_dispatcher_bind_and_management },
                    { "dispatcher_fire_immediate",
                      "fire dispatches an event immediately",
                      &tests_dispatcher_fire_immediate },
                    { "dispatcher_run",
                      "run folds dispatch over a trace",
                      &tests_dispatcher_run },
                    { "dispatcher_compile",
                      "the dispatcher compiles a static word",
                      &tests_dispatcher_compile },
                    { "dispatcher_merge",
                      "merge folds another dispatcher in",
                      &tests_dispatcher_merge },
                    { "dispatcher_typed_and_aggregate_queries",
                      "typed and aggregate queries report dispatcher state",
                      &tests_dispatcher_typed_and_aggregate_queries },
                    { "dispatcher_component_access",
                      "the dispatcher exposes its registry and queue",
                      &tests_dispatcher_component_access },
                }
            },
            dt::block_spec{
                "dispatcher_defer",
                "The dispatcher facade: the deferred surface and integration.",
                {
                    { "dispatcher_queue_pending",
                      "queue leaves an occurrence pending on the dispatcher",
                      &tests_dispatcher_queue_pending },
                    { "dispatcher_process_dispatches",
                      "process delivers the dispatcher's queued occurrences",
                      &tests_dispatcher_process_dispatches },
                    { "dispatcher_queue_then_bind_then_process",
                      "a handler bound after queueing still receives delivery",
                      &tests_dispatcher_queue_then_bind_then_process },
                    { "dispatcher_process_partial_and_all",
                      "the dispatcher processes a prefix or all",
                      &tests_dispatcher_process_partial_and_all },
                    { "dispatcher_fire_vs_queue",
                      "fire equals enqueue-then-process (deferral coherence)",
                      &tests_dispatcher_fire_vs_queue },
                }
            },
            dt::block_spec{
                "drive",
                "The fused drive over a trace.",
                {
                    { "drive_result_fields",
                      "a drive result carries its counts and consumed flag",
                      &tests_drive_result_fields },
                    { "drive_empty_trace",
                      "driving an empty trace yields an empty result",
                      &tests_drive_empty_trace },
                    { "drive_counts_and_consumed",
                      "drive counts passes and consumed occurrences",
                      &tests_drive_counts_and_consumed },
                    { "drive_empty_step",
                      "driving with an empty step passes each occurrence",
                      &tests_drive_empty_step },
                    { "drive_coherence_with_run",
                      "fused drive equals erased run (the coherence law)",
                      &tests_drive_coherence_with_run },
                }
            },
            dt::block_spec{
                "traits",
                "event_dispatcher_traits and typed-capability detection.",
                {
                    { "traits_real_dispatcher",
                      "dispatcher_traits reports a real dispatcher's capabilities",
                      &tests_traits_real_dispatcher },
                    { "traits_negative",
                      "dispatcher_traits rejects a non-dispatcher",
                      &tests_traits_negative },
                    { "traits_duck_typed_structural",
                      "the traits accept a structural duck-typed dispatcher",
                      &tests_traits_duck_typed_structural },
                    { "traits_clean_t_normalization",
                      "dispatcher_traits strips cv/ref before inspecting",
                      &tests_traits_clean_t_normalization },
                    { "traits_typed_bind_fire_queue",
                      "detects typed bind / fire / queue capability",
                      &tests_traits_typed_bind_fire_queue },
                }
            },
            dt::block_spec{
                "concepts",
                "The C++20 dispatcher concept layer.",
                {
                    { "concepts_core",
                      "the core dispatcher concept holds for dispatchers",
                      &tests_concepts_core },
                    { "concepts_typed_capability",
                      "typed-capability concepts detect per-event support",
                      &tests_concepts_typed_capability },
                    { "concepts_composite",
                      "the composite dispatcher concept matches the full surface",
                      &tests_concepts_composite },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_ED_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through event_dispatcher_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g. a
// fire<ev_two>(a, b) call, or a multi-argument template-id) need no defensive
// parentheses.
#define D_ED_CHECK(...)                                                       \
    ::djinterp::testing::event_dispatcher_check(                              \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_EVENT_DISPATCHER_TESTS_
