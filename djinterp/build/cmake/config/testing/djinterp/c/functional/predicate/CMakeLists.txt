# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/functional/predicate/CMakeLists.txt
#
#    Test leaf for functional/predicate.h, in DTest's C face: the runner in this
# directory, the section translation units globbed from the tests tree, the
# sources under test and their dependencies, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/functional")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/predicate_tests*.c")

djinterp_add_test_executable(c_predicate_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/predicate_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/functional/predicate.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
