# =============================================================================
# testing/djinterp/test/test_kind/
#   LEAF: builds test_kind_tests - the exe for inc/djinterp/test/test_kind.hpp.
#   See test_common/CMakeLists.txt for the full rationale; same shape with the
# test_kind_* sources.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_kind_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_kind_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_kind_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${_tests_dir}")
