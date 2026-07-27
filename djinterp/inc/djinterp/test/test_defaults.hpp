/******************************************************************************
* djinterp [test]                                            test_defaults.hpp
*
*   Default test framework configuration: the six built-in test kinds, the
* framework's default kind set, the node factories that stamp per-node
* metadata (name + descriptor), and an enriched-tree authoring surface that
* turns the plain `bool()` test functions into a rank-checked forest of
* kinded, metadata-carrying nodes.
*
*   This header is the single source of truth for the DTest framework's
* default vocabulary of test types.  All built-in kind constants, rank
* assignments, and leaf/interior flags are defined here and nowhere else.
* The generic infrastructure in test_kind.hpp, test_object.hpp, and
* test_tree.hpp is intentionally kind-agnostic; this header supplies the
* concrete vocabulary they resolve against.
*
*   KINDS, NOT A REGISTRY:
*   The old test_type<_Container> registry (and its test_type.hpp) is retired.
* Kinds now live as a flat range of test_kind records (test_kind.hpp); the
* resolved-query free functions (rank_of / is_leaf / can_be_child_of / ...)
* read that range directly.  default_test_kinds() returns the six-record set,
* ready to seed a test_tree so its rank-checked append_child structurally
* enforces the nesting suite > module > block > unit_test > {test_fn, assert}.
*
*   TEST METADATA (name + descriptor):
*   Presentation metadata is NOT an option slot (the test_option enum carries
* rendering/packaging knobs only, no metadata key).  It lives on each node's
* test_metadata container (test_object.hpp): every factory here stamps a
* "name" and a "descriptor" entry, and those are exactly what the report
* model and the PDF renderer read back out.
*
*   BUILT-IN KINDS (ranked lowest to highest):
*     0  assert       - single boolean assertion             (leaf)
*     1  test_fn      - a wrapper around one bool() function  (leaf)
*     2  unit_test    - one named test (a group of asserts)  (interior)
*     3  test_block   - a section: a group of unit tests     (interior)
*     4  test_module  - a module: a group of blocks          (interior)
*     5  test_suite   - the whole run: a group of modules    (interior)
*
*   ENRICHED AUTHORING:
*   module_spec / block_spec / test_spec are plain-data descriptions of a
* module's tests (each test carries a name, a descriptor, and its bool()
* pointer).  build_enriched_tree() lowers a module_spec into a six-kind
* test_tree, running each function once and evaluating its test_fn leaf, so
* the forest carries both structure and verdicts with per-node metadata.
* A runner then hands that same spec to a report_builder for the PDF (name
* -> unit header, descriptor -> the unit's assertion line).
*
*   RUNNER BRIDGE (the two views of one spec):
*   The one-call runners that lower a spec into BOTH framework models live
* here too.  They were factored out into test_spec_runner.hpp while that stayed
* a separate file; that header is now a thin shim onto this one.  run_module /
* run_suite build the six-kind tree and describe it (view 1: structure +
* per-node metadata) AND project the same spec onto a report_builder feeding
* the console and an optional PDF (view 2: the roll-up the document renders).
* describe_tree / tree_summary / drive_report are the lower-level pieces, so a
* runner can compose its own flow.
*
*   Bringing the runners here means this header now pulls in the report / PDF
* stack (test_report_runner.hpp) - the reporter is no longer opt-in per
* translation unit.  A consumer that wants only the kinds / factories / handler
* and NOT the reporter should include the granular headers it needs
* (test_kind.hpp, test_object.hpp, test_tree.hpp, test_handler.hpp) directly.
*
*   PORTABILITY:
*   C++11 minimum for the authoring surface, the tree, and the handler.  The
* report bridge rides the pdf stack's C++17 gate: below C++17 the document is
* simply skipped and a run stays console-only (see test_report_runner.hpp).
* Uses env.h for version detection and djinterp.hpp for namespace macros and
* constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST METADATA TYPE (tag lists)
* II.   TEST METADATA HELPERS
* III.  BUILT-IN KIND CONSTANTS
* IV.   DEFAULT KIND SET
* V.    NODE METADATA HELPERS
* VI.   CONVENIENCE OBJECT FACTORIES (stamp name + descriptor)
* VII.  ENRICHED-TREE AUTHORING (module_spec -> six-kind test_tree)
* VIII. TEST-RECORDING HELPERS
* IX.   NUMBERED-LEAF NODE TEMPLATE
* X.    VALUE-TAGGED EVENT TAGS
* XI.   DEFAULT TEST HANDLER (THRESHOLD-FILTERED)
* XII.  STATUS LABEL                (status_word)
* XIII. TREE VIEW                   (describe_tree, tree_summary)
* XIV.  REPORT VIEW                 (drive_report)
* XV.   ENTRY POINTS                (run_module, run_suite)
*
*
* path:      /inc/djinterp/test/test_defaults.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_TEST_DEFAULTS_
#define DJINTERP_TEST_DEFAULTS_ 1

// std
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_event.hpp"
#include "./test_handler.hpp"
#include "./test_options.hpp"
#include "./test_printer.hpp"
#include "./test_object.hpp"
#include "./test_callable.hpp"   // deferred thunk table (the test_fn leaves)
#include "./test_kind.hpp"       // test_kind record + kind-set + resolved queries
#include "./test_tree.hpp"       // rank-checked forest (supersedes retired test_type.hpp)
#include "./output/test_report_runner.hpp"  // report_builder - the report / PDF view the
                                     //   run_module / run_suite bridge drives
                                     //   (formerly reached via test_spec_runner.hpp)


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                I.   TEST METADATA TYPE                                   ///
///////////////////////////////////////////////////////////////////////////////

// test_metadata_type
//   type: a sorted vector of tag strings - a convenience container for the
// optional "tags" a node may carry (labels, categories, owners) alongside
// its "name" / "descriptor" entries.  The sorted invariant is maintained by
// the insertion helpers below.  (This is NOT an option slot: the test_option
// enum carries rendering / packaging knobs only.)
using test_metadata_type = std::vector<std::string>;


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST METADATA HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

// metadata_insert
//   function: inserts a tag into a sorted metadata vector
// at its sorted position.  Duplicates are silently ignored.
inline void
metadata_insert(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    // skip duplicate
    if ( (pos != _meta.end()) &&
         (*pos == _tag) )
    {
        return;
    }

    _meta.insert(pos, _tag);

    return;
}

// metadata_contains
//   function: returns true if _tag is present in the sorted
// metadata vector.  Uses binary search.
inline bool
metadata_contains(
    const test_metadata_type& _meta,
    const std::string&        _tag
)
{
    return std::binary_search(_meta.begin(),
                              _meta.end(),
                              _tag);
}

// metadata_remove
//   function: removes _tag from the sorted metadata vector.
// Returns true if the tag was found and removed.
inline bool
metadata_remove(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    if ( (pos == _meta.end()) ||
         (*pos != _tag) )
    {
        return false;
    }

    _meta.erase(pos);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                III. BUILT-IN KIND CONSTANTS                              ///
///////////////////////////////////////////////////////////////////////////////
//
//   Six kinds, ranked lowest to highest.  The two leaves (assert, test_fn)
// carry work; the four interior kinds carry structure.  Rank monotonicity
// (child rank <= parent rank) plus the leaf flag is what test_tree's
// append_child enforces, so the nesting
//
//     test_suite > test_module > test_block > unit_test > {test_fn, assert}
//
// is structural, not merely conventional.  The prior names (D_TEST_KIND_TEST,
// D_TEST_KIND_MODULE) are kept as aliases so existing call sites keep
// compiling against the renamed unit_test / test_module ids.

// D_TEST_KIND_ASSERT
//   constant: id for a single boolean assertion.  Rank 0, leaf.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_ASSERT      = 0;

// D_TEST_KIND_TEST_FN
//   constant: id for a wrapper around one bool() test function.  Rank 1,
// leaf.  This is the "fancy wrapper to a test function pointer" - the leaf
// whose verdict is the value the wrapped function returns.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_FN     = 1;

// D_TEST_KIND_UNIT_TEST
//   constant: id for one named unit test (a logical group of assertions,
// typically the body of one tests_* function).  Rank 2, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_UNIT_TEST   = 2;

// D_TEST_KIND_TEST_BLOCK
//   constant: id for a test block - a named section grouping unit tests.
// Rank 3, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_BLOCK  = 3;

// D_TEST_KIND_TEST_MODULE
//   constant: id for a test module - a group of blocks covering one header.
// Rank 4, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_MODULE = 4;

// D_TEST_KIND_TEST_SUITE
//   constant: id for the whole run - a group of modules.  Rank 5, interior,
// top of the hierarchy.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_SUITE  = 5;

// D_TEST_KIND_COUNT
//   constant: number of built-in kind constants.
D_STATIC_CONSTEXPR std::size_t  D_TEST_KIND_COUNT       = 6;

// ---- back-compat aliases (old spellings -> new ids) ----

// D_TEST_KIND_TEST
//   alias: the pre-rename spelling of D_TEST_KIND_UNIT_TEST.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST        = D_TEST_KIND_UNIT_TEST;

// D_TEST_KIND_MODULE
//   alias: the pre-rename spelling of D_TEST_KIND_TEST_MODULE.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_MODULE      = D_TEST_KIND_TEST_MODULE;


///////////////////////////////////////////////////////////////////////////////
///                IV.  DEFAULT KIND SET                                     ///
///////////////////////////////////////////////////////////////////////////////

// default_test_kinds
//   function: the framework's default six-record kind set, ranked lowest to
// highest, ready to seed a test_tree so its rank-checked append_child
// structurally enforces suite > module > block > unit_test > {test_fn,
// assert}.  Returned as a flat std::vector<test_kind> - the resolved-query
// free functions in test_kind.hpp (rank_of / is_leaf / name_of /
// can_be_child_of) accept any range of records, so this drops straight into
// test_tree<basic_test>(default_test_kinds()).  default_options is left null
// on every record; a caller that wants per-kind rendering defaults owns a
// test_option_set and points the record at it.
D_INLINE std::vector<test_kind>
default_test_kinds()
{
    std::vector<test_kind> kinds;

    kinds.reserve(D_TEST_KIND_COUNT);

    kinds.push_back(make_test_kind(D_TEST_KIND_ASSERT,      "assert",      0, true));
    kinds.push_back(make_test_kind(D_TEST_KIND_TEST_FN,     "test_fn",     1, true));
    kinds.push_back(make_test_kind(D_TEST_KIND_UNIT_TEST,   "unit_test",   2, false));
    kinds.push_back(make_test_kind(D_TEST_KIND_TEST_BLOCK,  "test_block",  3, false));
    kinds.push_back(make_test_kind(D_TEST_KIND_TEST_MODULE, "test_module", 4, false));
    kinds.push_back(make_test_kind(D_TEST_KIND_TEST_SUITE,  "test_suite",  5, false));

    return kinds;
}

// enriched_tree
//   type: the framework's default enriched forest - a test_tree of basic_test
// nodes classified by the six default kinds.  Named here (rather than in
// test_tree.hpp) because it needs basic_test and the default kind set, both
// of which land in this header.
using enriched_tree = test_tree<basic_test>;

// make_enriched_tree
//   function: a test_tree seeded with default_test_kinds(), so every
// append_child is rank/leaf checked against the six-kind vocabulary.
D_INLINE enriched_tree
make_enriched_tree()
{
    return enriched_tree(default_test_kinds());
}


///////////////////////////////////////////////////////////////////////////////
///                V.   NODE METADATA HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////
//   Presentation metadata is per-NODE, carried on basic_test's test_metadata
// container - never an option slot.  These two helpers are the canonical way
// to stamp / read the "name" and "descriptor" every factory below writes.

// set_identity
//   helper: writes the "name" and (when non-null) "descriptor" metadata
// entries onto a node.  The single choke point every factory routes through,
// so the two keys are spelled once.
D_INLINE void
set_identity(
    basic_test& _node,
    const char* _name,
    const char* _descriptor = nullptr
)
{
    if (_name != nullptr)
    {
        _node.metadata().set("name", _name);
    }

    if (_descriptor != nullptr)
    {
        _node.metadata().set("descriptor", _descriptor);
    }

    return;
}

// node_name / node_descriptor
//   helpers: read the "name" / "descriptor" back out (empty string if unset).
D_INLINE std::string
node_name(
    const basic_test& _node
)
{
    return _node.metadata().get("name");
}

D_INLINE std::string
node_descriptor(
    const basic_test& _node
)
{
    return _node.metadata().get("descriptor");
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  CONVENIENCE OBJECT FACTORIES                         ///
///////////////////////////////////////////////////////////////////////////////

//   Every factory routes its name / descriptor through set_identity, so the
// two presentation keys are spelled in exactly one place (section V).  The
// two leaves take a result; the four interior nodes are born pending and are
// resolved by their descendants once the tree is walked.

// make_assert
//   function: an assertion-level leaf (rank 0).  Stamps "name" and (when
// given) "descriptor", plus the optional pass / fail message strings.  Not
// D_CONSTEXPR / D_NOEXCEPT: populating the metadata container may allocate.
D_INLINE basic_test
make_assert(
    bool        _result,
    const char* _name         = nullptr,
    const char* _descriptor   = nullptr,
    const char* _message_pass = nullptr,
    const char* _message_fail = nullptr
)
{
    basic_test t(D_TEST_KIND_ASSERT, _result);

    set_identity(t, _name, _descriptor);

    if (_message_pass != nullptr)
    {
        t.metadata().set("message_pass", _message_pass);
    }
    if (_message_fail != nullptr)
    {
        t.metadata().set("message_fail", _message_fail);
    }

    return t;
}

// make_test_fn
//   function: a test_fn-level leaf (rank 1) - the "fancy wrapper around one
// bool() function".  Its verdict is _result, i.e. what the wrapped function
// returned.  Stamps "name" and (when given) "descriptor".
D_INLINE basic_test
make_test_fn(
    bool        _result,
    const char* _name       = nullptr,
    const char* _descriptor = nullptr
)
{
    basic_test t(D_TEST_KIND_TEST_FN, _result);

    set_identity(t, _name, _descriptor);

    return t;
}

// make_unit_test
//   function: a unit_test-level interior node (rank 2) - one named test (the
// body of one tests_* function), grouping its assertions or its test_fn leaf.
D_INLINE basic_test
make_unit_test(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    basic_test t = make_interior(D_TEST_KIND_UNIT_TEST, _name);

    if (_descriptor != nullptr)
    {
        t.metadata().set("descriptor", _descriptor);
    }

    return t;
}

// make_test_block
//   function: a test_block-level interior node (rank 3) - a named section
// grouping unit tests (one translation unit's worth of them).
D_INLINE basic_test
make_test_block(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    basic_test t = make_interior(D_TEST_KIND_TEST_BLOCK, _name);

    if (_descriptor != nullptr)
    {
        t.metadata().set("descriptor", _descriptor);
    }

    return t;
}

// make_test_module
//   function: a test_module-level interior node (rank 4) - a group of blocks
// covering one header / subsystem.
D_INLINE basic_test
make_test_module(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    basic_test t = make_interior(D_TEST_KIND_TEST_MODULE, _name);

    if (_descriptor != nullptr)
    {
        t.metadata().set("descriptor", _descriptor);
    }

    return t;
}

// make_test_suite
//   function: the test_suite root (rank 5) - the whole run, a group of
// modules; the intended argument to test_tree::add_root.
D_INLINE basic_test
make_test_suite(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    basic_test t = make_interior(D_TEST_KIND_TEST_SUITE, _name);

    if (_descriptor != nullptr)
    {
        t.metadata().set("descriptor", _descriptor);
    }

    return t;
}

// ---- back-compat spellings (old factory names -> new nodes) ----

// make_test_case
//   alias factory: the pre-rename spelling for a unit_test interior node.
D_INLINE basic_test
make_test_case(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    return make_unit_test(_name, _descriptor);
}

// make_module
//   alias factory: the pre-rename spelling for a test_module interior node.
D_INLINE basic_test
make_module(
    const char* _name,
    const char* _descriptor = nullptr
)
{
    return make_test_module(_name, _descriptor);
}

// ---- the template string ----
D_STATIC const char* const D_TEST_TPL_MASTER_SUITE =
    "============================================"
    "====================================\n"
    "  TESTING:     %module_name%\n"
    "============================================"
    "====================================\n"
    "  description: %module_description%\n"
    "  path:        %module_path%\n"
    "  date/time:   %timestamp_start%\n"
    "============================================"
    "====================================\n"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  MODULE: %description_short%\n"
    "  %description_long%\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "%test_modules%"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  COMPREHENSIVE TEST RESULTS\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "  MODULE SUMMARY:\n"
    "    Modules Tested:       %modules_tested%\n"
    "    Modules Passed:       %modules_passed%\n"
    "    Module Success Rate:  %modules_percent%\n"

    "\n"

    "  ASSERTION SUMMARY:\n"
    "    Total Assertions:     %asserts_total%\n"
    "    Assertions Passed:    %asserts_passed%\n"
    "    Assertions Failed:    %asserts_failed%\n"
    "    Assertion Pass Rate:  %asserts_percent%\n"

    "\n"

    "  UNIT TEST SUMMARY:\n"
    "    Total Unit Tests:     %tests_total%\n"
    "    Unit Tests Passed:    %tests_passed%\n"
    "    Unit Tests Failed:    %tests_failed%\n"
    "    Unit Test Pass Rate:  %tests_percent%\n"

    "\n"

    "  EXECUTION TIME:\n"
    "    Total Time:           %time_total%\n"

    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "============================================"
    "====================================\n"
    "  MODULE RESULTS: %module_name%\n"
    "============================================"
    "====================================\n"
    "  Assertions: %asserts_passed% / %asserts_total%"
    " (%asserts_percent%)\n"
    "  Unit Tests: %tests_passed% / %tests_total%"
    " (%tests_percent%)\n"
    "  Status:     %has_passed%\n"
    "============================================"
    "====================================\n";


///////////////////////////////////////////////////////////////////////////////
///                VII. ENRICHED-TREE AUTHORING                             ///
///////////////////////////////////////////////////////////////////////////////
//
//   The plain bool() test functions become a rank-checked forest of kinded,
// metadata-carrying nodes in two moves: describe the shape as plain data
// (module_spec / block_spec / test_spec - trivially constructible aggregates
// of string literals and function pointers), then lower that data into a
// six-kind test_tree.  The lowering stamps a "name" and a "descriptor" onto
// every node and runs each test function exactly once, recording its verdict
// on a test_fn leaf.  The very same spec is what a runner hands to a
// report_builder for the PDF - so the tree and the document are two views of
// one description, and the name / descriptor reach both.
//
//   The spec deliberately models today's reality: a tests_* function reports
// a single aggregate bool, so each maps to one unit_test wrapping one test_fn
// leaf.  The assert leaf (rank 0) is a valid child of unit_test and is ready
// for the day a function surfaces its individual checks; make_assert builds
// one directly in the meantime.

// test_fn_ptr
//   type: the signature every unit test presents - a nullary predicate
// returning true iff all its checks passed.  Mirrors the report runner's
// test_predicate_fn (test_report_runner.hpp, pulled in above); kept as a local
// spelling so the authoring aggregates below read without a cross-header type.
using test_fn_ptr = bool (*)();

// test_spec
//   aggregate: one unit test - a name, a descriptor, and the predicate to
// run.  Lowered to a unit_test node wrapping a test_fn leaf.
struct test_spec
{
    const char* name;
    const char* descriptor;
    test_fn_ptr fn;
};

// block_spec
//   aggregate: a named section grouping unit tests.  Lowered to a test_block
// node.
struct block_spec
{
    const char*            name;
    const char*            descriptor;
    std::vector<test_spec> tests;
};

// module_spec
//   aggregate: one header's worth of blocks.  Lowered to a test_module node.
struct module_spec
{
    const char*             name;
    const char*             descriptor;
    std::vector<block_spec> blocks;
};

// build_module_subtree
//   function: lowers one module_spec into _tree beneath _parent (normally the
// suite root), creating test_module > test_block > unit_test > test_fn nodes,
// stamping name + descriptor at every level, and running each predicate once
// so its test_fn leaf carries the verdict.  Every insertion goes through the
// tree's rank-checked append_child, so the nesting is validated, not assumed.
// Returns the new test_module node (nullptr if _parent was rejected).
inline enriched_tree::node_type*
build_module_subtree(
    enriched_tree&            _tree,
    enriched_tree::node_type* _parent,
    const module_spec&        _module
)
{
    enriched_tree::node_type* mod =
        _tree.append_child(_parent,
                           make_test_module(_module.name, _module.descriptor));

    if (mod == nullptr)
    {
        return nullptr;
    }

    std::size_t bi = 0;
    for (bi = 0; bi < _module.blocks.size(); ++bi)
    {
        const block_spec& b = _module.blocks[bi];

        enriched_tree::node_type* blk =
            _tree.append_child(mod, make_test_block(b.name, b.descriptor));

        if (blk == nullptr)
        {
            continue;
        }

        std::size_t ti = 0;
        for (ti = 0; ti < b.tests.size(); ++ti)
        {
            const test_spec& t = b.tests[ti];

            enriched_tree::node_type* ut =
                _tree.append_child(blk,
                                   make_unit_test(t.name, t.descriptor));

            if (ut == nullptr)
            {
                continue;
            }

            // run the predicate exactly once; its return is the leaf verdict.
            const bool verdict = (t.fn != nullptr) ? t.fn() : false;

            _tree.append_child(ut,
                               make_test_fn(verdict, t.name, t.descriptor));
        }
    }

    return mod;
}

// build_enriched_tree
//   function: the whole-run entry point.  Seeds a fresh six-kind tree, roots
// it at a test_suite carrying its own name + descriptor, lowers every module
// beneath it, and returns the built tree.  Running each predicate as it
// lowers means the returned tree is already evaluated: count_passed() /
// all_passed() are meaningful the instant it returns.
inline enriched_tree
build_enriched_tree(
    const char*                     _suite_name,
    const char*                     _suite_descriptor,
    const std::vector<module_spec>& _modules
)
{
    enriched_tree tree = make_enriched_tree();

    enriched_tree::node_type* suite =
        tree.add_root(make_test_suite(_suite_name, _suite_descriptor));

    std::size_t i = 0;
    for (i = 0; i < _modules.size(); ++i)
    {
        build_module_subtree(tree, suite, _modules[i]);
    }

    return tree;
}

// build_enriched_tree (single module)
//   convenience overload: a suite of exactly one module.
inline enriched_tree
build_enriched_tree(
    const char*        _suite_name,
    const char*        _suite_descriptor,
    const module_spec& _module
)
{
    return build_enriched_tree(_suite_name,
                               _suite_descriptor,
                               std::vector<module_spec>(1, _module));
}


///////////////////////////////////////////////////////////////////////////////
///                VIII. TEST-RECORDING HELPERS                             ///
///////////////////////////////////////////////////////////////////////////////
//   These bridges run a leaf assertion through the handler so the
// session counters stay authoritative - no shadow accounting on
// the call site.  They replace ad-hoc per-translation-unit macros
// and give every test file a uniform, debuggable entry point.

// status_for
//   helper: maps a boolean result to test_status::passed or
// test_status::failed.  Other statuses are reachable via the
// explicit record_status() entry point.
D_CONSTEXPR_INLINE test_status
status_for(
    bool _ok
) D_NOEXCEPT
{
    return _ok ? test_status::passed
               : test_status::failed;
}

// record_assertion
//   helper: appends an assertion-level basic_test to the handler's
// internal sink AND advances the handler's counters via
// test_handler::record.  One source of truth for the "did this
// assertion pass?" signal -- both the printed leaf and the counter
// tally come from the same expression.
template<typename _Handler>
inline void
record_assertion(
    _Handler&   _handler,
    bool        _ok,
    const char* _name,
    const char* _msg_pass = nullptr,
    const char* _msg_fail = nullptr)
{
    _handler.push_assertion(make_assert(_ok, _name, _msg_pass, _msg_fail));
    _handler.record(status_for(_ok));
    return;
}

// record_status
//   helper: variant for non-boolean outcomes (skip / error /
// pending).  Appends a basic_test stamped with the requested
// status to the handler's internal sink and forwards to the
// handler's counter.
template<typename _Handler>
inline void
record_status(
    _Handler&   _handler,
    test_status _status,
    const char* _name)
{
    basic_test t = make_assert(false, _name);
    t.set_status(static_cast<basic_test::status_type>(_status));
    _handler.push_assertion(t);
    _handler.record(_status);
    return;
}


// unit_test_tally
//   struct: per-suite summary of "unit tests" - each unit test
// is one logical grouping of assertions, typically the body of
// one test_array_* function.  Distinct from the handler's
// session_result, which counts at assertion granularity.
struct unit_test_tally
{
    std::size_t total;
    std::size_t passed;
    std::size_t failed;

    unit_test_tally() D_NOEXCEPT
        : total(0),
          passed(0),
          failed(0)
    {}

    // pass_rate
    //   returns the pass percentage on [0.0, 100.0].
    double pass_rate() const D_NOEXCEPT
    {
        return (total == 0)
                   ? 0.0
                   : (100.0 *
                        static_cast<double>(passed) /
                        static_cast<double>(total));
    }
};


// run_unit_test
//   helper: runs a unit-test functor, observing the handler's
// fail/error counters across the call to decide whether the
// unit test as a whole passed (delta zero) or failed (any new
// failures or errors).  Records the unit test as a test_fn-kind
// leaf in the handler's sink for printer rendering and updates
// the tally.
//
//   The functor runs ALL its `record_assertion` calls into the
// same handler -- which in turn pushes each leaf into the
// handler's sink.  Together they mirror the C-side framework's
// "Total Unit Tests / Total Assertions" split: each assertion
// is a leaf row, the wrapper adds a single roll-up leaf per
// unit test.
template<typename _Handler,
         typename _Fn>
inline void
run_unit_test(
    _Handler&        _handler,
    unit_test_tally& _tally,
    const char*      _name,
    _Fn&&            _body)
{
    const std::size_t fails_before  = _handler.failed();
    const std::size_t errors_before = _handler.errors();

    _body();

    const bool ok =
          (_handler.failed() == fails_before)
       && (_handler.errors() == errors_before);

    // Append a test_fn-kind leaf so the printer (and any future
    // tree-shaped reporter) can show the unit-test row inline
    // with its assertion children.
    _handler.push_assertion(make_test_fn(ok, _name));

    ++_tally.total;
    if (ok)
    {
        ++_tally.passed;
    }
    else
    {
        ++_tally.failed;
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                IX.  NUMBERED-LEAF NODE TEMPLATE                        ///
///////////////////////////////////////////////////////////////////////////////
//   Used by suite printers that want left-aligned numbering on
// every leaf node, matching the "[1/97] name ... [PASS]"
// cadence of the C-side reference output.  The %number%
// specifier comes from the printer's print_context (leaf_number,
// pre-incremented at emission).

// D_TEST_TPL_NODE_NUMBERED
//   template: leaf-only node line with left-aligned counter,
// space-separated name, and right-aligned status tag.
D_STATIC const char* const D_TEST_TPL_NODE_NUMBERED =
    "  [%number%/%total%] %name% %status%\n";

// D_TEST_TPL_SECTION_HEADER_NUMBERED
//   template: numbered section header for grouping rows in the
// inner walk.  The printer fills %section_number% and %name%.
D_STATIC const char* const D_TEST_TPL_SECTION_HEADER_NUMBERED =
    "\n"
    "  [%section_number%] %name%\n"
    "  ----------------------------------------------"
    "----------------------------\n";

// D_TEST_TPL_SECTION_FOOTER
//   template: per-section footer with a passed/total tally
// and a status verdict.
D_STATIC const char* const D_TEST_TPL_SECTION_FOOTER =
    "  --- %name% Summary ---\n"
    "    Assertions: %asserts_passed%/%asserts_total% "
    "(%asserts_percent%)\n"
    "    Status:     %has_passed%\n";


// ---------------------------------------------------------------------------
//  test_runner module / section templates and live-progress formats
//   These are consumed by test_runner.hpp (which includes this header):
// the D_TEST_TPL_* strings are rendered through text_template with the
// placeholders the runner binds; the D_TEST_FMT_PROGRESS_* strings are
// printf format strings written straight to stdout for live progress.
// Defaults below are intentionally plain - tune the wording later; the
// %placeholder% set is what the runner binds, and any unbound token
// simply renders empty.
// ---------------------------------------------------------------------------

// D_TEST_TPL_MODULE_BANNER
//   template: rendered when a module begins. Binds: name / display_name,
// description_short.
D_STATIC const char* const D_TEST_TPL_MODULE_BANNER =
    "--------------------------------------------"
    "------------------------------------\n"
    "  MODULE: %display_name%\n"
    "  %description_short%\n"
    "--------------------------------------------"
    "------------------------------------\n";

// D_TEST_TPL_MODULE_FOOTER
//   template: rendered when a module ends. Binds: time_total, has_passed
// (plus any assertion / test rollups the runner provides).
D_STATIC const char* const D_TEST_TPL_MODULE_FOOTER =
    "  --- Module Summary ---\n"
    "    Time:   %time_total%\n"
    "    Status: %has_passed%\n";

// D_TEST_TPL_SECTION_BANNER
//   template: rendered when a section begins. Binds: name.
D_STATIC const char* const D_TEST_TPL_SECTION_BANNER =
    "\n  --- %name% ---\n";

// D_TEST_TPL_SECTION_RESULT
//   template: rendered when a section ends. Binds: time_total,
// has_passed (plus any per-section rollups the runner provides).
D_STATIC const char* const D_TEST_TPL_SECTION_RESULT =
    "  --- %name% Result ---\n"
    "    Time:   %time_total%\n"
    "    Status: %has_passed%\n";

// D_TEST_FMT_PROGRESS_MODULE_BEGIN
//   printf format: live module-start line. Args: const char* short name.
D_STATIC const char* const D_TEST_FMT_PROGRESS_MODULE_BEGIN =
    "  >> module: %s\n";

// D_TEST_FMT_PROGRESS_MODULE_END
//   printf format: live module-end line. Args: const char* short name,
// int verdict, double seconds, size_t sections passed, size_t total.
D_STATIC const char* const D_TEST_FMT_PROGRESS_MODULE_END =
    "  << module: %s [verdict %d] %.3fs (%zu/%zu sections)\n";

// D_TEST_FMT_PROGRESS_SECTION_BEGIN
//   printf format: live section-start line. Args: const char* name.
D_STATIC const char* const D_TEST_FMT_PROGRESS_SECTION_BEGIN =
    "    -- section: %s\n";

// D_TEST_FMT_PROGRESS_SECTION_END
//   printf format: live section-end line. Args: const char* name,
// int verdict, double seconds, size_t passed, size_t total.
D_STATIC const char* const D_TEST_FMT_PROGRESS_SECTION_END =
    "    -- section: %s [verdict %d] %.3fs (%zu/%zu checks)\n";


///////////////////////////////////////////////////////////////////////////////
///                X.   VALUE-TAGGED EVENT TAGS                              ///
///////////////////////////////////////////////////////////////////////////////
//
//   Value-tagged events carry a severity / magnitude VALUE alongside a name
// and a message, so a listener can gate rendering on the value (see
// default_test_handler's threshold below).  Rather than a payload struct,
// each tag spells its payload explicitly through D_EVENT (from event.hpp):
// the integer value domain, then two C-string domains (name, message).  A
// separate tag per integer width lets a caller bind listeners distinctly per
// width.
//
//   The naming pattern is `on_test_event_N`, N the bit width (8, 16, 32, 64),
// mirroring the integer's numeric suffix and keeping fire / bind sites
// self-documenting.  These live in the same `events` namespace as the
// built-in lifecycle alphabet declared in test_event.hpp.

// on_test_event_8
//   value-tagged event.  Payload: (value, name, message).
D_EVENT(on_test_event_8,  std::int8_t,  const char*, const char*);

// on_test_event_16
//   value-tagged event.  Payload: (value, name, message).
D_EVENT(on_test_event_16, std::int16_t, const char*, const char*);

// on_test_event_32
//   value-tagged event.  Payload: (value, name, message).
D_EVENT(on_test_event_32, std::int32_t, const char*, const char*);

// on_test_event_64
//   value-tagged event.  Payload: (value, name, message).
D_EVENT(on_test_event_64, std::int64_t, const char*, const char*);



///////////////////////////////////////////////////////////////////////////////
///                XI.  DEFAULT TEST HANDLER (THRESHOLD-FILTERED)            ///
///////////////////////////////////////////////////////////////////////////////
//
//   default_test_handler extends test_handler with a printer
// listener bundle that:
//
//     - Lifecycle events (on_test_passed, on_test_failed, etc.)
//       ALWAYS forward to the printer; there is no value gate.
//
//     - Value-tagged events (on_test_event_N) are forwarded to
//       the printer only when the carried value is GREATER THAN
//       OR EQUAL TO the configured threshold.  Below-threshold
//       events still dispatch to other listeners - only the
//       printer's listener is gated.
//
//   The threshold is a single int64_t value used to compare
// against payloads of any narrower width.  Each value-tagged
// listener widens its int8_t / int16_t / int32_t / int64_t
// payload to int64_t before the comparison; the widening is
// always lossless because every signed narrow type's range is
// a subset of int64_t's range.
//
//   THRESHOLD DEFAULT:
//   The default threshold is INT64_MIN - i.e. every value-tagged
// event is printed.  Callers who want to silence below-warning
// events use set_threshold() with a higher value.

// default_test_handler
//   class: standard test_handler with the framework's default
// printer-listener bundle.  When a printer is attached via
// set_printer(), this class installs:
//
//     1. Lifecycle listeners - one per built-in lifecycle event
//        - that always render through the printer.
//     2. Value-tagged listeners - one per value-tagged event tag
//        in section X - that gate the printer call on
//        `value >= threshold()`.
class default_test_handler : public test_handler
{
public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------

    // threshold_type
    //   the integer type used for threshold comparisons.
    // Wide enough to losslessly accept any value-tagged
    // payload's value.
    using threshold_type = std::int64_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    //   constructs a default_test_handler with threshold set to
    // the minimum representable threshold_type value, so every
    // value-tagged event passes the gate and reaches the
    // printer.
    default_test_handler() D_NOEXCEPT
        : test_handler(),
          m_threshold(threshold_min())
    {}

    // from threshold
    //   constructs a default_test_handler with the supplied
    // threshold.  Any value-tagged event with `value < _t`
    // will be dropped from the printer's view.
    explicit default_test_handler(
        threshold_type _t
    ) D_NOEXCEPT
        : test_handler(),
          m_threshold(_t)
    {}

    // -----------------------------------------------------------------
    //  threshold accessors
    // -----------------------------------------------------------------

    // set_threshold
    //   updates the threshold.  Takes effect immediately for
    // subsequent dispatches; in-flight listener bodies that
    // already evaluated the threshold will complete with the
    // pre-update value.  Does not require re-installing the
    // listener bundle.
    void set_threshold(
        threshold_type _t
    ) D_NOEXCEPT
    {
        m_threshold = _t;

        return;
    }

    // threshold
    //   returns the current threshold value.
    D_CONSTEXPR threshold_type
    threshold() const D_NOEXCEPT
    {
        return m_threshold;
    }

    // threshold_min
    //   returns the minimum representable threshold_type value
    // - i.e. the threshold that admits every payload.  Provided
    // as a static helper so callers can write
    // `handler.set_threshold(default_test_handler::threshold_min())`
    // instead of pulling in <limits> at the call site.
    static D_CONSTEXPR threshold_type
    threshold_min() D_NOEXCEPT
    {
        // INT64_MIN is the most-negative representable int64_t.
        // We avoid <limits> here so this header stays light;
        // <cstdint> is already included for std::int64_t.
        return static_cast<threshold_type>(INT64_MIN);
    }

protected:
    // install_printer_listeners
    //   override: installs the framework's default printer
    // bundle on top of the base class's empty default.  The
    // base class's m_printer pointer is already set when this
    // is called (set_printer stored it before calling here).
    //
    //   Every binding's listener_id is appended to
    // m_printer_listener_ids so that the inherited
    // uninstall_printer_listeners() can tear the bundle down
    // on clear_printer() or destruction.
    virtual void install_printer_listeners()
    {
        // capture-by-value of the printer pointer keeps the
        // listener bodies independent of any later mutation
        // of m_printer.  The handler's destructor unbinds the
        // bundle before the pointer is invalidated; we never
        // outlive the printer the user attached.
        test_printer* const printer = m_printer;

        if (printer == nullptr)
        {
            return;
        }

        bind_lifecycle_listeners(printer);
        bind_value_tagged_listeners(printer);

        return;
    }

private:
    // bind_lifecycle_listeners
    //   helper: binds one listener per built-in lifecycle
    // event whose body forwards to the printer's per-node
    // rendering.  All lifecycle listeners are unconditional
    // - there is no value gate here.
    void bind_lifecycle_listeners(
        test_printer* _printer
    )
    {
        // on_test_passed: render the leaf with passed status.
        m_printer_listener_ids.push_back(
            events().bind<on_test_passed>(
                [_printer](const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::passed,
                        _obj->metadata().get("name"),
                        _obj->metadata().get("message_pass"),
                        static_cast<std::size_t>(0),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_failed: render the leaf with failed status.
        m_printer_listener_ids.push_back(
            events().bind<on_test_failed>(
                [_printer](const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::failed,
                        _obj->metadata().get("name"),
                        _obj->metadata().get("message_fail"),
                        static_cast<std::size_t>(0),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_skipped: render the leaf with skipped status.
        m_printer_listener_ids.push_back(
            events().bind<on_test_skipped>(
                [_printer](const basic_test* _obj) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::skipped,
                        _obj->metadata().get("name"),
                        _obj->metadata().get("message_pass"),
                        static_cast<std::size_t>(0),
                        static_cast<std::size_t>(0));

                    return;
                }));

        // on_test_error: render the leaf with error status and
        // the diagnostic message carried by the event.
        m_printer_listener_ids.push_back(
            events().bind<on_test_error>(
                [_printer](const basic_test* _obj,
                           const char*       _msg) D_NOEXCEPT
                {
                    if ( (_printer == nullptr) ||
                         (_obj     == nullptr) )
                    {
                        return;
                    }

                    _printer->print_node(
                        test_status::error,
                        _obj->metadata().get("name"),
                        std::string(_msg ? _msg : ""),
                        static_cast<std::size_t>(0),
                        static_cast<std::size_t>(0));

                    return;
                }));

        return;
    }

    // bind_value_tagged_listeners
    //   helper: binds one listener per value-tagged event tag
    // (one per integer width) whose body widens the payload
    // value to threshold_type and forwards to the printer
    // ONLY IF the value is at or above the configured
    // threshold.  Listeners capture `this` so the threshold
    // is read at dispatch time, not at bind time - set_threshold
    // calls take effect immediately without re-binding.
    void bind_value_tagged_listeners(
        test_printer* _printer
    )
    {
        // 8-bit
        m_printer_listener_ids.push_back(
            events().bind<on_test_event_8>(
                [_printer, this](std::int8_t _value,
                                 const char* _name,
                                 const char* _message) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_value),
                        _name,
                        _message);

                    return;
                }));

        // 16-bit
        m_printer_listener_ids.push_back(
            events().bind<on_test_event_16>(
                [_printer, this](std::int16_t _value,
                                 const char*  _name,
                                 const char*  _message) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_value),
                        _name,
                        _message);

                    return;
                }));

        // 32-bit
        m_printer_listener_ids.push_back(
            events().bind<on_test_event_32>(
                [_printer, this](std::int32_t _value,
                                 const char*  _name,
                                 const char*  _message) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        static_cast<threshold_type>(_value),
                        _name,
                        _message);

                    return;
                }));

        // 64-bit
        m_printer_listener_ids.push_back(
            events().bind<on_test_event_64>(
                [_printer, this](std::int64_t _value,
                                 const char*  _name,
                                 const char*  _message) D_NOEXCEPT
                {
                    print_value_event_if_above_threshold(
                        _printer,
                        _value,
                        _name,
                        _message);

                    return;
                }));

        return;
    }

    // print_value_event_if_above_threshold
    //   helper: shared body for every value-tagged listener.
    // Compares the widened value against the current threshold
    // and forwards to the printer if the gate passes.  The
    // printed `number` field is set to the value cast to
    // size_t so the printer can render it as a decimal token.
    void print_value_event_if_above_threshold(
        test_printer*  _printer,
        threshold_type _value,
        const char*    _name,
        const char*    _message
    ) D_NOEXCEPT
    {
        if (_printer == nullptr)
        {
            return;
        }

        if (_value < m_threshold)
        {
            return;
        }

        _printer->print_node(
            test_status::passed,
            std::string(_name    ? _name    : ""),
            std::string(_message ? _message : ""),
            static_cast<std::size_t>(0),
            static_cast<std::size_t>(_value));

        return;
    }


    threshold_type m_threshold;
};


///////////////////////////////////////////////////////////////////////////////
///                XII.  STATUS LABEL                                       ///
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
///                XIII. TREE VIEW                                          ///
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
///                XIV.  REPORT VIEW                                        ///
///////////////////////////////////////////////////////////////////////////////

// drive_report
//   projects one module_spec onto a report_builder, PRESERVING the block level
// so the whole nested tree reaches the document: one module band, then one
// UNIT per block (its "name" the card header, its "descriptor" the card
// description line), each carrying one CHECK per test (the test's "name" the
// TEST cell, its "descriptor" the DESCRIPTION cell).  So module > block > test
// lands as report module > unit > check, and every name + descriptor the
// runner authored is rendered.  Each predicate runs once to produce its verdict.
inline void
drive_report(
    report_builder&    _rb,
    const module_spec& _module
)
{
    // const char* -> std::string, tolerating a null descriptor.
    struct to_str { static std::string of(const char* _p) { return _p ? std::string(_p) : std::string(); } };

    _rb.module(_module.name, to_str::of(_module.descriptor));

    std::size_t bi = 0;
    for (bi = 0; bi < _module.blocks.size(); ++bi)
    {
        const block_spec& b = _module.blocks[bi];

        _rb.open_unit(to_str::of(b.name), to_str::of(b.descriptor));   // block -> card

        std::size_t ti = 0;
        for (ti = 0; ti < b.tests.size(); ++ti)
        {
            const test_spec& t = b.tests[ti];

            const bool ok = (t.fn != nullptr) ? t.fn() : false;

            _rb.check(to_str::of(t.name), to_str::of(t.descriptor), ok);   // test -> row
        }

        _rb.close_unit();
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                XIV-b. PIPE-TABLE VIEW                                   ///
///////////////////////////////////////////////////////////////////////////////
//   A results table in the Markdown |...| grammar that parse_table_text<>()
// reads back: a header row, a full-width dash rule (the header/body split), then
// one body row per test.  This is the ONE place the |...| representation is
// defined, so a format tweak happens here and every runner picks it up (the
// runners themselves only choose options).  Kept dependency-free - plain string
// assembly over <string> / <vector>, no link against the table subframework -
// so this widely-included header stays light; the text it emits is what that
// subframework parses.

// pipe_escape
//   neutralises a cell's text for the grid: a literal '|' would read as a
// column delimiter and a newline would break the row, so fold both away.  We
// substitute rather than backslash-escape so the result parses regardless of
// whether the grammar supports an escape sequence.  Test names and descriptors
// rarely contain either, so this is almost always a no-op.
inline std::string
pipe_escape(
    const std::string& _s
)
{
    std::string _out;
    _out.reserve(_s.size());

    std::size_t i = 0;
    for (i = 0; i < _s.size(); ++i)
    {
        const char _c = _s[i];

        if      (_c == '|')  { _out += '/';  }   // delimiter -> slash
        else if (_c == '\n') { _out += ' ';  }   // newline   -> space
        else if (_c == '\r') { /* drop */    }
        else                 { _out += _c;   }
    }

    return _out;
}


