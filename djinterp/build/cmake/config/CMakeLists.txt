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

# the version, from inc/djinterp/c/dversion.h, the one place that states it
# (three levels up is the project root)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../../inc/djinterp/c/dversion.h"
     _djinterp_version_lines
     REGEX "^#define D_VERSION_(MAJOR|MINOR|PATCH) +[0-9]+")
foreach(_line IN LISTS _djinterp_version_lines)
    if(_line MATCHES "^#define D_VERSION_(MAJOR|MINOR|PATCH) +([0-9]+)")
        set(_djinterp_version_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()
if( (NOT DEFINED _djinterp_version_MAJOR) OR
    (NOT DEFINED _djinterp_version_MINOR) OR
    (NOT DEFINED _djinterp_version_PATCH) )
    message(FATAL_ERROR
            "djinterp: inc/djinterp/c/dversion.h states no version")
endif()
set(_djinterp_version
    "${_djinterp_version_MAJOR}.${_djinterp_version_MINOR}")
set(_djinterp_version "${_djinterp_version}.${_djinterp_version_PATCH}")

project(djinterp
        LANGUAGES C CXX
        VERSION   ${_djinterp_version})
message(STATUS "djinterp ${PROJECT_VERSION}")

# c/filesys builds on POSIX where the target is POSIX, on Win32 where it is
# Windows, and otherwise on ISO C alone, where a call that needs the system
# fails with ENOSYS. Its headers choose by env when they are compiled; this
# says the same when the build is configured (decision 4 of the filesystem
# register, ruled 2026.11.04).
if( (NOT WIN32) AND
    ( (NOT UNIX) OR (CMAKE_SYSTEM_NAME STREQUAL "WASI") ) )
    message(STATUS "djinterp: ${CMAKE_SYSTEM_NAME} is neither POSIX nor "
                   "Windows: c/filesys builds on its ISO C backend, where a "
                   "call that needs the system fails with ENOSYS")
endif()


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
    # env moved from core/env/ to the top of the tree; its archive and
    # compress units are the util/ ones, whose headers the tree includes
    "${DJINTERP_SRC_ROOT}/djinterp/env/env.c"
    "${DJINTERP_SRC_ROOT}/djinterp/env/util/archive/env_archive.c"
    "${DJINTERP_SRC_ROOT}/djinterp/env/util/compress/env_compress.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/djinterp.c"
    # the compression options every DTest runner reaches, through the report
    # writer's archive (test_defaults.hpp): d_compress_options_init and its
    # kin, which the thread suites and others failed to link without
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/compress/compress_common.c"
    # and the sinks compress_common.c writes through
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
    # the output stream and its formatting half, which header-only C++ code
    # calls (pdf_builtin_backend.hpp's pdf_num, writer.hpp's sinks), so every
    # runner on DTest's report path needs them at link time
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/ostream/ostream.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/text/ostream_format.c"
    # the archive kernel the report runner packs through (d_archive_create_to_sink,
    # d_archive_extract and their kin), which three runners failed to link without
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/archive/archive_common.c"
    # the default memory source printf's heap fallback in ostream_format.c
    # draws from (d_mem_source_default), and the common layer it calls into;
    # both need only libc
    "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_source.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_common.c"
)

# ---------------------------------------------------------------------------
# Compression backends.  compress_common.c (in the runtime) and the
# compress/archive TUs call every codec whose header env_compress.h detects
# (zlib, bzip2, xz, zstd, lz4, brotli), so a header found without its library
# would fail at link time.  Each library CMake finds is linked; for each one
# it does not, the codec's knob in cfg_compress.h is set off, which that
# header supports ("turning one OFF while it is installed").  Both are PUBLIC,
# so every unit that links djinterp_c_runtime sees the same choice.
find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(djinterp_c_runtime PUBLIC ZLIB::ZLIB)
else()
    target_compile_definitions(djinterp_c_runtime PUBLIC
        D_CFG_COMPRESS_CODEC_DEFLATE=0)
endif()
foreach(_codec bz2:BZIP2 lzma:LZMA zstd:ZSTD lz4:LZ4)
    string(REPLACE ":" ";" _codec "${_codec}")
    list(GET _codec 0 _lib)
    list(GET _codec 1 _knob)
    find_library(DJINTERP_CODEC_${_lib} ${_lib})
    if(DJINTERP_CODEC_${_lib})
        target_link_libraries(djinterp_c_runtime PUBLIC
            ${DJINTERP_CODEC_${_lib}})
    else()
        target_compile_definitions(djinterp_c_runtime PUBLIC
            D_CFG_COMPRESS_CODEC_${_knob}=0)
    endif()
endforeach()
# brotli is three libraries: all three, or the codec is off
set(_brotli_libs "")
foreach(_lib brotlienc brotlidec brotlicommon)
    find_library(DJINTERP_CODEC_${_lib} ${_lib})
    if(DJINTERP_CODEC_${_lib})
        list(APPEND _brotli_libs ${DJINTERP_CODEC_${_lib}})
    endif()
endforeach()
list(LENGTH _brotli_libs _brotli_count)
if(_brotli_count EQUAL 3)
    target_link_libraries(djinterp_c_runtime PUBLIC ${_brotli_libs})
else()
    target_compile_definitions(djinterp_c_runtime PUBLIC
        D_CFG_COMPRESS_CODEC_BROTLI=0)
endif()


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


# -----------------------------------------------------------------------------
# 5. the benchmarks (off by default)
# -----------------------------------------------------------------------------
#   tests/bench holds timing programs, not tests: they report figures and
# never fail.  -DDJINTERP_BUILD_BENCHMARKS=ON builds them, best with
# -DCMAKE_BUILD_TYPE=Release, since their figures are meant at -O2.
option(DJINTERP_BUILD_BENCHMARKS "build the benchmarks under tests/bench" OFF)
if(DJINTERP_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()