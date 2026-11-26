# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/curl/CMakeLists.txt
#
#   Test leaf for the libcurl binding (c/modules/net/curl), in DTest's C
# face, a CTest test with its runner in this directory:
#     - c_net_curl_tests: the HTTP version a transfer asks for, and the one
#       its response came in, against a loopback peer that speaks HTTP/1.1
#       and HTTP/2 itself. HTTP/3's transfer goes to the server
#       DJINTERP_TEST_CURL_HTTP3_URL names, and skips without one. Without
#       libcurl the binding's stubs are built, and the suite checks they
#       refuse; with a libcurl lacking HTTP/2 or HTTP/3, those tests skip
#       and say so.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

find_package(Threads REQUIRED)
find_package(CURL QUIET)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/curl/curl_tests*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_curl_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/curl_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_net}/curl/curl.c"
        "${_net}/tcp/tcp.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/curl")
target_link_libraries(c_net_curl_tests PRIVATE djinterp_c_sync Threads::Threads)
if (WIN32)
    target_link_libraries(c_net_curl_tests PRIVATE ws2_32)
endif()
if (CURL_FOUND)
    target_link_libraries(c_net_curl_tests PRIVATE CURL::libcurl)
else()
    # headers CMake didn't find aren't used either, so nothing goes unlinked
    target_compile_definitions(c_net_curl_tests PRIVATE D_CFG_CURL=0)
    message(STATUS "djinterp c/modules/net/curl: libcurl not found; the "
                   "suite checks the stubs")
endif()
