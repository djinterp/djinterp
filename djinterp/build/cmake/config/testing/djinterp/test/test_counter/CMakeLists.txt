# ============================================================================
# build/cmake/config/testing/djinterp/test/CMakeLists.txt
#
# Test wiring for test_counter.hpp -- a header-only module (no src/), so this
# follows AGENTS-cmake.md recipe R1. It builds the `test_counter_tests`
# executable from the runner (which lives here in the config tree) plus the
# section TUs under tests/, and registers it with CTest as `test_counter_tests`.
#
# slug:       djinterp/test   (header at inc/djinterp/test/test_counter.hpp)
# component:  test_counter
#
# Note: `djinterp/test` is a shared, multi-component directory. If sibling
# components (e.g. test_timer) are wired in this same CMakeLists, add their
# `djinterp_add_test_executable(...)` calls alongside the one below rather than
# replacing this file. A group target over the directory is a one-liner:
#     djinterp_add_test_subtree(all_djinterp_test)
# add it once, here, if/when this directory should expose an aggregate.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")

# Section TUs define the tests_* functions. The runner is named explicitly
# below and lives in this config dir, so this glob over tests/ never matches it
# (no duplicate main()). A legacy *_main.cpp, if present, is excluded.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_counter_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_counter_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_counter_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${_tests_dir}")
