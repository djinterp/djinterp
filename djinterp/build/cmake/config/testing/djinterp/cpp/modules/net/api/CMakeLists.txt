# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/api/CMakeLists.txt
#
#  Leaf for net's Claude API client (cpp/modules/net/api), at C++98 and
# C++20 (cpp_net_tests.cmake):
#    - cpp_net_claude_tests: requests and replies over a loopback HTTP
#      server, through libcurl; without libcurl the program says so
#      and passes.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

djinterp_cpp_net_program(NAME claude CURL
    SOURCES
        "${_tests}/api/claude_tests.cpp"
        "${_tests}/api/claude_tests_main.cpp"
    C
        "${_net}/curl/curl.c"
        "${_net}/http/http.c"
        "${_net}/tcp/tcp.c")
