/*******************************************************************************
* djinterp [config]                                                cfg_archive.h
*
*   Build-time configuration for the archive facade: which formats are compiled
* in, whether the built-in writers are used in preference to a detected
* library, how much of a container's content is allowed to come from the
* machine it was built on, and how its entries are compressed when a caller
* does not say.
*
*   targets:  c/util/archive/archive_common.h -> D_INTERNAL_ARCHIVE_*
*                 (archive_common.c reads them through it)
*             cpp/util/archive/archive.hpp    -> D_INTERNAL_ARCHIVE_CAN_*
*   requires: cfg_common.h; cfg_compress.h (a container embeds a codec, so the
*             codec cascade must resolve first); env/archive/env_archive.h
*
*   ARCHIVE SITS ON COMPRESSION and this file says so by including its config
* before its own. A container is a directory plus a codec: excluding DEFLATE in
* cfg_compress.h must make the ZIP writer fall back to store, and that only
* resolves correctly if the codec knobs are already settled when these are read.
*
*
* path:      /inc/djinterp/config/c/util/archive/cfg_archive.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_UTIL_ARCHIVE_CFG_ARCHIVE_H
#define DJINTERP_CONFIG_C_UTIL_ARCHIVE_CFG_ARCHIVE_H 1

// djinterp
// (0) root first.
#include "../../../cfg_common.h"
// (0b) the dependency's config, resolved BEFORE this one -- see the banner.
#include "../compress/cfg_compress.h"
// (0c) the detection this cascade defaults from.
#include "../../../../env/util/archive/env_archive.h"

/*
TABLE OF CONTENTS
=================
0.    ARCHIVE CONFIGURATION
      ---------------------
      1.    Aggregate                    (D_CFG_ARCHIVE_ALL)
      2.    Per-format enables           (D_CFG_ARCHIVE_FORMAT_*)
      3.    Built-in writers             (D_CFG_ARCHIVE_PREFER_BUILTIN)
      4.    Reproducibility              (D_CFG_ARCHIVE_REPRODUCIBLE, _EPOCH)
      5.    Safety                       (D_CFG_ARCHIVE_CHECK_ENTRY_NAMES)
      6.    Composition                  (D_CFG_ARCHIVE_TAR_GZ_STAGING)
      7.    Entry compression            (D_CFG_ARCHIVE_STORE_ONLY, _LEVEL,
                                          _ZIP_METHOD, _SEVENZIP_METHOD)
      8.    Validation of the knobs
      9.    Derived values               (D_INTERNAL_ARCHIVE_*)
*/


// ===========================================================================
// 0.   ARCHIVE CONFIGURATION
// ===========================================================================

// --- 0.1  Aggregate ---

// D_CFG_ARCHIVE_ALL
//   brief: aggregate fallback for the per-format enables in 0.2. Individual
// knobs override it; the cascade is individual > aggregate > detection.
#ifndef D_CFG_ARCHIVE_ALL
#   define D_CFG_ARCHIVE_ALL 1
#endif


// --- 0.2  Per-format enables ---

// D_CFG_ARCHIVE_FORMAT_ZIP
//   brief: the ZIP container. Note that turning this OFF removes the format
// even though the built-in writer needs no library -- which is the point: this
// is a surface choice, not a dependency one.
#ifndef D_CFG_ARCHIVE_FORMAT_ZIP
#   define D_CFG_ARCHIVE_FORMAT_ZIP D_CFG_ARCHIVE_ALL
#endif

// D_CFG_ARCHIVE_FORMAT_TAR
//   brief: ustar, and with it tar.gz -- a tarball is a tar inside a gzip
// stream, so excluding tar excludes both.
#ifndef D_CFG_ARCHIVE_FORMAT_TAR
#   define D_CFG_ARCHIVE_FORMAT_TAR D_CFG_ARCHIVE_ALL
#endif

