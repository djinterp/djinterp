/******************************************************************************
* djinterp [test]                                    pack_facade/compress.hpp
*
* Instrumented drop-in for core/util/compress.hpp, used by the test_pack suite:
*   This header shadows the production compression facade when a test build
* defines DTEST_PACK_USE_FACADE_DOUBLE and puts this directory on the include
* path ahead of the real tree (e.g. -I .../pack_facade).  It reuses the
* production include guard (DJINTERP_UTILITY_COMPRESSION_), claimed at the top
* before any nested include, so that if anything else in the translation unit
* reaches for the real header that header collapses to a no-op and only this
* double is in force.
*
*   It reproduces exactly the surface test_pack.hpp consumes -- byte_blob,
* status, the codecs:: tags, try_compress<>, and codec_is_available<> -- but
* RECORDS rather than compresses: try_compress<Tag> writes the observable line
*
*       C|<tag>|<opt.level>|<payload>
*
* into _out and returns codec_stat<Tag>(), a per-codec status slot the tests
* steer.  Every routing / forwarding decision in the code under test therefore
* becomes a plain string the tests read straight back.
*
* THE HOOKS (the test-side controls this double adds over the real facade):
*   - codec_stat<Tag>()     a mutable status slot per codec, status_ok by
*                           default; assign it to force a codec's outcome.
*   - reset_codec_hooks()   restores every slot to status_ok.
*
* PORTABILITY:
*   Only <string> / <sstream> and the env-gated djinterp core are pulled in, so
* the double compiles in every language mode env.h reports (C++98 onward): the
* integer level is rendered through std::ostringstream rather than the
* C++11-only std::to_string, and no other post-C++98 facility is used.
*
* path:      /tests/djinterp/test/pack_facade/compress.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
******************************************************************************/

#ifndef DJINTERP_UTILITY_COMPRESSION_
#define DJINTERP_UTILITY_COMPRESSION_ 1

// std
#include <cstddef>
#include <sstream>
#include <string>
// djinterp  -- angle-bracket paths so the double is location-independent (a
// relative include would resolve against this file's own directory, not the
// real tree).
#include <djinterp/core/djinterp.hpp>               // NS_*, D_INLINE
#include <djinterp/core/util/compress_options.hpp>  // compress_options


NS_DJINTERP

// =============================================================================
// I.   SHARED TYPES
// =============================================================================

// byte_blob
//   type: binary blob container.  std::string exactly as in the production
// facade, so the recorded surface matches byte for byte.
typedef std::string byte_blob;

// status
//   enum: result code returned by every non-throwing operation.  The
// enumerators mirror the production facade so a forced status compares equal
// across the two.
enum status
{
    status_ok = 0,           // operation succeeded
    status_unavailable,      // codec not built into this environment
    status_invalid_argument, // a precondition on the inputs was violated
    status_buffer_error,     // allocation or size-overflow failure
    status_backend_error,    // a backend library reported an error
    status_unsupported       // operation is valid but not implemented here
};


// =============================================================================
// II.  CODEC TAGS
// =============================================================================

namespace codecs
{
    // store
    //   tag: no compression; bytes are copied verbatim.
    struct store
    {
    };

    // deflate
    //   tag: raw DEFLATE stream (RFC 1951).
    struct deflate
    {
    };

    // zlib
    //   tag: zlib-wrapped DEFLATE (RFC 1950).
    struct zlib
    {
    };

    // gzip
    //   tag: gzip-wrapped DEFLATE (RFC 1952).
    struct gzip
    {
    };

    // bzip2
    //   tag: bzip2 (.bz2) stream.
    struct bzip2
    {
    };

    // xz
    //   tag: xz / lzma stream.
    struct xz
    {
    };

    // zstd
    //   tag: Zstandard stream.
    struct zstd
    {
    };

    // lz4
    //   tag: LZ4 frame stream.
    struct lz4
    {
    };

    // brotli
    //   tag: Brotli stream.
    struct brotli
    {
    };
}  // namespace codecs


// =============================================================================
// II-b. RUNTIME CODEC ID
// =============================================================================