// pipe_table_row_helper
//   internal: append one "| a | b | c | d |\n" row, each cell left-padded to a
// common column width so the text is both human-readable and grid-parseable.
namespace internal
{
    inline std::string
    pipe_table_row_helper(
        const std::string& _a, std::size_t _wa,
        const std::string& _b, std::size_t _wb,
        const std::string& _c, std::size_t _wc,
        const std::string& _d, std::size_t _wd
    )
    {
        std::string _o;

        _o += "| "; _o += _a; _o.append(_wa - _a.size(), ' '); _o += ' ';
        _o += "| "; _o += _b; _o.append(_wb - _b.size(), ' '); _o += ' ';
        _o += "| "; _o += _c; _o.append(_wc - _c.size(), ' '); _o += ' ';
        _o += "| "; _o += _d; _o.append(_wd - _d.size(), ' '); _o += " |\n";

        return _o;
    }

    // pipe_table_rule_helper
    //   internal: a full-width dash rule the width of _header (the header/body
    // split, the form the table-parser tests use - interior pipes optional,
    // columns resolve off the data rows).
    inline std::string
    pipe_table_rule_helper(
        const std::string& _header
    )
    {
        const std::size_t _inner = (_header.size() >= 3) ? (_header.size() - 3) : 0;

        std::string _sep = "|";
        _sep.append(_inner, '-');
        _sep += "|\n";

        return _sep;
    }
}


