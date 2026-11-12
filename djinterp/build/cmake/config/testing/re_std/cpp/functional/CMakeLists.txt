# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/re_std/cpp/functional/CMakeLists.txt
#
#   Test leaf for re_std/cpp/functional/function.hpp, in DTest's C face: the
# runner in this directory, the section translation units globbed from the
# tests tree, and DTest's C face, which djinterp_c_runtime does not yet carry.
# re_std's functional headers are header-only. The target takes re_std's
# name, since djinterp's own functional suites are elsewhere.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/re_std/cpp/functional")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/function_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_(main|runner)\\.cpp$")

djinterp_add_test_executable(re_std_function_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/function_tests_runner.cpp"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
