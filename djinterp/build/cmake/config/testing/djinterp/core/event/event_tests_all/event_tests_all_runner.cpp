/******************************************************************************
* djinterp [test]                                    event_tests_all_runner.cpp
*
*   Aggregate runner for the entire event subsystem.  Each suite header now
* carries its own module_spec provider (spec mode); this file includes all
* five with DTEST_SPEC_MODE defined - which drops every suite's fixtures, so
* the otherwise-colliding fixture names (ev_plain, summing) never meet -
* collects the five specs, and hands them to run_suite.  run_suite lowers all
* five into one six-kind test_suite tree and drives a single console report;
* the document side is configured by the test_option_set below.
*
*   OUTPUT VIA TEST_OPTIONS - PER-MODULE PDFs, BUNDLED INTO A .zip:
*   Every knob rides one option set (test_options.hpp):
*     document       -> pdf                 emit PDF (not the txt default)
*     split          -> per_module          one PDF per suite (5 documents)
*     pack           -> archive             bundle those documents...
*     archive_format -> zip                 ...into a single .zip container
*     output_file    -> "event_tests.zip"   the container's path
*   With split == per_module, the five report modules (event_common,
* event_handler, event_table, event_registry, event_dispatcher) each render to
* their own PDF; pack == archive then collects all five - named
* event_common.pdf ... event_dispatcher.pdf inside the archive - into
* event_tests.zip.  (For five LOOSE PDFs instead, drop the pack / archive_format
* lines; for one combined PDF, drop the split line too.)
*
*   WHY zip AND NOT 7z:
*   A .7z can only be written through a 7z-capable library (libarchive, the LZMA
* SDK, or bit7z); with none present, format_is_writable<sevenzip> is false and
* NOTHING is written - the run stays console-only and reports "(report document
* was not written)".  The `zip` format, by contrast, has a self-contained
* STORED (uncompressed) fallback writer (test_zip_store.hpp) that needs no
* third-party library, so the bundle is always produced.  The result is a fully
* standard .zip - it opens by double-click in Windows Explorer and with 7-Zip.
* To emit an actual .7z, make a 7z-capable library visible to the build (so
* env_archive.h's D_ENV_ARCHIVE_CAN_WRITE_7Z becomes true) and switch
* archive_format back to sevenzip / output_file to "event_tests.7z" - no other
* change is needed.
*
*   PACKAGING IS OPT-IN in the report builder, so this runner defines
* D_TEST_REPORT_ENABLE_ARCHIVE (below, before the includes) to compile the
* archive path in.  That confines the archive-facade dependency (and the stored
* fallback) to this aggregate runner; the per-suite runners, which emit a
* single PDF each, do not define it and pay nothing for it.
*
*   FINDING THE FILE:
*   output_file is a RELATIVE name, so the .zip lands in the process's current
* working directory - under Visual Studio that is the Debugging "Working
* Directory" ($(ProjectDir) by default), NOT the folder holding the .exe.  Set
* output_file to an absolute path (e.g. "D:\\testing\\event_tests.zip") to pin
* it, or watch the console for the "wrote report: ..." line, which names what
* was written.
*
*   All of the machinery lives in the headers: each suite's spec beside its
* tests, the tree + report projection in test_defaults.hpp, the archive
* bundling in test_report_runner.hpp, the stored fallback in test_zip_store.hpp.
* This file is only the union and the entry point.  To run one suite alone, use
* that suite's own runner.
*
* path:      /build/cmake/config/testing/djinterp/core/event/event_tests_all_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#define DTEST_SPEC_MODE                    // suites expose spec providers, not fixtures
#define D_TEST_REPORT_ENABLE_ARCHIVE       // compile the archive packaging path into the builder

// std
#include <string>
#include <vector>
// djinterp
#include "../../../../../../../../tests/djinterp/core/event/event_common_tests.hpp"          // each resolved via the test include path
#include "../../../../../../../../tests/djinterp/core/event/event_handler_tests.hpp"
#include "../../../../../../../../tests/djinterp/core/event/event_table_tests.hpp"
#include "../../../../../../../../tests/djinterp/core/event/event_registry_tests.hpp"
#include "../../../../../../../../tests/djinterp/core/event/event_dispatcher_tests.hpp"

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    std::vector<dt::module_spec> specs;
    specs.push_back(tt::event_common_spec());
    specs.push_back(tt::event_handler_spec());
    specs.push_back(tt::event_table_spec());
    specs.push_back(tt::event_registry_spec());
    specs.push_back(tt::event_dispatcher_spec());

    // configure the report through the option set: one PDF per suite, all five
    // bundled into a single .zip (stored fallback needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::split>(dt::test_output_split::per_module);
    opts.set<dt::test_option::pack>(dt::test_output_pack::archive);
    opts.set<dt::test_option::archive_format>(dt::test_archive_format::zip);
    opts.set<dt::test_option::output_file>(std::string("event_tests.zip"));

    return dt::run_suite(
        "djinterp event subsystem",
        "Unit tests across event_common, event_handler, event_table, "
        "event_registry, and event_dispatcher.",
        specs,
        "djinterp event subsystem unit tests",   // report title
        opts);                                   // options: 5 PDFs -> event_tests.zip
}