// render_spec_pipe_table
//   renders a module_spec's tests as a |...| table with columns
// RESULT | # | TEST | DESCRIPTION, one row per test across every block.  Each
// predicate is run once to fill its RESULT cell (PASS / FAIL).
//
//   NOTE: this runs every predicate.  drive_report() also runs them once, so a
// caller that has already driven the report (run_module / run_suite below) runs
// each predicate TWICE when it emits this table.  That is harmless for the pure
// bool() predicates DTest expects; a caller holding a populated report can use
// render_report_pipe_table() instead, which runs nothing.
inline std::string
render_spec_pipe_table(
    const module_spec& _module
)
{
    struct cell { static std::string of(const char* _p)
        { return pipe_escape(_p ? std::string(_p) : std::string()); } };

    // 1) gather the rows -----------------------------------------------------
    std::vector<std::string> _res;
    std::vector<std::string> _num;
    std::vector<std::string> _name;
    std::vector<std::string> _desc;

    std::size_t _n = 0;
    std::size_t bi = 0;
    for (bi = 0; bi < _module.blocks.size(); ++bi)
    {
        const block_spec& _b = _module.blocks[bi];

        std::size_t ti = 0;
        for (ti = 0; ti < _b.tests.size(); ++ti)
        {
            const test_spec& _t = _b.tests[ti];
            const bool        _ok = (_t.fn != nullptr) ? _t.fn() : false;

            ++_n;
            _res.push_back(_ok ? "PASS" : "FAIL");
            _num.push_back(std::to_string(_n));
            _name.push_back(cell::of(_t.name));
            _desc.push_back(cell::of(_t.descriptor));
        }
    }

    // 2) column widths (header vs widest cell) -------------------------------
    const std::string _h_res = "RESULT", _h_num = "#", _h_name = "TEST", _h_desc = "DESCRIPTION";

    std::size_t _w_res  = _h_res.size();
    std::size_t _w_num  = _h_num.size();
    std::size_t _w_name = _h_name.size();
    std::size_t _w_desc = _h_desc.size();

    std::size_t i = 0;
    for (i = 0; i < _res.size(); ++i)
    {
        if (_res[i].size()  > _w_res)  { _w_res  = _res[i].size();  }
        if (_num[i].size()  > _w_num)  { _w_num  = _num[i].size();  }
        if (_name[i].size() > _w_name) { _w_name = _name[i].size(); }
        if (_desc[i].size() > _w_desc) { _w_desc = _desc[i].size(); }
    }

    // 3) assemble ------------------------------------------------------------
    std::string _out;

    const std::string _hdr =
        internal::pipe_table_row_helper(_h_res, _w_res, _h_num, _w_num,
                                        _h_name, _w_name, _h_desc, _w_desc);
    _out += _hdr;
    _out += internal::pipe_table_rule_helper(_hdr);

    for (i = 0; i < _res.size(); ++i)
    {
        _out += internal::pipe_table_row_helper(_res[i], _w_res, _num[i], _w_num,
                                                _name[i], _w_name, _desc[i], _w_desc);
    }

    return _out;
}


