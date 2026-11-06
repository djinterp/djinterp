# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/string_fn/
#                      CMakeLists.txt
#
#   Test leaf for string_fn.h's suite, in DTest's C face: the runner in this
# directory, the section translation units globbed from the tests tree (the
# standalone *_tests_sa*.c files there are left out), the sources under test,
# and DTest's C face, which djinterp_c_runtime does not yet carry (a request
# to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.05
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/string_fn_tests_*.c")
list(FILTER _sections EXCLUDE REGEX "_tests_sa[^/]*\\.c$")

djinterp_add_test_executable(c_string_fn_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/string_fn_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
