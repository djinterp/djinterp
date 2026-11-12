# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/util/archive/
#                      archive_kernel/CMakeLists.txt
#
#   Test leaf for the archive kernel's suite, in DTest's C face: the runner in
# this directory, the suite's translation units globbed from the tests tree,
# and DTest's C face.  The kernel itself (archive_common.c, the codecs and
# their libraries) arrives with djinterp_c_runtime, which the helper links.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/archive")

file(GLOB _sections CONFIGURE_DEPENDS
     "${_tests_dir}/archive_kernel_tests*.c")

djinterp_add_test_executable(c_archive_kernel_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/archive_kernel_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
