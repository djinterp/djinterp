/******************************************************************************
* djinterp [utility]                                        output_packaging.hpp
*
*   The runtime packaging vocabulary the document-output stack consumes:
* document_bundle collects a run's documents and drives them through this
* header's output_config to a sink.  Content production (state -> byte_blob)
* and assembly (bytes -> files / archive) are the two composing halves of the
* pipeline; this header owns the CONFIGURATION and DESTINATION vocabulary that
* sits between them:
*
*     DTest state
*        -> [ binding_env + format skeleton ]   one byte_blob per document
*        -> [ document_bundle: collate + name ]
*        -> [ output_packaging: pack_mode + output_config + sink ]   THIS LAYER
*        -> sink (disk or in-memory buffer)
*
*   WHAT THIS HEADER OWNS (and what it does NOT):
*   It defines pack_mode (none / compress / archive), output_config (the whole
* packaging decision), the output_sink hierarchy, and the naming / suffix
* helpers.  It does NOT define codec_id / format_id or the dispatch leaves -
* those are the compression and archive FACADES' vocabulary; this header pulls
* them in from compress.hpp / archive.hpp and re-uses them, so there is exactly
* one definition of each and no risk of a redefinition when a translation unit
* includes both this header and a facade.  (The facades own codec_id / format_id
* precisely because compress.cpp / archive.cpp - which are C++98 - dispatch on
* them; keeping them there is what lets the facades stay below the C++11 floor
* while this header stays at C++17.)
*
*   THE MAPPINGS ARE NOT CASTS:
*   codec_id / format_id here are the facades' own; the test layer's
* test_compressor / test_archive_format map INTO them by value (see
* test_output_config.hpp), never by static_cast - tar_gz / gz sit at different
* ordinals in the two enums.
*
*   PORTABILITY:
*   C++17 (output_config's naming policy is std::function, pack_mode is a scoped
* enum); self-suppresses below the floor, exactly as document_bundle does, so a
* pre-C++17 translation unit that reaches this header contributes nothing rather
* than failing to compile.
*
*
* TABLE OF CONTENTS
* =================
* I.    PACK MODE               (pack_mode)
* II.   NAMING & SUFFIX HELPERS (codec_suffix / format_extension /
*                                default_base_name)
* III.  OUTPUT CONFIG           (output_config)
* IV.   OUTPUT SINKS            (output_sink / disk_output_sink /
*                                buffer_output_sink)
*
*
* path:      /inc/djinterp/core/util/output/output_packaging.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_UTILITY_OUTPUT_PACKAGING_
#define DJINTERP_UTILITY_OUTPUT_PACKAGING_ 1

// std
#include <cstddef>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
// djinterp
#include "../../djinterp.hpp"        // NS_*, D_NODISCARD, D_NOEXCEPT, gates
#include "../compress.hpp"           // byte_blob, status, codec_id, leaves
#include "../archive.hpp"            // entry, entry_list, format_id, leaves
#include "../compress_options.hpp"   // compress_options
#include "../archive_options.hpp"    // archive_options


// output_packaging is the C++17 configuration face over the C++98 facades;
// below the floor it contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   PACK MODE                                            ///
///////////////////////////////////////////////////////////////////////////////

// pack_mode
//   enum: how a run's documents are packaged on the way to the sink.  A scoped
// enum so none / compress / archive do NOT leak into djinterp:: (an unscoped
// `none` would collide with other enumerators in the namespace).
enum class pack_mode
{
    none,       // emit each document verbatim
    compress,   // compress each document with output_config::codec
    archive     // bundle every document into one output_config::format container
};


///////////////////////////////////////////////////////////////////////////////
///                II.  NAMING & SUFFIX HELPERS                              ///
///////////////////////////////////////////////////////////////////////////////

// codec_suffix
//   function: the filename suffix appended to a document compressed with _c in
// `compress` mode (store adds nothing; the rest add their conventional
// extension).  This is the on-disk suffix, distinct from the facade's internal
// codec_id_label recorder id.
D_NODISCARD inline std::string
codec_suffix(
    codec_id _c
)
{
    switch (_c)
    {
        case codec_id_store:   { return std::string(); }
        case codec_id_deflate: { return ".deflate";    }
        case codec_id_zlib:    { return ".zz";         }
        case codec_id_gzip:    { return ".gz";         }
        case codec_id_bzip2:   { return ".bz2";        }
        case codec_id_xz:      { return ".xz";         }
        case codec_id_zstd:    { return ".zst";        }
        case codec_id_lz4:     { return ".lz4";        }
        case codec_id_brotli:  { return ".br";         }
        default:               { return std::string(); }
    }
}


// format_extension
//   function: the on-disk extension for an archive container of format _f, used
// to name the single container write in `archive` mode.
D_NODISCARD inline std::string
format_extension(
    format_id _f
)
{
    switch (_f)
    {
        case format_id_zip:      { return ".zip";        }
        case format_id_tar:      { return ".tar";        }
        case format_id_gz:       { return ".gz";         }
        case format_id_tar_gz:   { return ".tar.gz";     }
        case format_id_sevenzip: { return ".7z";         }
        case format_id_rar:      { return ".rar";        }
        default:                 { return std::string(); }
    }
}


// default_base_name
//   function: the filename for one document when output_config carries no
// naming policy.  A lone document is just <name><ext>; when a run emits several,
// the 0-based index is folded in (<name>_<index><ext>) so the writes do not
// collide.
D_NODISCARD inline std::string
default_base_name(
    const std::string&  _name,
    const std::string&  _ext,
    std::size_t         _index,
    std::size_t         _total
)
{
    if (_total <= std::size_t(1))
    {
        return _name + _ext;
    }

    {
        std::ostringstream _oss;

        _oss << _name << "_" << _index << _ext;

        return _oss.str();
    }
}


///////////////////////////////////////////////////////////////////////////////
///                III. OUTPUT CONFIG                                        ///
///////////////////////////////////////////////////////////////////////////////

// output_config
//   struct: the complete packaging decision document_bundle::write() consumes.
// pack selects the mode; codec / compress_opts tune `compress` mode; format /
// archive_opts tune `archive` mode; naming overrides the per-document filename
// policy (unset => default_base_name); archive_name is the container's base name
// in `archive` mode (format_extension supplies its extension).
struct output_config
{
    // naming_fn
    //   type: the per-document filename policy - (logical name, extension,
    // 0-based index, document count) -> filename.  A std::function for parity
    // with the rest of the document stack; an empty policy selects
    // default_base_name.
    typedef std::function<std::string(const std::string&,
                                      const std::string&,
                                      std::size_t,
                                      std::size_t)> naming_fn;

    // pack: none / compress / archive.
    pack_mode         pack;

    // codec: the codec used in `compress` mode.
    codec_id          codec;

    // format: the container used in `archive` mode.
    format_id         format;

    // compress_opts: codec tuning for `compress` mode.
    compress_options  compress_opts;

    // archive_opts: container tuning for `archive` mode.
    archive_options   archive_opts;

    // naming: per-document filename policy; empty => default_base_name.
    naming_fn         naming;

    // archive_name: the container's base name in `archive` mode.
    std::string       archive_name;

    // output_config
    //   constructor: no packaging by default, with the neutral gzip / zip
    // selections primed for when a caller flips pack to compress / archive.
    output_config()
        : pack(pack_mode::none),
          codec(codec_id_gzip),
          format(format_id_zip),
          archive_name("archive")
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  OUTPUT SINKS                                         ///
///////////////////////////////////////////////////////////////////////////////

// output_sink
//   class: the destination write() hands packaged bytes to.  A single virtual,
// write_item(name, bytes) -> true on success; a custom destination is a subclass
// (that is the whole extension surface document_bundle documents as
// DTEST-agnostic).
class output_sink
{
public:
    // ~output_sink
    //   destructor: virtual so subclasses destruct through a base handle.
    virtual
    ~output_sink()
    {
    }

    // write_item
    //   accepts one named payload; returns true iff it was written.  In `none` /
    // `compress` mode this is called once per document; in `archive` mode once,
    // with the lone container.
    D_NODISCARD virtual bool
    write_item(
        const std::string&  _name,
        const byte_blob&  _bytes
    ) = 0;
};


// disk_output_sink
//   class: writes each payload to a file whose path is chosen by a caller-
// supplied policy (filename -> destination path).  The bytes are written binary
// and the file is truncated; a failure to open or write is reported as false so
// write() can abort the run.
class disk_output_sink : public output_sink
{
public:
    // path_fn
    //   type: maps a payload's filename to the destination path on disk.
    typedef std::function<std::string(const std::string&)> path_fn;

    // disk_output_sink
    //   constructor: writes through _path.  When _path is empty the filename is
    // used as the path unchanged.
    explicit
    disk_output_sink(
        path_fn _path
    )
        : m_path(static_cast<path_fn&&>(_path))
    {
    }

    // write_item
    //   opens m_path(_name) (or _name when no policy is set) binary+truncating
    // and writes _bytes; returns false on any open / write failure.
    bool
    write_item(
        const std::string&  _name,
        const byte_blob&  _bytes
    ) override
    {
        const std::string _dest = m_path ? m_path(_name) : _name;

        std::ofstream _os(_dest.c_str(),
                          std::ios::out | std::ios::binary | std::ios::trunc);

        if (!_os.is_open())
        {
            return false;
        }

        if (!_bytes.empty())
        {
            _os.write(_bytes.data(),
                      static_cast<std::streamsize>(_bytes.size()));
        }

        return static_cast<bool>(_os);
    }

private:
    path_fn m_path;
};


// buffer_output_sink
//   class: concatenates every payload into one in-memory byte_blob, placing
// an optional separator between successive payloads (relevant only in `none` /
// `compress` mode; `archive` mode delivers a single payload).  Names are
// ignored.  Always succeeds.
class buffer_output_sink : public output_sink
{
public:
    // buffer_output_sink
    //   constructor: appends into _out, separating successive payloads with
    // _separator (empty => none).
    buffer_output_sink(
        byte_blob&        _out,
        const std::string&  _separator = std::string()
    )
        : m_out(_out),
          m_separator(_separator),
          m_count(0)
    {
    }

    // write_item
    //   appends _bytes to the target buffer (after a separator for the 2nd and
    // later payloads); the name is unused.  Never fails.
    bool
    write_item(
        const std::string&  _name,
        const byte_blob&  _bytes
    ) override
    {
        (void)_name;

        if ( (m_count != std::size_t(0)) &&
             (!m_separator.empty()) )
        {
            m_out += m_separator;
        }

        m_out += _bytes;
        ++m_count;

        return true;
    }

private:
    byte_blob&      m_out;
    const std::string m_separator;
    std::size_t       m_count;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_UTILITY_OUTPUT_PACKAGING_
