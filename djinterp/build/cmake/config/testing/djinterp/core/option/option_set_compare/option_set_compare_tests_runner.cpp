/******************************************************************************
* djinterp [test]                          option_set_compare_tests_runner.cpp
*
*   Entry point for the option_set_compare.hpp unit suite.  Assembles the four
* section blocks - each contributed by its own translation unit through the
* block-provider declared in option_set_compare_tests.hpp - into one
* module_spec and hands it to the framework runner.
*
*   Sections:
*     I+II. key lists + operations   (option_set_compare_tests_keylist.cpp)
*     III.  congruity                (option_set_compare_tests_congruity.cpp)
*     IV.   value carriers           (option_set_compare_tests_carriers.cpp)
*     V.    option_set_value_eq      (option_set_compare_tests_value_eq.cpp)
*
*   option_set_compare.hpp is entirely type-level and compiles at the framework's
* base standard, so this suite has no version gating - it links and reports the
* same set of tests at C++17 and C++20.  run_module returns 0 iff every test
* passed, which becomes the process exit status for CI.
*
*   run_module also accepts an optional trailing options argument (the shared
* module-report options surface, e.g. to emit a PDF report); it is omitted here
* so the suite depends only on the default console reporter.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_set_compare_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// djinterp
#include "option_set_compare_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_set_compare.hpp";
    mod.descriptor = "key lists, key/type congruity, value carriers, parameterized value equality";

    mod.blocks.push_back(tt::option_set_compare_keylist_block());     // I + II.
    mod.blocks.push_back(tt::option_set_compare_congruity_block());   // III.
    mod.blocks.push_back(tt::option_set_compare_carriers_block());    // IV.
    mod.blocks.push_back(tt::option_set_compare_value_eq_block());    // V.

    return dt::run_module(
        mod,
        "djinterp option_set_compare unit tests",
        "key lists + congruity + value carriers + value equality");
}
