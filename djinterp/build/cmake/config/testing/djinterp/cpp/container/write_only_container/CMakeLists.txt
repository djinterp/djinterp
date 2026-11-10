# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/container/write_only_container/CMakeLists.txt
#
#   Test leaf for write_only_container_tests, in DTest's spec idiom (docs/djinterp_testing.md,
# section 6.1): the runner in this directory, the section translation units
# globbed from the tests tree.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/write_only_container_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(write_only_container_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/write_only_container_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container"
        "${_tests_dir}")
