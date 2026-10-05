/*******************************************************************************
* djinterp [core]                                             archive_detect.hpp
*
*   Container recognition: "what format are these bytes, and how many members
* does the directory claim?"  The archive counterpart of compress_detect.hpp.
*
*   WHY IT IS NOT PART OF archive.hpp:
*   Recognition reads bytes.  It calls no backend, so a caller that only wants
* to IDENTIFY or COUNT -- a bundler choosing an extension, a report writer
* checking what it just wrote, a suite asserting another module emitted the
* container it claimed -- should not link the format dispatcher it never
* invokes.  Same split, same reason, as compress_options.hpp and
* compress_detect.hpp.
*
*   LINKAGE, STATED PLAINLY:
*   Requires archive_common.c for the signature table.  Does NOT require
* archive.cpp or any format backend.
*
*   TWO KINDS OF QUESTION LIVE HERE:
*   The format predicates forward to the kernel, so the framing rule stays with
* the format.  The ZIP directory probes do not: scanning back for an
* end-of-central-directory record and reading its member count is structural
* inspection with no kernel equivalent that avoids a backend -- d_archive_measure
* dispatches.  Those stay here, deliberately lenient.  They answer "this looks
* like format X and claims N members", not byte-for-byte exactness, which
* belongs to a suite that knows what it wrote.
*
*   tar has no magic at offset 0 -- `ustar` sits at byte 257 -- so recognising
* a tar needs a full 512-byte record.  d_format_signature_length reports that
* rather than a nominal small number.
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
* path:      /inc/djinterp/core/util/archive/archive_detect.hpp
* link(s):   archive_common.c
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.04
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL: LITTLE-ENDIAN READ HELPERS
      ------------------------------------

II.   FORMAT PREDICATES
      -----------------

III.  GENERAL SIGNATURE QUERY AND DETECTION
      -------------------------------------

IV.   ZIP DIRECTORY PROBES
      --------------------

V.    TAR STRUCTURE PROBES
      --------------------
*/

#ifndef DJINTERP_UTIL_ARCHIVE_ARCHIVE_DETECT_HPP
#define DJINTERP_UTIL_ARCHIVE_ARCHIVE_DETECT_HPP 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/archive/archive_common.h"   // the signature table


NS_DJINTERP


// =============================================================================
// I.   INTERNAL: LITTLE-ENDIAN READ HELPERS
// =============================================================================
//   ZIP records are little-endian regardless of host byte order, so the probes
// below decode explicitly rather than casting.

namespace archive_detect_internal
{
    // read_u16_le
    //   function: read a 16-bit little-endian value from _p at byte offset
    // _at.  The caller guarantees _at + 2 <= size.
    //
    // Parameter(s):
    //   _p:  base of the byte range.
    //   _at: offset of the low byte.
    // Return:
    //   the decoded value.
    D_INLINE unsigned int
    read_u16_le(
        const char* _p,
        std::size_t _at
    )
    {
        const unsigned char* u = (const unsigned char*)_p + _at;

        return (unsigned int)u[0] | ((unsigned int)u[1] << 8);
    }

    // read_u32_le
    //   function: read a 32-bit little-endian value from _p at byte offset
    // _at.  The caller guarantees _at + 4 <= size.
    //
    // Parameter(s):
    //   _p:  base of the byte range.
    //   _at: offset of the low byte.
    // Return:
    //   the decoded value.
    D_INLINE unsigned long
    read_u32_le(
        const char* _p,
        std::size_t _at
    )
    {
        const unsigned char* u = (const unsigned char*)_p + _at;

        return (  (unsigned long)u[0]         |
                 ((unsigned long)u[1] << 8)   |
                 ((unsigned long)u[2] << 16)  |
                 ((unsigned long)u[3] << 24) );
    }
}   // namespace archive_detect_internal


// zip_find_eocd is defined in section IV; looks_like_zip below needs it.
std::size_t zip_find_eocd(const std::string& _blob);


// =============================================================================
// II.  FORMAT PREDICATES
// =============================================================================
//   Each forwards to the kernel, so the framing rule lives with the format.

