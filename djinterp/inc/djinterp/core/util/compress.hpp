/******************************************************************************
* djinterp [utility]                                              compress.hpp
*
* djinterp portable compression facade:
*   A version-portable (C++98 - C++23), OS-cross-platform interface for buffer
* compression and decompression. The user selects a codec with a tag type and
* the call dispatches, at runtime, to whichever backend env_compress.h
* detected for this build:
*
*     using namespace djinterp::codecs;
*     byte_buffer packed = compress<gzip>(data);     // throwing
*     status s = try_compress<zstd>(data, out);       // non-throwing
*
* Tag dispatch (rather than an enum class) keeps the public surface identical
* across every C++ standard. Thin template entry points forward to non-template
* leaves defined in compress.cpp, so backend code is compiled once.
*
* codecs:
*   store, deflate, zlib, gzip, bzip2, xz, zstd, lz4, brotli
*
* availability:
*   each codec maps to a D_ENV_COMPRESSION_* capability flag. Unavailable
*   codecs compile fine and return status_unavailable at runtime; query at
*   compile time with codec_traits<Codec>::is_available.
*
* 
* path:      /inc/djinterp/core/util/compress.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_UTILITY_COMPRESSION_
#define DJINTERP_UTILITY_COMPRESSION_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../djinterp.hpp"
#include "../env/env_compress.h"
#include "./compress_options.hpp"  // compress_options (full, codec-aware)


// D_ENV_COMPRESSION_HAS_EXCEPTIONS
//   macro: 1 if C++ exceptions are enabled in this translation unit. the
// throwing convenience API is compiled only when this holds; the non-throwing
// try_* API is always available. defined here, before the conditional include
// below, so <stdexcept> is pulled in when needed.
#ifndef D_ENV_COMPRESSION_HAS_EXCEPTIONS
    #if ( defined(__cpp_exceptions) ||                                        \
          defined(__EXCEPTIONS)     ||                                        \
          defined(_CPPUNWIND) )
        #define D_ENV_COMPRESSION_HAS_EXCEPTIONS 1
    #else
        #define D_ENV_COMPRESSION_HAS_EXCEPTIONS 0
    #endif
#endif

#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    #include <stdexcept>
#endif


// =============================================================================
// I.   PORTABILITY HELPERS
// =============================================================================
// (D_ENV_COMPRESSION_HAS_EXCEPTIONS is defined above, ahead of the include it
// gates.)


NS_DJINTERP

// =============================================================================
// II.  SHARED TYPES
// =============================================================================

// byte_buffer
//   type: binary blob container. std::string is used because it is available
// in every C++ standard, tracks its own length (so embedded NULs are safe),
// and exposes contiguous data() / size() access.
typedef std::string byte_buffer;

// status
//   enum: result code returned by every non-throwing operation.
enum status
{
    status_ok = 0,           // operation succeeded
    status_unavailable,      // codec/format not built into this environment
    status_invalid_argument, // a precondition on the inputs was violated
    status_buffer_error,     // allocation or size-overflow failure
    status_backend_error,    // a backend library reported an error
    status_unsupported       // operation is valid but not implemented here
};

// status_message
//   function: returns a static, human-readable description for a status code.
const char* status_message(status _s);


// =============================================================================
// III. CODEC TAGS AND IDENTIFIERS
// =============================================================================

namespace codecs
{
    // store
    //   tag: no compression; bytes are copied verbatim.
    struct store
    {};

    // deflate
    //   tag: raw DEFLATE stream (RFC 1951), no header or trailer.
    struct deflate
    {};

    // zlib
    //   tag: zlib-wrapped DEFLATE (RFC 1950).
    struct zlib
    {};

    // gzip
    //   tag: gzip-wrapped DEFLATE (RFC 1952).
    struct gzip
    {};

    // bzip2
    //   tag: bzip2 (.bz2) stream.
    struct bzip2
    {};

    // xz
    //   tag: xz / lzma stream (liblzma).
    struct xz
    {};

    // zstd
    //   tag: Zstandard stream.
    struct zstd
    {};

    // lz4
    //   tag: LZ4 frame stream.
    struct lz4
    {};

    // brotli
    //   tag: Brotli stream.
    struct brotli
    {};
}  // namespace codecs

// codec_id
//   enum: stable runtime identifier for a codec, used by the dispatch leaves.
enum codec_id
{
    codec_id_store = 0,
    codec_id_deflate,
    codec_id_zlib,
    codec_id_gzip,
    codec_id_bzip2,
    codec_id_xz,
    codec_id_zstd,
    codec_id_lz4,
    codec_id_brotli
};


// =============================================================================
// IV.  CODEC TRAITS
// =============================================================================

// codec_traits
//   trait: maps a codec tag to its runtime id, availability, and name. The
// primary template is intentionally left undefined so that an unknown tag is a
// compile error. is_available is a compile-time constant (0/1) drawn from the
// env_compress.h capability flags.
template<typename _Codec>
struct codec_traits;

// codec_traits<codecs::store>
//   trait: store is always available (built-in copy).
template<>
struct codec_traits<codecs::store>
{
    enum { is_available = 1 };
    static codec_id    id()   { return codec_id_store; }
    static const char* name() { return "store"; }
};

// codec_traits<codecs::deflate>
//   trait: raw DEFLATE, available from any DEFLATE provider.
template<>
struct codec_traits<codecs::deflate>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_DEFLATE != 0) };
    static codec_id    id()   { return codec_id_deflate; }
    static const char* name() { return "deflate"; }
};

// codec_traits<codecs::zlib>
//   trait: zlib-wrapped DEFLATE.
template<>
struct codec_traits<codecs::zlib>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_ZLIB_WRAP != 0) };
    static codec_id    id()   { return codec_id_zlib; }
    static const char* name() { return "zlib"; }
};

// codec_traits<codecs::gzip>
//   trait: gzip-wrapped DEFLATE (any gzip-container provider).
template<>
struct codec_traits<codecs::gzip>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_GZIP_WRAP != 0) };
    static codec_id    id()   { return codec_id_gzip; }
    static const char* name() { return "gzip"; }
};

// codec_traits<codecs::bzip2>
//   trait: bzip2.
template<>
struct codec_traits<codecs::bzip2>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_BZIP2 != 0) };
    static codec_id    id()   { return codec_id_bzip2; }
    static const char* name() { return "bzip2"; }
};

// codec_traits<codecs::xz>
//   trait: xz / lzma.
template<>
struct codec_traits<codecs::xz>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_LZMA != 0) };
    static codec_id    id()   { return codec_id_xz; }
    static const char* name() { return "xz"; }
};

// codec_traits<codecs::zstd>
//   trait: Zstandard.
template<>
struct codec_traits<codecs::zstd>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_ZSTD != 0) };
    static codec_id    id()   { return codec_id_zstd; }
    static const char* name() { return "zstd"; }
};

// codec_traits<codecs::lz4>
//   trait: LZ4 frame.
template<>
struct codec_traits<codecs::lz4>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_LZ4 != 0) };
    static codec_id    id()   { return codec_id_lz4; }
    static const char* name() { return "lz4"; }
};

// codec_traits<codecs::brotli>
//   trait: Brotli (requires both encoder and decoder).
template<>
struct codec_traits<codecs::brotli>
{
    enum { is_available = (D_ENV_COMPRESSION_HAVE_BROTLI != 0) };
    static codec_id    id()   { return codec_id_brotli; }
    static const char* name() { return "brotli"; }
};


// =============================================================================
// V.   OPTIONS
// =============================================================================

// compress_options
//   struct: tuning knobs passed to a compression call.  The full, codec-aware
// definition (generic level plus the per-codec advanced knob-sets) lives in
// util/compress_option.hpp, included above; this facade no longer carries a
// minimal stub of its own, so the two cannot diverge.  The dispatch leaves read
// `level` plus whichever per-codec block matches the selected codec.


// =============================================================================
// VI.  DISPATCH LEAVES (defined in compress.cpp)
// =============================================================================

namespace internal
{
    // compress_buffer
    //   function: compresses [_in, _in + _n) with codec _id into _out.
    status compress_buffer(codec_id                _id,
                           const char*              _in,
                           std::size_t              _n,
                           const compress_options&  _opt,
                           byte_buffer&             _out);

    // decompress_buffer
    //   function: decompresses [_in, _in + _n) with codec _id into _out.
    status decompress_buffer(codec_id     _id,
                            const char*    _in,
                            std::size_t    _n,
                            byte_buffer&   _out);

    // codec_available
    //   function: runtime availability of a codec id.
    bool codec_available(codec_id _id);
}  // namespace internal


// =============================================================================
// VII. NON-THROWING TEMPLATE API
// =============================================================================

// try_compress
//   function: compresses _in into _out using codec _Codec. returns a status;
// never throws.
template<typename _Codec>
status
try_compress(
    const byte_buffer&       _in,
    byte_buffer&             _out,
    const compress_options&  _opt = compress_options()
)
{
    return internal::compress_buffer(codec_traits<_Codec>::id(),
                                     _in.data(),
                                     _in.size(),
                                     _opt,
                                     _out);
}

// try_decompress
//   function: decompresses _in into _out using codec _Codec. returns a status;
// never throws.
template<typename _Codec>
status
try_decompress(
    const byte_buffer&  _in,
    byte_buffer&        _out
)
{
    return internal::decompress_buffer(codec_traits<_Codec>::id(),
                                       _in.data(),
                                       _in.size(),
                                       _out);
}

// codec_is_available
//   function: runtime availability query for codec _Codec.
template<typename _Codec>
bool
codec_is_available()
{
    return internal::codec_available(codec_traits<_Codec>::id());
}


// =============================================================================
// VIII. THROWING CONVENIENCE API
// =============================================================================

#if D_ENV_COMPRESSION_HAS_EXCEPTIONS

// compression_error
//   class: exception thrown by the convenience API on failure.
class compression_error : public std::runtime_error
{
public:
    explicit compression_error(const std::string& _what)
        : std::runtime_error(_what)
    {}
};

// compress
//   function: compresses _in using codec _Codec and returns the result.
// throws compression_error on failure.
template<typename _Codec>
byte_buffer
compress(
    const byte_buffer&       _in,
    const compress_options&  _opt = compress_options()
)
{
    byte_buffer out;
    status      s;

    s = try_compress<_Codec>(_in, out, _opt);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw compression_error(std::string("compress<")
                                + codec_traits<_Codec>::name()
                                + ">: "
                                + status_message(s));
    }

    return out;
}

// decompress
//   function: decompresses _in using codec _Codec and returns the result.
// throws compression_error on failure.
template<typename _Codec>
byte_buffer
decompress(
    const byte_buffer&  _in
)
{
    byte_buffer out;
    status      s;

    s = try_decompress<_Codec>(_in, out);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw compression_error(std::string("decompress<")
                                + codec_traits<_Codec>::name()
                                + ">: "
                                + status_message(s));
    }

    return out;
}

#endif  // D_ENV_COMPRESSION_HAS_EXCEPTIONS


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_COMPRESSION_