// codec_id
//   enum: the runtime codec selector the production facade's dispatch leaves
// (internal::compress_buffer / internal::decompress_buffer) take.  The
// document_bundle / output_packaging layer chooses a codec at RUNTIME through
// this id rather than through the compile-time codecs:: tags, so the double
// mirrors the production enum here.  The enumerator order matches the
// production compress.hpp.  Plain enum, C++98, exactly as in the real facade.
enum codec_id
{
    codec_id_store = 0,  // codecs::store
    codec_id_deflate,    // codecs::deflate
    codec_id_zlib,       // codecs::zlib
    codec_id_gzip,       // codecs::gzip
    codec_id_bzip2,      // codecs::bzip2
    codec_id_xz,         // codecs::xz
    codec_id_zstd,       // codecs::zstd
    codec_id_lz4,        // codecs::lz4
    codec_id_brotli      // codecs::brotli
};


// =============================================================================
// III. RECORDING HELPERS (internal)
// =============================================================================

NS_INTERNAL

    // codec_label
    //   trait: maps a codec tag to the short id string the recorder emits (one
    // trivial specialization per tag).  The primary template is left undefined
    // so an unknown tag is a compile error, as the production codec_traits does.
    template<typename _Codec>
    struct codec_label;

    template<> struct codec_label<codecs::store>   { static const char* name() { return "store";   } };
    template<> struct codec_label<codecs::deflate> { static const char* name() { return "deflate"; } };
    template<> struct codec_label<codecs::zlib>    { static const char* name() { return "zlib";    } };
    template<> struct codec_label<codecs::gzip>    { static const char* name() { return "gzip";    } };
    template<> struct codec_label<codecs::bzip2>   { static const char* name() { return "bzip2";   } };
    template<> struct codec_label<codecs::xz>      { static const char* name() { return "xz";      } };
    template<> struct codec_label<codecs::zstd>    { static const char* name() { return "zstd";    } };
    template<> struct codec_label<codecs::lz4>     { static const char* name() { return "lz4";     } };
    template<> struct codec_label<codecs::brotli>  { static const char* name() { return "brotli";  } };

NS_END  // internal


// =============================================================================
// IV.  STATUS HOOKS
// =============================================================================

// codec_stat
//   function: the mutable status slot for codec _Codec, status_ok by default.
// try_compress<_Codec> and codec_is_available<_Codec> both read it, and a test
// writes it to force a codec's outcome.  Each codec type owns exactly one slot
// for the life of the program.
template<typename _Codec>
status&
codec_stat()
{
    static status s = status_ok;

    return s;
}

// reset_codec_hooks
//   function: restore every codec's status slot to status_ok, undoing any
// forcing an earlier test performed.
// Return:
//   none.
D_INLINE void
reset_codec_hooks()
{
    codec_stat<codecs::store>()   = status_ok;
    codec_stat<codecs::deflate>() = status_ok;
    codec_stat<codecs::zlib>()    = status_ok;
    codec_stat<codecs::gzip>()    = status_ok;
    codec_stat<codecs::bzip2>()   = status_ok;
    codec_stat<codecs::xz>()      = status_ok;
    codec_stat<codecs::zstd>()    = status_ok;
    codec_stat<codecs::lz4>()     = status_ok;
    codec_stat<codecs::brotli>()  = status_ok;

    return;
}


// =============================================================================
// V.   RECORDING API
// =============================================================================

// try_compress
//   function: RECORDS a compression request instead of performing it.  On a
// status_ok slot it writes "C|<tag>|<opt.level>|<payload>" into _out; otherwise
// it empties _out, matching the production contract that _out is unusable on a
// non-ok status.  Always returns codec_stat<_Codec>() and never throws.
template<typename _Codec>
status
try_compress(
    const byte_blob&       _in,
    byte_blob&             _out,
    const compress_options&  _opt = compress_options()
)
{
    status             s;
    std::ostringstream rec;

    s = codec_stat<_Codec>();

    // a failed codec records nothing and leaves _out empty
    if (s != status_ok)
    {
        _out.clear();

        return s;
    }

    rec << "C|"
        << internal::codec_label<_Codec>::name()
        << "|" << _opt.level
        << "|" << _in;
    _out = rec.str();

    return s;
}

// codec_is_available
//   function: reports a codec available exactly when its status slot is
// status_ok, so the availability routers are exercised through the same hook.
template<typename _Codec>
bool
codec_is_available()
{
    return (codec_stat<_Codec>() == status_ok);
}


// =============================================================================
// VI.  RUNTIME DISPATCH LEAVES (recording)
// =============================================================================
// The production facade routes runtime codec_id selections through these
// internal leaves; document_bundle::dispatch_compress calls compress_buffer
// directly.  The double is not part of test_pack's tag-only surface, but the
// output / document_bundle suites compile against it, so it must carry these
// leaves too.  They RECORD the SAME observable line try_compress emits
// ("C|<id>|<opt.level>|<payload>", and "D|<id>|<payload>" for the inverse) and
// route through the SAME per-codec status slot, so a runtime dispatch and a tag
// dispatch of one codec compare byte-for-byte and the existing codec_stat<> /
// reset_codec_hooks() controls steer both.

