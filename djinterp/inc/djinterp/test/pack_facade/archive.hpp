/******************************************************************************
* djinterp [test]                                     pack_facade/archive.hpp
*
* Instrumented drop-in for core/util/archive.hpp, used by the test_pack suite:
*   The archive-side companion to pack_facade/compress.hpp.  It shadows the
* production archive facade when a test build defines DTEST_PACK_USE_FACADE_DOUBLE
* and puts this directory on the include path ahead of the real tree.  It reuses
* the production include guard (DJINTERP_ARCHIVE_), claimed at the top before any
* nested include, so a stray include of the real header collapses to a no-op.
*
*   It reproduces exactly the surface test_pack.hpp consumes -- entry,
* entry_list, the formats:: tags, try_archive<>, and format_is_writable<> -- but
* RECORDS rather than builds an archive: try_archive<Tag> writes the observable
* line
*
*       A|<tag>|<count>|<first entry name>
*
* into _out and returns fmt_stat<Tag>(), a per-format status slot the tests
* steer.  byte_blob and status come from the sibling compress double, as in
* the production tree where archive.hpp includes compress.hpp.
*
*   The recorded tag ids are the ENUM-STYLE names the tests expect -- note
* "tar_gz" and "sevenzip", which deliberately differ from the production
* format_traits::name() ("tar.gz" and "7z"): the double records the selector's
* identity, not the on-disk extension.
*
* THE HOOKS:
*   - fmt_stat<Tag>()       a mutable status slot per format, status_ok by
*                           default; assign it to force a format's outcome.
*   - reset_format_hooks()  restores every slot to status_ok.
*
* PORTABILITY:
*   Only <string> / <vector> / <sstream> and the env-gated djinterp core are
* pulled in, so the double compiles in every language mode env.h reports
* (C++98 onward); the entry count is rendered through std::ostringstream.
*
* path:      /tests/djinterp/test/pack_facade/archive.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
******************************************************************************/

#ifndef DJINTERP_ARCHIVE_
#define DJINTERP_ARCHIVE_ 1

// std
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
// djinterp  -- angle-bracket paths so the double is location-independent; the
// sibling compress double is reached by basename (it sits next to this file).
#include <djinterp/core/djinterp.hpp>              // NS_*, D_INLINE
#include <djinterp/core/util/archive_options.hpp>  // archive_options
#include "compress.hpp"                            // byte_blob, status, tags


NS_DJINTERP

// =============================================================================
// I.   ARCHIVE ENTRY MODEL
// =============================================================================

// entry
//   struct: a single member of an archive.  The recorder reads only name /
// data / is_directory; the remaining production fields are carried so the
// surface matches and callers compile unchanged.
struct entry
{
    // name: path within the archive.
    std::string  name;

    // data: file contents (ignored when is_directory is true).
    byte_blob  data;

    // is_directory: true for a directory member with no data.
    bool         is_directory;

    // mode: unix permission bits; 0 selects a default.
    unsigned int mode;

    // mtime: modification time as a unix epoch in seconds; 0 selects "now".
    long         mtime;

    entry()
        : is_directory(false),
          mode(0),
          mtime(0)
    {
    }
};

// entry_list
//   type: an ordered collection of archive entries.
typedef std::vector<entry> entry_list;


// =============================================================================
// II.  FORMAT TAGS
// =============================================================================

namespace formats
{
    // zip
    //   tag: ZIP container.
    struct zip
    {
    };

    // tar
    //   tag: ustar container.
    struct tar
    {
    };

    // gz
    //   tag: single gzip stream.
    struct gz
    {
    };

    // tar_gz
    //   tag: gzip-wrapped tar.
    struct tar_gz
    {
    };

    // sevenzip
    //   tag: 7z container.
    struct sevenzip
    {
    };

    // rar
    //   tag: RAR container.
    struct rar
    {
    };
}  // namespace formats


// =============================================================================
// II-b. RUNTIME FORMAT ID
// =============================================================================

