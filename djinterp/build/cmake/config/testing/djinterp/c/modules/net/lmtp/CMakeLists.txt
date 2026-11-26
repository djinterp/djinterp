# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/lmtp/CMakeLists.txt
#
#   Test leaf for LMTP (c/modules/net/lmtp), in DTest's C face, a CTest test
# with its runner in this directory:
#     - c_net_lmtp_tests: the client and the server, each over a scripted
#       peer; both together over loopback TCP and a unix-domain socket; and
#       a real LMTP server, where the environment names one (lmtp_tests.h
#       says how), which skips otherwise. LMTP is the SMTP kernel's dialect,
#       so the kernel's sources build here too.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.24
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/lmtp/lmtp_tests*.c")
file(GLOB _lmtp  CONFIGURE_DEPENDS "${_net}/lmtp/lmtp_*.c")
file(GLOB _smtp  CONFIGURE_DEPENDS "${_net}/smtp/smtp_*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_lmtp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/lmtp_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_tests_dir}/net_tests_peer.c"
        ${_lmtp}
        ${_smtp}
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/lmtp")
target_link_libraries(c_net_lmtp_tests PRIVATE djinterp_c_sync Threads::Threads)
if (OpenSSL_FOUND)
    target_link_libraries(c_net_lmtp_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_lmtp_tests PRIVATE
                               D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
endif()
if (WIN32)
    target_link_libraries(c_net_lmtp_tests PRIVATE ws2_32)
endif()
