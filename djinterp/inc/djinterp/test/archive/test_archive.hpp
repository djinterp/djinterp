/*******************************************************************************
* djinterp [test]                                               test_archive.hpp
*
*   Shared, DEPENDENCY-LIGHT verification helpers for the archive facade
* (core/util/archive/archive.hpp).  It exists so that any test suite whose module
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
*   Target floor C++98, effective floor C++11 -- djinterp.hpp gates below it,
* so no includer compiles at C++98 today.  No third-party include.
*   Availability-aware by
* construction: the round-trip drivers report whatever status the facade
* returns, so on a build lacking a codec (gz unavailable, say) a caller sees
* status_unavailable rather than a spurious failure.
*
*
* path:      /inc/djinterp/test/archive/test_archive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL: LITTLE-ENDIAN READ HELPERS
      ------------------------------------

II.   ENTRY CONSTRUCTION
      ------------------

III.  ENTRY-LIST INSPECTION
      ---------------------

IV.   PAYLOAD COMPARISON
      ------------------

V.    FORMAT SNIFFERS (zip / gzip / tar shape + member counts)
      --------------------------------------------------------

VI.   ROUND-TRIP DRIVERS
      ------------------
*/

#ifndef DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_HPP
#define DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP11_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/util/archive/archive.hpp"   // entry, entry_list, byte_blob, formats
#include "../../core/util/archive/archive_detect.hpp"   // the recognition rules


NS_DJINTERP
NS_TEST

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

//   The format sniffers moved to core/util/archive/archive_detect.hpp, where
// container recognition sits next to the format vocabulary.  Re-exported here
// so every dt:: call site keeps compiling.
//
//   Two of them are NOT pure forwards to the kernel, and the detect header
// says why: looks_like_zip keeps its end-of-central-directory requirement (the
// kernel answers on the leading four bytes, so it accepts a truncated zip),
// and tar_has_ustar_magic keeps asking only about the magic (the kernel wants
// a full 512-byte record before it will answer for tar).  Both contracts are
// the ones callers have been asserting against.

using ::djinterp::looks_like_zip;
using ::djinterp::looks_like_gzip;
using ::djinterp::tar_has_ustar_magic;
using ::djinterp::tar_is_terminated;
using ::djinterp::zip_find_eocd;
using ::djinterp::zip_total_entries;
using ::djinterp::zip_local_methods;
using ::djinterp::format_signature_matches;
using ::djinterp::format_signature_length;
using ::djinterp::detect_format;


///////
///                VI.  ROUND-TRIP DRIVERS                                   ///
///////////////////////////////////////////////////////////////////////////////

// roundtrip
//   function: archive _in in format Format and immediately extract it back
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
template<typename Format>
status
roundtrip(
    const entry_list&      _in,
    entry_list&            _out,
    const archive_options& _opt = archive_options()
)
{
    byte_blob blob;
    status      s;

    s = try_archive<Format>(_in, blob, _opt);

    if (s != status_ok)
    {
        return s;
    }

    return try_extract<Format>(blob, _out);
}

// roundtrip_preserves_files
//   function: round-trip _in through Format and confirm every regular file
// came back byte-identical (see files_preserved).  For the name-carrying
// formats; not meaningful for gzip, which drops names.
//
// Parameter(s):
//   _in:  the entries to archive.
//   _opt: creation options (defaults to the facade defaults).
// Return:
//   true when the round-trip succeeded and all file payloads were preserved.
template<typename Format>
bool
roundtrip_preserves_files(
    const entry_list&      _in,
    const archive_options& _opt = archive_options()
)
{
    entry_list out;
    status     s;

    s = roundtrip<Format>(_in, out, _opt);

    if (s != status_ok)
    {
        return false;
    }

    return files_preserved(_in, out);
}


NS_END  // test
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_TEST_ARCHIVE_TEST_ARCHIVE_HPP
