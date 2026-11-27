# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/core/CMakeLists.txt
#
#   Test leaf for net's core (c/modules/net: net_common, net_memory and the
# URL), in DTest's C face, a CTest test with its runner in this directory:
#     - c_net_tests: errors, results, endpoints, connections and the
#       algorithms over them, framing and URLs, over a scripted connection
#       and in-memory pipes. It opens no socket.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

djinterp_add_test_executable(c_net_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/net_tests_runner.c"
        "${_tests_dir}/net_tests.c"
        "${_tests_dir}/net_tests_assert.c"
        "${_tests_dir}/net_tests_memory.c"
        "${_tests_dir}/net_tests_stream.c"
        "${_tests_dir}/net_tests_support.c"
        "${_tests_dir}/net_tests_url.c"
        "${_tests_dir}/net_tests_vocabulary.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}")
