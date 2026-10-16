#*****************************************************************************
# djinterp [tests]                                              CMakeLists.txt
#
#   Leaf recipe for the test_concurrent_tests suite.  One component, one directory:
# this file and its runner are the whole of this suite's build wiring, so the pair can be dropped in, moved, or deleted without touching another suite.
#
#   Cache roots published by the root config are used verbatim - never
# hand-rolled paths.
#
# path:  /build/cmake/config/testing/djinterp/test/sync/test_concurrent/CMakeLists.txt
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/sync")

# the section TUs for THIS component only - the stem is what tells sibling
# suites in the (flat) tests directory apart
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_concurrent_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_concurrent_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_concurrent_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test/sync"   # bare header-under-test include
        "${_tests_dir}")                            # bare suite-header include