// render_report_pipe_table
//   the same idea sourced from an already-populated test_report (module ->
// units -> checks), so it runs NO predicates.  Columns
// MODULE | UNIT | RESULT | CHECK, one row per check.
inline std::string
render_report_pipe_table(
    const test_report& _report
)
{
    struct word { static const char* of(test_status _s)
        {
            switch (_s)
            {
                case test_status::passed:  return "PASS";
                case test_status::failed:  return "FAIL";
                case test_status::skipped: return "SKIP";
                case test_status::error:   return "ERROR";
                default:                   return "PENDING";   // incl. pending
            }
        } };

    std::vector<std::string> _mod;
    std::vector<std::string> _unit;
    std::vector<std::string> _res;
    std::vector<std::string> _chk;

    std::size_t mi = 0;
    for (mi = 0; mi < _report.modules.size(); ++mi)
    {
        const report_module& _m = _report.modules[mi];

        std::size_t ui = 0;
        for (ui = 0; ui < _m.units.size(); ++ui)
        {
            const report_unit& _u = _m.units[ui];

            std::size_t ci = 0;
            for (ci = 0; ci < _u.checks.size(); ++ci)
            {
                const report_check& _c = _u.checks[ci];

                _mod.push_back(pipe_escape(_m.name));
                _unit.push_back(pipe_escape(_u.name));
                _res.push_back(word::of(_c.status));
                _chk.push_back(pipe_escape(_c.description));
            }
        }
    }

    const std::string _h_mod = "MODULE", _h_unit = "UNIT", _h_res = "RESULT", _h_chk = "CHECK";

    std::size_t _w_mod  = _h_mod.size();
    std::size_t _w_unit = _h_unit.size();
    std::size_t _w_res  = _h_res.size();
    std::size_t _w_chk  = _h_chk.size();

    std::size_t i = 0;
    for (i = 0; i < _res.size(); ++i)
    {
        if (_mod[i].size()  > _w_mod)  { _w_mod  = _mod[i].size();  }
        if (_unit[i].size() > _w_unit) { _w_unit = _unit[i].size(); }
        if (_res[i].size()  > _w_res)  { _w_res  = _res[i].size();  }
        if (_chk[i].size()  > _w_chk)  { _w_chk  = _chk[i].size();  }
    }

    std::string _out;

    const std::string _hdr =
        internal::pipe_table_row_helper(_h_mod, _w_mod, _h_unit, _w_unit,
                                        _h_res, _w_res, _h_chk, _w_chk);
    _out += _hdr;
    _out += internal::pipe_table_rule_helper(_hdr);

    for (i = 0; i < _res.size(); ++i)
    {
        _out += internal::pipe_table_row_helper(_mod[i], _w_mod, _unit[i], _w_unit,
                                                _res[i], _w_res, _chk[i], _w_chk);
    }

    return _out;
}


