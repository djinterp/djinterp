/******************************************************************************
* djinterp [test]                                     cofree_tests_runner.cpp
*
*   Runner (the main() entry point) for the cofree.hpp unit suite.  Per the DTest
* authoring guide, a runner is thin: it assembles the module's spec, chooses a
* handful of options, and hands both to run_module, whose exit code is the whole
* of main.  It carries no per-test logic, no report formatting, and no
* templating -- run_module (test_defaults.hpp) owns all of that.
*
*   WHERE THE SPEC COMES FROM.  cofree_tests.hpp predates the DTEST_SPEC_MODE
* two-faced-header idiom: rather than a single inline cofree_spec() provider, the
* suite exposes one block_spec provider per section, each DEFINED in its section
* translation unit (cofree_tests_type.cpp, ..._map.cpp, ..._extend.cpp,
* ..._unfold.cpp, ..._protocol.cpp) and DECLARED in cofree_tests.hpp.  This runner
* therefore assembles the module_spec here from those five providers, in the
* header's table-of-contents order.  (If the header is later migrated to carry an
* inline cofree_spec(), replace the aggregate below with a single call to it.)
*
*   The section TUs supply every per-test name / descriptor, so nothing about the
* individual tests is duplicated here; the runner only names the module and its
* blocks.  The block providers themselves gate their C++14 / C++20 rows on the
* relevant feature macros, so this runner needs no feature guards of its own.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/cofree_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.20
******************************************************************************/

// std
#include <string>
// djinterp -- cofree_tests.hpp pulls in test_defaults.hpp (module_spec,
//   run_module, the option set) and declares the section block providers.  The
//   bare include resolves through the INCLUDES the CMakeLists adds.
#include "cofree_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // the spec: one block per section, in the header's table-of-contents order.
    // Each provider is defined in its section TU and returns a fully-populated
    // block_spec (name + descriptor + the section's unit tests).
    const dt::module_spec spec{
        "cofree",
        "cofree<F, A>: the cofree comonad over a single-argument functor F -- a "
        "non-empty descending chain / tree whose every node carries a head value "
        "of type A over an F-layer of children.  Registers BOTH functor_traits "
        "and comonad_traits.  Exercises the type itself, cofree_map (the "
        "functorial map), cofree_extend / extract / duplicate (the comonad "
        "co-bind and its laws), unfold_cofree (the coiterative build), and the "
        "generic functor / comonad protocol.",
        {
            tt::cofree_type_block(),        // I.    the cofree type
            tt::cofree_map_block(),         // II.3a cofree_map / functor_map
            tt::cofree_extend_block(),      // II.3b cofree_extend / extract / duplicate
            tt::cofree_unfold_block(),      // II.3c unfold_cofree
            tt::cofree_protocol_block(),    // III.  registration & protocol
        }
    };

    // options: render a PDF beside the always-on console report.  Leave document
    // at its txt default (or drop these two lines) for a console-only run.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("cofree_tests.pdf"));

    return dt::run_module(
        spec,
        "cofree.hpp unit tests",
        "the cofree comonad over a functor F: map, extend, unfold, and the "
        "functor / comonad protocol",
        opts);
}
