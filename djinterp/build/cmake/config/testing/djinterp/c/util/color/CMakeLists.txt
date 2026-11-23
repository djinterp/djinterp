# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/util/color/CMakeLists.txt
#
#   Test leaf for c/util/color - color.h's CIEDE2000 difference - in the C
# spec form: the runner in this directory, the section translation units
# globbed from the tests tree, every color model's source (color.h includes
# them all, and each .c emits its model's out-of-line definitions), and DTest's
# C face, from djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.17
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/color")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/color_tests_*.c")

djinterp_add_test_executable(c_color_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/color_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_cmyk.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_convert.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_hsl.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_hsv.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_lab.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_rgb.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/color/color_ycbcr.c"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
