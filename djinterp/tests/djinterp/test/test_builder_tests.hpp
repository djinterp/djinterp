/******************************************************************************
* djinterp [test]                                        test_builder_tests.hpp
*
*   Declarations for the unit-test suite covering test_builder.hpp - the
* fluent, functional authoring surface over a test_tree.  Each free function
* exercises one facet of the builder and returns true if every check inside it
* passed.  Tests are grouped into translation units by the section of the
* header they cover:
*
*   - test_builder_tests_kinds_summary.cpp -> II default kinds, III test_summary,
*                                             VI make_suite, the kind constants
*   - test_builder_tests_structure.cpp     -> V structure: test_module / test_block
*                                             / test, the scoped module / block
*                                             forms, cursor discipline, rank safety
*   - test_builder_tests_assertions.cpp    -> V assertions: assert_ / test_fn /
*                                             assert_all / assert_any, value-vs-
*                                             predicate wrapping, AND conjunction
*   - test_builder_tests_inline_ops.cpp    -> V inline check / expect, node ops
*                                             skip / tag, composition add / add_if
*   - test_builder_tests_events.cpp        -> V events: on_* / fire / events(),
*                                             the run() lifecycle firing, verdict
*                                             consume, listener-exception capture
*   - test_builder_tests_run.cpp           -> V run() / summarize, evaluate error
*                                             capture, each / count_if / fold,
*                                             clear, tree / callables accessors
*
*   The lone shared check helper, test_builder_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean.  The D_TB_CHECK macro is the intended call site.
*
*   Fixtures.  A handful of tiny event handlers make run()'s event stream
* observable: node_counter counts per-node firings, consuming_counter also
* returns verdict::consume to prove the word halts, throwing_handler escapes to
* drive the on_listener_threw capture path, and threw_recorder / error_recorder
* / session_end_recorder read the payloads of on_listener_threw / on_test_error
* / on_session_end.  A custom event tag (on_custom_probe) exercises the generic
* on<>/ fire<> surface.  leaf_block_kinds is a kind set in which `block` is a
* leaf, used to drive test_tree's rank rejection through the builder.
*
*   PORTABILITY: the builder is C++11, present at every standard this suite is
* built under, so the test bodies need no language guard.
*
*   NOTE: test_builder and its neighbours live in djinterp::test; the tests
* live, flat, in djinterp::testing (hence the `dt` alias below).  The lifecycle
* event tags are djinterp::test names, so they are spelled dt::on_test_passed
* etc.; `verdict` resolves unqualified, as in the event suite.
*
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_TEST_BUILDER_TESTS_
#define DJINTERP_TEST_BUILDER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
// djinterp  -- framework header first, then the header under test.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_builder.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the builder and its neighbours, which live in djinterp::test; the
// tests live here in djinterp::testing.  Declared once for the whole suite.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// test_builder_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_builder_check(
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
//  event-handler fixtures
// =========================================================================

// node_counter
//   fixture: a void (always-pass) handler for a node-payload event; counts
// its firings and records the last node it saw.
struct node_counter
{
    int*                    calls;
    const dt::basic_test**  last;

    void operator()(const dt::basic_test* _n) const
    {
        if (calls != nullptr) { ++*calls; }
        if (last  != nullptr) { *last = _n; }
    }
};

// consuming_counter
//   fixture: like node_counter but returns verdict::consume, so a second
// handler bound after it on the same event must not run.
struct consuming_counter
{
    int* calls;

    verdict operator()(const dt::basic_test*) const
    {
        if (calls != nullptr) { ++*calls; }

        return verdict::consume;
    }
};

// throwing_handler
//   fixture: a node handler that escapes with an exception, to drive the
// builder's listener-exception capture (safe_fire -> on_listener_threw).
struct throwing_handler
{
    void operator()(const dt::basic_test*) const
    {
        throw std::runtime_error("handler boom");
    }
};

// threw_recorder
//   fixture: an on_listener_threw handler; counts firings and records the
// offending event's name (its first C-string payload).
struct threw_recorder
{
    int*         calls;
    std::string* event_name;

    void operator()(const char* _event, const char* /*what*/) const
    {
        if (calls      != nullptr) { ++*calls; }
        if (event_name != nullptr) { *event_name = (_event != nullptr) ? _event : ""; }
    }
};

