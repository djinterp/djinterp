# ============================================================================
# djinterp/core/container — container_metadata test suite
#
#   Leaf recipe.  The runner is listed explicitly from this directory (it
# lives in the config tree); the section TUs are globbed from the tests tree.
#
#   THREE include roots rather than the usual two.  The suite header includes
# container_metadata.hpp and metadata_traits.hpp both bare, and those two
# modules live under different slugs, so the metadata directory is listed
# alongside this one.  The composition test is what needs it: it asserts that
# a container_metadata satisfies none of the metadata protocol traits while a
# type written to the new names satisfies all three, and that comparison
# cannot be made without both headers in scope.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/container_metadata_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(container_metadata_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/container_metadata_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container"
        "${DJINTERP_INC_ROOT}/djinterp/util/metadata"
        "${_tests_dir}")
