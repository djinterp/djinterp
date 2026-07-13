/******************************************************************************
* djinterp [utility]                                              compress.cpp
*
*   Implementation of the djinterp portable compression facade. This unit
* holds every backend that compress.hpp forwards to. Each backend is gated
* on the matching D_ENV_COMPRESSION_HAVE_* flag from env_compress.h, and the
* third-party headers are included here (not in the public header), so merely
* including compress.hpp never pulls in a dependency.
*
* The store, deflate, zlib, and gzip codecs are implemented on top of zlib so
* that the formats the archive layer leans on work anywhere zlib is present.
* Codecs without a detected backend return status_unavailable at runtime.
*
* 
* path:      /src/djinterp/core/util/compress.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/
#include "../../../../inc/djinterp/core/util/compress.hpp"
#include "../../../../inc/djinterp/core/env/env_compress_link.h"


// -- backend headers (gated on detection) -------------------------------------
#if D_ENV_COMPRESSION_HAVE_ZLIB
    #include <zlib.h>
#endif

#if D_ENV_COMPRESSION_HAVE_BZIP2
    #include <bzlib.h>
#endif

#if D_ENV_COMPRESSION_HAVE_LZMA
    #include <lzma.h>
#endif

#if D_ENV_COMPRESSION_HAVE_ZSTD
    #include <zstd.h>
#endif

#if D_ENV_COMPRESSION_HAVE_LZ4
    #include <lz4.h>
    #if D_ENV_COMPRESSION_HAVE_LZ4_FRAME
        #include <lz4frame.h>
    #endif
#endif

#if D_ENV_COMPRESSION_HAVE_BROTLI
    #include <brotli/encode.h>
    #include <brotli/decode.h>
#endif


NS_DJINTERP

// =============================================================================
// I.   STATUS MESSAGES
// =============================================================================

const char*
status_message(status _s)
{
    switch (_s)
    {
        case status_ok:               return "ok";
        case status_unavailable:      return "codec/format unavailable in "
                                             "this build";
        case status_invalid_argument: return "invalid argument";
        case status_buffer_error:     return "buffer allocation or size error";
        case status_backend_error:    return "backend library error";
        case status_unsupported:      return "operation not supported";
        default:                      return "unknown status";
    }
}


NS_INTERNAL

// =============================================================================
// II.  STORE (built-in)
// =============================================================================

// store_copy
//   function: verbatim copy used by the `store` codec for both directions.
static status
store_copy(const char* _in, std::size_t _n, byte_buffer& _out)
{
    if ((_in == 0) && (_n != 0))
    {
        return status_invalid_argument;
    }

    // a guard against asking std::string to hold more than it can address
    if (_n > _out.max_size())
    {
        return status_buffer_error;
    }

    _out.assign(_in, _n);

    return status_ok;
}


// =============================================================================
// III. ZLIB-FAMILY (deflate / zlib / gzip)
// =============================================================================

#if D_ENV_COMPRESSION_HAVE_ZLIB

// zlib_window_bits
//   function: maps a codec id to the windowBits value that selects the raw
// DEFLATE, zlib, or gzip framing in zlib's deflateInit2 / inflateInit2.
static int
zlib_window_bits(codec_id _id)
{
    // 15 is the maximum window; negative => raw, +16 => gzip wrapper
    switch (_id)
    {
        case codec_id_deflate: return -15;  // raw, no header/trailer
        case codec_id_zlib:    return  15;  // zlib wrapper (RFC 1950)
        case codec_id_gzip:    return  31;  // gzip wrapper (15 | 16)
        default:               return  15;
    }
}

// zlib_compress
//   function: one-shot streaming compression for the zlib family.
static status
zlib_compress(codec_id                _id,
              const char*              _in,
              std::size_t              _n,
              const compress_options&  _opt,
              byte_buffer&             _out)
{
    z_stream strm;
    int      level;
    int      rc;

    level = (_opt.level < 0) ? Z_DEFAULT_COMPRESSION : _opt.level;

    // zero-initialize the stream control block
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    rc = deflateInit2(&strm,
                      level,
                      Z_DEFLATED,
                      zlib_window_bits(_id),
                      8,                 // default memLevel
                      Z_DEFAULT_STRATEGY);

    if (rc != Z_OK)
    {
        return status_backend_error;
    }

    // size the output to the worst case up front, then trim
    _out.resize(deflateBound(&strm, (uLong)_n));

    strm.next_in   = (Bytef*)_in;
    strm.avail_in  = (uInt)_n;
    strm.next_out  = (Bytef*)(&_out[0]);
    strm.avail_out = (uInt)_out.size();

    rc = deflate(&strm, Z_FINISH);

    if (rc != Z_STREAM_END)
    {
        deflateEnd(&strm);
        return status_backend_error;
    }

    _out.resize(strm.total_out);
    deflateEnd(&strm);

    return status_ok;
}

// zlib_decompress
//   function: one-shot streaming decompression for the zlib family, growing
// the output buffer as needed.
static status
zlib_decompress(codec_id      _id,
                const char*    _in,
                std::size_t    _n,
                byte_buffer&   _out)
{
    z_stream strm;
    int      rc;
    int      wbits;

    wbits = zlib_window_bits(_id);

    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.next_in  = Z_NULL;
    strm.avail_in = 0;

    rc = inflateInit2(&strm, wbits);

    if (rc != Z_OK)
    {
        return status_backend_error;
    }

    strm.next_in  = (Bytef*)_in;
    strm.avail_in = (uInt)_n;

    _out.clear();

    // inflate in chunks until the stream ends
    {
        const std::size_t CHUNK = 64u * 1024u;
        char              buf[64u * 1024u];

        do
        {
            strm.next_out  = (Bytef*)buf;
            strm.avail_out = (uInt)CHUNK;

            rc = inflate(&strm, Z_NO_FLUSH);

            if ((rc != Z_OK)         &&
                (rc != Z_STREAM_END) &&
                (rc != Z_BUF_ERROR))
            {
                inflateEnd(&strm);
                return status_backend_error;
            }

            _out.append(buf, CHUNK - strm.avail_out);

            // Z_BUF_ERROR with no progress and no input left means truncation
            if ((rc == Z_BUF_ERROR) && (strm.avail_in == 0))
            {
                break;
            }
        }
        while (rc != Z_STREAM_END);
    }

    inflateEnd(&strm);

    return (rc == Z_STREAM_END) ? status_ok : status_backend_error;
}

#endif  // D_ENV_COMPRESSION_HAVE_ZLIB


// =============================================================================
// IV.  BZIP2
// =============================================================================

#if D_ENV_COMPRESSION_HAVE_BZIP2

// bzip2_compress
//   function: one-shot bzip2 compression via the buffer-to-buffer API.
static status
bzip2_compress(const char*              _in,
               std::size_t              _n,
               const compress_options&  _opt,
               byte_buffer&             _out)
{
    unsigned int dest_len;
    int          block_size;
    int          rc;

    // bzip2's level is the 100k block size 1..9; default to 9
    block_size = (_opt.level < 1 || _opt.level > 9) ? 9 : _opt.level;

    // worst-case bound recommended by the bzip2 manual: n + 1% + 600 bytes
    dest_len = (unsigned int)(_n + (_n / 100u) + 600u);
    _out.resize(dest_len);

    rc = BZ2_bzBuffToBuffCompress(&_out[0],
                                  &dest_len,
                                  (char*)_in,
                                  (unsigned int)_n,
                                  block_size,
                                  0,    // verbosity
                                  0);   // workFactor (default)

    if (rc != BZ_OK)
    {
        return status_backend_error;
    }

    _out.resize(dest_len);

    return status_ok;
}

// bzip2_decompress
//   function: one-shot bzip2 decompression, growing the guess on overflow.
static status
bzip2_decompress(const char* _in, std::size_t _n, byte_buffer& _out)
{
    unsigned int dest_len;
    int          rc;
    int          attempt;

    // start at 4x and grow; bzip2 has no streaming size hint here
    dest_len = (unsigned int)((_n < 1024u) ? 4096u : (_n * 4u));

    for (attempt = 0; attempt < 8; ++attempt)
    {
        _out.resize(dest_len);

        rc = BZ2_bzBuffToBuffDecompress(&_out[0],
                                        &dest_len,
                                        (char*)_in,
                                        (unsigned int)_n,
                                        0,    // small
                                        0);   // verbosity

        if (rc == BZ_OK)
        {
            _out.resize(dest_len);
            return status_ok;
        }

        if (rc != BZ_OUTBUFF_FULL)
        {
            return status_backend_error;
        }

        // grow and retry
        dest_len *= 2u;
    }

    return status_buffer_error;
}

#endif  // D_ENV_COMPRESSION_HAVE_BZIP2


// =============================================================================
// V.   BROTLI
// =============================================================================

#if D_ENV_COMPRESSION_HAVE_BROTLI

// brotli_compress
//   function: one-shot Brotli compression.
static status
brotli_compress(const char*              _in,
                std::size_t              _n,
                const compress_options&  _opt,
                byte_buffer&             _out)
{
    size_t   out_len;
    int      quality;
    BROTLI_BOOL ok;

    quality = (_opt.level < 0 || _opt.level > 11) ? BROTLI_DEFAULT_QUALITY
                                                  : _opt.level;

    out_len = BrotliEncoderMaxCompressedSize(_n);
    if (out_len == 0)
    {
        out_len = _n + 1024u;   // fallback bound for tiny inputs
    }
    _out.resize(out_len);

    ok = BrotliEncoderCompress(quality,
                               BROTLI_DEFAULT_WINDOW,
                               BROTLI_MODE_GENERIC,
                               _n,
                               (const uint8_t*)_in,
                               &out_len,
                               (uint8_t*)(&_out[0]));

    if (!ok)
    {
        return status_backend_error;
    }

    _out.resize(out_len);

    return status_ok;
}

// brotli_decompress
//   function: one-shot Brotli decompression, growing the buffer on need.
static status
brotli_decompress(const char* _in, std::size_t _n, byte_buffer& _out)
{
    BrotliDecoderResult res;
    std::size_t         cap;
    int                 attempt;

    cap = (_n < 1024u) ? 4096u : (_n * 4u);

    for (attempt = 0; attempt < 16; ++attempt)
    {
        size_t out_len = cap;

        _out.resize(cap);

        res = BrotliDecoderDecompress(_n,
                                      (const uint8_t*)_in,
                                      &out_len,
                                      (uint8_t*)(&_out[0]));

        if (res == BROTLI_DECODER_RESULT_SUCCESS)
        {
            _out.resize(out_len);
            return status_ok;
        }

        // the one-shot API cannot signal "need more output" distinctly, so
        // grow the buffer and retry until a ceiling is hit
        cap *= 2u;
    }

    return status_backend_error;
}

#endif  // D_ENV_COMPRESSION_HAVE_BROTLI


// =============================================================================
// VI.  XZ / ZSTD / LZ4 (gated; route to backend or report unavailable)
// =============================================================================

#if D_ENV_COMPRESSION_HAVE_LZMA

static status
xz_compress(const char*              _in,
            std::size_t              _n,
            const compress_options&  _opt,
            byte_buffer&             _out)
{
    uint32_t preset;
    size_t   out_pos;
    size_t   bound;
    lzma_ret rc;

    preset  = (_opt.level < 0 || _opt.level > 9) ? 6u : (uint32_t)_opt.level;
    bound   = lzma_stream_buffer_bound(_n);
    out_pos = 0;

    _out.resize(bound);

    rc = lzma_easy_buffer_encode(preset,
                                 LZMA_CHECK_CRC64,
                                 NULL,
                                 (const uint8_t*)_in,
                                 _n,
                                 (uint8_t*)(&_out[0]),
                                 &out_pos,
                                 bound);

    if (rc != LZMA_OK)
    {
        return status_backend_error;
    }

    _out.resize(out_pos);

    return status_ok;
}

static status
xz_decompress(const char* _in, std::size_t _n, byte_buffer& _out)
{
    uint64_t    memlimit = UINT64_MAX;
    size_t      in_pos   = 0;
    size_t      out_pos  = 0;
    std::size_t cap;
    lzma_ret    rc;
    int         attempt;

    cap = (_n < 1024u) ? 4096u : (_n * 4u);

    for (attempt = 0; attempt < 16; ++attempt)
    {
        in_pos  = 0;
        out_pos = 0;
        _out.resize(cap);

        rc = lzma_stream_buffer_decode(&memlimit,
                                       0,
                                       NULL,
                                       (const uint8_t*)_in,
                                       &in_pos,
                                       _n,
                                       (uint8_t*)(&_out[0]),
                                       &out_pos,
                                       cap);

        if (rc == LZMA_OK)
        {
            _out.resize(out_pos);
            return status_ok;
        }

        if (rc != LZMA_BUF_ERROR)
        {
            return status_backend_error;
        }

        cap *= 2u;
    }

    return status_buffer_error;
}

#endif  // D_ENV_COMPRESSION_HAVE_LZMA


#if D_ENV_COMPRESSION_HAVE_ZSTD

static status
zstd_compress(const char*              _in,
              std::size_t              _n,
              const compress_options&  _opt,
              byte_buffer&             _out)
{
    size_t bound;
    size_t written;
    int    level;

    level   = (_opt.level < 0) ? ZSTD_CLEVEL_DEFAULT : _opt.level;
    bound   = ZSTD_compressBound(_n);

    _out.resize(bound);

    written = ZSTD_compress(&_out[0], bound, _in, _n, level);

    if (ZSTD_isError(written))
    {
        return status_backend_error;
    }

    _out.resize(written);

    return status_ok;
}

static status
zstd_decompress(const char* _in, std::size_t _n, byte_buffer& _out)
{
    unsigned long long raw;
    size_t             got;

    raw = ZSTD_getFrameContentSize(_in, _n);

    if (raw == ZSTD_CONTENTSIZE_ERROR)
    {
        return status_backend_error;
    }
    if (raw == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        // streaming would be required; not handled by this one-shot path
        return status_unsupported;
    }

    _out.resize((std::size_t)raw);

    got = ZSTD_decompress(&_out[0], (size_t)raw, _in, _n);

    if (ZSTD_isError(got) || (got != raw))
    {
        return status_backend_error;
    }

    return status_ok;
}

#endif  // D_ENV_COMPRESSION_HAVE_ZSTD


#if ( D_ENV_COMPRESSION_HAVE_LZ4 && D_ENV_COMPRESSION_HAVE_LZ4_FRAME )

static status
lz4_compress(const char*              _in,
             std::size_t              _n,
             const compress_options&  _opt,
             byte_buffer&             _out)
{
    size_t bound;
    size_t written;

    (void)_opt;   // frame defaults; level wiring omitted for brevity

    bound = LZ4F_compressFrameBound(_n, NULL);
    _out.resize(bound);

    written = LZ4F_compressFrame(&_out[0], bound, _in, _n, NULL);

    if (LZ4F_isError(written))
    {
        return status_backend_error;
    }

    _out.resize(written);

    return status_ok;
}

static status
lz4_decompress(const char* _in, std::size_t _n, byte_buffer& _out)
{
    LZ4F_dctx*          dctx = NULL;
    LZ4F_errorCode_t    err;
    std::size_t         cap;
    status              result;

    err = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(err))
    {
        return status_backend_error;
    }

    cap = (_n < 1024u) ? 4096u : (_n * 4u);
    _out.clear();
    result = status_ok;

    {
        const char* src     = _in;
        size_t      src_rem = _n;
        char        buf[64u * 1024u];

        while (src_rem > 0)
        {
            size_t dst_size = sizeof(buf);
            size_t src_size = src_rem;
            size_t hint;

            hint = LZ4F_decompress(dctx, buf, &dst_size,
                                   src, &src_size, NULL);

            if (LZ4F_isError(hint))
            {
                result = status_backend_error;
                break;
            }

            _out.append(buf, dst_size);
            src     += src_size;
            src_rem -= src_size;

            if (hint == 0)
            {
                break;   // frame complete
            }
        }
    }

    (void)cap;
    LZ4F_freeDecompressionContext(dctx);

    return result;
}

#endif  // LZ4 + LZ4_FRAME


// =============================================================================
// VII. DISPATCH LEAVES
// =============================================================================

bool
codec_available(codec_id _id)
{
    switch (_id)
    {
        case codec_id_store:
            return true;
        case codec_id_deflate:
        case codec_id_zlib:
        case codec_id_gzip:
            return (D_ENV_COMPRESSION_HAVE_ZLIB != 0);
        case codec_id_bzip2:
            return (D_ENV_COMPRESSION_HAVE_BZIP2 != 0);
        case codec_id_xz:
            return (D_ENV_COMPRESSION_HAVE_LZMA != 0);
        case codec_id_zstd:
            return (D_ENV_COMPRESSION_HAVE_ZSTD != 0);
        case codec_id_lz4:
            return (D_ENV_COMPRESSION_HAVE_LZ4 != 0);
        case codec_id_brotli:
            return (D_ENV_COMPRESSION_HAVE_BROTLI != 0);
        default:
            return false;
    }
}

status
compress_buffer(codec_id                _id,
                const char*              _in,
                std::size_t              _n,
                const compress_options&  _opt,
                byte_buffer&             _out)
{
    // validate the pointer/size relationship before touching a backend
    if ((_in == 0) && (_n != 0))
    {
        return status_invalid_argument;
    }

    switch (_id)
    {
        case codec_id_store:
            return store_copy(_in, _n, _out);

#if D_ENV_COMPRESSION_HAVE_ZLIB
        case codec_id_deflate:
        case codec_id_zlib:
        case codec_id_gzip:
            return zlib_compress(_id, _in, _n, _opt, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_BZIP2
        case codec_id_bzip2:
            return bzip2_compress(_in, _n, _opt, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_LZMA
        case codec_id_xz:
            return xz_compress(_in, _n, _opt, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_ZSTD
        case codec_id_zstd:
            return zstd_compress(_in, _n, _opt, _out);
#endif

#if ( D_ENV_COMPRESSION_HAVE_LZ4 && D_ENV_COMPRESSION_HAVE_LZ4_FRAME )
        case codec_id_lz4:
            return lz4_compress(_in, _n, _opt, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_BROTLI
        case codec_id_brotli:
            return brotli_compress(_in, _n, _opt, _out);
#endif

        default:
            return status_unavailable;
    }
}

status
decompress_buffer(codec_id     _id,
                  const char*    _in,
                  std::size_t    _n,
                  byte_buffer&   _out)
{
    if ((_in == 0) && (_n != 0))
    {
        return status_invalid_argument;
    }

    switch (_id)
    {
        case codec_id_store:
            return store_copy(_in, _n, _out);

#if D_ENV_COMPRESSION_HAVE_ZLIB
        case codec_id_deflate:
        case codec_id_zlib:
        case codec_id_gzip:
            return zlib_decompress(_id, _in, _n, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_BZIP2
        case codec_id_bzip2:
            return bzip2_decompress(_in, _n, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_LZMA
        case codec_id_xz:
            return xz_decompress(_in, _n, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_ZSTD
        case codec_id_zstd:
            return zstd_decompress(_in, _n, _out);
#endif

#if ( D_ENV_COMPRESSION_HAVE_LZ4 && D_ENV_COMPRESSION_HAVE_LZ4_FRAME )
        case codec_id_lz4:
            return lz4_decompress(_in, _n, _out);
#endif

#if D_ENV_COMPRESSION_HAVE_BROTLI
        case codec_id_brotli:
            return brotli_decompress(_in, _n, _out);
#endif

        default:
            return status_unavailable;
    }
}

NS_END  // internal


NS_END  // djinterp