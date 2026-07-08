/******************************************************************************
* djinterp [core]                                             archive_option.hpp
*
*   djinterp archive option vocabulary:
* The full, format-aware tuning surface for archive.hpp. archive.hpp ships a
* minimal archive_options (a generic `level` and a ZIP `store_only`); this
* header is its authoritative, expanded form. It layers the container-specific
* knobs the backends detected by env_archive.h expose - ZIP method /
* encryption / zip64, tar variant, gzip header fields, and the 7z method /
* solid / header options - on top of the complete codec surface from
* compress_option.hpp, which it embeds for the compressed stream inside the
* container:
*
*     archive_options opt;
*     opt.codec.level    = 9;                 // the stream codec's effort
*     opt.zip.method     = zip_method_deflate;
*     opt.zip.encryption = zip_encryption_aes256;
*     opt.zip.password   = "hunter2";
*     byte_buffer blob   = archive<zip>(entries, opt);
*
*   DESIGN:
*   Like the facade, the surface is plain data and version-portable
* (C++98 - C++23): plain enums, struct members defaulted in constructors (no
* in-class initializers, no enum class), and djinterp's OWN categorical enums
* that the dispatch leaves in archive.cpp translate to backend constants. No
* third-party archive header is pulled in.
*
*   THE EMBEDDED CODEC:
*   archive_options HAS-A compress_options (`codec`). The compressed formats
* (zip's deflate / bzip2 / lzma / zstd methods, gz, tar.gz, 7z) route their
* stream tuning through it, so the entire codec knob-set is reachable without
* duplication. Uncompressed tar and the store path ignore it. The top-level
* `level` is a convenience mirror of codec.level: when codec.level is left at
* its default (-1), `level` is used; otherwise codec.level wins.
*
*   AVAILABILITY:
*   Selecting a method or option a backend cannot honour (e.g. AES on a
* built-in ZIP writer, or any RAR creation without the rar/WinRAR tool) yields
* status_unavailable at runtime, exactly as the facade describes. Query the
* compile-time matrix through format_traits<Format>::can_{read,write}.
*
*   INTEGRATION:
*   This header defines djinterp::archive_options. archive.hpp should include it
* and drop its inline stub so the two never collide; the layout here is a
* strict superset of the stub (`level` and `store_only`, with their defaults,
* are preserved), so existing call sites are unaffected.
*
* path:      /inc/djinterp/core/util/archive_option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CATEGORICAL ENUMS
      i.    zip_method
      ii.   zip_encryption
      iii.  zip64_policy
      iv.   tar_format
      v.    sevenzip_method
II.   PER-FORMAT OPTION STRUCTS
      i.    zip_options
      ii.   tar_options
      iii.  gz_options
      iv.   sevenzip_options
      v.    rar_options
III.  AGGREGATE
      i.    archive_options
*/

#ifndef DJINTERP_ARCHIVE_OPTION_HPP_
#define DJINTERP_ARCHIVE_OPTION_HPP_ 1

#include <string>
#include "./compress_options.hpp"


NS_DJINTERP

// =============================================================================
// I.   CATEGORICAL ENUMS
// =============================================================================
// djinterp's own spellings for the containers' categorical knobs, mapped to
// backend constants by the dispatch leaves. The first enumerator of each is
// the neutral default.

// zip_method
//   enum: the per-entry compression method for ZIP. store and deflate work
// everywhere a DEFLATE codec is present; the rest need a backend that supports
// the modern ZIP methods (e.g. libzip or minizip-ng) plus the matching codec.
enum zip_method
{
    zip_method_default = 0,       // deflate where available, else store
    zip_method_store,             // method 0  : no compression
    zip_method_deflate,           // method 8  : DEFLATE
    zip_method_bzip2,             // method 12 : bzip2
    zip_method_lzma,              // method 14 : LZMA
    zip_method_zstd,              // method 93 : Zstandard
    zip_method_xz                 // method 95 : xz
};

// zip_encryption
//   enum: the ZIP encryption scheme. traditional (PKWARE / ZipCrypto) is
// portable but weak; the AES variants are the WinZip AE-x scheme and need a
// backend that implements them.
enum zip_encryption
{
    zip_encryption_none = 0,      // no encryption
    zip_encryption_traditional,   // PKWARE / ZipCrypto (legacy, weak)
    zip_encryption_aes128,        // WinZip AES-128
    zip_encryption_aes192,        // WinZip AES-192
    zip_encryption_aes256         // WinZip AES-256
};

// zip64_policy
//   enum: when to emit the ZIP64 extensions that lift the 4 GB / 65535-entry
// limits of the classic format.
enum zip64_policy
{
    zip64_auto = 0,               // enable only when a limit would be crossed
    zip64_always,                 // always emit ZIP64 records
    zip64_never                   // never emit ZIP64 (fails past the limits)
};

// tar_format
//   enum: the tar variant to emit. ustar is the portable default; pax encodes
// long names, large ids, and sub-second times via extended headers; gnu / v7 /
// oldgnu select those historical layouts.
enum tar_format
{
    tar_format_ustar = 0,         // POSIX.1-1988 ustar
    tar_format_pax,               // POSIX.1-2001 pax (extended headers)
    tar_format_gnu,               // GNU tar
    tar_format_v7,                // pre-POSIX V7
    tar_format_oldgnu             // legacy GNU
};

