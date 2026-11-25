# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/meta/math_vocabulary/CMakeLists.txt
#
#    Test leaf for meta/math_vocabulary.hpp (header-only): the runner in this
# directory and the section translation units globbed from the tests tree.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _sections CONFIGURE_DEPENDS
    "${_tests_dir}/math_vocabulary_tests_*.cpp")

djinterp_add_test_executable(math_vocabulary_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/math_vocabulary_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${_tests_dir}")
