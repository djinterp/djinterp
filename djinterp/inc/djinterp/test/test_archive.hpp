/******************************************************************************
* djinterp [test]                                             test_archive.hpp
*
*   Shared, DEPENDENCY-LIGHT verification helpers for the archive facade
* (core/util/archive.hpp).  It exists so that any test suite whose module
* PRODUCES or CONSUMES an archive -- not just archive.hpp's own tests -- can
* assert facts about the bytes without re-deriving container layouts each time:
* build sample entries, round-trip a set through a format, confirm the payload
* survived, and sniff a blob's shape (is this really a zip / gzip / ustar, how
* many members does its directory claim, which method did each entry use).
*
*   WHY IT LIVES HERE (not inside a single suite):
*   Several modules lean on archiving (report packaging, bundle writers, and
* the archive module itself).  Their tests all need the same handful of
* checks, so they are collected once, in djinterp::test, alongside the other
* shared test utilities (e.g. test_zip_store.hpp).  Everything here is built
* strictly on archive.hpp's PUBLIC surface (entry / entry_list / byte_blob /
* the format tags / try_archive / try_extract / format_is_writable), so it adds
* no dependency a caller of the facade does not already have, and it never
* reaches into archive.cpp internals.
*
*   WHAT IT IS NOT:
*   Not an assertion framework and not a test itself -- it returns plain bool /
* status / counts that a suite feeds to its own D_xx_CHECK macros.  The sniffers
* are intentionally lenient structural probes (front signature + trailing
* end-of-central-directory for zip, magic bytes for gzip, the ustar magic for
* tar); they confirm "this looks like format X and carries N members", not
* byte-for-byte bit-exactness, which belongs to a specific suite.
*
*   PORTABILITY:
*   C++98 - C++23, header-only, no third-party include.  Availability-aware by
* construction: the round-trip drivers report whatever status the facade
* returns, so on a build lacking a codec (gz unavailable, say) a caller sees
* status_unavailable rather than a spurious failure.
*
*
* TABLE OF CONTENTS
* =================
* I.    INTERNAL: LITTLE-ENDIAN READ HELPERS
* II.   ENTRY CONSTRUCTION
* III.  ENTRY-LIST INSPECTION
* IV.   PAYLOAD COMPARISON
* V.    FORMAT SNIFFERS (zip / gzip / tar shape + member counts)
* VI.   ROUND-TRIP DRIVERS
*
*
* path:      /inc/djinterp/test/test_archive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.21
******************************************************************************/

#ifndef DJINTERP_TEST_ARCHIVE_
#define DJINTERP_TEST_ARCHIVE_ 1

#ifndef __cplusplus
    #error "test_archive.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/util/archive.hpp"   // entry, entry_list, byte_blob, formats


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                I.   INTERNAL: LITTLE-ENDIAN READ HELPERS                 ///
///////////////////////////////////////////////////////////////////////////////

namespace archive_verify_internal
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
}   // namespace archive_verify_internal


///////////////////////////////////////////////////////////////////////////////
///                II.  ENTRY CONSTRUCTION                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_entry
//   function: build a fully specified archive entry.  A convenience over
// aggregate-initialising the struct field by field at every call site.
//
// Parameter(s):
//   _name:         path within the archive ('/' separated).
//   _data:         file contents; ignored when _is_directory is true.
//   _is_directory: true for a directory member (no data).
//   _mode:         unix permission bits; 0 lets the writer pick a default.
//   _mtime:        unix epoch seconds; 0 lets the writer pick "now".
// Return:
//   the populated entry.
D_INLINE entry
make_entry(
    const std::string& _name,
    const byte_blob& _data,
    bool               _is_directory,
    unsigned int       _mode,
    long               _mtime
)
{
    entry e;

    e.name         = _name;
    e.data         = _data;
    e.is_directory = _is_directory;
    e.mode         = _mode;
    e.mtime        = _mtime;

    return e;
}

// make_file_entry
//   function: build a regular-file entry with default mode / mtime.
//
// Parameter(s):
//   _name: path within the archive.
//   _data: file contents (may contain embedded NULs).
// Return:
//   the populated regular-file entry.
D_INLINE entry
make_file_entry(
    const std::string& _name,
    const byte_blob& _data
)
{
    return make_entry(_name, _data, false, 0u, 0L);
}

// make_text_entry
//   function: build a regular-file entry whose contents are the given text.
// A readability alias for make_file_entry with a string payload.
//
// Parameter(s):
//   _name: path within the archive.
//   _text: file contents as text.
// Return:
//   the populated regular-file entry.
D_INLINE entry
make_text_entry(
    const std::string& _name,
    const std::string& _text
)
{
    return make_entry(_name, _text, false, 0u, 0L);
}

// make_dir_entry
//   function: build a directory entry (no data) with default mode / mtime.
//
// Parameter(s):
//   _name: directory path within the archive.
// Return:
//   the populated directory entry.
D_INLINE entry
make_dir_entry(
    const std::string& _name
)
{
    return make_entry(_name, byte_blob(), true, 0u, 0L);
}


