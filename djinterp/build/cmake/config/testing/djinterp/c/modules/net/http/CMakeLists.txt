# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/http/CMakeLists.txt
#
#   Test leaf for HTTP (c/modules/net/http), in DTest's C face, a CTest test
# with its runner in this directory:
#     - c_net_http_tests: the vocabulary, and HTTP/1.1's parser and writer.
# http_fuzz_http1.c, a fuzzer's harness, isn't part of it.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/http/http_tests*.c")

djinterp_add_test_executable(c_net_http_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/http_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_net}/http/http.c"
        "${_net}/http/http1.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/http")