// format_magic_at
//   function: whether the bytes at _data carry _format's signature.  A blob
// shorter than the signature answers false rather than reading past the end.
inline bool
format_magic_at(
    enum d_format_id _format,
    const void*      _data,
    std::size_t      _size
)
{
    return (d_format_signature_matches(_format, _data, _size) != 0);
}

// looks_like_gzip
//   function: the gzip framing.  A pure forward -- the kernel checks the same
// three bytes (1F 8B 08) the hand-written probe did, at the same minimum size.
inline bool looks_like_gzip(const std::string& _b)
{ return format_magic_at(D_FORMAT_ID_GZ, _b.data(), _b.size()); }

// looks_like_zip
//   function: front signature AND a findable end-of-central-directory.
//
//   NOT a pure forward, deliberately.  d_format_signature_matches answers on
// the leading four bytes alone, so it says yes to a TRUNCATED zip -- one that
// begins PK\003\004 and was cut before its directory.  The question this name
// asks is whether the blob is a plausible container, and a truncated one is
// not.  The composite is what callers have been asserting against, so it is
// preserved rather than quietly weakened.  The kernel's front-only answer is
// still reachable as format_signature_matches(D_FORMAT_ID_ZIP, blob).
inline bool
looks_like_zip(
    const std::string& _b
)
{
    if (!format_magic_at(D_FORMAT_ID_ZIP, _b.data(), _b.size()))
    {
        return false;
    }

    return (zip_find_eocd(_b) != (std::size_t)-1);
}

// tar_has_ustar_magic
//   function: the `ustar` magic at byte 257, and nothing more.
//
//   NOT a forward either, for the opposite reason: the kernel requires a full
// 512-byte record before it will answer for tar (d_format_signature_length
// returns D_INTERNAL_TAR_BLOCK), which is the right rule for RECOGNISING a tar
// but a stricter question than this name asks.  A 300-byte fragment carrying
// the magic answers true here and false to the kernel.  Both are correct
// answers to different questions; use format_signature_matches(D_FORMAT_ID_TAR,
// blob) for the recognition one.
inline bool
tar_has_ustar_magic(
    const std::string& _b
)
{
    const char* p = _b.data();

    if (_b.size() < 263u)
    {
        return false;
    }

    return ( (p[257] == 'u') &&
             (p[258] == 's') &&
             (p[259] == 't') &&
             (p[260] == 'a') &&
             (p[261] == 'r') );
}


// =============================================================================
// III. GENERAL SIGNATURE QUERY AND DETECTION
// =============================================================================

// format_signature_matches
//   function: whether _blob carries _format's signature.
inline bool
format_signature_matches(
    enum d_format_id   _format,
    const std::string& _blob
)
{
    return format_magic_at(_format, _blob.data(), _blob.size());
}

// format_signature_length
//   function: how many leading bytes must be present to answer for _format.
// For tar this is a full record, not a short magic.
inline int
format_signature_length(
    enum d_format_id _format
)
{
    return d_format_signature_length(_format);
}

// detect_format
//   function: identify the container framing _blob.  Returns false and leaves
// _out_format untouched when nothing matches.
inline bool
detect_format(
    const void*       _data,
    std::size_t       _size,
    enum d_format_id* _out_format
)
{
    return (d_format_detect(_data, _size, _out_format) != 0);
}

inline bool
detect_format(
    const std::string& _blob,
    enum d_format_id*  _out_format
)
{
    return detect_format(_blob.data(), _blob.size(), _out_format);
}


// =============================================================================
// IV.  ZIP DIRECTORY PROBES / V.  TAR STRUCTURE PROBES
// =============================================================================
//   Lifted verbatim from test_archive.hpp so the byte-level behaviour is
// carried over rather than re-derived.  These have no kernel equivalent that
// avoids a backend, which is why they are implementations here and not
// forwards.

