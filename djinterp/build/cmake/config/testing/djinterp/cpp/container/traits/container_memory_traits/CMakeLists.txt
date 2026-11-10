# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/container/traits/container_memory_traits/CMakeLists.txt
#
#   Test leaf for container_memory_traits_tests, in DTest's spec idiom (docs/djinterp_testing.md,
# section 6.1): the runner in this directory, the section translation units
# globbed from the tests tree.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/traits")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/container_memory_traits_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(container_memory_traits_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/container_memory_traits_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container/traits"
        "${_tests_dir}")
