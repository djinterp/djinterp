# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/ftp/CMakeLists.txt
#
#   Test leaf for net's FTP client (c/modules/net/ftp), in DTest's C face, a
# CTest test with its runner in this directory:
#     - c_net_ftp_tests: ftp.h's own functions, and the plain client and
#       its helpers over a scripted transport and loopback sockets. It
#       links no TLS: FTPS is module ftps's leaf. It runs on POSIX
#       systems alone: it drives its sockets there.
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

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/ftp/ftp_tests*.c")
file(GLOB _ftp   CONFIGURE_DEPENDS "${_net}/ftp/ftp_*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_ftp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ftp_tests_runner.c"
        ${_tests}
        ${_ftp}
        "${_net}/tcp/tcp.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/ftp")
