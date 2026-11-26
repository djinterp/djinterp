# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/tcp/CMakeLists.txt
#
#  Leaf for TCP's C++ face (cpp/modules/net/tcp), at C++98 and C++20
# (cpp_net_tests.cmake):
#    - cpp_net_tcp_tests: connections and listeners over loopback.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

djinterp_cpp_net_program(NAME tcp
    SOURCES
        "${_tests}/tcp/tcp_tests.cpp"
        "${_tests}/tcp/tcp_tests_main.cpp"
    C
        "${_net}/tcp/tcp.c")
