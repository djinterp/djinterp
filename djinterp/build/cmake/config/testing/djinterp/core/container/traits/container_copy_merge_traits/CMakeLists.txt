# =============================================================================
# testing/djinterp/core/container/traits/  ::  container_copy_merge_traits (LEAF, AGENTS-cmake R1)
#   Builds container_copy_merge_traits_tests from the runner in THIS directory plus the section TUs
# globbed from the shared tests dir below. One target per directory: the glob is
# anchored on this component's stem, so no sibling suite's sections are captured,
# and no other leaf builds this target. Runnable alone: ctest -R container_copy_merge_traits_tests
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container/traits")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/container_copy_merge_traits_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(container_copy_merge_traits_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/container_copy_merge_traits_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container/traits"
        "${_tests_dir}")
