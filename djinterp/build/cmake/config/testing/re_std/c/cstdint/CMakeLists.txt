# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/re_std/c/cstdint/CMakeLists.txt
#
#   Test leaf for re_std/c/cstdint/dstdint.h: the conformance harness
# tests/re_std/c/cstdint/t_dstdint.c, built as a DTest suite
# (T_DSTDINT_DTEST), its own runner; dstdint.c, whose type, limit and constant
# checks compile alongside; and DTest's C face, which djinterp_c_runtime does
# not yet carry. The harness's other builds (the matrix of language levels
# and backends, and the freestanding check) are in its banner.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/re_std/c/cstdint")

djinterp_add_test_executable(c_dstdint_tests
    SOURCES
        "${_tests_dir}/t_dstdint.c"
        "${DJINTERP_SRC_ROOT}/re_std/c/cstdint/dstdint.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    DEFINES
        T_DSTDINT_DTEST=1
    INCLUDES
        "${_tests_dir}")
