#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   Leaf build for the option_compose.hpp unit suite (AGENTS-cmake R1).  Builds the
# exe option_compose_tests from this directory's runner plus the suite's section TUs,
# which live in the shared tests directory tests/djinterp/core/option/ and are
# selected by the glob below.  The helper adds inc/ and D_TESTING and registers
# the exe with CTest; the parent option/ aggregates it via its subtree call.
#
# path: /build/cmake/config/testing/djinterp/core/option/option_compose/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/option")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/option_compose_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(option_compose_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/option_compose_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/option"
        "${_tests_dir}")
