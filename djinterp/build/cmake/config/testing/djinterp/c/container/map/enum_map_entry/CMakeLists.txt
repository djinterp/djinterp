# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/container/map/enum_map_entry/CMakeLists.txt
#
#    Test leaf for container/map/enum_map_entry.h, in DTest's C face: the runner
# in this directory, the section translation units globbed from the tests tree,
# the sources under test and their dependencies, and DTest's C face, which
# djinterp_c_runtime does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/container/map")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/enum_map_entry_tests*.c")

djinterp_add_test_executable(c_enum_map_entry_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/enum_map_entry_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
