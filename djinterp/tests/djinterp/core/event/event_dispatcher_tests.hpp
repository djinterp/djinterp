/******************************************************************************
* djinterp [test]                                    event_dispatcher_tests.hpp
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
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_EVENT_DISPATCHER_TESTS_
#define DJINTERP_EVENT_DISPATCHER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <vector>
// djinterp  -- framework header FIRST (event_dispatcher.hpp requires it), then
// the header under test.
#include <djinterp/core/djinterp.hpp>
#include <djinterp/core/event/event_dispatcher.hpp>


NS_DJINTERP
NS_TESTING


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
