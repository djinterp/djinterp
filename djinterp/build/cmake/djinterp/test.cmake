# =============================================================================
# djinterp - build/cmake/djinterp/test.cmake
#
#   djinterp_c_test: DTest's C face, test_suite.c, for the suites to link
# rather than compile it each. Named for its module, c/modules/test.
# =============================================================================

djinterp_add_library(djinterp_c_test
    SOURCES
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    LINKS
        djinterp_c_runtime)

target_compile_definitions(djinterp_c_test PUBLIC D_TESTING=1)
