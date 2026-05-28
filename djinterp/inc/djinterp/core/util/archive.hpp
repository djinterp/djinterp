/*******************************************************************************
* djinterp [core]                                                  archive.hpp
*
*   djinterp portable archive facade:
* A version-portable (C++98 - C++23), OS-cross-platform interface for creating
* and extracting archives. The user selects a format with a tag type and the
* call dispatches, at runtime, to whichever backend env_archive.h detected for
* this build (with built-in, dependency-free writers for tar / zip / gz so the
* common formats work anywhere zlib is present):
*
*     using namespace djinterp::formats;
*     byte_buffer blob = archive<zip>(entries);       // throwing
*     status s = try_archive<tar_gz>(entries, blob);   // non-throwing
*     entry_list items = extract<zip>(blob);
*
* As with compression.hpp, tag dispatch keeps the public surface identical
* across every C++ standard, and thin template entry points forward to
* non-template leaves defined in archive.cpp.
*
* formats:
*   zip, tar, gz, tar_gz, sevenzip (7z), rar
*
* availability:
*   each format maps to D_ENV_ARCHIVE_CAN_{READ,WRITE}_* capability flags.
*   Unavailable operations compile fine and return status_unavailable at
*   runtime; query at compile time with format_traits<Format>::can_{read,write}.
*   Note: RAR creation requires the proprietary rar/WinRAR tool; no library can
*   write RAR.
*
* path:      /inc/cpp/archive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.05.23
*******************************************************************************/

#ifndef DJINTERP_ARCHIVE_HPP_
#define DJINTERP_ARCHIVE_HPP_ 1

#include <cstddef>
#include <string>
#include <vector>
#include "./env_archive.h"
#include "./compression.hpp"


namespace djinterp
{

// =============================================================================
// I.   ARCHIVE ENTRY MODEL
// =============================================================================

// entry
//   struct: a single member of an archive. For creation, populate name and
// data (or set is_directory). For extraction, these fields are filled in.
struct entry
{
    // name: path within the archive; use '/' as the separator on all OSes.
    std::string  name;

    // data: file contents (ignored when is_directory is true).
    byte_buffer  data;

    // is_directory: true for a directory member with no data.
    bool         is_directory;

    // mode: unix permission bits (e.g. 0644). 0 selects a sensible default.
    unsigned int mode;

    // mtime: modification time as a unix epoch in seconds. 0 selects "now".
    long         mtime;

    entry()
        : is_directory(false),
          mode(0),
          mtime(0)
    {}
};

// entry_list
//   type: an ordered collection of archive entries.
typedef std::vector<entry> entry_list;


// =============================================================================
// II.  FORMAT TAGS AND IDENTIFIERS
// =============================================================================

namespace formats
{
    // zip
    //   tag: the ZIP container (store or deflate).
    struct zip
    {};

    // tar
    //   tag: uncompressed POSIX ustar.
    struct tar
    {};

    // gz
    //   tag: a single gzip stream. Holds exactly one logical file.
    struct gz
    {};

    // tar_gz
    //   tag: a tar stream wrapped in gzip (a.k.a. .tgz).
    struct tar_gz
    {};

    // sevenzip
    //   tag: the 7z container. Named sevenzip because 7z is not a valid
    // identifier.
    struct sevenzip
    {};

    // rar
    //   tag: the RAR container. Read-capable via several backends; creation
    // requires the proprietary rar/WinRAR tool.
    struct rar
    {};
}  // namespace formats

// format_id
//   enum: stable runtime identifier for an archive format.
enum format_id
{
    format_id_zip = 0,
    format_id_tar,
    format_id_gz,
    format_id_tar_gz,
    format_id_sevenzip,
    format_id_rar
};


// =============================================================================
// III. FORMAT TRAITS
// =============================================================================

// format_traits
//   trait: maps a format tag to its runtime id, read/write availability, and
// name. The primary template is left undefined so an unknown tag is a compile
// error. can_read / can_write are compile-time constants from env_archive.h.
template<typename _Format>
struct format_traits;

// format_traits<formats::zip>
//   trait: ZIP. A built-in writer/reader guarantees availability.
template<>
struct format_traits<formats::zip>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_ZIP  != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_ZIP != 0) };
    static format_id   id()   { return format_id_zip; }
    static const char* name() { return "zip"; }
};

// format_traits<formats::tar>
//   trait: ustar.
template<>
struct format_traits<formats::tar>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_TAR  != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_TAR != 0) };
    static format_id   id()   { return format_id_tar; }
    static const char* name() { return "tar"; }
};

// format_traits<formats::gz>
//   trait: single gzip stream.
template<>
struct format_traits<formats::gz>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_GZ  != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_GZ != 0) };
    static format_id   id()   { return format_id_gz; }
    static const char* name() { return "gz"; }
};

