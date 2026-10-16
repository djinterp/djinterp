# =============================================================================
# djinterp [test]  threadsafe_container suite
#
#   Leaf CMakeLists for the threadsafe_container component, under the slug
# djinterp/core/container.  Header-only module: no src/ side.  The runner lives
# here (config tree); the section TUs are globbed from tests/.
#
#   The "concurrent" section drives real std::thread contention, so the target
# needs the thread runtime.  The test helper links it the same way it does for
# the framework's own concurrency modules (test_thread / test_concurrent /
# test_stress), so no extra wiring is required here.
#
#   Delivery note (guide 7.3): this leaf ships no parent aggregate.  Recursion
# only enters directories that carry a CMakeLists, so to reach this suite add a
# one-line djinterp_add_test_subtree(<unique_name>) at the nearest existing
# aggregate above djinterp/core/container (none is shipped here, to avoid an
# archive overwriting sibling wiring).
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container")

# the section TUs for THIS component only (the stem is the component name, so
# the non-recursive glob never picks up a sibling suite's translation units)
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/threadsafe_container_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# the runner lives beside this file; named via a variable only so the SOURCES
# line stays within the 79-column limit
set(_runner "threadsafe_container_tests_runner.cpp")

djinterp_add_test_executable(threadsafe_container_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/${_runner}"
        ${_sources}
    INCLUDES
        # bare "threadsafe_container.hpp" (suite header, normal face)
        "${DJINTERP_INC_ROOT}/djinterp/core/container"
        # bare "threadsafe_container_tests.hpp" (both faces)
        "${_tests_dir}")
