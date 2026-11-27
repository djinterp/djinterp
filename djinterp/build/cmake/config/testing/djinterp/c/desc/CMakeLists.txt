# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/desc/CMakeLists.txt
#
#   Test leaf for c/desc's suite, in DTest's C face
# (c/modules/test/test_suite.h), a CTest test with its runner in this
# directory:
#     - c_desc_tests: desc.h's core, which is header-only; the units
#       engine, its derived facts, its djin and its macro layer; desc's
#       machines; and the describers of memory's, filesys's and DTest's
#       types. They link desc's three units, option_env's, and the
#       stream's template unit (the runtime has the stream itself), beside
#       DTest's C face; and the describers' units. Memory's units, arena
#       and pool among them, are the runtime's, whose layout knobs are the
#       whole build's (ruled 2026.11.26), so the describers and the
#       runtime lay memory's types out alike; the suite checks that they
#       agree (d_desc_of_mem_agrees and its kin).
#
#
# path:      /build/cmake/config/testing/djinterp/c/desc/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
#                                                            revised: 2026.11.26
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
        "${_tests_dir}/desc_machines_tests.c"
        "${_tests_dir}/desc_of_tests_common.c"
        "${_tests_dir}/desc_of_mem_tests.c"
        "${_tests_dir}/desc_of_filesys_tests.c"
        "${_tests_dir}/desc_of_test_tests.c"
        "${_desc}/desc_units.c"
        "${_desc}/desc_options.c"
        "${_desc}/desc_machines.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/ostream_template.c"
        "${_option}/option_env_desc.c"
        "${_option}/option_env.c"
        "${_option}/option_env_registry.c"
        "${_option}/option_set_common.c"
        "${_option}/option_common.c"
        "${_desc}/of/mem_desc.c"
        "${_desc}/of/filesys_desc.c"
        "${_desc}/of/test_desc.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}")
