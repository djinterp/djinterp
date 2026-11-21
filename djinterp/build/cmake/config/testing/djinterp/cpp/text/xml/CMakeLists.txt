# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/text/
#                      xml/CMakeLists.txt
#
#   Test leaf for text's XML headers (xml/xml.hpp, xml/xml_dialect.hpp and
# xml/xml_template.hpp), in DTest's C face: the runner in this directory, the
# suite's translation units globbed from the tests tree, the check and run
# text's C++ suites share (text_tests_common.cpp), and DTest's C face, from
# djinterp_c_test. The headers are header-only.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/xml_tests*.cpp")

djinterp_add_test_executable(xml_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/xml_tests_runner.cpp"
        ${_sections}
        "${_tests_dir}/text_tests_common.cpp"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
