# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/cpp/modules/net/tls/CMakeLists.txt
#
#  Leaf for TLS's C++ face (cpp/modules/net/tls), at C++98 and C++20
# (cpp_net_tests.cmake):
#    - cpp_net_tls_tests: contexts and sessions over loopback through
#      the OpenSSL engine; without it the program says so and passes.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

djinterp_cpp_net_program(NAME tls OPENSSL
    SOURCES
        "${_tests}/tls/tls_tests.cpp"
        "${_tests}/tls/tls_tests_main.cpp"
    C
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c")
