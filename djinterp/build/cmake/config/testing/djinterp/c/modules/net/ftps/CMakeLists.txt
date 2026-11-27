# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/ftps/CMakeLists.txt
#
#   Test leaf for net's FTPS client (c/modules/net/ftps), in DTest's C
# face, a CTest test with its runner in this directory:
#     - c_net_ftps_tests: module ftps's three blocks: adapter, the TLS
#       layer under the FTP client's helpers over a scripted transport;
#       loopback, the client against a threaded server, both secured by
#       the toy engine; and interop, a real server where FTP_TEST_PORT
#       names one, else a skipped test. The adapter runs over OpenSSL
#       where CMake finds it, and is skipped where it doesn't. It runs
#       on POSIX systems alone: it drives its sockets and threads there.
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
    message(STATUS "djinterp c/modules/net/ftps: a POSIX suite; not built on "
                   "Windows")
    return()
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

# the suite's files, its support server among them
file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/ftps/ftps_tests*.c")
file(GLOB _ftp   CONFIGURE_DEPENDS "${_net}/ftp/ftp_*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_ftps_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ftps_tests_runner.c"
        ${_tests}
        "${_tests_dir}/ftp/ftp_tests_fake.c"
        "${_net}/ftps/ftps.c"
        ${_ftp}
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/ftps")
target_link_libraries(c_net_ftps_tests PRIVATE Threads::Threads)
if (OpenSSL_FOUND)
    target_link_libraries(c_net_ftps_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_ftps_tests PRIVATE
                               D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
    message(STATUS "djinterp c/modules/net/ftps: OpenSSL not found; the TLS "
                   "adapter's test skips")
endif()
