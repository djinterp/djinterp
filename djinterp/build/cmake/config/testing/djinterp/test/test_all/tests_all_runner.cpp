/******************************************************************************
* djinterp [test]                                       tests_all_runner.cpp
*
*   Single-process umbrella: runs every migrated suite's spec in ONE run_suite
* call, producing one combined report / PDF.  Defining DTEST_SPEC_MODE selects
* each header's spec-provider face; this TU is linked against EVERY suite's
* section TUs (the CMakeLists globs them), which define the tests_* bodies.
*
*   NOTE 1 (macros): each suite header defines its own D_xx_CHECK macro, used
* only by the section TUs.  A few suffixes repeat (TC / TO / TT), so we #undef
* after each include to avoid a macro-redefinition diagnostic when pulling every
* header into one TU.  (The individual runners include one header each and never
* hit this.)
*
*   NOTE 2 (scope): this umbrella covers the 15 migrated suites.  It does NOT
* include the pre-existing *_spec() suites (options / callable / context /
* builder / container / emit / handler / runner / event_*), because their
* headers are not part of this drop-in.  To fold them in, add their header +
* spec to the two marked spots below.  If you would rather run EVERYTHING with
* no editing, prefer the `all_djinterp_test` aggregate (ctest) over this exe -
* it depends on every registered suite exe, pre-existing ones included.
*
* path:  build/cmake/config/testing/djinterp/test/tests_all_runner.cpp
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
#include <vector>

// -- suite headers (spec-provider face) ------------------------------------
//    INCLUDES gives both tests roots, so every header is included bare.
#include "test_common_tests.hpp"
#ifdef D_TC_CHECK
#  undef D_TC_CHECK
#endif
#include "test_kind_tests.hpp"
#ifdef D_TK_CHECK
#  undef D_TK_CHECK
#endif
#include "test_object_tests.hpp"
#ifdef D_TO_CHECK
#  undef D_TO_CHECK
#endif
#include "test_tree_tests.hpp"
#include "test_session_tests.hpp"
#include "test_timer_tests.hpp"
#ifdef D_TT_CHECK
#  undef D_TT_CHECK
#endif
#include "test_counter_tests.hpp"
#ifdef D_TC_CHECK
#  undef D_TC_CHECK
#endif
#include "test_event_tests.hpp"
#ifdef D_TE_CHECK
#  undef D_TE_CHECK
#endif
#include "test_traits_tests.hpp"
#ifdef D_TT_CHECK
#  undef D_TT_CHECK
#endif
//#include "test_pack_tests.hpp"
//#ifdef D_TP_CHECK
//#  undef D_TP_CHECK
//#endif
#include "test_defaults_tests.hpp"
#ifdef D_TD_CHECK
#  undef D_TD_CHECK
#endif
#include "test_output_config_tests.hpp"     // under tests/.../output (INCLUDES root)
#ifdef D_OC_CHECK
#  undef D_OC_CHECK
#endif
#include "test_render_tests.hpp"
#ifdef D_TR_CHECK
#  undef D_TR_CHECK
#endif
#include "test_output_tests.hpp"
#ifdef D_TO_CHECK
#  undef D_TO_CHECK
#endif
#include "test_report_tests.hpp"
#ifdef D_RT_CHECK
#  undef D_RT_CHECK
#endif
// <-- add pre-existing suite headers here (spot A)


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("djinterp_all_tests.pdf"));

    const std::vector<dt::module_spec> modules = {
        tt::common_spec(),
        tt::test_kind_spec(),
        tt::object_spec(),
        tt::tree_spec(),
        tt::session_spec(),
        tt::timer_spec(),
        tt::counter_spec(),
        tt::event_spec(),
        tt::traits_spec(),
        //tt::pack_spec(),
        tt::defaults_spec(),
        tt::output_config_spec(),
        tt::render_spec(),
        tt::output_spec(),
        tt::report_spec(),
        // <-- add pre-existing suite specs here (spot B)
    };

    return dt::run_suite(
        "djinterp",
        "All djinterp DTest unit suites",
        modules,
        "djinterp - all unit tests",
        opts);
}
