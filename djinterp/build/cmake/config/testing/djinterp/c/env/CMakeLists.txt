# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/env/CMakeLists.txt
#
#   Test leaf for env/env.h, in DTest's C face: the runner
# in this directory, the section translation units globbed from the tests
# tree, the sources under test, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.06
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/env_tests*.c")

djinterp_add_test_executable(c_env_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/env_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
