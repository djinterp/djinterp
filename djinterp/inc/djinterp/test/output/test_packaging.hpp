/******************************************************************************
* djinterp [test]                                            test_packaging.hpp
*
*   The one place DTest's packaging KNOBS meet the framework's packaging
* VOCABULARY, and the one path a run's documents take to a sink.
*
*   WHAT IT REPLACES.  Three paths did render -> name -> compress/archive ->
* sink:
*     document_bundle + output_packaging   the designed one, reached only from
*                                          the now-deleted test_output.hpp
*     test_pack.hpp                        an enum -> tag hop straight into the
*                                          compress / archive facades
*     write_archived_report + test_zip_store
*                                          the runner's own, with a
*                                          dependency-free stored-ZIP fallback
*   The first is the one with a naming policy, a sink abstraction and both pack
*   modes.  This header re-homes the live path onto it, so the other two have
*   nothing left to do.
*
*   THE MAPPINGS ARE SWITCHES, NOT CASTS -- deliberately, and permanently.
* output_packaging.hpp says it outright: test_archive_format and format_id order
* `tar_gz` and `gz` differently, so a static_cast silently produces the wrong
* container.  The facades own codec_id / format_id because compress.cpp and
* archive.cpp are C++98 and dispatch on them; DTest owns its typed enums because
* they are part of an option_set schema.  Neither can move, so the bridge is
* real work rather than an accident -- and being three explicit switches is what
* keeps it correct when either side gains a member.
*
*   ONE ENUM DID collapse: the document format.  test_doc_type, doc_format and
* document_format are now one type, so that arm of the old bridge is gone
* entirely.  What remains here is only the packaging axis.
*
*   PORTABILITY:
*   C++17 (output_packaging's floor -- output_config carries a std::function
* naming policy); self-suppresses below it, as document_bundle does.
*
*
* TABLE OF CONTENTS
* =================
* I.    ENUM BRIDGE               (to_pack_mode / to_codec_id / to_format_id)
* II.   to_output_config          (knobs -> the packaging decision)
* III.  emit_report               (bundle a run's documents and write them)
*
*
* path:      /inc/djinterp/test/output/test_packaging.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.24
******************************************************************************/

#ifndef DJINTERP_TEST_PACKAGING_
#define DJINTERP_TEST_PACKAGING_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/util/output/output_packaging.hpp"
                                            // pack_mode, output_config, sinks,
                                            // codec_id / format_id, suffixes
#include "../../core/util/document/document_bundle.hpp"
                                            // document_bundle, write_to_disk
#include "../../core/util/document/document_format.hpp"
                                            // document_format, format_extension
#include "../test_options.hpp"              // the typed test_* packaging enums
#include "./test_report.hpp"                // test_report


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   ENUM BRIDGE                                         ///
///////////////////////////////////////////////////////////////////////////////

// to_pack_mode
//   function: DTest's pack selection -> the packaging vocabulary's.  The two
// agree member-for-member today; the switch is what keeps that true when one
// side gains a mode.
D_NODISCARD inline pack_mode
to_pack_mode(
    test_output_pack _pack
) D_NOEXCEPT
{
    switch (_pack)
    {
        case test_output_pack::compress: { return pack_mode::compress; }
        case test_output_pack::archive:  { return pack_mode::archive;  }
        case test_output_pack::none:     { return pack_mode::none;     }
    }

    return pack_mode::none;
}


// to_codec_id
//   function: DTest's codec selection -> the compression facade's id.  These
// happen to share an order; the switch does not depend on it.
D_NODISCARD inline codec_id
to_codec_id(
    test_compressor _codec
) D_NOEXCEPT
{
    switch (_codec)
    {
        case test_compressor::store:   { return codec_id_store;   }
        case test_compressor::deflate: { return codec_id_deflate; }
        case test_compressor::zlib:    { return codec_id_zlib;    }
        case test_compressor::gzip:    { return codec_id_gzip;    }
        case test_compressor::bzip2:   { return codec_id_bzip2;   }
        case test_compressor::xz:      { return codec_id_xz;      }
        case test_compressor::zstd:    { return codec_id_zstd;    }
        case test_compressor::lz4:     { return codec_id_lz4;     }
        case test_compressor::brotli:  { return codec_id_brotli;  }
    }

    return codec_id_gzip;
}


