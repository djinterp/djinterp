# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/container/tree/radix/CMakeLists.txt
#
#    Test leaf for container/tree/radix/ (header-only), part of the tree
# floor: the runner in this directory and the section translation units
# globbed from the tests tree.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/tree/radix")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/radix_tree_tests_*.cpp")

djinterp_add_test_executable(radix_tree_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/radix_tree_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${_tests_dir}")