// emit_spec_pipe_table
//   internal: print a module's |...| results table to stdout, bracketed by a
// caption line so it is easy to find (and to slice back out of) the console
// stream.  The one emission seam the entry points share.
namespace internal
{
    inline void
    emit_spec_pipe_table(
        const module_spec& _module
    )
    {
        std::fputs("\n|-- results table (", stdout);
        std::fputs((_module.name != nullptr) ? _module.name : "", stdout);
        std::fputs(") --|\n", stdout);
        std::fputs(render_spec_pipe_table(_module).c_str(), stdout);

        return;
    }
}


///////////////////////////////////////////////////////////////////////////////
///                XV.   ENTRY POINTS                                       ///
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
        build_enriched_tree(_module.name,
                            (_module.descriptor != nullptr) ? _module.descriptor : "",
                            _module);
    describe_tree(tree);

    // view 2: the report + optional PDF -------------------------------------
    report_builder rb;
    rb.set_title(_title);
    if (_subtitle != nullptr) { rb.set_subtitle(_subtitle); }
    rb.set_description(_module.descriptor);
    if (_pdf != nullptr)      { rb.use_pdf(_pdf); }

    drive_report(rb, _module);
    internal::emit_spec_pipe_table(_module);      // the |...| results table

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
        internal::emit_spec_pipe_table(_modules[i]);   // the |...| results table
    }

    return rb.finish();
}


