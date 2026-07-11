/******************************************************************************
* djinterp [test]                                       free_tests_runner.cpp
*
*   Entry point for the parser/free.hpp unit suite.  Assembles the section
* blocks into a module and runs it, emitting a single whole-run PDF report.
*
*   Strata covered end to end (validated against the parser stack): the monadic
* stratum (free_pure / free_lift / free_bind / free_map, lift_to_program /
* to_parser), the applicative stratum (ap_*), and the cross-stratum lifts.  The
* selective block contributes sel_pure / sel_lift; its sel_select / sel_branch
* predicates are gated behind DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED (see
* free_tests_selective.cpp) pending the selective.hpp monad-bridge fix.
*
* path:      /tests/djinterp/parse/parser/free_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "free_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


static int
run_free_suite()
{
    dt::module_spec mod;

    mod.blocks.push_back(free_monad_block());
    mod.blocks.push_back(free_applicative_block());
    mod.blocks.push_back(free_selective_block());
    mod.blocks.push_back(free_crossstratum_block());

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("free_tests_report.pdf"));
    opts.set<dt::test_option::split>(dt::test_output_split::whole_run);
    opts.set<dt::test_option::pack>(dt::test_output_pack::none);

    return dt::run_module(
        mod,
        "parser/free.hpp",
        "free strata over the parser carrier (FreeAp / FreeSel / Free)",
        opts);
}


NS_END  // testing
NS_END  // djinterp


int
main()
{
    return ::djinterp::testing::run_free_suite();
}