// format_id
//   enum: the runtime format selector the production facade's dispatch leaves
// (internal::archive_create / internal::archive_extract) take.  The
// document_bundle / output_packaging layer chooses a container at RUNTIME
// through this id rather than through the compile-time formats:: tags, so the
// double mirrors the production enum here.  The enumerator order matches the
// production archive.hpp -- NOTE gz precedes tar_gz, which is exactly the
// ordinal mismatch against test_archive_format that test_output_config guards
// against with a by-value switch.  Plain enum, C++98, as in the real facade.
enum format_id
{
    format_id_zip = 0,   // formats::zip
    format_id_tar,       // formats::tar
    format_id_gz,        // formats::gz
    format_id_tar_gz,    // formats::tar_gz
    format_id_sevenzip,  // formats::sevenzip
    format_id_rar        // formats::rar
};


// =============================================================================
// III. RECORDING HELPERS (internal)
// =============================================================================

NS_INTERNAL

    // format_label
    //   trait: maps a format tag to the short id string the recorder emits (one
    // trivial specialization per tag).  These are the enum-style ids the tests
    // expect ("tar_gz", "sevenzip"), NOT the production format_traits::name()
    // spellings ("tar.gz", "7z").  The primary is undefined so an unknown tag is
    // a compile error.
    template<typename _Format>
    struct format_label;

    template<> struct format_label<formats::zip>      { static const char* name() { return "zip";      } };
    template<> struct format_label<formats::tar>      { static const char* name() { return "tar";      } };
    template<> struct format_label<formats::gz>       { static const char* name() { return "gz";       } };
    template<> struct format_label<formats::tar_gz>   { static const char* name() { return "tar_gz";   } };
    template<> struct format_label<formats::sevenzip> { static const char* name() { return "sevenzip"; } };
    template<> struct format_label<formats::rar>      { static const char* name() { return "rar";      } };

NS_END  // internal


// =============================================================================
// IV.  STATUS HOOKS
// =============================================================================

// fmt_stat
//   function: the mutable status slot for format _Format, status_ok by default.
// try_archive<_Format> and format_is_writable<_Format> both read it, and a test
// writes it to force a format's outcome.  Each format type owns exactly one slot
// for the life of the program.
template<typename _Format>
status&
fmt_stat()
{
    static status s = status_ok;

    return s;
}

// reset_format_hooks
//   function: restore every format's status slot to status_ok, undoing any
// forcing an earlier test performed.
// Return:
//   none.
D_INLINE void
reset_format_hooks()
{
    fmt_stat<formats::zip>()      = status_ok;
    fmt_stat<formats::tar>()      = status_ok;
    fmt_stat<formats::gz>()       = status_ok;
    fmt_stat<formats::tar_gz>()   = status_ok;
    fmt_stat<formats::sevenzip>() = status_ok;
    fmt_stat<formats::rar>()      = status_ok;

    return;
}


// =============================================================================
// V.   RECORDING API
// =============================================================================

// try_archive
//   function: RECORDS an archive request instead of building it.  On a
// status_ok slot it writes "A|<tag>|<count>|<first entry name>" into _out;
// otherwise it empties _out.  The container knob-set is accepted but not part of
// the recorded surface.  Returns fmt_stat<_Format>() and never throws.
template<typename _Format>
status
try_archive(
    const entry_list&       _items,
    byte_blob&            _out,
    const archive_options&  _opt = archive_options()
)
{
    status             s;
    std::ostringstream rec;

    // the knob-set is deliberately unrecorded; silence the unused-parameter
    // warning without naming it in the trace
    (void)_opt;

    s = fmt_stat<_Format>();

    // a failed format records nothing and leaves _out empty
    if (s != status_ok)
    {
        _out.clear();

        return s;
    }

    rec << "A|"
        << internal::format_label<_Format>::name()
        << "|" << _items.size()
        << "|" << ( _items.empty() ? std::string() : _items[0].name );
    _out = rec.str();

    return s;
}

// format_is_writable
//   function: reports a format writable exactly when its status slot is
// status_ok, so the availability routers are exercised through the same hook.
template<typename _Format>
bool
format_is_writable()
{
    return (fmt_stat<_Format>() == status_ok);
}


// =============================================================================
// VI.  RUNTIME DISPATCH LEAVES (recording)
// =============================================================================
// The production facade routes runtime format_id selections through these
// internal leaves; document_bundle::dispatch_archive calls archive_create
// directly.  As on the compress side, the output / document_bundle suites
// compile against this double, so it must carry these leaves too.  archive_create
// RECORDS the SAME observable line try_archive emits
// ("A|<id>|<count>|<first entry name>") and routes through the SAME per-format
// status slot, so runtime and tag dispatch of one format compare identically and
// fmt_stat<> / reset_format_hooks() steer both.

