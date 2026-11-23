# =============================================================================
# djinterp - build/cmake/djinterp/test.cmake
#
#   djinterp_c_test: DTest's C face, test_suite.c, and its event alphabet,
# test_event.c, for the suites to link rather than compile them each. The
# C++ face's event suites link it for the alphabet, which they hold to C's
# in their parity section. Named for its module, c/modules/test.
# =============================================================================

djinterp_add_library(djinterp_c_test
    SOURCES
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_event.c"
    LINKS
        djinterp_c_runtime)

target_compile_definitions(djinterp_c_test PUBLIC D_TESTING=1)
