# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/http/CMakeLists.txt
#
#  Leaf for HTTP's C++ face (cpp/modules/net/http): two programs, each at
# C++98 and C++20 (cpp_net_tests.cmake):
#    - cpp_net_http_tests: http.hpp's requests and responses;
#    - cpp_net_http_helpers_tests: the string helpers http.hpp and
#      net_url.hpp keep for HTTP and URLs.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../cpp_net_tests.cmake")

set(_tests   "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/net")
set(_c_tests "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")

djinterp_cpp_net_program(NAME http
    SOURCES
        "${_tests}/http/http_tests.cpp"
        "${_tests}/http/http_tests_main.cpp"
    C
        "${_net}/http/http.c")

djinterp_cpp_net_program(NAME http_helpers
    SOURCES
        "${_tests}/http/http_tests_helpers.cpp"
    C
        "${_net}/http/http.c")
