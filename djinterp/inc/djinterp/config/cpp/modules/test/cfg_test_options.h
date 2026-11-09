/*******************************************************************************
* djinterp [config]                                           cfg_test_options.h
*
*   Build-time defaults for how DTest packages its report: whether the report
* file is written as it is, compressed or put in a container, with which codec
* and container, and whether the container's entries are stored or compressed.
*   Every knob here is a DEFAULT. A test_option_set's slots -- pack,
* compressor, archive_format, compress_opts and archive_opts -- are the runtime
* options and always win; these choose only what default_test_options() holds
* (the owner's ruling of 2026-11-09: complete configurability when the user
* decides, sensible defaults when not).
*   Each knob falls back to the library's own, so a build that sets only the
* library's knobs (cfg_output_packaging.h, cfg_archive.h) gets the same
* packaging from DTest, and one that sets these gets DTest's report packaged
* one way and everything else another.
*
*   targets:  cpp/modules/test/test_options.hpp -> D_INTERNAL_TEST_OUTPUT_*
*   requires: cfg_common.h; cfg_output_packaging.h (the library's defaults,
*             which these fall back to)
*
*
* path:      /inc/djinterp/config/cpp/modules/test/cfg_test_options.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_TEST_CFG_TEST_OPTIONS_H
#define DJINTERP_CONFIG_CPP_TEST_CFG_TEST_OPTIONS_H 1

// djinterp
#include "../../../cfg_common.h"                     // root first
#include "../../util/output/cfg_output_packaging.h"  // the library's defaults

/*
TABLE OF CONTENTS
=================
0.    DTEST OUTPUT CONFIGURATION
      --------------------------
      1.    Packaging                    (D_CFG_TEST_OUTPUT_PACK, _CODEC,
                                          _FORMAT)
      2.    Entry compression            (D_CFG_TEST_OUTPUT_STORE_ONLY,
                                          _LEVEL)
      3.    Validation of the knobs
      4.    Derived values               (D_INTERNAL_TEST_OUTPUT_*)
*/


// ===========================================================================
// 0.   DTEST OUTPUT CONFIGURATION
// ===========================================================================

// --- 0.1  Packaging ---

// D_CFG_TEST_OUTPUT_PACK
//   brief: what DTest's report file becomes: 0 itself, 1 compressed with
// D_CFG_TEST_OUTPUT_CODEC, 2 an entry of one container in
// D_CFG_TEST_OUTPUT_FORMAT. Default: D_CFG_OUTPUT_PACK, which is 0.
#ifndef D_CFG_TEST_OUTPUT_PACK
#   define D_CFG_TEST_OUTPUT_PACK D_CFG_OUTPUT_PACK
#endif

// D_CFG_TEST_OUTPUT_CODEC
//   brief: the codec the report is compressed with, by its dj_codec_id number,
// which is test_compressor's order too: 0 store, 1 deflate, 2 zlib, 3 gzip,
// 4 bzip2, 5 xz, 6 zstd, 7 lz4, 8 brotli. Default: D_CFG_OUTPUT_CODEC, which
// is 3.
#ifndef D_CFG_TEST_OUTPUT_CODEC
#   define D_CFG_TEST_OUTPUT_CODEC D_CFG_OUTPUT_CODEC
#endif

// D_CFG_TEST_OUTPUT_FORMAT
//   brief: the container the report is put in, by its dj_format_id number: 0
// zip, 1 tar, 2 gz, 3 tar.gz, 4 7z, 5 rar. Default: D_CFG_OUTPUT_FORMAT, which
// is 0.
//   test_archive_format orders gz and tar.gz the other way round; the derived
// value below converts, so the knob speaks the library's numbers.
#ifndef D_CFG_TEST_OUTPUT_FORMAT
#   define D_CFG_TEST_OUTPUT_FORMAT D_CFG_OUTPUT_FORMAT
#endif