// error_recorder
//   fixture: an on_test_error handler; counts firings and records the
// diagnostic message payload.
struct error_recorder
{
    int*         calls;
    std::string* message;

    void operator()(const dt::basic_test*, const char* _msg) const
    {
        if (calls   != nullptr) { ++*calls; }
        if (message != nullptr) { *message = (_msg != nullptr) ? _msg : ""; }
    }
};

// session_end_recorder
//   fixture: an on_session_end handler; captures the (passed, failed) counts
// the walk reports.
struct session_end_recorder
{
    std::size_t* passed;
    std::size_t* failed;

    void operator()(std::size_t _passed, std::size_t _failed) const
    {
        if (passed != nullptr) { *passed = _passed; }
        if (failed != nullptr) { *failed = _failed; }
    }
};


// =========================================================================
//  a custom event tag (for the generic on<> / fire<> surface)
// =========================================================================

// on_custom_probe
//   a user-declared test event carrying the visited node - proves on<> and
// fire<> work for events beyond the built-in lifecycle set.
D_TEST_EVENT(on_custom_probe);


// =========================================================================
//  a kind set in which `block` is a leaf (to drive rank rejection)
// =========================================================================

// leaf_block_kinds
//   the default three kinds, except the block kind is marked a LEAF - so a
// test appended under a block is structurally rejected by test_tree, letting
// a test exercise the builder's rejection path (and the explicit-tree ctor,
// which keeps the caller's kinds rather than re-installing the defaults).
inline std::vector<dt::test_kind>
leaf_block_kinds()
{
    std::vector<dt::test_kind> kinds;

    kinds.push_back(dt::make_test_kind(dt::k_kind_module, "module", 30, false));
    kinds.push_back(dt::make_test_kind(dt::k_kind_block,  "block",  20, true ));  // leaf!
    kinds.push_back(dt::make_test_kind(dt::k_kind_test,   "test",   10, true ));

    return kinds;
}

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// kinds / summary / factory
bool tests_default_kinds_records();
bool tests_kind_id_constants();
bool tests_summary_default_zeroed();
bool tests_summary_all_passed_any_failed();
bool tests_make_suite_fresh();

// structure
bool tests_structure_module_block_test_counts();
bool tests_structure_multiple_and_nesting();
bool tests_structure_block_without_module_toplevel();
bool tests_structure_test_without_scope_toplevel();
bool tests_structure_scoped_restores_cursor();
bool tests_structure_scoped_empty_body();
bool tests_structure_rank_rejection_leaf_block();
bool tests_structure_explicit_tree_keeps_kinds();

// assertions
bool tests_assert_value_true_false();
bool tests_assert_predicate_deferred_timing();
bool tests_assert_conjunction_and();
bool tests_assert_all_and_semantics();
bool tests_assert_any_or_semantics();
bool tests_assert_all_any_vacuous();
bool tests_assert_no_open_test_is_noop();
bool tests_assert_pending_without_clause();

// inline checks / node ops / composition
bool tests_inline_check_immediate();
bool tests_inline_expect_named();
bool tests_inline_check_not_current_test();
bool tests_inline_check_throw_is_error();
bool tests_nodeops_skip();
bool tests_nodeops_tag();
bool tests_nodeops_skip_tag_no_test_noop();
bool tests_compose_add_runs_body();
bool tests_compose_add_if_conditional();

// events
bool tests_events_on_passed_failed_counts();
bool tests_events_on_skipped_and_error();
bool tests_events_status_change_fired();
bool tests_events_module_start_per_interior();
bool tests_events_session_end_counts();
bool tests_events_verdict_consume_halts();
bool tests_events_listener_exception_captured();
bool tests_events_fire_custom_and_on();
bool tests_events_survive_clear();

