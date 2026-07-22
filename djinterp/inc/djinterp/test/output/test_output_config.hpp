/******************************************************************************
* djinterp [test]                                       test_output_config.hpp
*
*   The bridge from a test_option_set's KNOBS to the emit layer's VOCABULARY.
* test_options owns the typed test_* selection enums (test_output_pack /
* test_compressor / test_archive_format / test_doc_type); output_packaging owns
* the runtime vocabulary the bundle / sinks consume (pack_mode / codec_id /
* format_id / output_config); test_output owns doc_format.  This header is the
* single place those three meet - it lowers a configuration into an
* output_config + the list of doc_formats to emit, and nothing else.
*
*   WHY A SEPARATE HEADER:
*   test_output stays clean C++17 (it composes test_document / document_bundle)
* and must NOT pull in test_options' C++20 option_set face; test_options stays
* free of the emit engines.  The one junction that needs BOTH - the C++20
* configuration face and the C++17 emit vocabulary - is isolated here, behind
* the established free-accessor read surface (pack(opts), compressor(opts), ...),
* so it compiles wherever a test_option_set is readable (the C++20 face or the
* C++11 struct fallback alike).
*
*   THE MAPPINGS ARE BY VALUE, NOT BY CAST:
*   test_archive_format and format_id order their tar_gz / gz members
* differently, so static_cast<format_id>(archive_format) is WRONG; every mapping
* below is an explicit switch.  (The mapping arms are verified exhaustively in
* test_bridge_harness.cpp, including an assertion that a cast would mis-map.)
*
*   PORTABILITY:
*   C++17 (it names doc_format / output_config); self-suppresses below the floor.
*
*
* TABLE OF CONTENTS
* =================
* I.    ENUM MAPPINGS            (to_pack_mode / to_codec_id / to_format_id /
*                                 to_doc_format)
* II.   CONFIG LOWERING          (to_output_config / requested_doc_formats)
*
*
* path:      /inc/djinterp/test/test_output_config.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_OUTPUT_CONFIG_
#define DJINTERP_TEST_TEST_OUTPUT_CONFIG_ 1

// std
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"                  // NS_*, D_NODISCARD, gates
#include "../../core/util/output/output_packaging.hpp"   // pack_mode, codec_id, format_id, output_config
#include "../test_options.hpp"                    // test_option_set + the test_*
#include "./test_output.hpp"                     // doc_format
                                                 //   enums + free accessors


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   ENUM MAPPINGS                                        ///
///////////////////////////////////////////////////////////////////////////////

// to_pack_mode
//   function: a test_output_pack -> the packaging vocabulary's pack_mode.
D_NODISCARD inline pack_mode
to_pack_mode(
    test_output_pack _p
) D_NOEXCEPT
{
    switch (_p)
    {
        case test_output_pack::none:     { return pack_mode::none;     }
        case test_output_pack::compress: { return pack_mode::compress; }
        case test_output_pack::archive:  { return pack_mode::archive;  }
        default:                         { return pack_mode::none;     }
    }
}


// to_codec_id
//   function: a test_compressor -> the compression facade's codec_id.
D_NODISCARD inline codec_id
to_codec_id(
    test_compressor _c
) D_NOEXCEPT
{
    switch (_c)
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
        default:                       { return codec_id_gzip;    }
    }
}


// to_format_id
//   function: a test_archive_format -> the archive facade's format_id.  NB:
// tar_gz / gz sit at different ordinals in the two enums, so this MUST be a
// by-value switch, never a cast.
D_NODISCARD inline format_id
to_format_id(
    test_archive_format _f
) D_NOEXCEPT
{
    switch (_f)
    {
        case test_archive_format::zip:      { return format_id_zip;      }
        case test_archive_format::tar:      { return format_id_tar;      }
        case test_archive_format::tar_gz:   { return format_id_tar_gz;   }
        case test_archive_format::gz:       { return format_id_gz;       }
        case test_archive_format::sevenzip: { return format_id_sevenzip; }
        case test_archive_format::rar:      { return format_id_rar;      }
        default:                            { return format_id_zip;      }
    }
}


// to_doc_format
//   function: a test_doc_type -> the emit layer's doc_format.  test_doc_type has
// no markdown member today; doc_format::markdown is reachable only by binding a
// new test_doc_type member (and routing it here) when a markdown_print_policy
// lands.
D_NODISCARD inline doc_format
to_doc_format(
    test_doc_type _t
) D_NOEXCEPT
{
    switch (_t)
    {
        case test_doc_type::txt:  { return doc_format::text; }
        case test_doc_type::xml:  { return doc_format::xml;  }
        case test_doc_type::html: { return doc_format::html; }
        case test_doc_type::pdf:  { return doc_format::pdf;  }
        default:                  { return doc_format::text; }
    }
}


///////////////////////////////////////////////////////////////////////////////
///                II.  CONFIG LOWERING                                      ///
///////////////////////////////////////////////////////////////////////////////

// to_output_config
//   function: lower a test_option_set's packaging decision into the shared
// output_config that document_bundle / test_output consume.  The per-codec /
// per-format aggregates copy across unchanged - compress_opts / archive_opts ARE
// compress_options / archive_options.  Naming (output_config::naming /
// archive_name) is left at output_config's defaults here; a caller that wants
// file_name_pattern semantics installs a base_name_fn after lowering.
//
//   Read surface: the free accessors test_options already publishes
// (pack(opts), compressor(opts), archive_format(opts), compress_opts(opts),
// archive_opts(opts)) - the same convention as sinks(opts) / routes(opts).
D_NODISCARD inline output_config
to_output_config(
    const test_option_set& _opts
)
{
    output_config _cfg;

    _cfg.pack          = to_pack_mode(pack(_opts));
    _cfg.codec         = to_codec_id(compressor(_opts));
    _cfg.format        = to_format_id(archive_format(_opts));
    _cfg.compress_opts = compress_opts(_opts);
    _cfg.archive_opts  = archive_opts(_opts);

    return _cfg;
}


// requested_doc_formats
//   function: the formats a configuration asks to emit.  Today test_options
// carries one selected document type (the `document` slot), so this yields a
// single-element list; it returns a vector so the call sites are already shaped
// for a future format SET without churn.
D_NODISCARD inline std::vector<doc_format>
requested_doc_formats(
    const test_option_set& _opts
)
{
    std::vector<doc_format> _formats;

    _formats.push_back(to_doc_format(document(_opts)));

    return _formats;
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_OUTPUT_CONFIG_
