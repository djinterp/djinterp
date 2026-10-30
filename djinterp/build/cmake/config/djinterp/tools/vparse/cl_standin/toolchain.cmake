#*******************************************************************************
# vparse [cl_standin]                                            toolchain.cmake
#
# A toolchain file that puts the stand-in for cl (the script cl, beside this
# file) where cl would be, so that this project can be configured, built and
# tested as CMake does it for MSVC on a machine that has none.
#   CMake compiles its identification source with the stand-in, finds cl's
# macros in it, and reports MSVC 19.51: from there on every list file takes
# the branches it takes under cl. The units are compiled by a MinGW-w64 Clang
# under cl's identity, linked against the MinGW-w64 runtime, and run through
# Wine, which ctest is told to use.
#   Needs clang with lld, a MinGW-w64 GCC for 64-bit Windows (its headers and
# libraries), and Wine. See README.md, beside this file, for what a run shows
# and what it cannot. From the folder above this one, vparse, and then the
# window's build, which takes uxoxo:
#
#     cmake -S . -B out/standin -G Ninja \
#           -DCMAKE_TOOLCHAIN_FILE=cl_standin/toolchain.cmake
#     cmake -S ../../../../ui/vparse -B out/standin-ui -G Ninja \
#           -DCMAKE_TOOLCHAIN_FILE=$PWD/cl_standin/toolchain.cmake \
#           -DUXOXO_ROOT=...
#
# path:      /build/cmake/config/djinterp/tools/vparse/cl_standin/toolchain.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
#                                                            revised: 2026.10.27
#*******************************************************************************
set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(CMAKE_C_COMPILER   "${CMAKE_CURRENT_LIST_DIR}/cl")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/cl")
set(CMAKE_LINKER       "${CMAKE_CURRENT_LIST_DIR}/link")

# CMake looks for both under MSVC; this build has work for neither
set(CMAKE_RC_COMPILER "${CMAKE_CURRENT_LIST_DIR}/rc")
set(CMAKE_MT          "${CMAKE_CURRENT_LIST_DIR}/rc")

find_program(CMAKE_AR NAMES x86_64-w64-mingw32-ar llvm-ar REQUIRED)

# What a list file finds is the target's, in the MinGW-w64 sysroot the units
# are compiled against, and never the machine's own: a Linux header is
# nothing a Windows unit can read. uxoxo's build looks for GLFW, for one, and
# would otherwise build its GLFW backend against the GLFW this machine has.
set(CMAKE_FIND_ROOT_PATH              /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config, which uxoxo falls back to, the same way
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH}   "")

# the rules cl's platform files give for a link and an archive are replaced
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/override.cmake")

# CMake's checks link, through the script link, as cl's own do: a check that
# asks whether something links is answered as cl's linker would answer it --
# the argument parser's leaf asks whether the linker wraps malloc, and with
# no --wrap leaves its sweep out, as it does under cl

# what runs a test: the executables are Windows ones
find_program(CL_STANDIN_WINE NAMES wine64 wine
             PATHS /usr/lib/wine /usr/local/bin)

if(CL_STANDIN_WINE)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${CL_STANDIN_WINE}")
endif()
