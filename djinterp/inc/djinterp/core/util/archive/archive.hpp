/*******************************************************************************
* djinterp [core]                                                    archive.hpp
*
*   djinterp portable archive facade:
* A version-portable (C++98 - C++23), OS-cross-platform interface for creating
* and extracting archives. The user selects a format with a tag type and the
* call dispatches, at runtime, to whichever backend env_archive.h detected for
* this build (with built-in, dependency-free writers for tar / zip / gz so the
* common formats work anywhere zlib is present):
*
*     using namespace djinterp::formats;
*     byte_blob blob = archive<zip>(entries);       // throwing
*     status s = try_archive<tar_gz>(entries, blob);   // non-throwing
*     entry_list items = extract<zip>(blob);
*
* As with compress.hpp, tag dispatch keeps the public surface identical
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
*
*   RE-BASED ONTO THE SHARED KERNEL (2026-07-30)
*   ============================================
*   The public surface is unchanged.  `format_id`, the entry model and the
* container options now come from archive_common.h, which both languages
* compile, instead of being declared here in parallel with the C fork's copies.
* format_id's values are identical to before (zip = 0 through rar = 5) and are
* now pinned explicitly in the kernel -- test_packaging.hpp records that DTest's
* own format enum orders gz and tar_gz differently, so the bridge between them
* must stay a switch and must never become a cast.
*
*   archive.cpp no longer contains backend code.  What remains is four adapter
* functions over the kernel, which also carries dependency-free ustar and ZIP
* writers AND readers -- so tar and zip now work on a build with no third-party
* archive library at all, rather than depending on which headers happen to be
* installed.
*
*   ONE BEHAVIOURAL CHANGE A CALLER WILL SEE
*   ----------------------------------------
*   `entry::mtime == 0` used to mean "now".  It now means a pinned epoch
* (1980-01-01, the earliest instant both tar and zip represent).  Reading the
* clock makes two runs over identical entries produce different bytes, which
* defeats the differential test that compares a C archive against a C++ one --
* the reason this facade was re-based in the first place.  A caller who wants
* the current time now passes it explicitly, which is also the only way to say
* so unambiguously.
*
*
* path:      /inc/djinterp/core/util/archive/archive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_UTIL_ARCHIVE_ARCHIVE_HPP
#define DJINTERP_UTIL_ARCHIVE_ARCHIVE_HPP 1

// A test build that defines DTEST_PACK_USE_FACADE_DOUBLE gets the recording
// double instead of this facade, wherever this header is reached, so a unit
// never holds both (the owner's ruling of 2026.10.02, lane 4's Q4). The
// double keeps its own path-derived guard: the guard rule has no exceptions.
#ifdef DTEST_PACK_USE_FACADE_DOUBLE
    #include "../../../test/pack_facade/archive.hpp"  // the double
#else

// std
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/util/archive/env_archive.h"
#include "../../../env/util/compress/env_compress.h"  // D_ENV_COMPRESSION_*
#include "../../../c/util/archive/archive_common.h"       // format_id, the entry model, transforms
#include "./archive_options.hpp"    // archive_options (full, format-aware)
#include "../compress/compress.hpp"




NS_DJINTERP

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
    byte_blob  data;

    // is_directory: true for a directory member with no data.
    bool         is_directory;

    // mode: unix permission bits (e.g. 0644). 0 selects a sensible default.
    unsigned int mode;

    // mtime: modification time as a unix epoch in seconds. 0 selects a pinned
    // epoch (1980-01-01), NOT the current time -- see the banner. Pass an
    // explicit value to record a real timestamp.
    //   Widened to int64_t on the way into the kernel: `long` is 32 bits on
    // LLP64, which would put the 2038 boundary inside the framework and give
    // two platforms different representations of one declaration.
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
//   type: stable runtime identifier for an archive format.  An ALIAS of the
// kernel's enum, not a second declaration -- so a format crossing the language
// boundary needs no translation and cannot be translated wrongly.  Values are
// unchanged: zip = 0, tar = 1, gz = 2, tar_gz = 3, sevenzip = 4, rar = 5.
typedef enum d_format_id format_id;

D_STATIC const format_id format_id_zip      = D_FORMAT_ID_ZIP;
D_STATIC const format_id format_id_tar      = D_FORMAT_ID_TAR;
D_STATIC const format_id format_id_gz       = D_FORMAT_ID_GZ;
D_STATIC const format_id format_id_tar_gz   = D_FORMAT_ID_TAR_GZ;
D_STATIC const format_id format_id_sevenzip = D_FORMAT_ID_SEVENZIP;
D_STATIC const format_id format_id_rar      = D_FORMAT_ID_RAR;


// =============================================================================
// III. FORMAT TRAITS
// =============================================================================

// format_traits
//   trait: maps a format tag to its runtime id, read/write availability, and
// name. The primary template is left undefined so an unknown tag is a compile
// error. can_read / can_write are compile-time constants from env_archive.h.
template<typename Format>
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
//   struct: tuning knobs passed to a creation call.  The full, format-aware
// definition lives in archive_options.hpp, included above.
//
//   Unlike compress_options -- which now DERIVES from its kernel struct at zero
// cost -- archive_options is a parallel shape with an explicit lowering,
// because five of its knobs are text and the C++ vocabulary for text is
// std::string while the kernel's is a borrowed span.  That asymmetry is deliberate and its
// cost is recorded in archive_options.hpp; read that note before editing either
// field list.


// =============================================================================
// V.   DISPATCH LEAVES (defined in archive.cpp)
// =============================================================================

//   These are now ADAPTERS, not implementations: each lowers the C++ types to
// the kernel's borrowed forms and calls in.  No format knowledge remains in
// archive.cpp.
NS_INTERNAL
    // archive_create
    //   function: builds an archive of [_items, _items + _count) in format
    // _fmt into _out.
    status archive_create(format_id               _fmt,
                          const entry*            _items,
                          std::size_t             _count,
                          const archive_options&  _opt,
                          byte_blob&            _out);

    // archive_extract
    //   function: extracts the archive in [_in, _in + _n) of format _fmt into
    // _out.
    status archive_extract(format_id   _fmt,
                           const char* _in,
                           std::size_t _n,
                           entry_list& _out);

    // format_can_write / format_can_read
    //   function: runtime capability of a format id.
    bool format_can_write(format_id _fmt);
    bool format_can_read(format_id _fmt);
NS_END  // namespace internal


// =============================================================================
// VI.  NON-THROWING TEMPLATE API
// =============================================================================

// try_archive
//   function: builds an archive of _items in format Format into _out.
// returns a status; never throws.
template<typename Format>
status
try_archive(
    const entry_list&      _items,
    byte_blob&           _out,
    const archive_options& _opt = archive_options()
)
{
    const entry* first;

    // an empty vector must not be indexed; pass a null base with count 0
    first = _items.empty() ? (const entry*)0 : &_items[0];

    return internal::archive_create(format_traits<Format>::id(),
                                    first,
                                    _items.size(),
                                    _opt,
                                    _out);
}

// try_extract
//   function: extracts archive _in of format Format into _out. returns a
// status; never throws.
template<typename Format>
status
try_extract(
    const byte_blob&  _in,
    entry_list&         _out
)
{
    return internal::archive_extract(format_traits<Format>::id(),
                                     _in.data(),
                                     _in.size(),
                                     _out);
}

// format_is_writable / format_is_readable
//   function: runtime capability query for format Format.
template<typename Format>
bool
format_is_writable()
{
    return internal::format_can_write(format_traits<Format>::id());
}

template<typename Format>
bool
format_is_readable()
{
    return internal::format_can_read(format_traits<Format>::id());
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
//   function: builds and returns an archive of _items in format Format.
// throws archive_error on failure.
template<typename Format>
byte_blob
archive(
    const entry_list&       _items,
    const archive_options&  _opt = archive_options()
)
{
    byte_blob out;
    status      s;

    s = try_archive<Format>(_items, out, _opt);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw archive_error(std::string("archive<")
                            + format_traits<Format>::name()
                            + ">: "
                            + status_message(s));
    }

    return out;
}

// extract
//   function: extracts and returns the members of archive _in in format
// Format. throws archive_error on failure.
template<typename Format>
entry_list
extract(
    const byte_blob&  _in
)
{
    entry_list out;
    status     s;

    s = try_extract<Format>(_in, out);

    // raise on any non-success status
    if (s != status_ok)
    {
        throw archive_error(std::string("extract<")
                            + format_traits<Format>::name()
                            + ">: "
                            + status_message(s));
    }

    return out;
}

#endif  // D_ENV_COMPRESSION_HAS_EXCEPTIONS


NS_END  // djinterp


#endif  // DTEST_PACK_USE_FACADE_DOUBLE

#endif  // DJINTERP_UTIL_ARCHIVE_ARCHIVE_HPP
