/******************************************************************************
* djinterp [test]                                    foldable_tests_runner.cpp
*
*   Entry point for the foldable.hpp unit suite.  Assembles the six section
* blocks -- each contributed by its own translation unit through the block-
* providers declared in foldable_tests.hpp -- into a single module_spec and
* hands it to the framework runner.
*
*   Sections:
*     0 + I.  protocol & traits ......... foldable_tests_protocol.cpp
*     II.1    fold_left ................. foldable_tests_fold_left.cpp
*     II.2    fold_right ............... foldable_tests_fold_right.cpp
*     II.3    fold_map ................. foldable_tests_fold_map.cpp
*     II.4-6  collect / length / empty . foldable_tests_collect.cpp
*     II.7    fold_any / fold_all ...... foldable_tests_predicates.cpp
*
*   The results are emitted as a single PDF alongside the live console report.
* Configuration rides the shared module-report option set: document = pdf,
* output_file = the report path, split = whole_run (one document for the run),
* pack = none (written loose, no container).  The option-set value face (set<>)
* is C++20 and PDF rendering rides the C++17 pdf gate; below either tier the run
* degrades to a clean console-only report rather than failing to build.
* run_module returns 0 iff every executed test passed, which becomes the process
* exit status for CI.
*
*
* path:      /tests/djinterp/core/functional/foldable_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "foldable.hpp";
    mod.descriptor =
        "Foldable protocol + generic folds "
        "(fold_left / fold_right / fold_map, collect, predicates)";

    mod.blocks.push_back(tt::foldable_protocol_block());     // 0 + I.
    mod.blocks.push_back(tt::foldable_fold_left_block());    // II.1
    mod.blocks.push_back(tt::foldable_fold_right_block());   // II.2
    mod.blocks.push_back(tt::foldable_fold_map_block());     // II.3
    mod.blocks.push_back(tt::foldable_collect_block());      // II.4-6
    mod.blocks.push_back(tt::foldable_predicate_block());    // II.7

    // configure the report through the option set: one PDF for the whole run,
    // written loose (the stored path needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("foldable_tests_report.pdf"));
    opts.set<dt::test_option::split>(dt::test_output_split::whole_run);
    opts.set<dt::test_option::pack>(dt::test_output_pack::none);

    return dt::run_module(
        mod,
        "djinterp foldable<> unit tests",
        "protocol + fold_left + fold_right + fold_map + collect + predicates",
        opts);
}
