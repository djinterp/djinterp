# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/util/ostream/
#                      ostream/CMakeLists.txt
#
#   Test leaf for the output stream's suite, in DTest's C face: the runner in
# this directory, the suite's translation units globbed from the tests tree,
# the stream's units djinterp_c_runtime does not carry (its destinations, and
# what the string and file ones need), and DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/ostream")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/ostream_tests*.c")

# the locale race runs two threads where the platform has POSIX threads
find_package(Threads)
set(_threads "")
if (Threads_FOUND)
    set(_threads Threads::Threads)
endif()

djinterp_add_test_executable(c_ostream_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ostream_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_string.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_file.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream_console.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_open.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}"
    LIBRARIES
        ${_threads})