// D_CFG_ARCHIVE_FORMAT_GZ
//   brief: the single-member gzip container. Needs a gzip codec; excluding
// DEFLATE in cfg_compress.h removes this regardless of this knob.
#ifndef D_CFG_ARCHIVE_FORMAT_GZ
#   define D_CFG_ARCHIVE_FORMAT_GZ D_CFG_ARCHIVE_ALL
#endif

// D_CFG_ARCHIVE_FORMAT_7Z
//   brief: the 7z container. Read and write both need a backend library.
#ifndef D_CFG_ARCHIVE_FORMAT_7Z
#   define D_CFG_ARCHIVE_FORMAT_7Z D_CFG_ARCHIVE_ALL
#endif

// D_CFG_ARCHIVE_FORMAT_RAR
//   brief: the RAR container, read-only in every configuration -- no library
// can create RAR, and the proprietary tool backend is not auto-linked.
#ifndef D_CFG_ARCHIVE_FORMAT_RAR
#   define D_CFG_ARCHIVE_FORMAT_RAR D_CFG_ARCHIVE_ALL
#endif


// --- 0.3  Built-in writers ---

// D_CFG_ARCHIVE_PREFER_BUILTIN
//   brief: use the kernel's dependency-free ustar and ZIP writers even when a
// library was detected (1, the default), or prefer the library (0).
//   ON is the parity choice. The built-in writers produce the same bytes on
// every build because they ARE the same code; a detected library produces
// whatever that library's version produces, so two machines with different
// libarchive releases diverge. OFF is for a build that wants a library's
// broader format coverage -- ZIP64, modern methods, encryption -- and accepts
// that its output is versioned.
//   This knob does not affect READING. A container is read by whatever can
// read it.
#ifndef D_CFG_ARCHIVE_PREFER_BUILTIN
#   define D_CFG_ARCHIVE_PREFER_BUILTIN 1
#endif


// --- 0.4  Reproducibility ---

// D_CFG_ARCHIVE_REPRODUCIBLE
//   brief: an entry whose mtime is 0 records the pinned epoch (1, the default)
// rather than the current time (0).
//   ON is what makes two runs over identical entries produce identical bytes,
// which is the precondition for comparing a C-built archive against a C++-built
// one at all. It is a behaviour change from the facade's historical
// documentation, which read "0 selects now" -- and it is the deliberate one,
// because "now" cannot be compared to anything.
//   OFF restores clock-reading for a build that wants archives to carry real
// creation times and does not run a parity suite. A caller wanting a real
// timestamp under either setting passes it explicitly, which is also the only
// unambiguous way to ask.
#ifndef D_CFG_ARCHIVE_REPRODUCIBLE
#   define D_CFG_ARCHIVE_REPRODUCIBLE 1
#endif

// D_CFG_ARCHIVE_EPOCH
//   brief: the timestamp recorded for an entry with no mtime, as a Unix epoch
// in seconds. Default 315532800 -- 1980-01-01T00:00:00Z, which is the ZIP
// format's own epoch and therefore the earliest instant both tar and ZIP
// represent without a special case.
//   Lowering it below the ZIP epoch makes ZIP output clamp and tar output not,
// so the two formats stop agreeing about the same entry. Raise it, do not lower
// it.
#ifndef D_CFG_ARCHIVE_EPOCH
#   define D_CFG_ARCHIVE_EPOCH 315532800L
#endif


// --- 0.5  Safety ---

// D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
//   brief: reject absolute paths and `..` components in entry names (1, the
// default).
//   This is the path-traversal guard: a crafted archive whose entries are named
// `../../etc/passwd` writes outside the directory it is extracted into. The
// check also rejects backslash separators and drive prefixes, because a name
// written on Windows and extracted on POSIX would otherwise smuggle `..\` past
// a check that only looked for `../`.
//   OFF exists only for a build that has already validated its inputs upstream
// and is extracting archives it produced itself. It is not a performance knob;
// the check is a single pass over a name that is about to be copied anyway.
#ifndef D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
#   define D_CFG_ARCHIVE_CHECK_ENTRY_NAMES 1
#endif


// --- 0.6  Composition ---

