/******************************************************************************
* djinterp [tests]                                        sync_tests_runner.cpp
*
*   Aggregate runner for the whole djinterp/cpp/modules/test/sync subframework.
* Drives run_suite over every module spec in one process, producing a single
* combined report.
*
*   DTEST_SPEC_MODE is defined before the aggregate header, which requires it:
* every module suite is pulled in through its provider face only.
*
*   This executable links the SAME section TUs as the per-module executables -
* the per-module runners are excluded from its source list, since each of them
* carries its own main().
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/sync/sync_tests/sync_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
#include <vector>
// djinterp
#include "sync_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("sync_tests.pdf"));

    const std::vector<dt::module_spec> specs = tt::sync_all_specs();

    return dt::run_suite(
        "djinterp/test/sync",
        "Complete unit-test suite for the test/sync subframework: threads, "
        "coordination primitives, the concurrent and stress harnesses, race "
        "and invariant detection, deadlock detection, and the umbrella.",
        specs,
        "djinterp test/sync subframework unit tests",
        opts);
}