// run / summarize / traversal / clear / accessors
bool tests_run_summary_tally();
bool tests_run_pending_is_leaf_only();
bool tests_run_evaluate_throw_error_message();
bool tests_run_idempotent_and_incremental();
bool tests_traversal_each_visits_all();
bool tests_traversal_count_if();
bool tests_traversal_fold();
bool tests_clear_resets_state();
bool tests_accessors_tree_and_callables();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
// =========================================================================

inline dt::module_spec
builder_spec()
{
    return dt::module_spec{
        "test_builder",
        "The fluent authoring surface over a test_tree: default kinds and "
        "summary, structure building with rank safety, predicate-algebra "
        "assertions, inline checks and node ops, the event lifecycle, and "
        "run / summarize / clear.",
        {
            dt::block_spec{
                "kinds_summary",
                "Default kinds, the run summary, and the make_suite factory.",
                {
                    { "default_kinds_records",
                      "default_kinds seeds module>block>test with the leaf flags",
                      &tests_default_kinds_records },
                    { "kind_id_constants",
                      "the reserved kind ids hold their documented values",
                      &tests_kind_id_constants },
                    { "summary_default_zeroed",
                      "a default summary is all-zero and reads as all-passed",
                      &tests_summary_default_zeroed },
                    { "summary_all_passed_any_failed",
                      "all_passed / any_failed reflect the failing terms",
                      &tests_summary_all_passed_any_failed },
                    { "make_suite_fresh",
                      "make_suite yields a fresh, usable builder",
                      &tests_make_suite_fresh },
                }
            },
            dt::block_spec{
                "structure",
                "Building the forest: modules, blocks, tests, scoped forms, "
                "cursor discipline, and rank safety.",
                {
                    { "structure_module_block_test_counts",
                      "module/block/test build the nested forest and counts",
                      &tests_structure_module_block_test_counts },
                    { "structure_multiple_and_nesting",
                      "multiple modules and nested blocks tally correctly",
                      &tests_structure_multiple_and_nesting },
                    { "structure_block_without_module_toplevel",
                      "a block with no open module becomes a top-level root",
                      &tests_structure_block_without_module_toplevel },
                    { "structure_test_without_scope_toplevel",
                      "a test with no open scope becomes a top-level root",
                      &tests_structure_test_without_scope_toplevel },
                    { "structure_scoped_restores_cursor",
                      "the scoped module/block forms restore the prior cursor",
                      &tests_structure_scoped_restores_cursor },
                    { "structure_scoped_empty_body",
                      "a scoped form with an empty body still opens the node",
                      &tests_structure_scoped_empty_body },
                    { "structure_rank_rejection_leaf_block",
                      "a test under a leaf block is rejected; counters untouched",
                      &tests_structure_rank_rejection_leaf_block },
                    { "structure_explicit_tree_keeps_kinds",
                      "the explicit-tree ctor keeps the caller's kinds",
                      &tests_structure_explicit_tree_keeps_kinds },
                }
            },
            dt::block_spec{
                "assertions",
                "Predicate algebra conjoined into the current test.",
                {
                    { "assert_value_true_false",
                      "a bare boolean value is captured now",
                      &tests_assert_value_true_false },
                    { "assert_predicate_deferred_timing",
                      "a predicate is deferred to run() time",
                      &tests_assert_predicate_deferred_timing },
                    { "assert_conjunction_and",
                      "repeated assert_ conjoins under AND",
                      &tests_assert_conjunction_and },
                    { "assert_all_and_semantics",
                      "assert_all passes iff every clause passes",
                      &tests_assert_all_and_semantics },
                    { "assert_any_or_semantics",
                      "assert_any passes iff any clause passes",
                      &tests_assert_any_or_semantics },
                    { "assert_all_any_vacuous",
                      "assert_all() is vacuously true; assert_any() vacuously false",
                      &tests_assert_all_any_vacuous },
                    { "assert_no_open_test_is_noop",
                      "an assertion with no open test binds nothing",
                      &tests_assert_no_open_test_is_noop },
                    { "assert_pending_without_clause",
                      "a test with no clause stays pending",
                      &tests_assert_pending_without_clause },
                }
            },
            dt::block_spec{
                "inline_ops",
                "Immediate inline checks, node ops, and body composition.",
                {
                    { "inline_check_immediate",
                      "check evaluates now and folds into the counts",
                      &tests_inline_check_immediate },
                    { "inline_expect_named",
                      "expect is a named inline check",
                      &tests_inline_expect_named },
                    { "inline_check_not_current_test",
                      "check does not become the current test",
                      &tests_inline_check_not_current_test },
                    { "inline_check_throw_is_error",
                      "a throwing inline check records as error",
                      &tests_inline_check_throw_is_error },
                    { "nodeops_skip",
                      "skip marks the current test skipped",
                      &tests_nodeops_skip },
                    { "nodeops_tag",
                      "tag writes metadata on the current test",
                      &tests_nodeops_tag },
                    { "nodeops_skip_tag_no_test_noop",
                      "skip / tag with no open test are no-ops",
                      &tests_nodeops_skip_tag_no_test_noop },
                    { "compose_add_runs_body",
                      "add runs a body against the live cursor",
                      &tests_compose_add_runs_body },
                    { "compose_add_if_conditional",
                      "add_if runs the body only when the condition holds",
                      &tests_compose_add_if_conditional },
                }
            },
            dt::block_spec{
                "events",
                "The event lifecycle fired by run() and the subscription sugar.",
                {
                    { "events_on_passed_failed_counts",
                      "on_passed / on_failed fire once per matching leaf",
                      &tests_events_on_passed_failed_counts },
                    { "events_on_skipped_and_error",
                      "on_skipped / on_error fire for their leaves",
                      &tests_events_on_skipped_and_error },
                    { "events_status_change_fired",
                      "on_status_change fires when a deferred leaf resolves",
                      &tests_events_status_change_fired },
                    { "events_module_start_per_interior",
                      "on_module_start fires once per interior node",
                      &tests_events_module_start_per_interior },
                    { "events_session_end_counts",
                      "on_session_end carries the pass/fail counts",
                      &tests_events_session_end_counts },
                    { "events_verdict_consume_halts",
                      "verdict::consume halts the rest of an event's word",
                      &tests_events_verdict_consume_halts },
                    { "events_listener_exception_captured",
                      "a throwing listener is caught and re-reported",
                      &tests_events_listener_exception_captured },
                    { "events_fire_custom_and_on",
                      "on<>/ fire<> work for a custom event",
                      &tests_events_fire_custom_and_on },
                    { "events_survive_clear",
                      "event subscriptions survive a clear",
                      &tests_events_survive_clear },
                }
            },
            dt::block_spec{
                "run",
                "The terminal run / summarize, evaluation error capture, "
                "traversal combinators, clear, and accessors.",
                {
                    { "run_summary_tally",
                      "run tallies passed / failed / skipped over the forest",
                      &tests_run_summary_tally },
                    { "run_pending_is_leaf_only",
                      "pending counts only unimplemented leaf tests",
                      &tests_run_pending_is_leaf_only },
                    { "run_evaluate_throw_error_message",
                      "a throwing thunk records error with its diagnostic",
                      &tests_run_evaluate_throw_error_message },
                    { "run_idempotent_and_incremental",
                      "re-running is stable and reflects newly added tests",
                      &tests_run_idempotent_and_incremental },
                    { "traversal_each_visits_all",
                      "each visits every node and can mutate it",
                      &tests_traversal_each_visits_all },
                    { "traversal_count_if",
                      "count_if counts nodes matching a predicate",
                      &tests_traversal_count_if },
                    { "traversal_fold",
                      "fold left-folds an accumulator over the nodes",
                      &tests_traversal_fold },
                    { "clear_resets_state",
                      "clear empties the forest and bookkeeping",
                      &tests_clear_resets_state },
                    { "accessors_tree_and_callables",
                      "tree() and callables() expose the backing stores",
                      &tests_accessors_tree_and_callables },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TB_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through test_builder_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas need no
// defensive parentheses.
#define D_TB_CHECK(...)                                                        \
    ::djinterp::testing::test_builder_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_BUILDER_TESTS_
