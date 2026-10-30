/******************************************************************************
* djinterp [tests]                      read_write_container_tests_runner.cpp
*
*   The runner for the read_write_container.hpp suite: a thin main() that
* selects the suite header's provider face, builds a test_option_set, and hands
* the spec to run_module.  It formats nothing itself - run_module drives the
* console report and (per the options below) renders the run's own PDF.
*
*   The module under test is header-only and link-free.  See the BUILD
* PREREQUISITE note in the suite header for the include-path defect the
* harness works around and the CMakeLists does not.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/read_write_container/read_write_container_tests_runner.cpp
*                read_write_container/read_write_container_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "read_write_container_tests.hpp"   // resolved via the INCLUDES the
                                            //   CMakeLists adds (tests dir)

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("read_write_container_tests.pdf"));

    return dt::run_module(
        tt::read_write_container_spec(),
        "read_write_container.hpp unit tests",
        "the full-capability baseline of the access trio, whose whole "
        "contract is that the handle's surface equals the held container's "
        "surface member for member, for every container wrapped",
        opts);
}
