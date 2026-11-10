# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/functional/fn_builder/CMakeLists.txt
#
#    Test leaf for functional/fn_builder.h, in DTest's C face: the runner in
# this directory, the section translation units globbed from the tests tree, the
# sources under test and their dependencies, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/functional")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/fn_builder_tests*.c")

djinterp_add_test_executable(c_fn_builder_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/fn_builder_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/functional/fn_builder.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
