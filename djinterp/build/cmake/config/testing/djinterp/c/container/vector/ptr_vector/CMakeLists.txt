# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/container/vector/ptr_vector/CMakeLists.txt
#
#    Test leaf for container/vector/ptr_vector.h, in DTest's C face: the runner
# in this directory, the section translation units globbed from the tests tree,
# the sources under test and their dependencies, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/container/vector")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/ptr_vector_tests*.c")

djinterp_add_test_executable(c_ptr_vector_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ptr_vector_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/vector/ptr_vector.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/array/array_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/vector/vector_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}"
    # libm, until djinterp_c_runtime links it (a request to lane A)
    LIBRARIES
        "$<$<NOT:$<C_COMPILER_ID:MSVC>>:m>")
