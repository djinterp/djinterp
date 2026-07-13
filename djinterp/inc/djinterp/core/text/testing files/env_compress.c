/******************************************************************************
* djinterp [utility]                                            env_compress.c
*
*   Runtime implementations for the query declarations in env_compress.h.
* These are plain C functions (extern "C" when compiled as C++) and are gated
* on the same D_ENV_COMPRESSION_HAVE_* flags as the rest of the layer, so this
* unit links cleanly regardless of which codec libraries are present.
*
* 
* path:      /src/djinterp/core/env/env_compress.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/
#include "../../../../inc/djinterp/core/env/env_compress.h"
#include "../../../../inc/djinterp/core/env/env_compress_link.h"


#if D_ENV_COMPRESSION_HAVE_ZLIB
    #include <zlib.h>
#endif


// d_env_compression_codec_name
//   function: human-readable name for a codec identifier.
const char*
d_env_compression_codec_name(int codec)
{
    switch (codec)
    {
        case D_ENV_COMPRESSION_CODEC_ZLIB:       return "zlib";
        case D_ENV_COMPRESSION_CODEC_ZLIBNG:     return "zlib-ng";
        case D_ENV_COMPRESSION_CODEC_MINIZ:      return "miniz";
        case D_ENV_COMPRESSION_CODEC_BZIP2:      return "bzip2";
        case D_ENV_COMPRESSION_CODEC_LZMA:       return "xz/lzma";
        case D_ENV_COMPRESSION_CODEC_ZSTD:       return "zstd";
        case D_ENV_COMPRESSION_CODEC_LZ4:        return "lz4";
        case D_ENV_COMPRESSION_CODEC_BROTLI:     return "brotli";
        case D_ENV_COMPRESSION_CODEC_LIBDEFLATE: return "libdeflate";
        case D_ENV_COMPRESSION_CODEC_NONE:       return "none";
        default:                                 return "none";
    }
}

// d_env_compression_zlib_runtime_version
//   function: zlib runtime version string, or "unavailable" when not linked.
const char*
d_env_compression_zlib_runtime_version(void)
{
#if D_ENV_COMPRESSION_HAVE_ZLIB
    return zlibVersion();
#else
    return "unavailable";
#endif
}

// d_env_compression_print_info
//   function: print detected codec libraries and versions to stdout.
void
d_env_compression_print_info(void)
{
    printf("djinterp compression codecs:\n");

    printf("  zlib       : %s",
           D_ENV_COMPRESSION_HAVE_ZLIB ? "yes" : "no");
#if D_ENV_COMPRESSION_HAVE_ZLIB
    printf(" (build " D_ENV_COMPRESSION_ZLIB_VERSION_STR ", runtime %s)",
           d_env_compression_zlib_runtime_version());
#endif
    printf("\n");

    printf("  zlib-ng    : %s\n",
           D_ENV_COMPRESSION_HAVE_ZLIBNG ? "yes" : "no");
    printf("  miniz      : %s\n",
           D_ENV_COMPRESSION_HAVE_MINIZ ? "yes" : "no");
    printf("  libdeflate : %s\n",
           D_ENV_COMPRESSION_HAVE_LIBDEFLATE ? "yes" : "no");
    printf("  bzip2      : %s\n",
           D_ENV_COMPRESSION_HAVE_BZIP2 ? "yes" : "no");
    printf("  xz/lzma    : %s\n",
           D_ENV_COMPRESSION_HAVE_LZMA ? "yes" : "no");
    printf("  zstd       : %s\n",
           D_ENV_COMPRESSION_HAVE_ZSTD ? "yes" : "no");
    printf("  lz4        : %s\n",
           D_ENV_COMPRESSION_HAVE_LZ4 ? "yes" : "no");
    printf("  brotli     : %s\n",
           D_ENV_COMPRESSION_HAVE_BROTLI ? "yes" : "no");

    printf("  capabilities: deflate=%d gzip=%d gzip_wrap=%d any=%d\n",
           D_ENV_COMPRESSION_HAVE_DEFLATE,
           D_ENV_COMPRESSION_HAVE_GZIP,
           D_ENV_COMPRESSION_HAVE_GZIP_WRAP,
           D_ENV_COMPRESSION_HAVE_ANY);

    return;
}
