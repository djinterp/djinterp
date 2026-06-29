/******************************************************************************
* djinterp [test]                                        dtest_tests_runner.cpp
*
*   The umbrella runner for the whole DTest suite.  It drives EVERY component's
* tests through one report_builder - so the live console and the structured
* report come from a single run - then, instead of the single-PDF `use_pdf`
* path the per-component runners take, packages ONE PDF PER MODULE into a
* single 7z through the emit stack (test/output).
*
*   WHY TWO LAYERS:
*   report_builder runs the tests and accumulates the test_report; it can write
* one styled PDF (use_pdf) but cannot archive.  Archiving is the new packaging
* layer's job (document_bundle + output_packaging): build one DEFERRED PDF
* producer per module, then let write()'s `archive` mode fold them into one
* container.  The run and the emit compose - report_builder up front, the
* packaging stack at the back.
*
*   SHARED TEST LISTS:
*   Each module's tests come from its register_<component>() entry point (one
* small *_tests_runner.hpp each), the same registrations the standalone runners
* drive.  So this file is a list of module registrations plus the archive emit
* and nothing is duplicated.
******************************************************************************/

// std
#include <cstddef>
#include <cstdio>
#include <string>
// djinterp
#include <djinterp/core/djinterp.hpp>                    // report_builder, test_report
#include <djinterp/test/output/test_report_runner.hpp>   // report_builder, test_report
#include <djinterp/test/output/test_output.hpp>
//#include "carrier_tests_runner.hpp"        // register_carrier
//#include "counter_tests_runner.hpp"        // register_counter
//#include "timer_tests_runner.hpp"          // register_timer
#include "../../../../../../../tests/djinterp/test/test_common_tests.hpp"    // register_test_common
#include "../../../../../../../tests/djinterp/test/test_counter_tests.hpp"   // register_test_counter
#include "../../../../../../../tests/djinterp/test/test_kind_tests.hpp"      // register_test_kind
#include "../../../../../../../tests/djinterp/test/test_object_tests.hpp"    // register_test_object
#include "../../../../../../../tests/djinterp/test/test_session_tests.hpp"   // register_test_session
#include "../../../../../../../tests/djinterp/test/test_timer_tests.hpp"     // register_test_timer
#include "../../../../../../../tests/djinterp/test/test_tree_tests.hpp"      // register_test_tree
//#include "../../../../../../../tests/djinterp/test/type_traits_tests_runner.hpp"    // register_type_traits


int
main()
{
    ::djinterp::test::report_builder rb;          // default_test_options() = shared settings

    rb.set_title("djinterp - full DTest suite");
    rb.use_archive_per_module("dtest_tests");     // -> dtest_tests.7z, one PDF per module

    register_carrier(rb);
    register_counter(rb);
    register_timer(rb);
    register_test_common(rb);
    register_test_counter(rb);
    register_test_kind(rb);
    register_test_object(rb);
    register_test_session(rb);
    register_test_timer(rb);
    register_test_tree(rb);
    register_type_traits(rb);

    return rb.finish();          // finish() -> emit_report() -> per-module PDFs in the 7z
}