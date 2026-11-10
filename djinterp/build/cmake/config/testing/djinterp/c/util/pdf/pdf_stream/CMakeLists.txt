# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/util/pdf/
#                      pdf_stream/CMakeLists.txt
#
#   Test leaf for the PDF writer's stream suite, in DTest's C face: the runner
# in this directory, the suite's translation units globbed from the tests
# tree, the C PDF units, and DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/pdf")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/pdf_stream_tests*.c")

djinterp_add_test_executable(c_pdf_stream_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pdf_stream_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_builtin_backend.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_document.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_backend.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_primitives.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
