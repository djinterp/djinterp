# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/text/
#                      markup_string/CMakeLists.txt
#
#   Test leaf for text's markup string templates (markup_string_template.hpp
# and the HTML and XML facades over it), in DTest's C face: the runner in this
# directory, the suite's translation units globbed from the tests tree, the
# check and run text's C++ suites share (text_tests_common.cpp), and DTest's C
# face, from djinterp_c_test. The headers are header-only.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/markup_string_tests*.cpp")

djinterp_add_test_executable(markup_string_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/markup_string_tests_runner.cpp"
        ${_sections}
        "${_tests_dir}/text_tests_common.cpp"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
