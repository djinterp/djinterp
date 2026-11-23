#*******************************************************************************
# djinterp [test]                                                 CMakeLists.txt
#
# Test leaf for the interpreter's module, interp/: the spine's suite,
# tests/djinterp/cpp/modules/interp/spine_tests.cpp, one source with its own
# main.
#   The spine is header-only, so the suite needs nothing past the inc/ root
# the helper adds, and links nothing past djinterp_c_runtime. It runs from
# C++11 (below it the spine is empty and the suite has no sections); the
# helper builds it at the tree's C++ level, with D_TESTING=1.
#   The interpreter's skeleton has a leaf of its own, skeleton/, which the
# tree's build takes in here: its suite runs on DTest's C face at each C++
# level from C++11.
#   Run them with
#     ctest -R "interp_spine_tests|interp_skeleton"
#
# path:      /build/cmake/config/testing/djinterp/cpp/modules/interp/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
#                                                            revised: 2026.11.22
#*******************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/interp")

djinterp_add_test_executable(interp_spine_tests
    SOURCES
        "${_tests_dir}/spine_tests.cpp")

# the skeleton: its own leaf, the core's C sources and its C++ levels
add_subdirectory(skeleton)
