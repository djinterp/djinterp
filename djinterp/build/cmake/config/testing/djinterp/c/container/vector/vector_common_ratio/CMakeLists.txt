# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/container/vector/vector_common_ratio/CMakeLists.txt
#
#    Test leaf for container/vector/vector_common.h with its growth ratio set
# as a factor: the vector_common leaf's runner and
# sections, and the sources under test, built with D_VECTOR_GROWTH_FACTOR=1.5.
# The ratio is then derived as 1536/1024, so every growth takes the quotient
# and remainder path with a remainder, which the default 2/1 never does, and
# the factor's compile-time check runs under the project's own flags.
# D_TESTS_VECTOR_GROWTH_EXPECT tells the tests the numerator to expect.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.16
# =============================================================================

set(_tests_dir  "${DJINTERP_TESTS_ROOT}/djinterp/c/container/vector")
set(_runner_dir "${CMAKE_CURRENT_SOURCE_DIR}/../vector_common")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/vector_common_tests*.c")

djinterp_add_test_executable(c_vector_common_ratio_tests
    SOURCES
        "${_runner_dir}/vector_common_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/vector/vector_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/array/array_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    DEFINES
        D_VECTOR_GROWTH_FACTOR=1.5
        D_TESTS_VECTOR_GROWTH_EXPECT=1536
    INCLUDES
        "${_tests_dir}"
    # libm, until djinterp_c_runtime links it
    LIBRARIES
        "$<$<NOT:$<C_COMPILER_ID:MSVC>>:m>")
