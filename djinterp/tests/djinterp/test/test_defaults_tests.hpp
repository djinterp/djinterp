/*******************************************************************************
* djinterp [test]                                        test_defaults_tests.hpp
*
*   Declarations for the unit-test suite covering test_defaults.hpp - the
* top-level test-authoring surface.  This suite exercises the plain-data +
* pure-function core: the sorted tag-metadata helpers (metadata_insert /
* _contains / _remove), the six built-in kind constants and their back-compat
* aliases, default_test_kinds and its resolved queries (rank_of / is_leaf /
* name_of), the node-identity helpers (set_identity / node_name /
* node_descriptor) and the object factories that route through them
* (make_assert / make_test_fn / make_unit_test / make_test_block /
* make_test_module / make_test_suite), the authoring aggregates (test_spec /
* block_spec / module_spec), status_for, the unit_test_tally roll-up, and
* status_word.  Each free function exercises one facet and returns true iff
* every check inside it passed.  Sections (one translation unit each):
*
*   - test_defaults_tests_metadata.cpp    -> II   sorted tag-metadata helpers
*   - test_defaults_tests_kinds.cpp       -> III/IV kind constants + default set
*   - test_defaults_tests_factories.cpp   -> V/VI  identity helpers + factories
*   - test_defaults_tests_authoring.cpp   -> spec aggregates, status_for,
*                                              unit_test_tally, status_word
*   - test_defaults_tests_integration.cpp -> integration tier: build_enriched_tree
*                                              / drive_report / run_module / run_suite,
*                                              observed on the REAL test_tree and
*                                              report_builder (rb.report() model +
*                                              exit codes)
*
*   Tests live, flat, in djinterp::testing; the authoring surface lives in
* djinterp::test.
*
*
* path:      /tests/djinterp/test/test_defaults_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_DEFAULTS_TESTS_
#define DJINTERP_TEST_DEFAULTS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first, then the header under test.  test_defaults.hpp
// is BOTH the header under test (normal mode) AND the authoring surface that
// supplies module_spec / run_module (spec mode), so it is included in both modes.
#include <djinterp/djinterp.hpp>
#include <djinterp/test/test_defaults.hpp>


NS_DJINTERP
NS_TESTING

// dt names the authoring surface (djinterp::test).
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // check helpers: normal (section-file) mode only


// =========================================================================
//  shared check helpers
// =========================================================================

// test_defaults_check
//   reports a single check.  On failure prints the stringized expression with
// its source location; always returns the condition so callers fold it into a
// running result.
inline bool
test_defaults_check(
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

// double_eq
//   tolerant floating comparison for the pass_rate checks.
inline bool
double_eq(double _a, double _b)
{
    double d = _a - _b;
    if (d < 0.0) { d = -d; }
    return d < 1e-9;
}


#endif  // !DTEST_SPEC_MODE  (check helpers)


// =========================================================================
//  test declarations (defined in the section TUs)
// =========================================================================

// metadata helpers
bool tests_metadata_insert_sorted();
bool tests_metadata_insert_duplicate();
bool tests_metadata_contains();
bool tests_metadata_remove();
bool tests_metadata_interleaved_order();

// kind constants + default set
bool tests_kinds_constant_values();
bool tests_kinds_aliases();
bool tests_kinds_default_set_size();
bool tests_kinds_default_ranks();
bool tests_kinds_default_leaf_flags();
bool tests_kinds_default_names();
bool tests_kinds_default_options_null();

// identity helpers + factories
bool tests_factory_set_identity();
bool tests_factory_node_readers();
bool tests_factory_make_assert();
bool tests_factory_make_assert_messages();
bool tests_factory_make_test_fn();
bool tests_factory_interior_nodes();
bool tests_factory_leaf_verdicts();

// authoring aggregates, status_for, tally, status_word
bool tests_authoring_spec_shape();
bool tests_authoring_status_for();
bool tests_authoring_tally_default();
bool tests_authoring_tally_pass_rate();
bool tests_authoring_status_word();

// integration tier (build_enriched_tree / drive_report / run_module / run_suite)
bool tests_integration_tree_shape();
bool tests_integration_tree_kinds();
bool tests_integration_tree_all_pass();
bool tests_integration_tree_all_fail();
bool tests_integration_tree_multi_module();
bool tests_integration_drive_report();
bool tests_integration_drive_report_events();
bool tests_integration_run_module_pass();
bool tests_integration_run_module_fail();
bool tests_integration_run_suite();
bool tests_integration_describe_summary();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
defaults_spec()
{
    return dt::module_spec{
        "test_defaults",
        "test_defaults.hpp - the top-level authoring surface: the sorted "
        "tag-metadata helpers, the six kind constants and default kind set with "
        "its resolved queries, the identity helpers and object factories, the "
        "authoring aggregates / status_for / tally / status_word, and the "
        "integration tier (build_enriched_tree / drive_report / run_module / "
        "run_suite).",
        {
            dt::block_spec{
                "metadata",
                "The sorted tag-metadata helpers: insert / contains / remove.",
                {
                    { "tests_metadata_insert_sorted",      "metadata_insert keeps the tag list sorted and de-duplicated", &tests_metadata_insert_sorted },
                    { "tests_metadata_insert_duplicate",   "inserting an existing tag is a no-op (no duplicate)", &tests_metadata_insert_duplicate },
                    { "tests_metadata_contains",           "metadata_contains reports whether a tag is present", &tests_metadata_contains },
                    { "tests_metadata_remove",             "metadata_remove erases a present tag and reports whether it removed anything", &tests_metadata_remove },
                    { "tests_metadata_interleaved_order",  "interleaved inserts still yield a fully sorted tag list", &tests_metadata_interleaved_order },
                }
            },
            dt::block_spec{
                "kinds",
                "The six kind constants, the back-compat aliases, and "
                "default_test_kinds with its resolved queries.",
                {
                    { "tests_kinds_constant_values",     "the six kind constants carry their documented id values", &tests_kinds_constant_values },
                    { "tests_kinds_aliases",             "the back-compat kind aliases resolve to their canonical constants", &tests_kinds_aliases },
                    { "tests_kinds_default_set_size",    "default_test_kinds holds exactly the built-in kinds", &tests_kinds_default_set_size },
                    { "tests_kinds_default_ranks",       "each default kind carries the hierarchy rank the taxonomy requires", &tests_kinds_default_ranks },
                    { "tests_kinds_default_leaf_flags",  "the leaf flags mark assert and test_fn as leaves, the rest interior", &tests_kinds_default_leaf_flags },
                    { "tests_kinds_default_names",       "each default kind resolves to its documented name", &tests_kinds_default_names },
                    { "tests_kinds_default_options_null","the default kinds carry no per-kind option set (all null)", &tests_kinds_default_options_null },
                }
            },
            dt::block_spec{
                "factories",
                "The identity helpers (set_identity / node_name / node_descriptor) "
                "and the object factories.",
                {
                    { "tests_factory_set_identity",         "set_identity stores a node's name and descriptor in its metadata", &tests_factory_set_identity },
                    { "tests_factory_node_readers",         "node_name / node_descriptor return empty for a node with no identity set", &tests_factory_node_readers },
                    { "tests_factory_make_assert",          "make_assert builds an assert leaf with the evaluated verdict", &tests_factory_make_assert },
                    { "tests_factory_make_assert_messages", "make_assert records the pass / fail messages in metadata", &tests_factory_make_assert_messages },
                    { "tests_factory_make_test_fn",         "make_test_fn builds a test_fn leaf with the evaluated verdict", &tests_factory_make_test_fn },
                    { "tests_factory_interior_nodes",       "the interior factories (unit_test / block / module / suite) build pending grouping nodes", &tests_factory_interior_nodes },
                    { "tests_factory_leaf_verdicts",        "a leaf's verdict follows the boolean it was built from", &tests_factory_leaf_verdicts },
                }
            },
            dt::block_spec{
                "authoring",
                "The authoring aggregates, status_for, the unit_test_tally "
                "roll-up, and status_word.",
                {
                    { "tests_authoring_spec_shape",     "the authoring aggregates (test_spec / block_spec / module_spec) store their name / descriptor / children", &tests_authoring_spec_shape },
                    { "tests_authoring_status_for",     "status_for maps a boolean to passed / failed", &tests_authoring_status_for },
                    { "tests_authoring_tally_default",  "a default unit_test_tally starts at all zeros", &tests_authoring_tally_default },
                    { "tests_authoring_tally_pass_rate","unit_test_tally::pass_rate is the passed/total percentage", &tests_authoring_tally_pass_rate },
                    { "tests_authoring_status_word",    "status_word maps each status to its lowercase word", &tests_authoring_status_word },
                }
            },
            dt::block_spec{
                "integration",
                "The integration tier: the six-kind tree build, drive_report, and "
                "the run_module / run_suite entry points.",
                {
                    { "tests_integration_tree_shape",          "build_enriched_tree lowers a spec into the six-kind tree with the expected node count", &tests_integration_tree_shape },
                    { "tests_integration_tree_kinds",          "the enriched tree carries one node of each structural kind for a single module", &tests_integration_tree_kinds },
                    { "tests_integration_tree_all_pass",       "an all-passing spec builds a tree whose leaves all pass", &tests_integration_tree_all_pass },
                    { "tests_integration_tree_all_fail",       "an all-failing spec builds a tree whose leaves all fail", &tests_integration_tree_all_fail },
                    { "tests_integration_tree_multi_module",   "a multi-module spec builds one module subtree per module", &tests_integration_tree_multi_module },
                    { "tests_integration_drive_report",        "drive_report projects a spec onto a report_builder, one check per test", &tests_integration_drive_report },
                    { "tests_integration_drive_report_events", "drive_report projects module / unit / check into the report model in order", &tests_integration_drive_report_events },
                    { "tests_integration_run_module_pass",     "run_module returns exit code 0 for an all-passing module", &tests_integration_run_module_pass },
                    { "tests_integration_run_module_fail",     "run_module returns exit code 1 when a check fails", &tests_integration_run_module_fail },
                    { "tests_integration_run_suite",           "run_suite drives every module and returns the aggregate exit code", &tests_integration_run_suite },
                    { "tests_integration_describe_summary",    "describe_tree / tree_summary report the pass / fail roll-up", &tests_integration_describe_summary },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


// D_TD_CHECK
//   evaluates its expression once and routes the result through
// test_defaults_check, capturing the expression text + source location.
// Variadic so expressions with top-level commas need no extra parentheses.
#define D_TD_CHECK(...)                                                        \
    ::djinterp::testing::test_defaults_check(                                  \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_DEFAULTS_TESTS_
