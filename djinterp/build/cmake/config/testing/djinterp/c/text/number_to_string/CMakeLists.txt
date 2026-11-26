# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/text/number_to_string/CMakeLists.txt
#
#   Test leaf for c/text's numbers in words, number_to_string_en.h to
# number_to_string_pt.h, in the C spec form: the runner in this directory,
# the section translation units globbed from the tests tree, the six
# languages' sources, and DTest's C face, from djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS
    "${_tests_dir}/number_to_string_tests_*.c")

djinterp_add_test_executable(c_number_to_string_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/number_to_string_tests_runner.c"
        ${_sections}
        "${_src_dir}/number_to_string_en.c"
        "${_src_dir}/number_to_string_es.c"
        "${_src_dir}/number_to_string_fr.c"
        "${_src_dir}/number_to_string_it.c"
        "${_src_dir}/number_to_string_la.c"
        "${_src_dir}/number_to_string_pt.c"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