// zip_find_eocd
//   function: locate the ZIP end-of-central-directory record by scanning
// backward for its "PK\5\6" signature (the record is within the last 22 bytes
// when there is no archive comment, which the built-in writer never emits).
//
// Parameter(s):
//   _blob: the candidate archive bytes.
// Return:
//   the offset of the EOCD signature, or (std::size_t)-1 when not found.
inline std::size_t
zip_find_eocd(
    const std::string& _blob
)
{
    const std::size_t npos = (std::size_t)-1;
    const char*       p    = _blob.data();
    std::size_t       n    = _blob.size();
    std::size_t       i;

    // the smallest possible EOCD is 22 bytes
    if (n < 22u)
    {
        return npos;
    }

    // scan backward from the latest position an EOCD could begin
    for (i = n - 22u + 1u; i-- > 0; )
    {
        if ( (p[i] == 'P')                          &&
             (p[i + 1] == 'K')                       &&
             ((unsigned char)p[i + 2] == 0x05u)      &&
             ((unsigned char)p[i + 3] == 0x06u) )
        {
            return i;
        }
    }

    return npos;
}

// zip_total_entries
//   function: read the member count the ZIP end-of-central-directory record
// advertises (the "total entries" field), letting a caller assert how many
// files a producer wrote without a full extract.
//
// Parameter(s):
//   _blob: the candidate archive bytes.
// Return:
//   the advertised entry count, or -1 when no EOCD is present.
D_INLINE long
zip_total_entries(
    const std::string& _blob
)
{
    std::size_t eocd = zip_find_eocd(_blob);

    if (eocd == (std::size_t)-1)
    {
        return -1L;
    }

    // total-entries-on-all-disks lives 10 bytes into the EOCD record
    return (long)archive_detect_internal::read_u16_le(_blob.data(), eocd + 10u);
}

// zip_local_methods
//   function: walk the local file headers from the front of a ZIP, appending
// each entry's 2-byte compression method to _out (0 = stored, 8 = deflate),
// stopping at the central directory.  Lets a suite confirm that compression
// actually engaged (or that store was forced).
//
// Parameter(s):
//   _blob: the ZIP bytes.
//   _out:  receives one method code per local header (cleared first).
// Return:
//   true when the local-header chain parsed without running past the end.
D_INLINE bool
zip_local_methods(
    const std::string&         _blob,
    std::vector<unsigned int>& _out
)
{
    const char* in  = _blob.data();
    std::size_t n   = _blob.size();
    std::size_t pos = 0;

    _out.clear();

    while (pos + 4u <= n)
    {
        unsigned long sig = archive_detect_internal::read_u32_le(in, pos);

        // the central directory ends the run of local headers
        if (sig != 0x04034b50UL)
        {
            break;
        }

        // a local header is 30 bytes before the name / extra / data
        if (pos + 30u > n)
        {
            return false;
        }

        {
            unsigned int  method    = archive_detect_internal::read_u16_le(in, pos + 8u);
            unsigned long comp_size = archive_detect_internal::read_u32_le(in, pos + 18u);
            unsigned int  name_len  = archive_detect_internal::read_u16_le(in, pos + 26u);
            unsigned int  extra_len = archive_detect_internal::read_u16_le(in, pos + 28u);
            std::size_t   data_at   = pos + 30u + name_len + extra_len;

            if (data_at + comp_size > n)
            {
                return false;
            }

            _out.push_back(method);
            pos = data_at + comp_size;
        }
    }

    return true;
}

// tar_is_terminated
//   function: whether a tar stream ends with the mandatory two 512-byte zero
// blocks (a 1024-byte all-zero tail).
//
// Parameter(s):
//   _blob: the tar bytes.
// Return:
//   true when the final 1024 bytes are all zero.
D_INLINE bool
tar_is_terminated(
    const std::string& _blob
)
{
    const char* p = _blob.data();
    std::size_t n = _blob.size();
    std::size_t i;

    if (n < 1024u)
    {
        return false;
    }

    for (i = n - 1024u; i < n; ++i)
    {
        if (p[i] != 0)
        {
            return false;
        }
    }

    return true;
}

NS_END  // djinterp


#endif  // DJINTERP_UTIL_ARCHIVE_ARCHIVE_DETECT_HPP
