# =============================================================================
# djinterp - root CMakeLists
#
#   The build/test config tree mirrors inc/, src/, and tests/ exactly.  A
# given module's headers, sources, tests, and CMakeLists are all reachable
# at the same relative path under those four roots:
#
#     <root>/inc/djinterp/core/container/array/array.hpp
#     <root>/src/djinterp/core/container/array/...               (if any)
#     <root>/tests/djinterp/core/container/array/array_tests.hpp
#     <root>/build/cmake/config/testing/djinterp/core/container/array/CMakeLists.txt
#
#   The exe lands at the same relative path under bin/:
#     <root>/bin/<arch>-<Config>/testing/djinterp/core/container/array/array_tests.exe
#
#
#   GENERATOR / TOOLSET
#
#   Configure (from project root):
#
#     cmake -G "Visual Studio 17 2022" -A x64 -T ClangCL ^
#           -S build/cmake/config ^
#           -B build/cmake/msvs
#     cmake --build build/cmake/msvs --config Debug
#
#   In cmake-gui:
#     Source:   <root>/build/cmake/config
#     Build:    <root>/build/cmake/msvs
#     Toolset:  ClangCL  (leave blank for default MSVC)
#
#
#   AGGREGATE TARGETS
#
#   Any directory in the testing tree may be turned into an aggregate
# meta-target by giving it a CMakeLists.txt that calls
# djinterp_add_test_subtree(<name>).  The aggregate depends on every test
# exe defined in any descendant directory.  Aggregates compose freely; the
# same leaf may belong to many aggregates at different depths.  See
# djinterp_module.cmake for the full description.
# =============================================================================

cmake_minimum_required(VERSION 3.21)

project(djinterp
        LANGUAGES C CXX
        VERSION   0.1.0)


# -----------------------------------------------------------------------------
# 0. resolve the project root and well-known subtrees
# -----------------------------------------------------------------------------
#   This file lives at <root>/build/cmake/config/CMakeLists.txt, so the
# project root is three levels up.  All other roots are derived from it.
get_filename_component(DJINTERP_PROJECT_ROOT
                       "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
                       ABSOLUTE)

set(DJINTERP_PROJECT_ROOT "${DJINTERP_PROJECT_ROOT}"
    CACHE PATH "Absolute path to the djinterp project root" FORCE)

set(DJINTERP_INC_ROOT     "${DJINTERP_PROJECT_ROOT}/inc"   CACHE PATH "" FORCE)
set(DJINTERP_SRC_ROOT     "${DJINTERP_PROJECT_ROOT}/src"   CACHE PATH "" FORCE)
set(DJINTERP_TESTS_ROOT   "${DJINTERP_PROJECT_ROOT}/tests" CACHE PATH "" FORCE)
set(DJINTERP_BIN_ROOT     "${DJINTERP_PROJECT_ROOT}/bin"   CACHE PATH "" FORCE)
set(DJINTERP_CONFIG_ROOT  "${CMAKE_CURRENT_SOURCE_DIR}"    CACHE PATH "" FORCE)


# -----------------------------------------------------------------------------
# 1. wire in the helper module
# -----------------------------------------------------------------------------
#   djinterp_module.cmake lives next to this CMakeLists.  We add the current
# source dir to CMAKE_MODULE_PATH so include(djinterp_module) resolves -
# this also makes the module available to every subdirectory CMakeLists in
# case they need to call its functions explicitly.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(djinterp_module)


# -----------------------------------------------------------------------------
# 2. enable CTest
# -----------------------------------------------------------------------------
#   Each test exe registers itself as a CTest test (via add_test) inside
# djinterp_add_test_executable.  Run them all from the build dir with:
#     ctest --output-on-failure -C Debug
include(CTest)


# -----------------------------------------------------------------------------
# 3. shared C runtime - linked into every test/non-test exe
# -----------------------------------------------------------------------------
add_library(djinterp_c_runtime STATIC
    "${DJINTERP_SRC_ROOT}/djinterp/core/env/env.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/djinterp.c"
)

target_include_directories(djinterp_c_runtime PUBLIC
    "${DJINTERP_INC_ROOT}"
)

target_compile_features(djinterp_c_runtime PRIVATE c_std_17)

set_target_properties(djinterp_c_runtime PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    FOLDER                    "djinterp/runtime"
)

if (MSVC)
    target_compile_definitions(djinterp_c_runtime PRIVATE _UNICODE UNICODE)
    target_compile_options(djinterp_c_runtime PRIVATE
        /W3 /sdl /permissive- /Zc:preprocessor)
endif()


# -----------------------------------------------------------------------------
# 4. recurse into the testing tree
# -----------------------------------------------------------------------------
#   The testing tree mirrors tests/ exactly: testing/djinterp/<rel>/...
# Each directory either calls djinterp_add_test_executable() (leaf) or
# djinterp_add_test_subtree() (aggregate).  Recursion is handled by the
# subtree helper, so this root only needs to enter the top of the tree.
add_subdirectory(testing)