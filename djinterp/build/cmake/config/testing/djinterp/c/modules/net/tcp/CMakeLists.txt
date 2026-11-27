# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/tcp/CMakeLists.txt
#
#   Test leaf for TCP (c/modules/net/tcp), in DTest's C face, a CTest test
# with its runner in this directory:
#     - c_net_tcp_tests: options, loopback connections and listeners, their
#       limits and errors, unix-domain sockets, and transfers between
#       threads. It listens on 127.0.0.1 and resolves localhost; its
#       watchdog ends a run that hangs.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

find_package(Threads REQUIRED)

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/tcp/tcp_tests*.c")

djinterp_add_test_executable(c_net_tcp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tcp_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_net}/tcp/tcp.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/tcp")
target_link_libraries(c_net_tcp_tests PRIVATE djinterp_c_sync Threads::Threads)
if (WIN32)
    target_link_libraries(c_net_tcp_tests PRIVATE ws2_32)
endif()
