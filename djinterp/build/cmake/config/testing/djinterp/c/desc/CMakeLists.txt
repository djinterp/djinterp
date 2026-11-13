# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/desc/CMakeLists.txt
#
#   Test leaf for c/desc's suite, in DTest's C face
# (c/modules/test/test_suite.h), a CTest test with its runner in this
# directory:
#     - c_desc_tests: desc.h's core, which is header-only, and the units
#       engine, its derived facts, its djin and its macro layer, which link
#       desc's two units and option_env's, beside DTest's C face.
#
#
# path:      /build/cmake/config/testing/djinterp/c/desc/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
#                                                            revised: 2026.11.12
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/desc")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")
set(_desc      "${DJINTERP_SRC_ROOT}/djinterp/c/desc")
set(_option    "${DJINTERP_SRC_ROOT}/djinterp/c/option")

djinterp_add_test_executable(c_desc_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/desc_tests_runner.c"
        "${_tests_dir}/desc_tests.c"
        "${_tests_dir}/desc_units_tests.c"
        "${_desc}/desc_units.c"
        "${_desc}/desc_options.c"
        "${_option}/option_env.c"
        "${_option}/option_env_registry.c"
        "${_option}/option_set_common.c"
        "${_option}/option_common.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}")