NS_INTERNAL

    // format_id_label
    //   helper: the short id string for a runtime format_id (the enum-style ids
    // the tests expect: "tar_gz", "sevenzip"), mirroring format_label<Tag>.
    D_INLINE const char*
    format_id_label(
        format_id _format
    )
    {
        switch (_format)
        {
            case format_id_zip:      { return "zip";      }
            case format_id_tar:      { return "tar";      }
            case format_id_gz:       { return "gz";       }
            case format_id_tar_gz:   { return "tar_gz";   }
            case format_id_sevenzip: { return "sevenzip"; }
            case format_id_rar:      { return "rar";      }
            default:                 { return "?";        }
        }
    }

    // format_id_stat
    //   helper: routes a runtime format_id to the SAME status slot the tag API
    // uses, so reset_format_hooks() and fmt_stat<Tag>() steer runtime dispatch
    // as well.
    D_INLINE status&
    format_id_stat(
        format_id _format
    )
    {
        switch (_format)
        {
            case format_id_tar:      { return fmt_stat<formats::tar>();      }
            case format_id_gz:       { return fmt_stat<formats::gz>();       }
            case format_id_tar_gz:   { return fmt_stat<formats::tar_gz>();   }
            case format_id_sevenzip: { return fmt_stat<formats::sevenzip>(); }
            case format_id_rar:      { return fmt_stat<formats::rar>();      }
            case format_id_zip:
            default:                 { return fmt_stat<formats::zip>();      }
        }
    }

    // archive_create
    //   function: the runtime-dispatch archive-creation leaf.  RECORDS
    // "A|<id>|<count>|<first entry name>" into _out on a status_ok slot; empties
    // _out and returns the forced status otherwise.  A null base is valid only
    // at count 0.  The signature mirrors the production internal::archive_create
    // exactly.
    D_INLINE status
    archive_create(
        format_id               _format,
        const entry*            _items,
        std::size_t             _count,
        const archive_options&  _opt,
        byte_blob&            _out
    )
    {
        status s;

        // the container knob-set is deliberately unrecorded, as in try_archive
        (void)_opt;

        // a null base is only valid when there are no items
        if ( (_items == 0) &&
             (_count != 0) )
        {
            _out.clear();

            return status_invalid_argument;
        }

        s = format_id_stat(_format);

        // a failed format records nothing and leaves _out empty
        if (s != status_ok)
        {
            _out.clear();

            return s;
        }

        {
            std::ostringstream rec;

            rec << "A|"
                << format_id_label(_format)
                << "|" << _count
                << "|" << ( ( _items && (_count != 0) )
                                ? _items[0].name
                                : std::string() );
            _out = rec.str();
        }

        return s;
    }

    // archive_extract
    //   function: the runtime-dispatch extraction leaf.  On a status_ok slot it
    // yields a single synthetic entry whose name is the format id and whose data
    // is the input blob, so a caller can observe the request; empties _out
    // otherwise.  The signature mirrors the production internal::archive_extract
    // exactly.
    D_INLINE status
    archive_extract(
        format_id     _format,
        const char*   _in,
        std::size_t   _n,
        entry_list&   _out
    )
    {
        status s;

        if ( (_in == 0) &&
             (_n  != 0) )
        {
            _out.clear();

            return status_invalid_argument;
        }

        s = format_id_stat(_format);

        if (s != status_ok)
        {
            _out.clear();

            return s;
        }

        {
            entry e;

            e.name = format_id_label(_format);
            e.data = ( _in ? byte_blob(_in, _n) : byte_blob() );

            _out.clear();
            _out.push_back(e);
        }

        return s;
    }

    // format_can_write / format_can_read
    //   function: runtime capability of a format id.  In the double a format is
    // both readable and writable exactly when its status slot is status_ok (the
    // recorder keeps a single slot per format), so reset_format_hooks() and
    // fmt_stat<Tag>() steer capability queries and dispatch alike.  Mirror the
    // production internal::format_can_{write,read}.
    D_INLINE bool
    format_can_write(
        format_id _format
    )
    {
        return (format_id_stat(_format) == status_ok);
    }

    D_INLINE bool
    format_can_read(
        format_id _format
    )
    {
        return (format_id_stat(_format) == status_ok);
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_ARCHIVE_
