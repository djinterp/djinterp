# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      ostream_format_null/CMakeLists.txt
#
#   Test leaf for printf's heap fallback with no heap, in DTest's C face: the
# runner in this directory, the suite's translation unit, and the memory
# source compiled with D_CFG_MEM_SOURCE_DEFAULT_NULL (its value, 1, since the
# knob must be an integer literal). The runtime's ostream_format.c, linked as
# usual, resolves its source through this copy: the source's symbols are all
# defined here, so the runtime's own copy is never taken from the library.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

djinterp_add_test_executable(c_ostream_format_null_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ostream_format_null_tests_runner.c"
        "${_tests_dir}/ostream_format_null_tests.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_source.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    DEFINES
        D_CFG_MEM_SOURCE_DEFAULT=1
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
