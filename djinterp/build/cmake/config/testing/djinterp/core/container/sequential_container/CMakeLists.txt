# ============================================================================
# sequential_container test suite - leaf recipe
#
#   Globs this component's section TUs (keyed on the
# `sequential_container_tests_` stem, so it never captures a sibling suite),
# lists the runner from this config directory, and wires the executable.
# Header-only module, so there is no src/ to add.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/sequential_container_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(sequential_container_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sequential_container_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container"   # module dir
        "${_tests_dir}")                                 # suite-header dir