// D_CFG_ARCHIVE_TAR_GZ_STAGING
//   brief: write tar.gz through a caller's stage (1, the default), or report
// the format unsupported for writing (0).
//   The kernel cannot compose the two transforms in one pass: that needs a
// streaming tar feeding the codec's input, and the codec leaves consume whole
// buffers. So the tar is written into a stage the caller supplies and
// compressed from there: d_archive_stage_size and d_archive_create_staged in
// C, a vector the facade owns in C++. Both faces support the same formats.
//   Reading needs no stage and is not governed by this knob.
#ifndef D_CFG_ARCHIVE_TAR_GZ_STAGING
#   define D_CFG_ARCHIVE_TAR_GZ_STAGING 1
#endif


// --- 0.7  Entry compression ---
//   The defaults a container's entries are written with when the caller leaves
// the matching archive_options knob unset. A knob the caller sets always wins:
// these choose only what "not said" means (the owner's ruling of 2026-11-09:
// complete configurability when the user decides, sensible defaults when not).

// D_CFG_ARCHIVE_STORE_ONLY
//   brief: write every ZIP and 7z entry uncompressed (1), or let each format
// compress its entries (0, the default).
//   A stored entry's bytes are the file's own, so they can be read, diffed or
// searched inside the container without a codec. A compressed one is smaller;
// for text-like content such as an uncompressed PDF, much smaller. tar is never
// compressed and a gzip stream always is, so neither format reads this knob.
#ifndef D_CFG_ARCHIVE_STORE_ONLY
#   define D_CFG_ARCHIVE_STORE_ONLY 0
#endif

// D_CFG_ARCHIVE_LEVEL
//   brief: the container's compression effort on the generic 0-9 scale, or -1
// (the default) for the codec's own default, D_CFG_COMPRESS_DEFAULT_LEVEL.
//   The level reaches the codec inside the container too, unless that codec's
// own level is set.
#ifndef D_CFG_ARCHIVE_LEVEL
#   define D_CFG_ARCHIVE_LEVEL -1
#endif

// D_CFG_ARCHIVE_ZIP_METHOD
//   brief: the method a ZIP entry is written with, by its number in the ZIP
// format: 0 store, 8 deflate, 12 bzip2, 93 zstd, 95 xz; or -1 (the default),
// which is deflate where a DEFLATE provider is built in and store where none
// is.  14 (lzma) is not offered: ZIP's LZMA is a raw LZMA1 stream, which the
// codec set does not make.
//   An entry whose method has no codec in the build is stored instead, one
// entry at a time, so a method named here never makes a container unwritable.
#ifndef D_CFG_ARCHIVE_ZIP_METHOD
#   define D_CFG_ARCHIVE_ZIP_METHOD -1
#endif

// D_CFG_ARCHIVE_SEVENZIP_METHOD
//   brief: the method a 7z container is written with: 0 LZMA2 (the default),
// 1 LZMA, 2 bzip2, 3 deflate, 4 zstd, 5 copy (stored).
#ifndef D_CFG_ARCHIVE_SEVENZIP_METHOD
#   define D_CFG_ARCHIVE_SEVENZIP_METHOD 0
#endif


// --- 0.8  Validation of the knobs ---

#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_ALL)
#   error "D_CFG_ARCHIVE_ALL must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_FORMAT_ZIP)
#   error "D_CFG_ARCHIVE_FORMAT_ZIP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_FORMAT_TAR)
#   error "D_CFG_ARCHIVE_FORMAT_TAR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_FORMAT_GZ)
#   error "D_CFG_ARCHIVE_FORMAT_GZ must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_FORMAT_7Z)
#   error "D_CFG_ARCHIVE_FORMAT_7Z must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_FORMAT_RAR)
#   error "D_CFG_ARCHIVE_FORMAT_RAR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_PREFER_BUILTIN)
#   error "D_CFG_ARCHIVE_PREFER_BUILTIN must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_REPRODUCIBLE)
#   error "D_CFG_ARCHIVE_REPRODUCIBLE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_CHECK_ENTRY_NAMES)
#   error "D_CFG_ARCHIVE_CHECK_ENTRY_NAMES must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_TAR_GZ_STAGING)
#   error "D_CFG_ARCHIVE_TAR_GZ_STAGING must be 0 or 1"
#endif

