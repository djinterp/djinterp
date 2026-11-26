# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/ftp/CMakeLists.txt
#
#  Leaf for the FTP and FTPS clients' C++ faces (cpp/modules/net/ftp and
# ftps), at C++98 and C++20 (cpp_net_tests.cmake):
#    - cpp_net_ftp_tests: ftp::client and ftps::client against the
#      threaded server of ftps_tests_support.c, the toy engine securing
#      both ends.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

file(GLOB _ftp  CONFIGURE_DEPENDS "${_net}/ftp/ftp_*.c")

djinterp_cpp_net_program(NAME ftp OPENSSL
    SOURCES
        "${_tests}/ftp/ftp_client_tests.cpp"
    C
        ${_ftp}
        "${_net}/ftps/ftps.c"
        "${_net}/tls/tls.c"
        "${_net}/tls/tls_openssl.c"
        "${_net}/tcp/tcp.c"
        "${_c_tests}/ftps/ftps_tests_support.c")
