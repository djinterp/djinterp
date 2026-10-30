################################################################################
# djinterp [test]                                                  CMakeLists.txt
#
#   Test leaf for the portable compression facade (djinterp/core/util/compress).
# Builds the `compress_tests` executable from the runner (this dir), the section
# translation units (tests/), and the production compress.cpp.
#
#   Recipe R2 (production sources compiled straight into the exe): compress.cpp
# is consumed only by this suite, so there is no reason to stand up a shared
# static library for it.
#
# path: /build/cmake/config/testing/djinterp/core/util/CMakeLists.txt
################################################################################

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/cpp/util")

# section TUs (one per semantic region); exclude any legacy *_main.cpp so a
# stray second main() cannot collide with the runner at link time
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/compress_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# production implementation under test
file(GLOB _prod CONFIGURE_DEPENDS "${_src_dir}/compress.cpp")

# ------------------------------------------------------------------------------
# Compression backend libraries.
#   compress.cpp includes each backend's header only when it is present and
# calls into it only then.  On MSVC the matching import libraries are pulled in
# automatically by env_compress_link.h (#pragma comment(lib, ...)); on every
# other toolchain the build must supply them.  We link whatever is installed -
# in a sane environment the set of detected headers matches the set of
# available libraries, and any codec whose header is absent is compiled out and
# needs no library.
# ------------------------------------------------------------------------------
set(_backend_libs "")

if(NOT MSVC)
    find_library(DJINTERP_COMPRESS_ZLIB_LIB         z)
    find_library(DJINTERP_COMPRESS_BZIP2_LIB        bz2)
    find_library(DJINTERP_COMPRESS_LZMA_LIB         lzma)
    find_library(DJINTERP_COMPRESS_ZSTD_LIB         zstd)
    find_library(DJINTERP_COMPRESS_LZ4_LIB          lz4)
    find_library(DJINTERP_COMPRESS_BROTLIENC_LIB    brotlienc)
    find_library(DJINTERP_COMPRESS_BROTLIDEC_LIB    brotlidec)
    find_library(DJINTERP_COMPRESS_BROTLICOMMON_LIB brotlicommon)

    # order matters for static linking: the brotli front-ends depend on
    # brotlicommon, so it is listed after them
    foreach(_cache_var
            DJINTERP_COMPRESS_ZLIB_LIB
            DJINTERP_COMPRESS_BZIP2_LIB
            DJINTERP_COMPRESS_LZMA_LIB
            DJINTERP_COMPRESS_ZSTD_LIB
            DJINTERP_COMPRESS_LZ4_LIB
            DJINTERP_COMPRESS_BROTLIENC_LIB
            DJINTERP_COMPRESS_BROTLIDEC_LIB
            DJINTERP_COMPRESS_BROTLICOMMON_LIB)
        if(${_cache_var})
            list(APPEND _backend_libs "${${_cache_var}}")
        endif()
    endforeach()
endif()

# ------------------------------------------------------------------------------
# The test executable.
#   INCLUDES carries the three bare-include roots this suite reaches for:
#   - inc/djinterp/cpp/util   -> "compress.hpp"        (the facade under test)
#   - inc/djinterp/test        -> "test_compress.hpp"   (shared testing module)
#   - tests/djinterp/cpp/util -> "compress_tests.hpp"  (this suite's header)
# The helper additionally puts ${DJINTERP_INC_ROOT} on the path and links
# djinterp_c_runtime, defines D_TESTING=1, and registers the CTest entry.
# ------------------------------------------------------------------------------
djinterp_add_test_executable(compress_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/compress_tests_runner.cpp"
        ${_sources}
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test"
        "${_tests_dir}"
    LIBRARIES
        ${_backend_libs})