///////////////////////////////////////////////////////////////////////////////
///                III. ENTRY-LIST INSPECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// normalize_name
//   function: drop a single trailing '/' so a directory name compares equal
// whether or not a format appended the separator (zip stores "dir/", tar and
// the source entry may carry "dir").
//
// Parameter(s):
//   _name: a raw entry name.
// Return:
//   _name without one trailing '/', if present.
D_INLINE std::string
normalize_name(
    const std::string& _name
)
{
    if ( (!_name.empty()) &&
         (_name[_name.size() - 1] == '/') )
    {
        return _name.substr(0, _name.size() - 1);
    }

    return _name;
}

// find_entry
//   function: locate the first entry whose name matches _name, comparing with
// trailing-slash tolerance (see normalize_name).
//
// Parameter(s):
//   _items: the list to search.
//   _name:  the name to find.
// Return:
//   a pointer to the matching entry, or a null pointer when absent.
D_INLINE const entry*
find_entry(
    const entry_list&  _items,
    const std::string& _name
)
{
    const std::string want = normalize_name(_name);
    std::size_t       i;

    for (i = 0; i < _items.size(); ++i)
    {
        if (normalize_name(_items[i].name) == want)
        {
            return &_items[i];
        }
    }

    return (const entry*)0;
}

// has_entry
//   function: whether an entry named _name is present (slash-tolerant).
//
// Parameter(s):
//   _items: the list to search.
//   _name:  the name to find.
// Return:
//   true when a matching entry exists.
D_INLINE bool
has_entry(
    const entry_list&  _items,
    const std::string& _name
)
{
    return find_entry(_items, _name) != (const entry*)0;
}

// count_files
//   function: number of non-directory members in _items.
//
// Parameter(s):
//   _items: the list to scan.
// Return:
//   the count of regular-file entries.
D_INLINE std::size_t
count_files(
    const entry_list& _items
)
{
    std::size_t n = 0;
    std::size_t i;

    for (i = 0; i < _items.size(); ++i)
    {
        if (!_items[i].is_directory)
        {
            ++n;
        }
    }

    return n;
}

