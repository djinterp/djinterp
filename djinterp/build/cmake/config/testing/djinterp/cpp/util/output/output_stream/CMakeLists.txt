# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/util/output/
#                      output_stream/CMakeLists.txt
#
#   Test leaf for the output stream's C++ suite, in DTest's C face: the runner
# in this directory, the suite's translation units globbed from the tests tree,
# the C units the C++ face needs that djinterp_c_runtime does not carry, and
# DTest's C face, from djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util/output")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/output_stream_tests*.cpp")

djinterp_add_test_executable(output_stream_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/output_stream_tests_runner.cpp"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_string.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_file.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_console.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
    LIBRARIES
        djinterp_c_filesys
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