// sevenzip_method
//   enum: the 7z stream method. lzma2 is the modern default; finer tuning
// (dictionary, etc.) flows through the embedded codec's lzma block.
enum sevenzip_method
{
    sevenzip_method_lzma2 = 0,    // LZMA2 (default)
    sevenzip_method_lzma,         // LZMA
    sevenzip_method_ppmd,         // PPMd
    sevenzip_method_bzip2,        // bzip2
    sevenzip_method_deflate,      // DEFLATE
    sevenzip_method_copy          // stored, no compression
};


// =============================================================================
// II.  PER-FORMAT OPTION STRUCTS
// =============================================================================
// One block of container-specific knobs per format. A block is consulted only
// when its format is the one selected at the call site. The compressed stream
// itself is tuned through archive_options::codec, not here.

// zip_options
//   struct: ZIP container options. The method's compression effort and
// advanced codec knobs come from archive_options::codec; these cover the
// container-level choices DEFLATE-or-otherwise.
struct zip_options
{
    // method: per-entry compression method.
    zip_method     method;

    // encryption: encryption scheme for entry data.
    zip_encryption encryption;

    // password: passphrase for encryption; required when encryption is not
    // none, ignored otherwise.
    std::string    password;

    // zip64: ZIP64-extension policy.
    zip64_policy   zip64;

    // utf8_names: flag entry names as UTF-8 (the language-encoding bit) rather
    // than the historical code page.
    bool           utf8_names;

    zip_options()
        : method(zip_method_default),
          encryption(zip_encryption_none),
          zip64(zip64_auto),
          utf8_names(true)
    {}
};

// tar_options
//   struct: tar container options. tar is uncompressed; for tar.gz the gzip
// stream is tuned through archive_options::codec.
struct tar_options
{
    // format: the tar variant to emit.
    tar_format format;

    // numeric_owner: store raw uid / gid only, without resolving owner and
    // group names.
    bool       numeric_owner;

    tar_options()
        : format(tar_format_ustar),
          numeric_owner(false)
    {}
};

// gz_options
//   struct: gzip-stream header options. The compression level and codec knobs
// come from archive_options::codec; these are the optional FNAME / MTIME
// header fields.
struct gz_options
{
    // store_name: write the original file name into the gzip header (FNAME).
    bool        store_name;

    // store_mtime: write a modification time into the gzip header. When set and
    // no time is supplied, the current time is used.
    bool        store_mtime;

    // original_name: the name to record when store_name is set; empty falls
    // back to the sole entry's name.
    std::string original_name;

    gz_options()
        : store_name(false),
          store_mtime(true)
    {}
};

// sevenzip_options
//   struct: 7z container options. The LZMA dictionary and related tuning flow
// through archive_options::codec; these are the container-level choices.
struct sevenzip_options
{
    // method: the stream compression method.
    sevenzip_method method;

    // solid: pack entries into shared solid blocks for a better ratio at the
    // cost of random access.
    bool            solid;

    // header_compression: compress the archive header (names, sizes).
    bool            header_compression;

    // header_encryption: encrypt the header as well as the data; requires a
    // password.
    bool            header_encryption;

    // password: passphrase for AES-256 encryption; empty leaves the archive
    // unencrypted.
    std::string     password;

    // threads: worker threads for compression. 0 lets the backend choose.
    int             threads;

    sevenzip_options()
        : method(sevenzip_method_lzma2),
          solid(true),
          header_compression(true),
          header_encryption(false),
          threads(0)
    {}
};

// rar_options
//   struct: RAR creation options. No library can write RAR; these apply only
// when the proprietary rar / WinRAR tool drives creation, and are otherwise
// inert (the call returns status_unavailable).
struct rar_options
{
    // level: RAR compression level (0 = store .. 5 = best). -1 forwards the
    // generic effort.
    int         level;

    // solid: build a solid archive.
    bool        solid;

    // recovery_record: add a recovery record for damage repair.
    bool        recovery_record;

    // password: passphrase for encryption; empty leaves the archive
    // unencrypted.
    std::string password;

    rar_options()
        : level(-1),
          solid(false),
          recovery_record(false)
    {}
};


// =============================================================================
// III. AGGREGATE
// =============================================================================

// archive_options
//   struct: the complete set of tuning knobs passed to an archive-creation
// call. The compressed stream inside the container is tuned through `codec`;
// the per-format blocks cover container-level choices.
struct archive_options
{
    // level: convenience effort mirroring codec.level. -1 selects the backend
    // default. When codec.level is left at its own default (-1), this value is
    // used; otherwise codec.level takes precedence.
    int              level;

    // store_only: for ZIP, force the store method even when a DEFLATE backend
    // is available. Equivalent to zip.method = zip_method_store, and wins over
    // it when set. Ignored by other formats.
    bool             store_only;

    // codec: full tuning for the compressed stream inside the container. zip's
    // compressing methods, gz, tar.gz, and 7z route through it; uncompressed
    // tar and the store path ignore it.
    compress_options codec;

    // comment: archive-level comment (ZIP and 7z); empty writes none.
    std::string      comment;

    // preserve_permissions: carry entry.mode into the container where the
    // format records unix permissions.
    bool             preserve_permissions;

    // preserve_mtime: carry entry.mtime into the container.
    bool             preserve_mtime;

    // per-format options. consulted only for the selected format.
    zip_options      zip;
    tar_options      tar;
    gz_options       gz;
    sevenzip_options sevenzip;
    rar_options      rar;

    archive_options()
        : level(-1),
          store_only(false),
          preserve_permissions(true),
          preserve_mtime(true)
    {}
};

}  // namespace djinterp


#endif  // DJINTERP_ARCHIVE_OPTION_HPP_
