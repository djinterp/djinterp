/*******************************************************************************
* djinterp [config]                                                cfg_archive.h
*
*   Build-time configuration for the archive facade: which formats are compiled
* in, whether the built-in writers are used in preference to a detected
* library, and how much of a container's content is allowed to come from the
* machine it was built on.
*
*   targets:  core/util/archive_common.h -> D_INTERNAL_ARCHIVE_*
*             core/util/archive.h        -> D_INTERNAL_ARCHIVE_*
*             core/util/archive/archive.hpp      -> D_INTERNAL_ARCHIVE_TAR_GZ_STAGING
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
*                                                            revised: 2026.09.29
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
      7.    Validation of the knobs
      8.    Derived values               (D_INTERNAL_ARCHIVE_*)
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
//   brief: let the C++ face implement tar.gz by staging the tar in a byte_blob
// and compressing it (1, the default), or report the format unsupported (0).
//   The kernel cannot compose the two transforms in one pass: that needs a
// streaming tar feeding the codec's input, and the codec leaves consume whole
// buffers. The C++ face has somewhere to stage and the C face does not, so with
// this ON the two faces genuinely differ in what they support -- an honest
// asymmetry, recorded rather than hidden.
//   Set 0 for a build that wants the two faces to expose exactly the same
// formats, at the cost of losing tar.gz from both.
#ifndef D_CFG_ARCHIVE_TAR_GZ_STAGING
#   define D_CFG_ARCHIVE_TAR_GZ_STAGING 1
#endif


// --- 0.7  Validation of the knobs ---

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


// --- 0.8  Derived values (D_INTERNAL_*) ---

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

// D_INTERNAL_ARCHIVE_EPOCH
//   brief: the timestamp an entry with no mtime records. Resolves to the pinned
// epoch under D_CFG_ARCHIVE_REPRODUCIBLE and to 0 otherwise, where 0 is the
// module's signal to read the clock.
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


#endif  // DJINTERP_CONFIG_C_UTIL_ARCHIVE_CFG_ARCHIVE_H
