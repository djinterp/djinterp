# =============================================================================
# djinterp - build/cmake/djinterp/runtime.cmake
#
#   djinterp_c_runtime: the shared C runtime, linked into every test and
# program. Needs libc only, and libm where the platform keeps it apart.
#   Besides env, the root and the stream it always held, it holds the plain
# and event kinds of the counter and the timer, dtime, string_fn and the
# memory functions string_fn calls (ruled 2026.11.17). Their atomic and mutex
# kinds are djinterp_c_sync's (sync.cmake).
# =============================================================================

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
    # the language root's time and string functions, and the memory
    # functions the string functions call (d_memcpy)
    "${DJINTERP_SRC_ROOT}/djinterp/c/dtime.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    # the counter and the timer, plain and event kinds: libc only
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_event.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_event.c"
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

# libm, where the platform keeps it apart from libc, for every unit that
# links the runtime
if (NOT MSVC)
    find_library(DJINTERP_LIBM m)
    if (DJINTERP_LIBM)
        target_link_libraries(djinterp_c_runtime PUBLIC "${DJINTERP_LIBM}")
    endif()
endif()
