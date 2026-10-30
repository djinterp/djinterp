# ============================================================================
# ordered_container test suite - leaf recipe
#
#   Globs this component's section TUs from the flat tests directory, lists the
# runner from this config directory, and wires the executable.  The glob keys
# on the `ordered_container_tests_` stem, so it never pulls in a sibling
# suite's translation units.  Header-only module, so there is no src/ to add.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/ordered_container_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(ordered_container_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ordered_container_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container"   # module dir
        "${_tests_dir}")                                 # suite-header dir
