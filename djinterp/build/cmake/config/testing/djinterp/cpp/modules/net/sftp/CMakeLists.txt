# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/sftp/CMakeLists.txt
#
#  Leaf for the SFTP client's C++ face (cpp/modules/net/sftp), at C++98
# and C++20 (cpp_net_tests.cmake):
#    - cpp_net_sftp_tests: sftp::client over the in-process server of
#      the C layer; libssh2 is linked where CMake finds it.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

file(GLOB _sftp CONFIGURE_DEPENDS "${_net}/sftp/sftp_*.c")
file(GLOB _ssh  CONFIGURE_DEPENDS "${_net}/ssh/ssh_*.c")

djinterp_cpp_net_program(NAME sftp LIBSSH2
    SOURCES
        "${_tests}/sftp/sftp_client_tests.cpp"
    C
        ${_sftp}
        ${_ssh}
        "${_net}/tcp/tcp.c")
