# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/lmtp/CMakeLists.txt
#
#  Leaf for LMTP's C++ facade (cpp/modules/net/lmtp), at C++98 and C++20
# (cpp_net_tests.cmake):
#    - cpp_net_lmtp_tests: the facade's client and server over loopback TCP
#      and a unix-domain socket, a scripted server that hangs up, and a real
#      LMTP server where the environment names one. The facade holds the
#      SMTP face to LMTP's dialect, so smtp.cpp and the kernel build here.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.24
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_net   "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_cpp   "${DJINTERP_SRC_ROOT}/djinterp/cpp/modules/net")

file(GLOB _smtp CONFIGURE_DEPENDS "${_net}/smtp/smtp_*.c")

djinterp_cpp_net_program(NAME lmtp OPENSSL
    SOURCES
        "${_tests}/lmtp/lmtp_tests.cpp"
        "${_tests}/lmtp/lmtp_tests_main.cpp"
    C
        ${_smtp}
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
    CXX
        "${_cpp}/smtp/smtp.cpp"
        "${_cpp}/lmtp/lmtp.cpp")
