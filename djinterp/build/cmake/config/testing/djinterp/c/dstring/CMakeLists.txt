# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/dstring/CMakeLists.txt
#
#   Test leaf for dstring.h's suite, in DTest's C face: the runner in this
# directory, the section translation units globbed from the tests tree (the
# spec's and those built call by call), the sources under test, and DTest's
# C face, which djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.05
# revised:   2026.11.06 (the sections built call by call join the spec)
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/dstring_tests*.c")

djinterp_add_test_executable(c_dstring_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dstring_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
