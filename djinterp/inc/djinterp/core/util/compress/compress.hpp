/*******************************************************************************
* djinterp [core]                                                   compress.hpp
*
* djinterp portable compression facade:
*   A version-portable (C++98 - C++23), OS-cross-platform interface for buffer
* compression and decompression. The user selects a codec with a tag type and
* the call dispatches, at runtime, to whichever backend env_compress.h
* detected for this build:
*
*     using namespace djinterp::codecs;
*     byte_blob packed = compress<gzip>(data);     // throwing
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
*   RE-BASED ONTO THE SHARED KERNEL (2026-07-30)
*   ============================================
*   The public surface above is unchanged.  What changed is where the
* definitions come from.  This header used to declare `status`, `codec_id` and
* the option set itself, in parallel with the C fork's own copies -- two
* definitions of one formal object, so parity was a coincidence rather than a
* construction.  All three now come from compress_common.h, which both
* languages compile.
*
*   The C++ spellings are preserved as ALIASES plus named constants, not as
* fresh enums: `status_ok`, `codec_id_gzip` and `deflate_strategy_rle` still
* work, over one underlying declaration.
*
*   The backend code is gone from compress.cpp.  What remains there is three
* adapter functions that lower a byte_blob to the kernel's sink and call in.
*
*   TWO THINGS A CALLER MAY NOTICE
*   ------------------------------
*   1. `status` GAINED MEMBERS AND ITS NUMERIC VALUES MOVED.  The kernel
*      distinguishes failures the request caused (unavailable, corrupt input,
*      wrong password) from failures the machinery hit (buffer too small, out
*      of memory), and puts them in disjoint numeric ranges so a caller can ask
*      "is this retryable?" by range instead of by enumerating cases.  Every
*      previous enumerator still exists and still means what it did.  Code that
*      compares against the NAMES is unaffected; code that persisted or
*      transmitted the integer VALUE is not.  Nothing in this facade or in
*      archive.hpp did that -- both only ever compare against status_ok -- but
*      an external caller might have.
*
*   2. A default-constructed compress_options is PRISTINE, not zero.  See
*      compress_options.hpp; the short version is that 0 is a meaningful value
*      for most of these knobs, so a zero-filled set is a fully-specified
*      request wearing the costume of an untouched one.
*
*
* path:      /inc/djinterp/core/util/compress/compress.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UTIL_COMPRESS_COMPRESS_HPP
#define DJINTERP_UTIL_COMPRESS_COMPRESS_HPP 1

// A test build that defines DTEST_PACK_USE_FACADE_DOUBLE gets the recording
// double instead of this facade, wherever this header is reached, so a unit
// never holds both (the owner's ruling of 2026.10.02, lane 4's Q4). The
// double keeps its own path-derived guard: the guard rule has no exceptions.
#ifdef DTEST_PACK_USE_FACADE_DOUBLE
    #include "../../../test/pack_facade/compress.hpp"  // the double
#else

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/util/compress/env_compress.h"
#include "../../../c/util/compress/compress_common.h"     // status, codec_id, the transforms
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
    // std
    #include <stdexcept>
#endif


NS_DJINTERP

// =============================================================================
// II.  SHARED TYPES
// =============================================================================

// byte_blob
//   type: binary blob container. std::string is used because it is available
// in every C++ standard, tracks its own length (so embedded NULs are safe),
// and exposes contiguous data() / size() access.
typedef std::string byte_blob;

// status
//   type: result code returned by every non-throwing operation.  An ALIAS of
// the kernel's enum, not a second declaration -- so a status crossing the
// language boundary needs no translation and cannot be translated wrongly.
typedef enum d_pack_status status;

//   The original six enumerators, preserved.  A caller writing `status_ok` or
// `status_unavailable` is unaffected by the re-basing.
D_STATIC const status status_ok               = D_PACK_STATUS_OK;
D_STATIC const status status_unavailable      = D_PACK_STATUS_UNAVAILABLE;
D_STATIC const status status_invalid_argument = D_PACK_STATUS_INVALID_ARGUMENT;
D_STATIC const status status_unsupported      = D_PACK_STATUS_UNSUPPORTED;
D_STATIC const status status_backend_error    = D_PACK_STATUS_BACKEND_ERROR;
//   status_buffer_error was documented as "allocation or size-overflow
// failure", which the kernel now splits in two because they call for different
// responses: a buffer that is merely too small can be grown and retried, and
// an allocation that failed cannot.  The old name keeps the retryable meaning.
D_STATIC const status status_buffer_error     = D_PACK_STATUS_BUFFER_TOO_SMALL;

//   New, from the kernel: distinctions a facade over real backends needs and
// could not previously express.
D_STATIC const status status_corrupt_input    = D_PACK_STATUS_CORRUPT_INPUT;
D_STATIC const status status_truncated_input  = D_PACK_STATUS_TRUNCATED_INPUT;
D_STATIC const status status_wrong_password   = D_PACK_STATUS_WRONG_PASSWORD;
D_STATIC const status status_no_memory        = D_PACK_STATUS_NO_MEMORY;

// status_message
//   function: returns a static, human-readable description for a status code.
inline const char*
status_message(
    status _s
)
{
    return d_pack_status_message(_s);
}

// status_is_retryable
//   function: whether _s says the request was fine and only the machinery ran
// short, so growing a buffer and calling again may succeed.  A formal failure
// is never retryable, which is why the two kinds occupy disjoint ranges.
inline bool
status_is_retryable(
    status _s
)
{
    return (_s == status_buffer_error);
}


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
//   type: stable runtime identifier for a codec, used by the dispatch leaves.
// An ALIAS of the kernel's enum.  The values are unchanged from the previous
// declaration (store = 0 through brotli = 8) and are pinned explicitly in the
// kernel, because test_packaging.hpp's bridge passes them by value and an
// implicit ordering is one insertion away from selecting the wrong codec.
typedef enum d_codec_id codec_id;

D_STATIC const codec_id codec_id_store   = D_CODEC_ID_STORE;
D_STATIC const codec_id codec_id_deflate = D_CODEC_ID_DEFLATE;
D_STATIC const codec_id codec_id_zlib    = D_CODEC_ID_ZLIB;
D_STATIC const codec_id codec_id_gzip    = D_CODEC_ID_GZIP;
D_STATIC const codec_id codec_id_bzip2   = D_CODEC_ID_BZIP2;
D_STATIC const codec_id codec_id_xz      = D_CODEC_ID_XZ;
D_STATIC const codec_id codec_id_zstd    = D_CODEC_ID_ZSTD;
D_STATIC const codec_id codec_id_lz4     = D_CODEC_ID_LZ4;
D_STATIC const codec_id codec_id_brotli  = D_CODEC_ID_BROTLI;


// =============================================================================
// IV.  CODEC TRAITS
// =============================================================================

// codec_traits
//   trait: maps a codec tag to its runtime id, availability, and name. The
// primary template is intentionally left undefined so that an unknown tag is a
// compile error. is_available is a compile-time constant (0/1) drawn from the
// env_compress.h capability flags.
template<typename Codec>
struct codec_traits;

// codec_traits<codecs::store>
//   trait: store is always available -- the kernel implements it
// unconditionally, which is what keeps a build with no third-party codec a
// working build rather than a degraded one.
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
//   struct: tuning knobs passed to a compression call.  Defined in
// compress_options.hpp, which now DERIVES it from the kernel's
// d_compress_options at zero cost rather than declaring a parallel copy.  The
// dispatch leaves read `level` plus whichever per-codec block matches the
// selected codec -- and the kernel resolves any knob left UNSET from its own
// pinned default table, so C and C++ hand a backend identical parameters.


// =============================================================================
// VI.  DISPATCH LEAVES (defined in compress.cpp)
// =============================================================================
//   These are now ADAPTERS, not implementations.  Each lowers a byte_blob to a
// d_pack_sink and calls the kernel; there is no backend code left in
// compress.cpp.  They remain non-template and out-of-line so the template
// entry points below stay thin and the kernel is called from one place.

namespace internal
{
    // compress_buffer
    //   function: compresses [_in, _in + _n) with codec _id into _out.
    status compress_buffer(codec_id                 _id,
                           const char*              _in,
                           std::size_t              _n,
                           const compress_options&  _opt,
                           byte_blob&               _out);

    // decompress_buffer
    //   function: decompresses [_in, _in + _n) with codec _id into _out.
    status decompress_buffer(codec_id     _id,
                             const char*  _in,
                             std::size_t  _n,
                             byte_blob&   _out);

    // codec_available
    //   function: runtime availability of a codec id.
    bool codec_available(codec_id _id);
}  // namespace internal


// =============================================================================
// VII. NON-THROWING TEMPLATE API
// =============================================================================

// try_compress
//   function: compresses _in into _out using codec Codec. returns a status;
// never throws.
template<typename Codec>
status
try_compress(
    const byte_blob&         _in,
    byte_blob&               _out,
    const compress_options&  _opt = compress_options()
)
{
    return internal::compress_buffer(codec_traits<Codec>::id(),
                                     _in.data(),
                                     _in.size(),
                                     _opt,
                                     _out);
}

// try_decompress
//   function: decompresses _in into _out using codec Codec. returns a status;
// never throws.
template<typename Codec>
status
try_decompress(
    const byte_blob&  _in,
    byte_blob&        _out
)
{
    return internal::decompress_buffer(codec_traits<Codec>::id(),
                                       _in.data(),
                                       _in.size(),
                                       _out);
}

// codec_is_available
//   function: runtime availability query for codec Codec.
template<typename Codec>
bool
codec_is_available()
{
    return internal::codec_available(codec_traits<Codec>::id());
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
//   function: compresses _in using codec Codec and returns the result.
// throws compression_error on failure.
template<typename Codec>
byte_blob
compress(
    const byte_blob&         _in,
    const compress_options&  _opt = compress_options()
)
{
    byte_blob out;
    status    s;

    s = try_compress<Codec>(_in, out, _opt);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw compression_error(std::string("compress<")
                                + codec_traits<Codec>::name()
                                + ">: "
                                + status_message(s));
    }

    return out;
}

// decompress
//   function: decompresses _in using codec Codec and returns the result.
// throws compression_error on failure.
template<typename Codec>
byte_blob
decompress(
    const byte_blob&  _in
)
{
    byte_blob out;
    status    s;

    s = try_decompress<Codec>(_in, out);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw compression_error(std::string("decompress<")
                                + codec_traits<Codec>::name()
                                + ">: "
                                + status_message(s));
    }

    return out;
}

#endif  // D_ENV_COMPRESSION_HAS_EXCEPTIONS


NS_END  // djinterp


#endif  // DTEST_PACK_USE_FACADE_DOUBLE

#endif  // DJINTERP_UTIL_COMPRESS_COMPRESS_HPP
