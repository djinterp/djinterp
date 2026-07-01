/******************************************************************************
* djinterp [test]                                      event_registry_tests.hpp
*
*   Declarations for the unit-test suite covering event_registry.hpp.  Each
* free function exercises one entity of the typed subscription layer and
* returns true if every check inside it passed, false otherwise.  Tests are
* grouped into translation units by the section of event_registry.hpp they
* cover:
*
*   - event_registry_tests_results.cpp       -> I.   dispatch_result/run_result
*   - event_registry_tests_fused_step.cpp    -> II.  fused_step (staging)
*   - event_registry_tests_bind_dispatch.cpp -> III. bind + dispatch (folds)
*   - event_registry_tests_run.cpp           -> III. run (the outer fold)
*   - event_registry_tests_compile.cpp       -> III. compile (the fused path)
*   - event_registry_tests_management.cpp    -> III. management/merge/queries
*
*   The lone shared check helper, event_registry_check, reports a failing
* check (with its stringized expression and source location) and forwards the
* boolean so the calling test can fold it into a running result.  The
* D_ER_CHECK macro is the intended call site.
*
*   IMPORTANT (include order): event_registry.hpp enforces that the djinterp
* framework header is included first (it #errors otherwise), so this test
* header includes djinterp/core/djinterp.hpp before event_registry.hpp.  The
* section TUs include only this header and inherit that ordering.
*
*   Fixtures.  Three event tags model payloads of arity one (ev_int), two
* (ev_two), and zero (ev_none), declared with the D_EVENT macros.  The handler
* functors are deliberately C++11-clean (no generic lambdas) and -- crucially
* -- take their payload value domains by value or const-reference, the only
* shapes handler_traits accepts (a non-const lvalue-reference handler such as
* verdict(int&) is NOT a compatible handler and will not bind).  unary_rec /
* binary_rec / nullary_rec record invocation order and return a configurable
* verdict; summing reads its payload to prove delivery; void_h returns void to
* exercise the always-pass normalization; step_rec is a raw verdict(void*)
* closure used to drive fused_step directly.
*
*   NOTE: the entities under test live in djinterp (and djinterp::internal);
* the tests themselves live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_EVENT_REGISTRY_TESTS_
#define DJINTERP_EVENT_REGISTRY_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <functional>
#include <tuple>
#include <vector>
// djinterp  -- framework header FIRST (event_registry.hpp requires it), then
// the header under test.
#include <djinterp/core/djinterp.hpp>
#include <djinterp/core/event/event_registry.hpp>


NS_DJINTERP
NS_TESTING


// =========================================================================
//  shared check helper
// =========================================================================

// event_registry_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
event_registry_check(
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
//  shared fixtures -- handlers (compatible: by value / const-ref only)
// =========================================================================

// unary_rec
//   fixture: a handler for ev_int that records its id into a shared log (to
// witness invocation order and masking) and returns a configurable verdict.
struct unary_rec
{
    std::vector<int>* log;
    int               id;
    verdict           result;

    verdict operator()(int) const
    {
        if (log)
        {
            log->push_back(id);
        }

        return result;
    }
};

// summing
//   fixture: a handler for ev_int that adds its delivered payload value into a
// shared sink, proving the dispatched arguments reach the handler.
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

// binary_rec
//   fixture: a handler for ev_two that records both delivered payload values.
struct binary_rec
{
    int* a;
    int* b;

    verdict operator()(int _x, int _y) const
    {
        if (a)
        {
            *a = _x;
        }

        if (b)
        {
            *b = _y;
        }

        return verdict::pass;
    }
};

// nullary_rec
//   fixture: a handler for ev_none (empty payload); counts its invocations.
struct nullary_rec
{
    int* n;

    verdict operator()() const
    {
        if (n)
        {
            ++*n;
        }

        return verdict::pass;
    }
};

// void_h
//   fixture: a void-returning handler for ev_int, exercising the always-pass
// normalization (void is treated as verdict::pass).
struct void_h
{
    int* n;

    void operator()(int) const
    {
        if (n)
        {
            ++*n;
        }
    }
};


// =========================================================================
//  shared fixtures -- raw erased step for direct fused_step construction
// =========================================================================

// step_rec
//   fixture: a verdict(void*) closure (the erased letter type a fused_step
// folds).  Records its id and returns a configurable verdict; ignores the
// payload pointer so direct fused_step tests need not populate the payload.
struct step_rec
{
    std::vector<int>* log;
    int               id;
    verdict           result;

    verdict operator()(void*) const
    {
        if (log)
        {
            log->push_back(id);
        }

        return result;
    }
};


// =========================================================================
//  test declarations
// =========================================================================

// I.   dispatch_result + run_result
bool tests_dispatch_result_fields_and_consumed();
bool tests_run_result_aggregate();

// II.  fused_step (the staging proposition)
bool tests_fused_step_empty();
bool tests_fused_step_fold_order_and_size();
bool tests_fused_step_short_circuit();
bool tests_fused_step_operator_and_run_one();

// III. bind + dispatch (the inner fold)
bool tests_bind_returns_valid_id_and_registers();
bool tests_bind_distinct_ids();
bool tests_dispatch_no_handlers();
bool tests_dispatch_single_pass();
bool tests_dispatch_single_consume();
bool tests_dispatch_order_and_count();
bool tests_dispatch_consume_short_circuit();
bool tests_dispatch_disabled_masked();
bool tests_dispatch_payload_delivery();
bool tests_dispatch_void_handler_and_nullary();
bool tests_dispatch_multi_event_isolation_and_cvref();

// III. run (the outer fold)
bool tests_run_empty_trace();
bool tests_run_counts_pass();
bool tests_run_consumed_count();
bool tests_run_no_handlers();
bool tests_run_short_circuit_accumulation();

// III. compile (the fused path)
bool tests_compile_empty();
bool tests_compile_size_and_fold();
bool tests_compile_masks_disabled();
bool tests_compile_snapshot_independent();

// III. delegated management + merge + typed/aggregate queries + table access
bool tests_unbind();
bool tests_enable_disable();
bool tests_is_enabled_contains();
bool tests_merge();
bool tests_typed_queries();
bool tests_aggregate_queries();
bool tests_table_access();


NS_END  // testing
NS_END  // djinterp


// D_ER_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through event_registry_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g. a
// dispatch<ev_two>(a, b) call, or a multi-argument template-id) need no
// defensive parentheses.
#define D_ER_CHECK(...)                                                       \
    ::djinterp::testing::event_registry_check(                                \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_EVENT_REGISTRY_TESTS_