#if ( D_CFG_NORM(D_CFG_ARCHIVE_EPOCH) < 315532800L )
#   error "D_CFG_ARCHIVE_EPOCH must not precede the ZIP epoch (315532800)"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARCHIVE_STORE_ONLY)
#   error "D_CFG_ARCHIVE_STORE_ONLY must be 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_ARCHIVE_LEVEL) < -1) ||                                \
      (D_CFG_NORM(D_CFG_ARCHIVE_LEVEL) > 9) )
#   error "D_CFG_ARCHIVE_LEVEL must be -1 or on the 0-9 generic scale"
#endif
#if ( (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != -1) &&                          \
      (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != 0)  &&                          \
      (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != 8)  &&                          \
      (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != 12) &&                          \
      (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != 93) &&                          \
      (D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD) != 95) )
#   error "D_CFG_ARCHIVE_ZIP_METHOD must be -1, 0, 8, 12, 93 or 95"
#endif
#if ( (D_CFG_NORM(D_CFG_ARCHIVE_SEVENZIP_METHOD) < 0) ||                       \
      (D_CFG_NORM(D_CFG_ARCHIVE_SEVENZIP_METHOD) > 5) )
#   error "D_CFG_ARCHIVE_SEVENZIP_METHOD must be 0 to 5"
#endif


// --- 0.9  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_ARCHIVE_CAN_WRITE_* / _CAN_READ_*
//   brief: 1 when the format is both reachable in this build AND wanted. tar
// and ZIP are writable with no library at all, which is why their write gates
// read the built-in flags rather than a detection.
#define D_INTERNAL_ARCHIVE_CAN_WRITE_ZIP                                       \
    ( D_ENV_ARCHIVE_CAN_WRITE_ZIP &&                                           \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_ZIP) )
#define D_INTERNAL_ARCHIVE_CAN_READ_ZIP                                        \
    ( D_ENV_ARCHIVE_CAN_READ_ZIP &&                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_ZIP) )
#define D_INTERNAL_ARCHIVE_CAN_WRITE_TAR                                       \
    ( D_ENV_ARCHIVE_CAN_WRITE_TAR &&                                           \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_TAR) )
#define D_INTERNAL_ARCHIVE_CAN_READ_TAR                                        \
    ( D_ENV_ARCHIVE_CAN_READ_TAR &&                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_TAR) )

//   gz rides on the codec cascade, not the archive one: a build that excluded
// DEFLATE has no gzip stream to put a member in, whatever this file says.
#define D_INTERNAL_ARCHIVE_CAN_WRITE_GZ                                        \
    ( D_INTERNAL_COMPRESS_GZIP_WRAP &&                                         \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_GZ) )
#define D_INTERNAL_ARCHIVE_CAN_READ_GZ  D_INTERNAL_ARCHIVE_CAN_WRITE_GZ

//   tar.gz needs BOTH halves, and the staging knob on top -- three conditions,
// resolved here so the module tests one symbol.
#define D_INTERNAL_ARCHIVE_CAN_WRITE_TGZ                                       \
    ( D_INTERNAL_ARCHIVE_CAN_WRITE_TAR &&                                      \
      D_INTERNAL_ARCHIVE_CAN_WRITE_GZ  &&                                      \
      D_CFG_IS_ON(D_CFG_ARCHIVE_TAR_GZ_STAGING) )

#define D_INTERNAL_ARCHIVE_CAN_WRITE_7Z                                        \
    ( D_ENV_ARCHIVE_CAN_WRITE_7Z &&                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_7Z) )
#define D_INTERNAL_ARCHIVE_CAN_READ_7Z                                         \
    ( D_ENV_ARCHIVE_CAN_READ_7Z &&                                             \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_7Z) )
