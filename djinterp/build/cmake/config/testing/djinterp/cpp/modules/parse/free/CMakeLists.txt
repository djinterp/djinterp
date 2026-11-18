#*******************************************************************************
# djinterp [parse]                                                CMakeLists.txt
#
# Test leaf for parse's free strata (cpp/modules/parse/parser/free.hpp).
#   Builds free_tests from the runner in this directory and the section
# translation units of tests/djinterp/cpp/modules/parse/free/
# (free_tests_*.cpp), in the whole project's build, as the other C++ parse
# leaves under this aggregate are. The selective block's two tests stay
# behind DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED until the selective
# stratum's monad bridge is fixed.
#
#
# path:      /build/cmake/config/testing/djinterp/cpp/modules/parse/free/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
#                                                            revised: 2026.11.18
#*******************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/parse/free")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/free_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(free_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/free_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/functional"
        "${_tests_dir}")