NS_INTERNAL

    // codec_id_label
    //   helper: the short id string for a runtime codec_id, mirroring
    // codec_label<Tag>::name() for the tag surface.
    D_INLINE const char*
    codec_id_label(
        codec_id _id
    )
    {
        switch (_id)
        {
            case codec_id_store:   { return "store";   }
            case codec_id_deflate: { return "deflate"; }
            case codec_id_zlib:    { return "zlib";    }
            case codec_id_gzip:    { return "gzip";    }
            case codec_id_bzip2:   { return "bzip2";   }
            case codec_id_xz:      { return "xz";      }
            case codec_id_zstd:    { return "zstd";    }
            case codec_id_lz4:     { return "lz4";     }
            case codec_id_brotli:  { return "brotli";  }
            default:               { return "?";       }
        }
    }

    // codec_id_stat
    //   helper: routes a runtime codec_id to the SAME status slot the tag API
    // uses, so reset_codec_hooks() and codec_stat<Tag>() steer runtime dispatch
    // as well.
    D_INLINE status&
    codec_id_stat(
        codec_id _id
    )
    {
        switch (_id)
        {
            case codec_id_deflate: { return codec_stat<codecs::deflate>(); }
            case codec_id_zlib:    { return codec_stat<codecs::zlib>();    }
            case codec_id_gzip:    { return codec_stat<codecs::gzip>();    }
            case codec_id_bzip2:   { return codec_stat<codecs::bzip2>();   }
            case codec_id_xz:      { return codec_stat<codecs::xz>();      }
            case codec_id_zstd:    { return codec_stat<codecs::zstd>();    }
            case codec_id_lz4:     { return codec_stat<codecs::lz4>();     }
            case codec_id_brotli:  { return codec_stat<codecs::brotli>();  }
            case codec_id_store:
            default:               { return codec_stat<codecs::store>();   }
        }
    }

    // compress_buffer
    //   function: the runtime-dispatch compression leaf.  RECORDS
    // "C|<id>|<opt.level>|<payload>" into _out on a status_ok slot; empties _out
    // and returns the forced status otherwise.  The signature mirrors the
    // production internal::compress_buffer exactly.
    D_INLINE status
    compress_buffer(
        codec_id                 _id,
        const char*              _in,
        std::size_t              _n,
        const compress_options&  _opt,
        byte_blob&             _out
    )
    {
        status s;

        // validate the pointer / size relationship as the production leaf does
        if ( (_in == 0) &&
             (_n  != 0) )
        {
            _out.clear();

            return status_invalid_argument;
        }

        s = codec_id_stat(_id);

        // a failed codec records nothing and leaves _out empty
        if (s != status_ok)
        {
            _out.clear();

            return s;
        }

        {
            std::ostringstream rec;

            rec << "C|"
                << codec_id_label(_id)
                << "|" << _opt.level
                << "|" << ( _in ? std::string(_in, _n) : std::string() );
            _out = rec.str();
        }

        return s;
    }

    // decompress_buffer
    //   function: the runtime-dispatch decompression leaf.  RECORDS
    // "D|<id>|<payload>" into _out on a status_ok slot; empties _out otherwise.
    // The signature mirrors the production internal::decompress_buffer exactly.
    D_INLINE status
    decompress_buffer(
        codec_id      _id,
        const char*   _in,
        std::size_t   _n,
        byte_blob&  _out
    )
    {
        status s;

        if ( (_in == 0) &&
             (_n  != 0) )
        {
            _out.clear();

            return status_invalid_argument;
        }

        s = codec_id_stat(_id);

        if (s != status_ok)
        {
            _out.clear();

            return s;
        }

        {
            std::ostringstream rec;

            rec << "D|"
                << codec_id_label(_id)
                << "|" << ( _in ? std::string(_in, _n) : std::string() );
            _out = rec.str();
        }

        return s;
    }

    // codec_available
    //   function: runtime availability of a codec id.  In the double a codec is
    // "available" exactly when its status slot is status_ok, so reset_codec_hooks()
    // and codec_stat<Tag>() steer availability queries and dispatch alike.
    // Mirrors the production internal::codec_available.
    D_INLINE bool
    codec_available(
        codec_id _id
    )
    {
        return (codec_id_stat(_id) == status_ok);
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_COMPRESSION_
