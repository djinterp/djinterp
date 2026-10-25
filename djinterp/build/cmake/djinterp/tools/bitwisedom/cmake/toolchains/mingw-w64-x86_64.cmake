#*******************************************************************************
# bitwisedom [cmake]                                      mingw-w64-x86_64.cmake
#
# Cross-compiling for 64-bit Windows from Linux with MinGW-w64 (the posix
# thread model, for std::thread and the rest of the library). The window
# gets the Win32 + Direct3D 11 host; nothing else needs installing.
#
#     cmake -S . -B build/mingw -G Ninja \
#           -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
#           -DDJINTERP_ROOT=... -DUXOXO_ROOT=...
#
# path:      /build/cmake/djinterp/tools/bitwisedom/cmake/toolchains/mingw-w64-x86_64.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.04
#*******************************************************************************
set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config too looks in the sysroot only, or it hands back the build
# machine's libraries (its GLFW, say) for a Windows target
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH}   "")

# one self-contained .exe: no MinGW runtime DLLs to ship beside it
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")