// run_module  (options-driven overload)
//   the same one-suite flow as the convenience overload above, but the report
// / PDF view is configured by a caller-supplied test_option_set
// (test_options.hpp) instead of the bare _pdf char*.  The option set already
// carries the document / output_file / split / show decisions, so this overload
// SEEDS the report_builder from it and does NOT call use_pdf().  Select the PDF
// face the documented way -
//     opts.set<test_option::document>(test_doc_type::pdf);
//     opts.set<test_option::output_file>("suite.pdf");
// - or leave `document` at its `text` default for a console-only run.  The
// six-kind tree view is identical to the convenience overload; only the report
// configuration seam differs (a caller-owned option set vs the framework
// default plus use_pdf()).
inline int
run_module(
    const module_spec&     _module,
    const char*            _title,
    const char*            _subtitle,
    const test_option_set& _opts
)
{
    // view 1: the six-kind tree ---------------------------------------------
    enriched_tree tree =
        build_enriched_tree(_module.name,
                            (_module.descriptor != nullptr) ? _module.descriptor : "",
                            _module);
    describe_tree(tree);

    // view 2: the report, configured by the option set ----------------------
    report_builder rb(_opts);                       // seed from caller options
    rb.set_title(_title);
    if (_subtitle != nullptr) { rb.set_subtitle(_subtitle); }
    rb.set_description(_module.descriptor);
    // no use_pdf(): _opts already decides document / output_file / split

    drive_report(rb, _module);
    internal::emit_spec_pipe_table(_module);      // the |...| results table

    return rb.finish();
}

// run_suite  (options-driven overload)
//   the union-of-suites flow, configured by a caller-supplied test_option_set
// instead of the bare _pdf char*: the option set decides document /
// output_file / split / show, so the builder is seeded from it and use_pdf()
// is not called.  With split == per_module the run writes one document per
// module; whole_run (the default) writes a single document covering every
// suite.
inline int
run_suite(
    const char*                     _suite_name,
    const char*                     _suite_descriptor,
    const std::vector<module_spec>& _modules,
    const char*                     _title,
    const test_option_set&          _opts
)
{
    // view 1: one tree over every module ------------------------------------
    enriched_tree tree =
        build_enriched_tree(_suite_name, _suite_descriptor, _modules);
    tree_summary(tree);

    // view 2: one report, a band per module, configured by the option set ---
    report_builder rb(_opts);
    rb.set_title(_title);
    rb.set_description(_suite_descriptor);

    std::size_t i = 0;
    for (i = 0; i < _modules.size(); ++i)
    {
        drive_report(rb, _modules[i]);
        internal::emit_spec_pipe_table(_modules[i]);   // the |...| results table
    }

    return rb.finish();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_DEFAULTS_