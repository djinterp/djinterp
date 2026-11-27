# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/tls/CMakeLists.txt
#
#   Test leaf for net's TLS engine (c/modules/net/tls), in DTest's C face, a
# CTest test with its runner in this directory:
#     - c_net_tls_tests: contexts, sessions and the engine's view of them,
#       over an OpenSSL-family engine and certificates the run makes.
#       Without OpenSSL there is nothing to test with, and the suite's one
#       test says so, skipped.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

find_package(OpenSSL QUIET)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/tls/tls_tests*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_tls_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tls_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_tests_dir}/net_tests_pki.c"
        "${_tests_dir}/net_tests_support.c"
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        ${_codec}
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/tls")
if (OpenSSL_FOUND)
    target_link_libraries(c_net_tls_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_tls_tests PRIVATE
                               D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
    message(STATUS "djinterp c/modules/net/tls: OpenSSL not found; the suite "
                   "skips")
endif()