#define D_INTERNAL_ARCHIVE_CAN_WRITE_RAR                                       \
    ( D_ENV_ARCHIVE_CAN_WRITE_RAR &&                                           \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_RAR) )
#define D_INTERNAL_ARCHIVE_CAN_READ_RAR                                        \
    ( D_ENV_ARCHIVE_CAN_READ_RAR &&                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_FORMAT_RAR) )

// D_INTERNAL_ARCHIVE_PREFER_BUILTIN / _CHECK_ENTRY_NAMES
//   brief: strict 0/1 gates for the module's `#if`s. Both are forced ON in a
// test build: a parity suite against library-written containers measures the
// library, and a suite that skips the traversal guard cannot assert it.
#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_PREFER_BUILTIN) )
#   define D_INTERNAL_ARCHIVE_PREFER_BUILTIN 1
#else
#   define D_INTERNAL_ARCHIVE_PREFER_BUILTIN 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_ARCHIVE_CHECK_ENTRY_NAMES) )
#   define D_INTERNAL_ARCHIVE_CHECK_ENTRY_NAMES 1
#else
#   define D_INTERNAL_ARCHIVE_CHECK_ENTRY_NAMES 0
#endif

// D_INTERNAL_ARCHIVE_EPOCH / _USE_CLOCK
//   brief: what an entry with no mtime records. Under
// D_CFG_ARCHIVE_REPRODUCIBLE it is the pinned epoch, D_CFG_ARCHIVE_EPOCH, and
// _USE_CLOCK is 0; otherwise _USE_CLOCK is 1 and the module reads the clock,
// falling back to the epoch only if the clock fails.
#if D_CFG_IS_ON(D_CFG_ARCHIVE_REPRODUCIBLE)
#   define D_INTERNAL_ARCHIVE_EPOCH     D_CFG_ARCHIVE_EPOCH
#   define D_INTERNAL_ARCHIVE_USE_CLOCK 0
#else
#   define D_INTERNAL_ARCHIVE_EPOCH     D_CFG_ARCHIVE_EPOCH
#   define D_INTERNAL_ARCHIVE_USE_CLOCK 1
#endif

// D_INTERNAL_ARCHIVE_TAR_GZ_STAGING
//   brief: 1 when the C++ face composes tar.gz from two kernel calls.
#if D_CFG_IS_ON(D_CFG_ARCHIVE_TAR_GZ_STAGING)
#   define D_INTERNAL_ARCHIVE_TAR_GZ_STAGING 1
#else
#   define D_INTERNAL_ARCHIVE_TAR_GZ_STAGING 0
#endif

// D_INTERNAL_ARCHIVE_STORE_ONLY / _LEVEL / _ZIP_METHOD / _SEVENZIP_METHOD
//   brief: the entry-compression defaults d_archive_options_resolve gives an
// unset knob. _LEVEL resolves -1 to the codec cascade's default level; a
// _ZIP_METHOD of -1 is resolved at run time, from whether DEFLATE can compress.
#if D_CFG_IS_ON(D_CFG_ARCHIVE_STORE_ONLY)
#   define D_INTERNAL_ARCHIVE_STORE_ONLY 1
#else
#   define D_INTERNAL_ARCHIVE_STORE_ONLY 0
#endif

#if ( D_CFG_NORM(D_CFG_ARCHIVE_LEVEL) < 0 )
#   define D_INTERNAL_ARCHIVE_LEVEL D_INTERNAL_COMPRESS_DEFAULT_LEVEL
#else
#   define D_INTERNAL_ARCHIVE_LEVEL D_CFG_NORM(D_CFG_ARCHIVE_LEVEL)
#endif

#define D_INTERNAL_ARCHIVE_ZIP_METHOD      D_CFG_NORM(D_CFG_ARCHIVE_ZIP_METHOD)
#define D_INTERNAL_ARCHIVE_SEVENZIP_METHOD                                     \
    D_CFG_NORM(D_CFG_ARCHIVE_SEVENZIP_METHOD)


#endif  // DJINTERP_CONFIG_C_UTIL_ARCHIVE_CFG_ARCHIVE_H
