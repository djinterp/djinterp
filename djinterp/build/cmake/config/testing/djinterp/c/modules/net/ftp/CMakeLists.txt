# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/ftp/CMakeLists.txt
#
#   Test leaf for net's FTP client (c/modules/net/ftp), in DTest's C face, a
# CTest test with its runner in this directory:
#     - c_net_ftp_tests: module ftp's three blocks: units, ftp.h's own
#       functions and the client's helpers over a scripted transport;
#       ftps, the client over loopback against a threaded server, both
#       secured by the toy engine; and interop, a real server where
#       FTP_TEST_PORT names one, else a skipped test. The TLS adapter's
#       test runs over OpenSSL where CMake finds it, and is skipped
#       where it doesn't. It runs on POSIX systems alone: it drives its
#       sockets and threads there.
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
    message(STATUS "djinterp c/modules/net/ftp: a POSIX suite; not built on "
                   "Windows")
    return()
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/ftp/ftp_tests*.c")
file(GLOB _ftp   CONFIGURE_DEPENDS "${_net}/ftp/ftp_*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_ftp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ftp_tests_runner.c"
        ${_tests}
        "${_tests_dir}/ftp/ftps_tests_support.c"
        ${_ftp}
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/ftp")
target_link_libraries(c_net_ftp_tests PRIVATE Threads::Threads)
if (OpenSSL_FOUND)
    target_link_libraries(c_net_ftp_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_ftp_tests PRIVATE
                               D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
    message(STATUS "djinterp c/modules/net/ftp: OpenSSL not found; the TLS "
                   "adapter's test skips")
endif()
