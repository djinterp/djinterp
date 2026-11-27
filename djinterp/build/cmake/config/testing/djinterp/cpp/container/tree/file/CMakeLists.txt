# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/container/tree/file/CMakeLists.txt
#
#    Test leaf for container/tree/file/ (header-only), the tree floor's file
# half: the runner in this directory, the section translation units globbed
# from the tests tree; the runtime has memory/pool.c, which the nodes' pool
# calls. Where a MinGW-w64 cross-compiler is installed
# (and the host is not Windows), it also registers two compile-only checks,
# at C++17 and C++20, of file_tree_windows_probe.cpp, which compiles the
# Windows scanner's and populator's native code.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/tree/file")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/file_tree_tests_*.cpp")

djinterp_add_test_executable(file_tree_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_tree_tests_runner.cpp"
        ${_sections}
    INCLUDES
        "${_tests_dir}")

# the Windows scanner's native code, compiled by a cross-compiler
find_program(DJINTERP_MINGW_CXX
    NAMES x86_64-w64-mingw32-g++-posix x86_64-w64-mingw32-g++)
if (BUILD_TESTING AND DJINTERP_MINGW_CXX AND NOT WIN32)
    foreach(_std 17 20)
        add_test(NAME    file_tree_windows_mingw_cxx${_std}
                 COMMAND "${DJINTERP_MINGW_CXX}" -std=c++${_std}
                         -fsyntax-only -Wall -Wextra -Werror
                         "-I${DJINTERP_INC_ROOT}"
                         "${_tests_dir}/file_tree_windows_probe.cpp")
        set_tests_properties(file_tree_windows_mingw_cxx${_std}
                             PROPERTIES LABELS container)
    endforeach()
endif()
