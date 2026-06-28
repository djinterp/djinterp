/******************************************************************************
* djinterp [core]                                          output_packaging.hpp
*
*   The packaging vocabulary for document output, lifted out of test_options
* into reusable, DTest-agnostic core machinery (handoff §3.6).  It answers two
* questions about a finished run's bytes - HOW they are packaged and WHERE they
* are written - and nothing about what they contain:
*
*     pack_mode      none      - each document written verbatim
*                    compress  - each document run through one codec
*                    archive   - all documents wrapped as one container
*     codec / format WHICH codec (pack == compress) or container (== archive),
*                    named by the core runtime selectors codec_id / format_id so
*                    no parallel enum is invented and no drift can open up.
*     output_sink    WHERE the bytes land: a disk path, an in-memory buffer, or
*                    a caller-supplied target (subclass output_sink).
*     base_name_fn   the filename a (logical name, ext, index, total) maps to,
*                    computed at write time so naming is dynamic without
*                    touching the bundle.
*
*   ONE VOCABULARY, EVERY FORMAT:
*   Nothing here mentions txt / xml / html / md / pdf, test_report, or
* test_options.  A document is already a byte_buffer by the time it reaches this
* layer (the load-bearing fact: byte_buffer IS std::string IS the return of
* pdf_template::render_pdf() IS archive::entry::data - documents flow
* render -> buffer -> entry with zero conversion).  The bundle + write()
* orchestrator that consumes this vocabulary lives in document_bundle.hpp; this
* header is the knobs alone, so test_options can embed it (an output_packaging
* slot + a doc_options slot) in place of spelling the knobs out itself.
*
*   BUILT ON THE PORTABLE FACADES:
*   compress / archive selection reuses codec_id (compression.hpp) and format_id
* (archive.hpp) directly, and the full per-codec / per-format tuning rides the
* two aggregate slots compress_opts / archive_opts, which ARE the surface of
* compress_options / archive_options.  The write orchestrator dispatches those
* runtime selectors through the facades' internal leaves, so every codec and
* container the build detected is reachable without a compile-time tag here.
*
*   PORTABILITY:
*   C++17 (parallel to the rest of the document stack - binding_env / section);
* self-suppresses below the floor.  The facades it composes are themselves
* portable to C++98, so the floor is this layer's choice, not theirs.
*
*
* TABLE OF CONTENTS
* =================
* I.    PACKAGING POLICY        (pack_mode; base_name_fn + default_base_name)
* II.   CODEC / FORMAT NAMING   (codec_suffix / format_extension)
* III.  OUTPUT SINKS            (output_sink; disk_output_sink; buffer_output_sink)
* IV.   OUTPUT CONFIG           (output_config: the whole packaging decision)
*
*
* path:      /inc/djinterp/core/output/output_packaging.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_CORE_OUTPUT_PACKAGING_
#define DJINTERP_CORE_OUTPUT_PACKAGING_ 1

// std
#include <cstddef>
#include <fstream>
#include <functional>
#include <string>
#include <utility>
// djinterp
#include "../djinterp.hpp"      // NS_*, D_NODISCARD, D_NOEXCEPT, language gates
#include "../compression.hpp"   // byte_buffer, codec_id, compress_options
#include "../archive.hpp"       // entry_list, format_id, archive_options


// The packaging vocabulary is C++17 to sit beside the rest of the document
// stack; below the floor this module contributes nothing rather than failing to
// compile (the facades it composes remain portable independently).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   PACKAGING POLICY                                     ///
///////////////////////////////////////////////////////////////////////////////

// pack_mode
//   enum: what becomes of a run's documents once rendered.  `none` writes each
// verbatim; `compress` runs each through one codec; `archive` wraps them all as
// the entries of a single container.  Lifted verbatim from test_options'
// test_output_pack; the default everywhere is `none`.
enum class pack_mode
{
    none,
    compress,
    archive
};


// base_name_fn
//   type: the naming policy.  Maps (logical name, extension, index, total) to
// the filename a document is written under - the in-archive path in archive
// mode, the on-disk / in-buffer name otherwise.  Evaluated at write time, so a
// run can be named dynamically (by index, timestamp, hash, ...) without the
// bundle carrying any of it.
using base_name_fn =
    std::function<std::string(const std::string&,   // logical name
                              const std::string&,    // extension (e.g. ".pdf")
                              std::size_t,            // 0-based index
                              std::size_t)>;          // total document count


// default_base_name
//   function: the baseline naming policy.  `name + ext` for a lone document
// ("report" + ".pdf" -> "report.pdf"); for a multi-document run the 1-based
// index is folded in to keep names distinct ("report" + ".pdf", item 0 of 3
// -> "report_1.pdf").  An empty name falls back to "document".
D_NODISCARD inline std::string
default_base_name(
    const std::string&  _name,
    const std::string&  _ext,
    std::size_t         _index,
    std::size_t         _total
)
{
    std::string _base = _name.empty() ? std::string("document") : _name;

    // disambiguate only when more than one document shares the policy
    if (_total > std::size_t(1))
    {
        _base += "_";
        _base += std::to_string(_index + std::size_t(1));
    }

    _base += _ext;

    return _base;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  CODEC / FORMAT NAMING                                ///
///////////////////////////////////////////////////////////////////////////////

// codec_suffix
//   function: the conventional filename suffix for a codec (".gz" for gzip,
// ".zst" for zstd, ...), appended to a document's name in `compress` mode.
// `store` (a passthrough) and any unrecognized id contribute nothing.
D_NODISCARD inline const char*
codec_suffix(
    codec_id _codec
) D_NOEXCEPT
{
    switch (_codec)
    {
        case codec_id_store:   { return "";         }
        case codec_id_deflate: { return ".deflate"; }
        case codec_id_zlib:    { return ".zz";      }
        case codec_id_gzip:    { return ".gz";      }
        case codec_id_bzip2:   { return ".bz2";     }
        case codec_id_xz:      { return ".xz";      }
        case codec_id_zstd:    { return ".zst";     }
        case codec_id_lz4:     { return ".lz4";     }
        case codec_id_brotli:  { return ".br";      }
        default:               { return "";         }
    }
}


// format_extension
//   function: the conventional extension for an archive container (".zip",
// ".tar.gz", ...), used to name the single container produced in `archive`
// mode.  An unrecognized id falls back to ".bin".
D_NODISCARD inline const char*
format_extension(
    format_id _format
) D_NOEXCEPT
{
    switch (_format)
    {
        case format_id_zip:      { return ".zip";    }
        case format_id_tar:      { return ".tar";    }
        case format_id_gz:       { return ".gz";     }
        case format_id_tar_gz:   { return ".tar.gz"; }
        case format_id_sevenzip: { return ".7z";     }
        case format_id_rar:      { return ".rar";    }
        default:                 { return ".bin";    }
    }
}


///////////////////////////////////////////////////////////////////////////////
///                III. OUTPUT SINKS                                         ///
///////////////////////////////////////////////////////////////////////////////

// output_sink
//   class: the abstract write target.  write() hands a sink a sequence of
// (name, bytes) pairs - one per document in `none` / `compress` mode, or a
// single (container name, archive bytes) pair in `archive` mode.  A custom
// destination (a database row, a network stream, a tar pipe) is a subclass that
// overrides write_item; the orchestrator knows nothing of any one sink.
class output_sink
{
public:
    // ~output_sink
    //   destructor: virtual, so a sink may be owned through a base pointer.
    virtual ~output_sink() = default;

    // write_item
    //   writes one named blob to the target.  Returns true on success; a false
    // return aborts the run's write and propagates out of write() as failure.
    virtual bool
    write_item(
        const std::string&  _name,
        const byte_buffer&  _bytes
    ) = 0;
};


// disk_output_sink
//   class: writes each blob to a file on disk.  The caller supplies a path
// policy mapping a filename to an absolute (or working-directory-relative) path,
// keeping the sink ignorant of any directory layout.  Bytes are written binary,
// so embedded NULs and non-text payloads (pdf) survive intact.
class disk_output_sink : public output_sink
{
public:
    // path_fn
    //   type: maps a filename to the path it is written at.
    using path_fn = std::function<std::string(const std::string&)>;

    // disk_output_sink
    //   constructor: a disk sink writing through _path - the policy that turns a
    // filename into a destination path.
    explicit disk_output_sink(
        path_fn _path
    )
        : m_path(static_cast<path_fn&&>(_path))
    {}

    // write_item
    //   writes _bytes (binary) to m_path(_name).  Returns false if the path
    // policy is unset or the stream could not be opened or written.
    bool
    write_item(
        const std::string&  _name,
        const byte_buffer&  _bytes
    ) override
    {
        // an unset path policy cannot name a destination
        if (!m_path)
        {
            return false;
        }

        const std::string _full = m_path(_name);

        std::ofstream _out(_full.c_str(),
                           std::ios::binary | std::ios::out | std::ios::trunc);

        // the destination could not be opened
        if (!_out)
        {
            return false;
        }

        _out.write(_bytes.data(),
                   static_cast<std::streamsize>(_bytes.size()));

        return static_cast<bool>(_out);
    }

private:
    path_fn m_path;
};


// buffer_output_sink
//   class: accumulates each blob into one in-memory byte_buffer, with an
// optional separator written between consecutive blobs.  Useful for testing,
// transmission, or composing a run in memory before a single downstream write.
class buffer_output_sink : public output_sink
{
public:
    // buffer_output_sink
    //   constructor: a buffer sink appending into _out, with _separator placed
    // between (not before the first, nor after the last) blobs.
    explicit buffer_output_sink(
        byte_buffer&  _out,
        std::string   _separator = std::string()
    )
        : m_out(_out),
          m_separator(static_cast<std::string&&>(_separator)),
          m_wrote_any(false)
    {}

    // write_item
    //   appends the separator (except before the first blob) then _bytes to the
    // target buffer.  Always succeeds.  The _name is unused: a flat buffer has
    // no per-blob filenames.
    bool
    write_item(
        const std::string&  _name,
        const byte_buffer&  _bytes
    ) override
    {
        (void)_name;

        // separate consecutive blobs, but never lead or trail with a separator
        if (m_wrote_any && !m_separator.empty())
        {
            m_out.append(m_separator);
        }

        m_out.append(_bytes);
        m_wrote_any = true;

        return true;
    }

private:
    byte_buffer&  m_out;
    std::string   m_separator;
    bool          m_wrote_any;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  OUTPUT CONFIG                                        ///
///////////////////////////////////////////////////////////////////////////////

// output_config
//   struct: the whole packaging decision for one run - the mode, the codec /
// container selected for the non-`none` modes, the full per-codec / per-format
// tuning (compress_opts / archive_opts), the naming policy, and the base name
// of the single container produced in `archive` mode.  This is exactly the slot
// test_options embeds (alongside a doc_options slot) instead of carrying the
// packaging knobs itself.  The defaults render a run uncompressed under the
// baseline naming policy - the "just write the files" case.
struct output_config
{
    // pack: how the run's documents are packaged (default: verbatim).
    pack_mode         pack;

    // codec: the codec used when pack == compress (default: gzip).
    codec_id          codec;

    // format: the container used when pack == archive (default: zip).
    format_id         format;

    // compress_opts: full per-codec tuning for the compress path.
    compress_options  compress_opts;

    // archive_opts: full per-format tuning for the archive path.
    archive_options   archive_opts;

    // naming: the (name, ext, index, total) -> filename policy.
    base_name_fn      naming;

    // archive_name: the logical name of the lone container in archive mode; its
    // extension is supplied by format_extension(format).
    std::string       archive_name;

    // output_config
    //   constructor: the baseline packaging decision - verbatim writes, gzip /
    // zip selected for the modes that need them, the default naming policy, and
    // a container named "report".
    output_config()
        : pack(pack_mode::none),
          codec(codec_id_gzip),
          format(format_id_zip),
          compress_opts(),
          archive_opts(),
          naming(&default_base_name),
          archive_name("report")
    {}
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_CORE_OUTPUT_PACKAGING_
