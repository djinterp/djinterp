# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/text/
#                      text_core/CMakeLists.txt
#
#   Test leaf for text's core C++ headers (text_align.hpp, text_template.hpp
# and text_pattern.hpp), in DTest's C face: the runner in this directory, the
# suite's translation units globbed from the tests tree, the check and run
# text's C++ suites share (text_tests_common.cpp), and DTest's C face.
# The headers are header-only, so nothing else is compiled here.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/text_core_tests*.cpp")

djinterp_add_test_executable(text_core_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/text_core_tests_runner.cpp"
        ${_sections}
        "${_tests_dir}/text_tests_common.cpp"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
