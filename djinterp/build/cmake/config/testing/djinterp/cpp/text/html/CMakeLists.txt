# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/text/
#                      html/CMakeLists.txt
#
#   Test leaf for text's HTML headers (html/html.hpp, html/html_dialect.hpp,
# html/html_printer.hpp and html/html_template.hpp), in DTest's C face: the
# runner in this directory, the suite's translation units globbed from the
# tests tree, the check and run text's C++ suites share (text_tests_common.cpp),
# and DTest's C face. The headers are header-only.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/html_tests*.cpp")

djinterp_add_test_executable(html_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/html_tests_runner.cpp"
        ${_sections}
        "${_tests_dir}/text_tests_common.cpp"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
