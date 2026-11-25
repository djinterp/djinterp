# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/sftp/CMakeLists.txt
#
#   Test leaf for net's SFTP client (c/modules/net/sftp), in DTest's C
# face, a CTest test with its runner in this directory:
#     - c_net_sftp_tests: the codec; the client against an in-process
#       server, over a socket pair and over the toy SSH engine; the
#       client over connections that fail as told; and a real server
#       where SFTP_TEST_* name one, else a skipped test. libssh2 is
#       linked where CMake finds it, and its engine left out where it
#       isn't. It runs on POSIX systems alone: it drives its socket
#       pairs and threads there.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

if (WIN32)
    message(STATUS "djinterp c/modules/net/sftp: a POSIX suite; not built on "
                   "Windows")
    return()
endif()

find_package(Threads REQUIRED)
find_path(DJINTERP_LIBSSH2_INCLUDE NAMES libssh2.h)
find_library(DJINTERP_LIBSSH2_LIBRARY NAMES ssh2 libssh2)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/sftp/sftp_tests*.c")
file(GLOB _sftp  CONFIGURE_DEPENDS "${_net}/sftp/sftp_*.c")
file(GLOB _ssh   CONFIGURE_DEPENDS "${_net}/ssh/ssh_*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_sftp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sftp_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_ssh_toy.c"
        ${_sftp}
        ${_ssh}
        "${_net}/tcp/tcp.c"
        ${_codec}
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/sftp")
target_link_libraries(c_net_sftp_tests PRIVATE Threads::Threads)
if (DJINTERP_LIBSSH2_INCLUDE AND DJINTERP_LIBSSH2_LIBRARY)
    target_include_directories(c_net_sftp_tests PRIVATE
                               "${DJINTERP_LIBSSH2_INCLUDE}")
    target_link_libraries(c_net_sftp_tests PRIVATE
                          "${DJINTERP_LIBSSH2_LIBRARY}")
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_sftp_tests PRIVATE D_CFG_SSH_LIBSSH2=0)
endif()
