/*******************************************************************************
* djinterp [core]                                            compress_detect.hpp
*
*   Stream recognition: "what codec produced these bytes?"  Magic-byte
* predicates and a detector over them, as a C++ face on compress_common.h's
* signature table.
*
*   WHY IT IS NOT PART OF compress.hpp:
*   Detection reads bytes against a constant table.  It calls no backend, so a
* caller that only wants to IDENTIFY a blob -- a bundler picking an extension,
* a report writer sniffing an input, a suite asserting that another module
* emitted the framing it claimed -- should not have to link the codec
* dispatcher it never invokes.  This is the same split compress_options.hpp
* already makes for the option vocabulary, for the same reason.
*
*   LINKAGE, STATED PLAINLY:
*   This header is not free of linkage; it requires compress_common.c, where
* the signature table and the walk live.  It does NOT require compress.cpp or
* any codec backend.  That is the whole distinction, and "header-only" would be
* the wrong word for it.
*
*   NO TAGS HERE:
*   The surface is expressed in codec_id, the kernel enum, not in the codecs::
*   tag types -- those are declared in compress.hpp, and including it would
* reintroduce exactly the dependency this header exists to avoid.  The
* tag-dispatched spellings (signature_of, has_expected_signature<Codec>) stay
* in the test layer, one thin hop above these.
*
*   store, raw DEFLATE and brotli carry no fixed magic, so they answer 1
* unconditionally.  That is a property of the formats, not a shortcut.
*
*   PORTABILITY:
*   Target floor is C++98.  The EFFECTIVE floor today is C++11, and not because
* of anything in this header -- djinterp.hpp gates below it outright ("djinterp
* requires C++11 or later"), so no header that includes it can compile at C++98
* regardless of what it uses.  Nothing here needs C++11 in its own right, so
* this returns to the target floor the day the gate moves.  Verified building
* at C++11, C++17 and C++20.
*
*
* path:      /inc/djinterp/core/util/compress/compress_detect.hpp
* link(s):   compress_common.c
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.04
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    NAMED MAGIC PREDICATES
      ----------------------

II.   GENERAL SIGNATURE QUERY
      -----------------------

III.  DETECTION
      ---------

IV.   CHECKSUM
      --------
*/

#ifndef DJINTERP_UTIL_COMPRESS_COMPRESS_DETECT_HPP
#define DJINTERP_UTIL_COMPRESS_COMPRESS_DETECT_HPP 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/compress/compress_common.h"   // the signature table


NS_DJINTERP


// =============================================================================
// I.   NAMED MAGIC PREDICATES
// =============================================================================
//   One per codec that has a fixed framing.  Each forwards to the kernel, so
// the framing rule lives with the codec rather than being restated here.

// has_gzip_magic / has_zlib_magic / ...
//   function: whether the leading bytes are the framing this codec emits.
//   A blob shorter than the signature answers false rather than reading past
// the end.
inline bool
codec_magic_at(
    enum d_codec_id _codec,
    const void*     _data,
    std::size_t     _size
)
{
    return (d_codec_signature_matches(_codec, _data, _size) != 0);
}

inline bool has_gzip_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_GZIP,  _b.data(), _b.size()); }

inline bool has_zlib_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_ZLIB,  _b.data(), _b.size()); }

inline bool has_bzip2_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_BZIP2, _b.data(), _b.size()); }

inline bool has_xz_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_XZ,    _b.data(), _b.size()); }

inline bool has_zstd_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_ZSTD,  _b.data(), _b.size()); }

inline bool has_lz4_frame_magic(const std::string& _b)
{ return codec_magic_at(D_CODEC_ID_LZ4,   _b.data(), _b.size()); }


// =============================================================================
// II.  GENERAL SIGNATURE QUERY
// =============================================================================

// codec_signature_matches
//   function: whether _blob carries the framing _codec is required to emit.
inline bool
codec_signature_matches(
    enum d_codec_id    _codec,
    const std::string& _blob
)
{
    return codec_magic_at(_codec, _blob.data(), _blob.size());
}

// codec_signature_length
//   function: how many leading bytes the codec's signature occupies.  Zero for
// the codecs that carry no fixed magic.
inline int
codec_signature_length(
    enum d_codec_id _codec
)
{
    return d_codec_signature_length(_codec);
}


// =============================================================================
// III. DETECTION
// =============================================================================
//   The kernel tries the codecs in an order that puts longer, more specific
// signatures first, so a blob is not claimed by a shorter prefix.

// detect_codec
//   function: identify the codec that framed _blob.  Returns false and leaves
// _out_codec untouched when nothing matches -- which includes every stored,
// raw-DEFLATE and brotli stream, since those carry no magic to detect.  A
// negative answer is therefore "no framing found", never "not a stream".
inline bool
detect_codec(
    const void*      _data,
    std::size_t      _size,
    enum d_codec_id* _out_codec
)
{
    return (d_codec_detect(_data, _size, _out_codec) != 0);
}

inline bool
detect_codec(
    const std::string& _blob,
    enum d_codec_id*   _out_codec
)
{
    return detect_codec(_blob.data(), _blob.size(), _out_codec);
}


// =============================================================================
// IV.  CHECKSUM
// =============================================================================
//   Here rather than in the archive layer because BOTH need it: the gzip
// trailer carries a CRC32 of the uncompressed data, and every ZIP local header
// and central-directory record carries one per entry.  IEEE 802.3, reflected
// polynomial 0xEDB88320; seed a fresh run with 0.

inline unsigned long
pack_crc32(
    unsigned long      _seed,
    const std::string& _data
)
{
    return (unsigned long)d_pack_crc32((uint32_t)_seed,
                                       _data.data(),
                                       _data.size());
}


NS_END  // djinterp


#endif  // DJINTERP_UTIL_COMPRESS_COMPRESS_DETECT_HPP
