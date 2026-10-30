/*****************************************************************************
* djinterp [testing]                      memory_strategy_all_tests_runner.cpp
*
* Aggregate runner for the whole memory-strategy axis.  Unlike the per-module
* runners, this one includes ALL six suite headers in their spec face and
* hands the union of their providers to run_suite, which emits ONE combined
* report (a band per module) over the entire axis.  Co-compiling the six
* headers in a single TU is well-formed because each suite gives its check
* macro a distinct suffix (D_MT_ / D_MC_ / D_AL_ / D_SB_ / D_BA_ / D_PL_) -
* so there is no macro collision - and every provider, declaration, and test
* name is unique across the axis.
*
*   D_MS_TEST_POOL_LAYER is defined for this target (see the CMakeLists) so the
* pool module joins the combined run; drop that define to omit it.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/memory/memory_all/memory_strategy_all_tests_runner.cpp
*                                          memory_strategy_all_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE            // every suite exposes its spec provider

// std
#include <string>
#include <vector>
// djinterp - all six memory-axis suites, in spec face.
//   The container-shape binding (DContainerMemoryStrategy) is NOT part of
// this axis: its header lives under inc/djinterp/cpp/container/traits and
// it is exercised by container_memory_strategy_tests on the container axis.
#include "memory_strategy_traits_tests.hpp"
#include "memory_strategy_concepts_tests.hpp"
#include "allocator_memory_strategy_tests.hpp"
#include "static_buffer_strategy_tests.hpp"
#include "monotonic_byte_arena_tests.hpp"
#include "pool_memory_strategy_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("memory_strategy_all_tests.pdf"));
    // opts.set<dt::test_option::split>(dt::test_split::per_module);  // one PDF per module

    std::vector<dt::module_spec> modules = {
        tt::memory_strategy_traits_spec(),
        tt::memory_strategy_concepts_spec(),
        tt::allocator_memory_strategy_spec(),
        tt::static_buffer_strategy_spec(),
        tt::monotonic_byte_arena_spec(),
        tt::pool_memory_strategy_spec(),
    };

    return dt::run_suite(
        "memory-strategy axis",
        "the whole axis: the agnostic core, the C++20 concept layer, the "
        "concrete strategies (inline buffer, byte arenas, allocator and pool "
        "adapters)",
        modules,
        "memory-strategy axis - full suite",
        opts);
}
