/******************************************************************************
* djinterp [test]                                          test_spec_runner.hpp
*
*   The one-call bridge from a module_spec to BOTH framework models, factored
* out of the individual suite runners so each of those collapses to a single
* statement.  A runner authors its module as plain data (a module_spec from
* test_defaults.hpp) and hands it here; this header does the rest:
*
*     view 1 - the SIX-KIND TREE (test_defaults.hpp):
*       build_enriched_tree lowers the spec into a rank-checked forest of
*       test_suite > test_module > test_block > unit_test > test_fn nodes,
*       stamping name + descriptor onto every node and evaluating each leaf.
*       describe_tree walks it, exhibiting the kind vocabulary and the
*       structure; tree_summary prints the compact roll-up.
*
*     view 2 - the REPORT MODEL (test_report_runner.hpp):
*       drive_report projects the same spec onto a report_builder - one unit
*       per test (its "name" the unit header) carrying one assertion whose
*       line is the test's "descriptor" - reaching the live console and, when
*       a path is given, a PDF.
*
*   The two models stay distinct (the tree carries structure + per-node
* metadata; the report carries the roll-up the document renders); authoring
* the spec once and projecting it into both is the point, so the name +
* descriptor a runner writes reach the tree and the PDF alike.
*
*   ENTRY POINTS:
*     run_module(spec, title[, subtitle][, pdf])    one suite  -> exit code
*     run_suite (name, desc, specs, title[, pdf])   the union  -> exit code
*   Both return a process exit code (0 = all leaves passed) ready to hand back
*   from main().  The lower-level pieces (describe_tree, tree_summary,
*   drive_report) are exposed for runners that want to compose their own flow.
*
*   DEPENDENCY NOTE:
*   test_defaults.hpp is kept light (it does not pull in the report / PDF
*   stack); this header is where the spec authoring surface and the
*   report_builder are deliberately brought together, so only files that
*   actually run a suite pay for the reporter.
*
*
* TABLE OF CONTENTS
* =================
* I.    STATUS LABEL
* II.   TREE VIEW        (describe_tree, tree_summary)
* III.  REPORT VIEW      (drive_report)
* IV.   ENTRY POINTS     (run_module, run_suite)
*
*
* path:      /inc/djinterp/test/test_spec_runner.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#ifndef DJINTERP_TEST_SPEC_RUNNER_
#define DJINTERP_TEST_SPEC_RUNNER_ 1

#ifndef __cplusplus
    #error "test_spec_runner.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_defaults.hpp"          // module_spec, build_enriched_tree, kinds
#include "./output/test_report_runner.hpp"     // report_builder + report model


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.    STATUS LABEL                                       ///
///////////////////////////////////////////////////////////////////////////////

// status_word
//   maps a node status code to a short label for the structural dump.
D_INLINE const char*
status_word(basic_test::status_type _s)
{
    switch (_s)
    {
        case basic_test::status_passed:  return "passed";
        case basic_test::status_failed:  return "failed";
        case basic_test::status_skipped: return "skipped";
        case basic_test::status_pending: return "pending";
        default:                         return "error";
    }
}


///////////////////////////////////////////////////////////////////////////////
///                II.   TREE VIEW                                          ///
///////////////////////////////////////////////////////////////////////////////

// describe_tree
//   flat-walks the forest, printing every named node indented by its kind's
// rank (suite outermost, leaves deepest) with its resolved kind name and
// status.  Interior nodes stay pending - only the test_fn leaves carry a
// verdict, since rolling child results up into parents is the report model's
// job, not the tree's.  The anonymous conjunctive root is skipped.
inline void
describe_tree(const enriched_tree& _tree)
{
    std::printf("  six-kind tree (%zu nodes):\n", _tree.size());

    for (auto it = _tree.begin(); it != _tree.end(); ++it)
    {
        const basic_test& node = *it;

        const std::string name = node.metadata().get("name");
        if (name.empty())
        {
            continue;                       // the implied conjunctive root
        }

        const test_type_id  id   = node.type_id();
        const char*         kind = name_of(_tree.kinds(), id);
        const std::uint16_t rank = rank_of(_tree.kinds(), id);

        // indent by rank: suite(5) -> 0, ..., test_fn(1) -> 8, assert(0) -> 10.
        const int indent = (5 - static_cast<int>(rank)) * 2;

        std::printf("  %*s[%-11s] %-30s (%s)\n",
                    indent, "",
                    kind ? kind : "?",
                    name.c_str(),
                    status_word(node.status()));
    }

    return;
}

// tree_summary
//   prints the compact roll-up for a built tree: node count and leaf
// verdicts.  Interior nodes are pending by construction, so the health
// signal is the leaf failure count, not all_passed().
inline void
tree_summary(const enriched_tree& _tree)
{
    std::printf("  six-kind tree: %zu nodes, leaves passed %zu, failed %zu\n",
                _tree.size(),
                _tree.count_passed(),
                _tree.count_failed());

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                III.  REPORT VIEW                                        ///
///////////////////////////////////////////////////////////////////////////////

// drive_report
//   projects one module_spec onto a report_builder: one module band, then one
// unit per test (its "name" the unit header) carrying a single assertion whose
// line is the test's "descriptor".  Both reach the console and any attached
// PDF.  Blocks are a tree-level grouping; the report model is module > unit >
// check, so a block's tests appear directly under the module band here.  Each
// predicate runs once to produce its recorded verdict.
inline void
drive_report(
    report_builder&    _rb,
    const module_spec& _module
)
{
    _rb.module(_module.name, _module.descriptor);

    std::size_t bi = 0;
    for (bi = 0; bi < _module.blocks.size(); ++bi)
    {
        const block_spec& b = _module.blocks[bi];

        std::size_t ti = 0;
        for (ti = 0; ti < b.tests.size(); ++ti)
        {
            const test_spec& t = b.tests[ti];

            const bool ok = (t.fn != nullptr) ? t.fn() : false;

            _rb.open_unit(t.name);          // "name"       -> unit header
            _rb.check(t.descriptor, ok);    // "descriptor" -> assertion line
            _rb.close_unit();
        }
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.   ENTRY POINTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// run_module
//   the whole flow for ONE suite: lower the spec into the six-kind tree and
// describe it, then project the same spec onto a report_builder feeding the
// console and (when _pdf is non-null) a PDF.  Returns the report's process
// exit code.
inline int
run_module(
    const module_spec& _module,
    const char*        _title,
    const char*        _subtitle = nullptr,
    const char*        _pdf      = nullptr
)
{
    // view 1: the six-kind tree ---------------------------------------------
    enriched_tree tree =
        build_enriched_tree("event subsystem",
                            "One suite of the djinterp event core.",
                            _module);
    describe_tree(tree);

    // view 2: the report + optional PDF -------------------------------------
    report_builder rb;
    rb.set_title(_title);
    if (_subtitle != nullptr) { rb.set_subtitle(_subtitle); }
    rb.set_description(_module.descriptor);
    if (_pdf != nullptr)      { rb.use_pdf(_pdf); }

    drive_report(rb, _module);

    return rb.finish();
}

// run_suite
//   the whole flow for the UNION of suites: build one tree rooted at a
// test_suite carrying every module as a subtree (compact summary), then a
// single report with one module band per spec, to the console and (when _pdf
// is non-null) one PDF covering them all.  Returns the report's exit code.
inline int
run_suite(
    const char*                     _suite_name,
    const char*                     _suite_descriptor,
    const std::vector<module_spec>& _modules,
    const char*                     _title,
    const char*                     _pdf = nullptr
)
{
    // view 1: one tree over every module ------------------------------------
    enriched_tree tree =
        build_enriched_tree(_suite_name, _suite_descriptor, _modules);
    tree_summary(tree);

    // view 2: one report, a band per module ---------------------------------
    report_builder rb;
    rb.set_title(_title);
    rb.set_description(_suite_descriptor);
    if (_pdf != nullptr) { rb.use_pdf(_pdf); }

    std::size_t i = 0;
    for (i = 0; i < _modules.size(); ++i)
    {
        drive_report(rb, _modules[i]);
    }

    return rb.finish();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_SPEC_RUNNER_