// --- 0.2  Entry compression ---

// D_CFG_TEST_OUTPUT_STORE_ONLY
//   brief: whether the report's entries in a ZIP or 7z container are stored
// (1) or compressed (0); -1, the default, leaves archive_opts.store_only unset,
// so the container follows cfg_archive.h's D_CFG_ARCHIVE_STORE_ONLY.
//   A stored report can be searched inside the container without a codec; a
// compressed one is much smaller, since DTest's PDFs are not compressed
// within.
#ifndef D_CFG_TEST_OUTPUT_STORE_ONLY
#   define D_CFG_TEST_OUTPUT_STORE_ONLY -1
#endif

// D_CFG_TEST_OUTPUT_LEVEL
//   brief: the container's compression effort for the report, 0-9; -1, the
// default, leaves archive_opts.level unset, so the container follows
// cfg_archive.h's D_CFG_ARCHIVE_LEVEL.
#ifndef D_CFG_TEST_OUTPUT_LEVEL
#   define D_CFG_TEST_OUTPUT_LEVEL -1
#endif


// --- 0.3  Validation of the knobs ---

#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_PACK) < 0) ||                             \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_PACK) > 2) )
#   error "D_CFG_TEST_OUTPUT_PACK must be 0 (none), 1 (compress) or 2 (archive)"
#endif
#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_CODEC) < 0) ||                            \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_CODEC) > 8) )
#   error "D_CFG_TEST_OUTPUT_CODEC must be a dj_codec_id number, 0 to 8"
#endif
#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) < 0) ||                           \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) > 5) )
#   error "D_CFG_TEST_OUTPUT_FORMAT must be a dj_format_id number, 0 to 5"
#endif
#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_STORE_ONLY) < -1) ||                      \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_STORE_ONLY) > 1) )
#   error "D_CFG_TEST_OUTPUT_STORE_ONLY must be -1, 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_TEST_OUTPUT_LEVEL) < -1) ||                           \
      (D_CFG_NORM(D_CFG_TEST_OUTPUT_LEVEL) > 9) )
#   error "D_CFG_TEST_OUTPUT_LEVEL must be -1 or on the 0-9 generic scale"
#endif


// --- 0.4  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_TEST_OUTPUT_PACK / _CODEC
//   brief: the knobs as the integers test_option_set's constructor converts to
// DTestOutputPack and test_compressor, whose orders they share.
#define D_INTERNAL_TEST_OUTPUT_PACK   D_CFG_NORM(D_CFG_TEST_OUTPUT_PACK)
#define D_INTERNAL_TEST_OUTPUT_CODEC  D_CFG_NORM(D_CFG_TEST_OUTPUT_CODEC)

// D_INTERNAL_TEST_OUTPUT_FORMAT
//   brief: the container as a test_archive_format ordinal: dj_format_id's
// gz (2) and tar.gz (3) swap places.
#if ( D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) == 2 )
#   define D_INTERNAL_TEST_OUTPUT_FORMAT 3
#elif ( D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT) == 3 )
#   define D_INTERNAL_TEST_OUTPUT_FORMAT 2
#else
#   define D_INTERNAL_TEST_OUTPUT_FORMAT D_CFG_NORM(D_CFG_TEST_OUTPUT_FORMAT)
#endif

// D_INTERNAL_TEST_OUTPUT_STORE_ONLY / _LEVEL
//   brief: the entry-compression knobs; -1 means archive_opts leaves its own
// knob unset.
#define D_INTERNAL_TEST_OUTPUT_STORE_ONLY                                     \
    D_CFG_NORM(D_CFG_TEST_OUTPUT_STORE_ONLY)
#define D_INTERNAL_TEST_OUTPUT_LEVEL  D_CFG_NORM(D_CFG_TEST_OUTPUT_LEVEL)


#endif  // DJINTERP_CONFIG_CPP_TEST_CFG_TEST_OPTIONS_H