// format_traits<formats::tar_gz>
//   trait: gzip-wrapped tar.
template<>
struct format_traits<formats::tar_gz>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_GZ   != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_TGZ != 0) };
    static format_id   id()   { return format_id_tar_gz; }
    static const char* name() { return "tar.gz"; }
};

// format_traits<formats::sevenzip>
//   trait: 7z.
template<>
struct format_traits<formats::sevenzip>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_7Z  != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_7Z != 0) };
    static format_id   id()   { return format_id_sevenzip; }
    static const char* name() { return "7z"; }
};

// format_traits<formats::rar>
//   trait: RAR. write requires the rar/WinRAR tool.
template<>
struct format_traits<formats::rar>
{
    enum { can_read  = (D_ENV_ARCHIVE_CAN_READ_RAR  != 0) };
    enum { can_write = (D_ENV_ARCHIVE_CAN_WRITE_RAR != 0) };
    static format_id   id()   { return format_id_rar; }
    static const char* name() { return "rar"; }
};


// =============================================================================
// IV.  OPTIONS
// =============================================================================

// archive_options
//   struct: tuning knobs passed to a creation call.
struct archive_options
{
    // level: compression effort passed through to the codec. -1 = default.
    int  level;

    // store_only: for ZIP, store entries without DEFLATE even when a DEFLATE
    // backend is available. Ignored by other formats.
    bool store_only;

    archive_options()
        : level(-1),
          store_only(false)
    {}
};


// =============================================================================
// V.   DISPATCH LEAVES (defined in archive.cpp)
// =============================================================================

namespace internal
{
    // archive_create
    //   function: builds an archive of [_items, _items + _count) in format
    // _fmt into _out.
    status archive_create(format_id               _fmt,
                          const entry*             _items,
                          std::size_t              _count,
                          const archive_options&   _opt,
                          byte_buffer&             _out);

    // archive_extract
    //   function: extracts the archive in [_in, _in + _n) of format _fmt into
    // _out.
    status archive_extract(format_id     _fmt,
                          const char*     _in,
                          std::size_t     _n,
                          entry_list&     _out);

    // format_can_write / format_can_read
    //   function: runtime capability of a format id.
    bool format_can_write(format_id _fmt);
    bool format_can_read(format_id _fmt);
}  // namespace internal


// =============================================================================
// VI.  NON-THROWING TEMPLATE API
// =============================================================================

// try_archive
//   function: builds an archive of _items in format _Format into _out.
// returns a status; never throws.
template<typename _Format>
status
try_archive(
    const entry_list&       _items,
    byte_buffer&            _out,
    const archive_options&  _opt = archive_options()
)
{
    const entry* first;

    // an empty vector must not be indexed; pass a null base with count 0
    first = _items.empty() ? (const entry*)0 : &_items[0];

    return internal::archive_create(format_traits<_Format>::id(),
                                    first,
                                    _items.size(),
                                    _opt,
                                    _out);
}

// try_extract
//   function: extracts archive _in of format _Format into _out. returns a
// status; never throws.
template<typename _Format>
status
try_extract(
    const byte_buffer&  _in,
    entry_list&         _out
)
{
    return internal::archive_extract(format_traits<_Format>::id(),
                                     _in.data(),
                                     _in.size(),
                                     _out);
}

// format_is_writable / format_is_readable
//   function: runtime capability query for format _Format.
template<typename _Format>
bool
format_is_writable()
{
    return internal::format_can_write(format_traits<_Format>::id());
}

template<typename _Format>
bool
format_is_readable()
{
    return internal::format_can_read(format_traits<_Format>::id());
}


// =============================================================================
// VII. THROWING CONVENIENCE API
// =============================================================================

#if D_ENV_COMPRESSION_HAS_EXCEPTIONS

// archive_error
//   class: exception thrown by the convenience API on failure.
class archive_error : public std::runtime_error
{
public:
    explicit archive_error(const std::string& _what)
        : std::runtime_error(_what)
    {}
};

// archive
//   function: builds and returns an archive of _items in format _Format.
// throws archive_error on failure.
template<typename _Format>
byte_buffer
archive(
    const entry_list&       _items,
    const archive_options&  _opt = archive_options()
)
{
    byte_buffer out;
    status      s;

    s = try_archive<_Format>(_items, out, _opt);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw archive_error(std::string("archive<")
                            + format_traits<_Format>::name()
                            + ">: "
                            + status_message(s));
    }

    return out;
}

// extract
//   function: extracts and returns the members of archive _in in format
// _Format. throws archive_error on failure.
template<typename _Format>
entry_list
extract(
    const byte_buffer&  _in
)
{
    entry_list out;
    status     s;

    s = try_extract<_Format>(_in, out);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw archive_error(std::string("extract<")
                            + format_traits<_Format>::name()
                            + ">: "
                            + status_message(s));
    }

    return out;
}

#endif  // D_ENV_COMPRESSION_HAS_EXCEPTIONS

}  // namespace djinterp


#endif  // DJINTERP_ARCHIVE_HPP_
