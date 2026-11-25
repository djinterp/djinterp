# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/container/tree/floor/CMakeLists.txt
#
#    Test leaf for the rest of the tree floor (header-only): tree_path.hpp,
# avl/AVLtree.hpp, nary/nary_tree_traits.hpp and nary/nary_tree_concepts.hpp.
# The runner in this directory and the section translation units globbed
# from the tests tree.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/tree")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/tree_floor_tests_*.cpp")

djinterp_add_test_executable(tree_floor_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tree_floor_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${_tests_dir}")