// to_format_id
//   function: DTest's container selection -> the archive facade's id.
//
//   THIS IS THE ONE THAT CANNOT BE A CAST.  test_archive_format orders
// { zip, tar, tar_gz, gz, ... } and format_id orders { zip, tar, gz, tar_gz,
// ... } -- `tar_gz` and `gz` are transposed.  A static_cast between them
// compiles cleanly and writes the wrong container.
D_NODISCARD inline format_id
to_format_id(
    test_archive_format _format
) D_NOEXCEPT
{
    switch (_format)
    {
        case test_archive_format::zip:      { return format_id_zip;      }
        case test_archive_format::tar:      { return format_id_tar;      }
        case test_archive_format::tar_gz:   { return format_id_tar_gz;   }
        case test_archive_format::gz:       { return format_id_gz;       }
        case test_archive_format::sevenzip: { return format_id_sevenzip; }
        case test_archive_format::rar:      { return format_id_rar;      }
    }

    return format_id_zip;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  to_output_config                                    ///
///////////////////////////////////////////////////////////////////////////////

// to_output_config
//   function: the four packaging knobs lowered into the single decision
// document_bundle::write consumes.  Taken as explicit values rather than as an
// option_set so this header does not have to know the accessor spellings -- a
// caller reads them from its own options and passes them through.
D_NODISCARD inline output_config
to_output_config(
    test_output_pack    _pack,
    test_compressor     _codec,
    test_archive_format _format,
    const std::string&  _archive_name = std::string("report")
)
{
    output_config _cfg;

    _cfg.pack         = to_pack_mode(_pack);
    _cfg.codec        = to_codec_id(_codec);
    _cfg.format       = to_format_id(_format);
    _cfg.archive_name = _archive_name;

    return _cfg;
}


///////////////////////////////////////////////////////////////////////////////
///                III. emit_report                                         ///
///////////////////////////////////////////////////////////////////////////////

// report_document_producer
//   type: one rendered document -- a logical name, a format, and the bytes.
// The bytes are produced LAZILY, so a bundle can be assembled from every format
// a run might emit and only the selected ones are ever rendered.
using report_document_producer = std::function<byte_blob()>;


// bundle_report
//   function: collect a run's documents into a bundle.  Each entry names the
// document and its format; the extension comes from document_format.hpp's
// single table, so a bundle and a bare write agree on what a file is called.
//
//   The producers are the caller's -- render_report_pdf_bytes_table for PDF,
// render_table_model_to_string or a document_template render for the string
// formats.  Keeping them out of this header is what stops the packaging layer
// depending on the PDF engine.
D_NODISCARD inline document_bundle
bundle_report(
    const std::string&                                          _base_name,
    const std::vector<std::pair<document_format,
                                report_document_producer> >&    _documents
)
{
    document_bundle _bundle;

    for (std::size_t _i = 0; _i < _documents.size(); ++_i)
    {
        _bundle.add(_base_name,
                    std::string(format_extension(_documents[_i].first)),
                    _documents[_i].second);
    }

    return _bundle;
}


// emit_report_to_disk
//   function: bundle a run's documents, package them per the knobs, and write
// them through _path.  This is the whole of what write_archived_report and
// test_zip_store were doing by hand -- with a naming policy, both pack modes and
// a sink abstraction they did not have.
D_NODISCARD inline bool
emit_report_to_disk(
    const std::string&                                          _base_name,
    const std::vector<std::pair<document_format,
                                report_document_producer> >&    _documents,
    test_output_pack                                            _pack,
    test_compressor                                             _codec,
    test_archive_format                                         _format,
    disk_output_sink::path_fn                                   _path
)
{
    const document_bundle _bundle = bundle_report(_base_name, _documents);

    const output_config _cfg =
        to_output_config(_pack, _codec, _format, _base_name);

    return write_to_disk(_bundle, _cfg,
                         static_cast<disk_output_sink::path_fn&&>(_path));
}


// emit_report_to_buffer
//   function: the in-memory counterpart -- every packaged payload concatenated
// into _out.  What a test of the emit path wants, and what an embedder that
// ships the report somewhere other than a file wants.
D_NODISCARD inline bool
emit_report_to_buffer(
    const std::string&                                          _base_name,
    const std::vector<std::pair<document_format,
                                report_document_producer> >&    _documents,
    test_output_pack                                            _pack,
    test_compressor                                             _codec,
    test_archive_format                                         _format,
    byte_blob&                                                _out
)
{
    const document_bundle _bundle = bundle_report(_base_name, _documents);

    const output_config _cfg =
        to_output_config(_pack, _codec, _format, _base_name);

    buffer_output_sink _sink(_out);

    return write(_bundle, _cfg, _sink);
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_PACKAGING_
