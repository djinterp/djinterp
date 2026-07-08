/******************************************************************************
* djinterp [test]                             option_diff_tests_runner.cpp
*
*   Entry point for the option_diff.hpp unit suite.  Assembles the six section
* blocks - each contributed by its own translation unit through the block-
* provider declared in option_diff_tests.hpp - into a single module_spec and
* hands it to the framework runner.
*
*   Sections:
*     A.II  (+A.I). compile-time key-level diff   (option_diff_tests_ct_keys.cpp)
*     A.III + A.IV. compile-time value-level diff  (option_diff_tests_ct_values.cpp)
*     A.V.          compile-time merge bridge      (option_diff_tests_ct_merge.cpp)
*     B.I.          runtime key-level diff         (option_diff_tests_rt_keys.cpp)
*     B.II.         runtime value-level diff       (option_diff_tests_rt_values.cpp)
*     B.III.        runtime merge                  (option_diff_tests_rt_merge.cpp)
*
*   option_diff.hpp is split into a compile-time trait half (PART A) and a
* runtime function half (PART B).  PART A's merge bridge rides the concept-
* constrained option_set_override engine, so the trait sections are gated to
* C++20 and emit empty below it; the runtime sections are standard-agnostic.
* This runner therefore links and reports a clean run at any standard level.
* run_module returns 0 iff every executed test passed, which becomes the
* process exit status for CI.
*
*   run_module also accepts an optional trailing PDF path; passing a filename
* there additionally emits a PDF report of the run.  It is omitted here, so the
* suite reports to the console only and depends on no rendering backend.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_diff_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

// djinterp
#include "../../.././../../../../../tests/djinterp/core/option/option_diff_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_diff.hpp";
    mod.descriptor =
        "compile-time + runtime key / value diff, diff summary, and merge";

    mod.blocks.push_back(tt::option_diff_ct_keys_block());     // A.II (+ A.I)
    mod.blocks.push_back(tt::option_diff_ct_values_block());   // A.III + A.IV
    mod.blocks.push_back(tt::option_diff_ct_merge_block());    // A.V
    mod.blocks.push_back(tt::option_diff_rt_keys_block());     // B.I
    mod.blocks.push_back(tt::option_diff_rt_values_block());   // B.II
    mod.blocks.push_back(tt::option_diff_rt_merge_block());    // B.III

    return dt::run_module(
        mod,
        "djinterp option_diff<> unit tests",
        "compile-time traits + runtime functions: keys, values, summary, merge");
}
