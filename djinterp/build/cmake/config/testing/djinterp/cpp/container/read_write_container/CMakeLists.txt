# ============================================================================
# djinterp [tests]                        read_write_container - leaf recipe
#
#   Builds the unit-test executable for read_write_container.hpp,
# the full-capability baseline of the access
# trio.  The runner lives here in the config tree; the
# section translation units are globbed from the tests tree.
#
# path:    /build/cmake/config/testing/djinterp/core/container/access/
#              read_write_container/CMakeLists.txt
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container")

# the glob keys on the component stem, so the two sibling suites' translation
# units in the same flat tests directory are never pulled into this executable
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/read_write_container_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(read_write_container_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/read_write_container_tests_runner.cpp"
        ${_sources}
    INCLUDES
        # bare header-under-test, and its sibling capability tag
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container"
        "${_tests_dir}")   # bare suite-header
