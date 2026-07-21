/******************************************************************************
* djinterp [core]                                            output_packaging.hpp
*
*   Runtime vocabulary for packaging (compression + archiving) configuration:
* The shared types and enums that bundle / sinks consume, and that test_output_config
* maps test_option_set selections into. Defines the codec_id, format_id, and
* pack_mode enums, plus the output_config struct that carries full packaging
* configuration through the emit pipeline.
*
*   ROLE IN THE EMIT STACK:
*   test_options / test_output_config own the typed, user-facing test_* selection
* enums (test_compressor / test_archive_format / test_doc_type / test_output_pack).
* This header owns the runtime vocabulary those selections map INTO:
*
*     test_compressor::gzip     -> codec_id_gzip
*     test_archive_format::zip  -> format_id_zip
*     test_output_pack::archive -> pack_mode::archive
*
* The emit engines (document_bundle / sinks) consume output_config, never the
* test_* enums. This separation keeps test infrastructure confined to the test
* layer; emit remains test-agnostic.
*
*   ARCHITECTURE:
*   - codec_id: Opaque enum representing a compression algorithm.
*   - format_id: Opaque enum representing an archive format.
*   - pack_mode: How to package output: none (plain), compress, or archive.
*   - output_config: Aggregate struct carrying a complete packaging decision.
*     Routes to compress_options and archive_options for codec/format-specific knobs.
*
*   PORTABILITY:
*   C++98 compatible. Plain enums, POD structs, no modern features.
*
*
* TABLE OF CONTENTS
* =================
* I.    PACK MODE                     (pack_mode enum)
* II.   CODEC ID                      (codec_id enum + constants)
* III.  FORMAT ID                     (format_id enum + constants)
* IV.   OUTPUT CONFIG                 (output_config struct)
*
*
* path:      /inc/djinterp/core/util/output/output_packaging.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_CORE_UTIL_OUTPUT_PACKAGING_
#define DJINTERP_CORE_UTIL_OUTPUT_PACKAGING_ 1

// djinterp
#include "../compress_options.hpp"   // compress_options
#include "../archive_options.hpp"    // archive_options


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   PACK MODE                                            ///
///////////////////////////////////////////////////////////////////////////////

// pack_mode
//   enum: How to package the output.  none: emit the document(s) uncompressed
// and unarchived.  compress: compress each document individually.  archive:
// bundle all documents into an archive container (zip / tar / 7z / etc.).
enum pack_mode
{
    pack_mode_none     = 0,  // No packaging
    pack_mode_compress = 1,  // Compress each document
    pack_mode_archive  = 2   // Bundle into archive
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CODEC ID                                             ///
///////////////////////////////////////////////////////////////////////////////

// codec_id
//   enum: A specific compression codec / algorithm.  The enumerators are
// assigned explicitly to match the test layer's test_compressor enum so a
// by-value mapping works; they are NOT portable across versions, but neither
// is test_compressor (both are test-layer enums).
enum codec_id
{
    codec_id_store   = 0,  // No compression (store-only)
    codec_id_deflate = 1,  // Raw DEFLATE (RFC 1951)
    codec_id_zlib    = 2,  // zlib-wrapped DEFLATE (RFC 1950)
    codec_id_gzip    = 3,  // gzip-wrapped DEFLATE (RFC 1952)
    codec_id_bzip2   = 4,  // bzip2 (.bz2)
    codec_id_xz      = 5,  // xz / LZMA
    codec_id_zstd    = 6,  // Zstandard
    codec_id_lz4     = 7,  // LZ4 frame
    codec_id_brotli  = 8   // Brotli
};


///////////////////////////////////////////////////////////////////////////////
///                III. FORMAT ID                                            ///
///////////////////////////////////////////////////////////////////////////////

// format_id
//   enum: A specific archive or packaging format.  Like codec_id, the
// enumerators are assigned to align with test_archive_format for direct
// mapping; tar_gz and gz sit at different ordinals in the two enums, hence
// test_output_config always uses an explicit switch, never a cast.
enum format_id
{
    format_id_zip      = 0,  // ZIP archive
    format_id_tar      = 1,  // TAR archive (uncompressed)
    format_id_tar_gz   = 2,  // TAR + gzip
    format_id_gz       = 3,  // Single-file gzip
    format_id_sevenzip = 4,  // 7z archive
    format_id_rar      = 5   // RAR archive
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  OUTPUT CONFIG                                        ///
///////////////////////////////////////////////////////////////////////////////

// output_config
//   struct: The complete packaging configuration that emit engines consume.
// Carries the pack mode (none / compress / archive), codec, format, and the
// full option sets for the chosen codec and format.  Naming and output routing
// are left to higher layers (document_bundle / sinks) so this struct stays
// configuration-only, with no side effects.
//
//   USAGE:
//   A caller (like test_output_config::to_output_config) populates this struct,
// then passes it to document_bundle or a sink.  The emit engine reads pack,
// codec, format, and the appropriate compress_opts / archive_opts for the
// requested codec / format.
struct output_config
{
    pack_mode         pack;           // none / compress / archive
    codec_id          codec;          // which codec to use
    format_id         format;         // which archive format to use
    compress_options  compress_opts;  // codec-specific compression options
    archive_options   archive_opts;   // format-specific archive options

    // constructor
    //   Initialize to sensible defaults: no packaging, gzip codec, zip format.
    output_config()
        : pack(pack_mode_none)
        , codec(codec_id_gzip)
        , format(format_id_zip)
    {
        // compress_opts and archive_opts initialize via their own constructors
    }
};


NS_END  // djinterp


#endif  // DJINTERP_CORE_UTIL_OUTPUT_PACKAGING_
