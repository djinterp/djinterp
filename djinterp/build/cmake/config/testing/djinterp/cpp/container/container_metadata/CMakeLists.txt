# ============================================================================
# djinterp/cpp/container — container_metadata test suite
#
#   Leaf recipe.  The runner is listed explicitly from this directory (it
# lives in the config tree); the section TUs are globbed from the tests tree.
#
#   The suite header includes container_metadata.hpp and metadata_traits.hpp
# by relative path; the carrier tests need both, since a carrier must satisfy
# the metadata protocol's traits, which live in cpp/util/metadata.
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
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util/metadata"
        "${_tests_dir}")
