# ============================================================================
# djinterp/util/metadata — metadata_traits test suite
#
#   Leaf recipe.  The runner is listed explicitly from this directory (it
# lives in the config tree); the section TUs are globbed from the tests tree.
# INCLUDES carries both roots the suite header's two faces need: the module
# directory, for the normal-mode include of the header under test, and the
# tests directory, for the bare suite-header include used by both faces.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/util/metadata")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/metadata_traits_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(metadata_traits_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/metadata_traits_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/util/metadata"
        "${_tests_dir}")
