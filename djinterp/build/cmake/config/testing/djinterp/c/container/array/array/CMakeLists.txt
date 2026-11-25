# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/container/array/array/CMakeLists.txt
#
#    Test leaf for container/array/array.h, in DTest's C face: the runner in
# this directory, the section translation units globbed from the tests tree, the
# sources under test and their dependencies, and DTest's C face, which
# it links as the framework's library, djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/container/array")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/array_tests*.c")

djinterp_add_test_executable(c_array_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/array_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/array/array.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/array/array_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    INCLUDES
        "${_tests_dir}"
    LIBRARIES
        djinterp_c_test)
