/******************************************************************************
* djinterp [test]          threadsafe_container_tests_runner.cpp
*
*   Entry point for the threadsafe_container suite.  Defines DTEST_SPEC_MODE
* (selecting the suite header's provider face), configures a test_option_set,
* and hands the spec to run_module.  run_module returns 0 iff the module passes
* clean, so `return run_module(...)` is the whole of main.
******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider

// std
#include <string>
// djinterp
#include "threadsafe_container_tests.hpp"   // via INCLUDES (tests dir)

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("threadsafe_container_tests.pdf"));

    return dt::run_module(
        tt::threadsafe_container_spec(),
        "threadsafe_container.hpp unit tests",
        "runtime concurrency building blocks (base, accessors, atomic state, "
        "optimistic reads, CAS, snapshot, batch, ranges)",
        opts);
}
