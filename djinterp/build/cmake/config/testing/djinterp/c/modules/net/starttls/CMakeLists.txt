# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/starttls/CMakeLists.txt
#
#   Test leaf for STARTTLS (c/modules/net/starttls), in DTest's C face, a
# CTest test with its runner in this directory:
#     - c_net_starttls_tests: the upgrade of a plain session over the
#       scripted peer's toy engine, over TCP on 127.0.0.1, and, with
#       OpenSSL found, over its engine on certificates the run makes;
#       without it, those tests are one skipped test. It runs on POSIX
#       systems alone: it drives its sockets and its watchdog there.
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
    message(STATUS "djinterp c/modules/net/starttls: a POSIX suite; not built "
                   "on Windows")
    return()
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/starttls/starttls_tests*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_starttls_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/starttls_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_tests_dir}/net_tests_peer.c"
        "${_tests_dir}/net_tests_pki.c"
        "${_tests_dir}/net_tests_support.c"
        "${_net}/starttls/starttls.c"
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        ${_codec}
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/starttls")
target_link_libraries(c_net_starttls_tests PRIVATE Threads::Threads)
if (OpenSSL_FOUND)
    target_link_libraries(c_net_starttls_tests PRIVATE
                          OpenSSL::SSL OpenSSL::Crypto)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_starttls_tests PRIVATE
                               D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
    message(STATUS "djinterp c/modules/net/starttls: OpenSSL not found; its "
                   "engine's tests skip")
endif()
