# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/container/map/min_enum_map/CMakeLists.txt
#
#    Test leaf for container/map/min_enum_map.h, in DTest's C face: the runner
# in this directory, the section translation units globbed from the tests tree,
# the sources under test and their dependencies, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/container/map")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/min_enum_map_tests*.c")

djinterp_add_test_executable(c_min_enum_map_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/min_enum_map_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/map/min_enum_map.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
