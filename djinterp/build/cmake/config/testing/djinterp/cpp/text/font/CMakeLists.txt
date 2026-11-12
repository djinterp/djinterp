# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/text/
#                      font/CMakeLists.txt
#
#   Test leaf for font.hpp's suite, on DTest's report builder: the runner in
# this directory and the suite's section translation units, globbed from the
# tests tree. font.hpp is header-only and C++17 at its floor; the helper
# builds at C++20.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/font_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_tests_(runner|main)\\.cpp$")

djinterp_add_test_executable(font_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/font_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${_tests_dir}")
