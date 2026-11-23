# ============================================================================
# djinterp/util/color - color_rgb test suite
#
#   Leaf recipe.  The runner is listed explicitly from this directory (it
# lives in the config tree); the section TUs are globbed from the tests tree.
# INCLUDES carries both roots the suite header's two faces need: the module
# directory, for the header under test, and the tests directory, for the bare
# suite-header include used by both faces.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util/color")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/color_rgb_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_runner\\.cpp$")

djinterp_add_test_executable(color_rgb_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/color_rgb_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util/color"
        "${_tests_dir}")
