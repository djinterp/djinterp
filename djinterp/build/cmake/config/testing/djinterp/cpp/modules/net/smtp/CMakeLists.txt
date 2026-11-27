# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/cpp/modules/net/smtp/CMakeLists.txt
#
#  Leaf for SMTP's C++ facade (cpp/modules/net/smtp), at C++98 and C++20
# (cpp_net_tests.cmake):
#    - cpp_net_smtp_tests: the facade's client and server over
#      loopback, plain and over TLS on certificates the run makes.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

file(GLOB _smtp CONFIGURE_DEPENDS "${_net}/smtp/smtp_*.c")

djinterp_cpp_net_program(NAME smtp OPENSSL
    SOURCES
        "${_tests}/smtp/smtp_tests.cpp"
        "${_tests}/smtp/smtp_tests_main.cpp"
    C
        ${_smtp}
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        "${_c_tests}/net_tests_pki.c"
    CXX
        "${DJINTERP_SRC_ROOT}/djinterp/cpp/modules/net/smtp/smtp.cpp")
