# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/cpp/modules/net/core/CMakeLists.txt
#
#  Leaf for net's C++ core (cpp/modules/net): four programs, each at C++98
# and C++20 (cpp_net_tests.cmake):
#    - cpp_net_core_tests: net_common.hpp and net_url.hpp;
#    - cpp_net_client_tests: client.hpp, over loopback TCP;
#    - cpp_net_server_tests: server.hpp, plain and over TLS;
#    - cpp_net_reactor_tests: reactor.hpp, where epoll is; without it
#      the program says so and passes.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

djinterp_cpp_net_program(NAME core
    SOURCES
        "${_tests}/net_tests.cpp"
        "${_tests}/net_tests_url.cpp"
        "${_tests}/net_tests_main.cpp"
    C
        "${_c_tests}/net_tests_support.c")

djinterp_cpp_net_program(NAME client
    SOURCES
        "${_tests}/client_tests.cpp"
        "${_tests}/client_tests_main.cpp"
    C
        "${_net}/tcp/tcp.c")

djinterp_cpp_net_program(NAME server OPENSSL
    SOURCES
        "${_tests}/server_tests.cpp"
        "${_tests}/server_tests_main.cpp"
    C
        "${_net}/http/http.c"
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c")

djinterp_cpp_net_program(NAME reactor
    SOURCES
        "${_tests}/reactor_tests.cpp"
        "${_tests}/reactor_tests_main.cpp"
    C
        "${_net}/tcp/tcp.c")