// count_dirs
//   function: number of directory members in _items.
//
// Parameter(s):
//   _items: the list to scan.
// Return:
//   the count of directory entries.
D_INLINE std::size_t
count_dirs(
    const entry_list& _items
)
{
    std::size_t n = 0;
    std::size_t i;

    for (i = 0; i < _items.size(); ++i)
    {
        if (_items[i].is_directory)
        {
            ++n;
        }
    }

    return n;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  PAYLOAD COMPARISON                                   ///
///////////////////////////////////////////////////////////////////////////////

// file_data
//   function: copy the contents of the non-directory entry named _name into
// _out (slash-tolerant name match).
//
// Parameter(s):
//   _items: the list to search.
//   _name:  the file name to fetch.
//   _out:   receives the file contents on success (untouched on failure).
// Return:
//   true when a regular-file entry of that name was found.
D_INLINE bool
file_data(
    const entry_list&  _items,
    const std::string& _name,
    byte_blob&       _out
)
{
    const entry* e = find_entry(_items, _name);

    if ( (e == (const entry*)0) ||
         (e->is_directory) )
    {
        return false;
    }

    _out = e->data;

    return true;
}

// files_preserved
//   function: confirm every regular file in _original survives in _restored
// with byte-identical contents, matching by name (slash-tolerant) and ignoring
// order, mode, and mtime.  This is the round-trip payload check for the
// name-carrying formats (zip, tar, tar.gz, 7z); gzip carries no name and is
// checked by comparing data directly instead.
//
// Parameter(s):
//   _original: the entries handed to the writer.
//   _restored: the entries returned by the reader.
// Return:
//   true when each original file has a data-equal counterpart in _restored.
D_INLINE bool
files_preserved(
    const entry_list& _original,
    const entry_list& _restored
)
{
    std::size_t i;

    for (i = 0; i < _original.size(); ++i)
    {
        const entry& src = _original[i];

        // directories carry no payload; their representation is format-specific
        if (src.is_directory)
        {
            continue;
        }

        {
            const entry* got = find_entry(_restored, src.name);

            if ( (got == (const entry*)0) ||
                 (got->is_directory)      ||
                 (got->data != src.data) )
            {
                return false;
            }
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   FORMAT SNIFFERS                                      ///
///////////////////////////////////////////////////////////////////////////////

// zip_find_eocd
//   function: locate the ZIP end-of-central-directory record by scanning
// backward for its "PK\5\6" signature (the record is within the last 22 bytes
// when there is no archive comment, which the built-in writer never emits).
//
// Parameter(s):
//   _blob: the candidate archive bytes.
// Return:
//   the offset of the EOCD signature, or (std::size_t)-1 when not found.
D_INLINE std::size_t
zip_find_eocd(
    const byte_blob& _blob
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

// looks_like_zip
//   function: lenient structural check that _blob is a ZIP: it both opens with
// a known ZIP signature (a local file header, or the EOCD for an empty archive)
// and carries an end-of-central-directory record.
//
// Parameter(s):
//   _blob: the candidate archive bytes.
// Return:
//   true when _blob has the shape of a ZIP container.
D_INLINE bool
looks_like_zip(
    const byte_blob& _blob
)
{
    const char* p = _blob.data();

    if (_blob.size() < 22u)
    {
        return false;
    }

    {
        const bool local_sig = ( (p[0] == 'P')                     &&
                                 (p[1] == 'K')                      &&
                                 ((unsigned char)p[2] == 0x03u)     &&
                                 ((unsigned char)p[3] == 0x04u) );
        const bool eocd_sig  = ( (p[0] == 'P')                     &&
                                 (p[1] == 'K')                      &&
                                 ((unsigned char)p[2] == 0x05u)     &&
                                 ((unsigned char)p[3] == 0x06u) );

        return ( (local_sig || eocd_sig) &&
                 (zip_find_eocd(_blob) != (std::size_t)-1) );
    }
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
    const byte_blob& _blob
)
{
    std::size_t eocd = zip_find_eocd(_blob);

    if (eocd == (std::size_t)-1)
    {
        return -1L;
    }

    // total-entries-on-all-disks lives 10 bytes into the EOCD record
    return (long)archive_verify_internal::read_u16_le(_blob.data(), eocd + 10u);
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
    const byte_blob&         _blob,
    std::vector<unsigned int>& _out
)
{
    const char* in  = _blob.data();
    std::size_t n   = _blob.size();
    std::size_t pos = 0;

    _out.clear();

    while (pos + 4u <= n)
    {
        unsigned long sig = archive_verify_internal::read_u32_le(in, pos);

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
            unsigned int  method    = archive_verify_internal::read_u16_le(in, pos + 8u);
            unsigned long comp_size = archive_verify_internal::read_u32_le(in, pos + 18u);
            unsigned int  name_len  = archive_verify_internal::read_u16_le(in, pos + 26u);
            unsigned int  extra_len = archive_verify_internal::read_u16_le(in, pos + 28u);
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

// looks_like_gzip
//   function: check the two gzip magic bytes (0x1F 0x8B) plus the DEFLATE
// compression-method byte (0x08) that a standard gzip stream carries.
//
// Parameter(s):
//   _blob: the candidate stream bytes.
// Return:
//   true when _blob begins with a gzip / DEFLATE header.
D_INLINE bool
looks_like_gzip(
    const byte_blob& _blob
)
{
    const char* p = _blob.data();

    return ( (_blob.size() >= 3u)                &&
             ((unsigned char)p[0] == 0x1Fu)      &&
             ((unsigned char)p[1] == 0x8Bu)      &&
             ((unsigned char)p[2] == 0x08u) );
}

// tar_has_ustar_magic
//   function: check the "ustar" magic at offset 257 of the first 512-byte
// header, the marker of a POSIX ustar stream.
//
// Parameter(s):
//   _blob: the candidate archive bytes.
// Return:
//   true when the first header carries the ustar magic.
D_INLINE bool
tar_has_ustar_magic(
    const byte_blob& _blob
)
{
    const char* p = _blob.data();

    if (_blob.size() < 263u)
    {
        return false;
    }

    return ( (p[257] == 'u') &&
             (p[258] == 's') &&
             (p[259] == 't') &&
             (p[260] == 'a') &&
             (p[261] == 'r') );
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
    const byte_blob& _blob
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


///////////////////////////////////////////////////////////////////////////////
///                VI.  ROUND-TRIP DRIVERS                                   ///
///////////////////////////////////////////////////////////////////////////////

// roundtrip
//   function: archive _in in format _Format and immediately extract it back
// into _out.  The intermediate container bytes are discarded; callers that
// need them should drive try_archive / try_extract directly.
//
// Parameter(s):
//   _in:  the entries to archive.
//   _out: receives the extracted entries (only meaningful on status_ok).
//   _opt: creation options (defaults to the facade defaults).
// Return:
//   status_ok when both halves succeed, otherwise the first failing status
// (so an unavailable codec surfaces as status_unavailable, not a crash).
template<typename _Format>
status
roundtrip(
    const entry_list&      _in,
    entry_list&            _out,
    const archive_options& _opt = archive_options()
)
{
    byte_blob blob;
    status      s;

    s = try_archive<_Format>(_in, blob, _opt);

    if (s != status_ok)
    {
        return s;
    }

    return try_extract<_Format>(blob, _out);
}

// roundtrip_preserves_files
//   function: round-trip _in through _Format and confirm every regular file
// came back byte-identical (see files_preserved).  For the name-carrying
// formats; not meaningful for gzip, which drops names.
//
// Parameter(s):
//   _in:  the entries to archive.
//   _opt: creation options (defaults to the facade defaults).
// Return:
//   true when the round-trip succeeded and all file payloads were preserved.
template<typename _Format>
bool
roundtrip_preserves_files(
    const entry_list&      _in,
    const archive_options& _opt = archive_options()
)
{
    entry_list out;
    status     s;

    s = roundtrip<_Format>(_in, out, _opt);

    if (s != status_ok)
    {
        return false;
    }

    return files_preserved(_in, out);
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_ARCHIVE_
