# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/text/
#                      document_templates/CMakeLists.txt
#
#   Test leaf for the document-template suite (cpp/util/document/templates and
# the PDF face over DPdfCanvas), on DTest's report builder. The suite is one
# header of inline predicates in the tests tree; the runner in this directory
# is its only translation unit.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/text")

djinterp_add_test_executable(document_templates_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/document_templates_tests_runner.cpp"
    INCLUDES
        "${_tests_dir}")
