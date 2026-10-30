# =============================================================================
# djinterp [test]  concurrency_strategy_traits suite
#
#   Leaf CMakeLists for the concurrency_strategy_traits component, under the
# slug djinterp/core/container/sync.  Header-only module: no src/ side.  The
# runner lives here (config tree); the section TUs are globbed from tests/.
#
#   Delivery note (guide 7.3): this leaf ships no parent aggregate.  Recursion
# only enters directories that carry a CMakeLists, so to reach this suite add a
# one-line djinterp_add_test_subtree(<unique_name>) at the nearest existing
# aggregate above djinterp/core/container/sync (none is shipped here, to avoid
# an archive overwriting sibling wiring).
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/sync")

# the section TUs for THIS component only (the stem is the component name, so
# the glob never picks up a sibling suite's translation units)
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/concurrency_strategy_traits_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# the runner lives beside this file; named via a variable only so the SOURCES
# line stays within the 79-column limit
set(_runner "concurrency_strategy_traits_tests_runner.cpp")

djinterp_add_test_executable(concurrency_strategy_traits_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/${_runner}"
        ${_sources}
    INCLUDES
        # bare "concurrency_strategy_traits.hpp" (suite header, normal face)
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container/sync"
        # bare "concurrency_strategy_traits_tests.hpp" (both faces)
        "${_tests_dir}")

# The real_primitives section includes the /core/sync/ primitives full-from-inc
# (<djinterp/core/sync/atomic.hpp> etc.).  Those resolve through the inc/ root
# djinterp_add_test_executable already adds, so no extra INCLUDES entry for the
# sync slug is required (guide 6.2).
