/*******************************************************************************
* djinterp [config]                                       cfg_output_packaging.h
*
*   Build-time defaults for output packaging: what output_config does with a
* document when its caller does not say -- write it as it is, compress it, or
* put it in a container -- and with which codec and container.
*   Every knob here is a DEFAULT. output_config's members are the runtime
* options and always win; these choose only what a default-constructed
* output_config holds (the owner's ruling of 2026-11-09: complete
* configurability when the user decides, sensible defaults when not). How a
* container compresses its entries is cfg_archive.h's, and a codec's tuning is
* cfg_compress.h's.
*
*   targets:  cpp/util/output/output_packaging.hpp -> D_INTERNAL_OUTPUT_*
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/cpp/util/output/cfg_output_packaging.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_UTIL_OUTPUT_CFG_OUTPUT_PACKAGING_H
#define DJINTERP_CONFIG_CPP_UTIL_OUTPUT_CFG_OUTPUT_PACKAGING_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides

/*
TABLE OF CONTENTS
=================
0.    OUTPUT PACKAGING CONFIGURATION
      ------------------------------
      1.    Defaults                     (D_CFG_OUTPUT_PACK, _CODEC, _FORMAT)
      2.    Validation of the knobs
      3.    Derived values               (D_INTERNAL_OUTPUT_*)
*/


// ===========================================================================
// 0.   OUTPUT PACKAGING CONFIGURATION
// ===========================================================================

// --- 0.1  Defaults ---

// D_CFG_OUTPUT_PACK
//   brief: what a document becomes on its way to the sink: 0 itself (the
// default), 1 compressed with D_CFG_OUTPUT_CODEC, 2 an entry of one container
// in D_CFG_OUTPUT_FORMAT. pack_mode's order: none, compress, archive.
#ifndef D_CFG_OUTPUT_PACK
#   define D_CFG_OUTPUT_PACK 0
#endif

// D_CFG_OUTPUT_CODEC
//   brief: the codec compress mode uses, by its dj_codec_id number: 0 store,
// 1 deflate, 2 zlib, 3 gzip (the default), 4 bzip2, 5 xz, 6 zstd, 7 lz4,
// 8 brotli.
#ifndef D_CFG_OUTPUT_CODEC
#   define D_CFG_OUTPUT_CODEC 3
#endif

// D_CFG_OUTPUT_FORMAT
//   brief: the container archive mode uses, by its dj_format_id number: 0 zip
// (the default), 1 tar, 2 gz, 3 tar.gz, 4 7z, 5 rar.
//   Whether the container's entries are stored or compressed is cfg_archive.h's
// D_CFG_ARCHIVE_STORE_ONLY, and the method its _ZIP_METHOD and
// _SEVENZIP_METHOD.
#ifndef D_CFG_OUTPUT_FORMAT
#   define D_CFG_OUTPUT_FORMAT 0
#endif


// --- 0.2  Validation of the knobs ---

#if ( (D_CFG_NORM(D_CFG_OUTPUT_PACK) < 0) ||                                  \
      (D_CFG_NORM(D_CFG_OUTPUT_PACK) > 2) )
#   error "D_CFG_OUTPUT_PACK must be 0 (none), 1 (compress) or 2 (archive)"
#endif
#if ( (D_CFG_NORM(D_CFG_OUTPUT_CODEC) < 0) ||                                 \
      (D_CFG_NORM(D_CFG_OUTPUT_CODEC) > 8) )
#   error "D_CFG_OUTPUT_CODEC must be a dj_codec_id number, 0 to 8"
#endif
#if ( (D_CFG_NORM(D_CFG_OUTPUT_FORMAT) < 0) ||                                \
      (D_CFG_NORM(D_CFG_OUTPUT_FORMAT) > 5) )
#   error "D_CFG_OUTPUT_FORMAT must be a dj_format_id number, 0 to 5"
#endif


// --- 0.3  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_OUTPUT_PACK / _CODEC / _FORMAT
//   brief: the knobs as the integers output_config's constructor converts to
// pack_mode, DCodecId and format_id.
#define D_INTERNAL_OUTPUT_PACK    D_CFG_NORM(D_CFG_OUTPUT_PACK)
#define D_INTERNAL_OUTPUT_CODEC   D_CFG_NORM(D_CFG_OUTPUT_CODEC)
#define D_INTERNAL_OUTPUT_FORMAT  D_CFG_NORM(D_CFG_OUTPUT_FORMAT)


#endif  // DJINTERP_CONFIG_CPP_UTIL_OUTPUT_CFG_OUTPUT_PACKAGING_H
