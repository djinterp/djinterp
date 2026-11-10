# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/util/pdf/
#                      pdf_stream/CMakeLists.txt
#
#   Test leaf for the C++ PDF writer's stream suite, in DTest's C face: the
# runner in this directory, the suite's translation units globbed from the
# tests tree, the C PDF units it is held to, the units the stream's string
# destination needs, and DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util/pdf")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/pdf_stream_tests*.cpp")

djinterp_add_test_executable(pdf_stream_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pdf_stream_tests_runner.cpp"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_builtin_backend.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_document.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_backend.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/pdf/pdf_primitives.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_string.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_file.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_console.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_open.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
