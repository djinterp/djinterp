/******************************************************************************
* djinterp [core]                                            compress_option.hpp
*
*   djinterp compression option vocabulary:
* The full, codec-aware tuning surface for compress.hpp. compress.hpp
* ships a minimal compress_options (a single generic `level`); this header is
* its authoritative, expanded form. It collects every knob the codecs detected
* by env_compress.h expose - DEFLATE window / strategy, bzip2 block size, xz /
* lzma filter parameters, the wide zstd advanced-parameter set, the LZ4 frame
* descriptor, and Brotli quality / window - behind one aggregate:
*
*     compress_options opt;
*     opt.level        = 9;        // generic effort, mapped per codec
*     opt.zstd.window_log = 27;    // advanced, honoured only for zstd
*     byte_buffer p = compress<zstd>(data, opt);
*
*   DESIGN:
*   Like the facade, the surface is plain data and version-portable
* (C++98 - C++23): tag-free POD-ish structs, plain enums, and defaults set in
* constructors (no in-class initializers, no enum class). The categorical knobs
* are djinterp's OWN enums; the dispatch leaves in compress.cpp translate
* them to backend constants, so this header pulls in no third-party codec
* headers and adds no dependency.
*
*   SENTINELS:
*   Every advanced knob has a "leave it to the backend" sentinel so the common
* path touches nothing. For most integer knobs that is 0 (a value their natural
* range excludes); for the few where 0 is itself meaningful (xz lc / lp / pb)
* the sentinel is -1. Each field documents its own. A codec-specific effort
* field (zstd.level, lz4.level, brotli.quality), when moved off its sentinel,
* overrides the generic compress_options::level for that codec.
*
*   INTEGRATION:
*   This header defines djinterp::compress_options. compress.hpp should
* include it and drop its inline stub so the two never collide; the per-codec
* blocks below are a strict superset of the stub (the `level` field and its -1
* default are preserved), so existing call sites are unaffected.
*
* path:      /inc/djinterp/core/util/compress_option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CATEGORICAL ENUMS
      i.    deflate_strategy
      ii.   lzma_check
      iii.  lzma_mode
      iv.   lzma_match_finder
      v.    zstd_strategy
      vi.   lz4_block_size
      vii.  lz4_block_mode
      viii. brotli_mode
II.   PER-CODEC OPTION STRUCTS
      i.    deflate_options    (zlib / zlib-ng / miniz / libdeflate)
      ii.   bzip2_options      (bzip2)
      iii.  lzma_options       (liblzma : xz)
      iv.   zstd_options       (Zstandard)
      v.    lz4_options        (LZ4 frame)
      vi.   brotli_options     (Brotli)
III.  AGGREGATE
      i.    compress_options
*/

#ifndef DJINTERP_COMPRESS_OPTION_HPP_
#define DJINTERP_COMPRESS_OPTION_HPP_ 1

#include <cstddef>


NS_DJINTERP

// =============================================================================
// I.   CATEGORICAL ENUMS
// =============================================================================
// djinterp's own spellings for the codecs' categorical knobs. They are mapped
// to backend constants by the dispatch leaves; the first enumerator of each is
// the neutral "let the codec / level decide" default.

// deflate_strategy
//   enum: DEFLATE strategy hint (the zlib Z_* strategies). tunes how the
// encoder trades ratio against structure; the default suits most inputs.
enum deflate_strategy
{
    deflate_strategy_default = 0, // Z_DEFAULT_STRATEGY
    deflate_strategy_filtered,    // Z_FILTERED      (small, partly random data)
    deflate_strategy_huffman_only,// Z_HUFFMAN_ONLY  (entropy code only)
    deflate_strategy_rle,         // Z_RLE           (match distance 1; PNG-ish)
    deflate_strategy_fixed        // Z_FIXED         (no dynamic Huffman trees)
};

// lzma_check
//   enum: the integrity check embedded in an xz container.
enum lzma_check
{
    lzma_check_none = 0,          // LZMA_CHECK_NONE
    lzma_check_crc32,             // LZMA_CHECK_CRC32
    lzma_check_crc64,             // LZMA_CHECK_CRC64  (the xz default)
    lzma_check_sha256             // LZMA_CHECK_SHA256
};

// lzma_mode
//   enum: the lzma encoder mode (fast or normal); default derives it from the
// preset / generic level.
enum lzma_mode
{
    lzma_mode_default = 0,        // derive from the preset
    lzma_mode_fast,               // LZMA_MODE_FAST
    lzma_mode_normal              // LZMA_MODE_NORMAL
};

// lzma_match_finder
//   enum: the lzma match finder; default derives it from the preset.
enum lzma_match_finder
{
    lzma_mf_default = 0,          // derive from the preset
    lzma_mf_hc3,                  // LZMA_MF_HC3  (hash chain, 3 bytes)
    lzma_mf_hc4,                  // LZMA_MF_HC4  (hash chain, 4 bytes)
    lzma_mf_bt2,                  // LZMA_MF_BT2  (binary tree, 2 bytes)
    lzma_mf_bt3,                  // LZMA_MF_BT3  (binary tree, 3 bytes)
    lzma_mf_bt4                   // LZMA_MF_BT4  (binary tree, 4 bytes)
};

// zstd_strategy
//   enum: the Zstandard match strategy, from fastest to highest ratio; default
// lets the chosen level pick.
enum zstd_strategy
{
    zstd_strategy_default = 0,    // let the level decide
    zstd_strategy_fast,           // ZSTD_fast
    zstd_strategy_dfast,          // ZSTD_dfast
    zstd_strategy_greedy,         // ZSTD_greedy
    zstd_strategy_lazy,           // ZSTD_lazy
    zstd_strategy_lazy2,          // ZSTD_lazy2
    zstd_strategy_btlazy2,        // ZSTD_btlazy2
    zstd_strategy_btopt,          // ZSTD_btopt
    zstd_strategy_btultra,        // ZSTD_btultra
    zstd_strategy_btultra2        // ZSTD_btultra2
};

// lz4_block_size
//   enum: the maximum block size inside an LZ4 frame.
enum lz4_block_size
{
    lz4_block_size_default = 0,   // LZ4F_default
    lz4_block_size_64kb,          // LZ4F_max64KB
    lz4_block_size_256kb,         // LZ4F_max256KB
    lz4_block_size_1mb,           // LZ4F_max1MB
    lz4_block_size_4mb            // LZ4F_max4MB
};

// lz4_block_mode
//   enum: whether LZ4 frame blocks reference earlier blocks (better ratio) or
// stand alone (parallel / seekable decode).
enum lz4_block_mode
{
    lz4_block_mode_linked = 0,    // LZ4F_blockLinked       (default)
    lz4_block_mode_independent    // LZ4F_blockIndependent
};

// brotli_mode
//   enum: the Brotli content-mode hint.
enum brotli_mode
{
    brotli_mode_generic = 0,      // BROTLI_MODE_GENERIC
    brotli_mode_text,             // BROTLI_MODE_TEXT  (UTF-8 text)
    brotli_mode_font              // BROTLI_MODE_FONT  (WOFF2)
};


// =============================================================================
// II.  PER-CODEC OPTION STRUCTS
// =============================================================================
// One block of advanced knobs per codec family. A block is consulted only when
// its codec is the one selected at the call site; fields left at their
// sentinels fall back to backend defaults. Unavailable codecs ignore their
// block entirely and the call returns status_unavailable at runtime.

// deflate_options
//   struct: advanced tuning for raw DEFLATE, zlib-wrapped, and gzip-wrapped
// streams (zlib / zlib-ng / miniz / libdeflate). The stream wrapper is fixed
// by the codec tag, so only the window magnitude is given here.
struct deflate_options
{
    // window_bits: base-2 log of the sliding window (8..15). 0 selects the
    // backend maximum (15).
    int              window_bits;

    // mem_level: encoder working-memory / speed trade (1..9). 0 selects the
    // backend default (8).
    int              mem_level;

    // strategy: DEFLATE strategy hint.
    deflate_strategy strategy;

    deflate_options()
        : window_bits(0),
          mem_level(0),
          strategy(deflate_strategy_default)
    {}
};

// bzip2_options
//   struct: advanced tuning for bzip2.
struct bzip2_options
{
    // block_size_100k: block size in units of 100 kB (1..9). 0 derives it from
    // the generic level (else the backend maximum, 9).
    int  block_size_100k;

    // work_factor: effort spent on repetitive data before the fallback
    // algorithm engages (0..250). 0 selects the backend default (30).
    int  work_factor;

    // verbosity: backend diagnostic verbosity (0..4); 0 is silent.
    int  verbosity;

    // small_decompress: decompress with the low-memory algorithm.
    bool small_decompress;

    bzip2_options()
        : block_size_100k(0),
          work_factor(0),
          verbosity(0),
          small_decompress(false)
    {}
};

// lzma_options
//   struct: advanced tuning for liblzma (the xz container). The preset is
// taken from the generic level; these refine the resulting filter.
struct lzma_options
{
    // extreme: apply the preset's "extreme" variant (slower, marginally
    // smaller).
    bool              extreme;

    // check: integrity check written into the container.
    lzma_check        check;

    // dict_size: dictionary size in bytes. 0 derives it from the preset.
    std::size_t       dict_size;

    // lc: literal context bits (0..4); lc + lp <= 4. -1 keeps the preset
    // default (3).
    int               lc;

    // lp: literal position bits (0..4). -1 keeps the preset default (0).
    int               lp;

    // pb: position bits (0..4). -1 keeps the preset default (2).
    int               pb;

    // mode: encoder mode.
    lzma_mode         mode;

    // nice_len: "nice" match length to stop searching at (2..273). 0 derives
    // it from the preset.
    int               nice_len;

    // mf: match finder.
    lzma_match_finder mf;

    // depth: match-finder search depth. 0 lets the backend choose from mf and
    // nice_len.
    int               depth;

    // threads: worker threads for the multi-threaded encoder. 0 or 1 encode
    // single-threaded; the .xz output is interoperable either way.
    unsigned int      threads;

    lzma_options()
        : extreme(false),
          check(lzma_check_crc64),
          dict_size(0),
          lc(-1),
          lp(-1),
          pb(-1),
          mode(lzma_mode_default),
          nice_len(0),
          mf(lzma_mf_default),
          depth(0),
          threads(0)
    {}
};

// zstd_options
//   struct: advanced tuning for Zstandard, mirroring its advanced-parameter
// API. Every numeric knob takes 0 as "backend default".
struct zstd_options
{
    // level: Zstandard level on its own scale (roughly -7..22; negatives are
    // the fast modes). 0 means "unset" - the generic compress_options::level is
    // consulted first.
    int           level;

    // window_log: base-2 log of the match window. 0 = backend default.
    int           window_log;

    // hash_log: base-2 log of the hash table. 0 = backend default.
    int           hash_log;

    // chain_log: base-2 log of the match-chain table. 0 = backend default.
    int           chain_log;

    // search_log: base-2 log of the search effort. 0 = backend default.
    int           search_log;

    // min_match: minimum match length. 0 = backend default.
    int           min_match;

    // target_length: target match length for the optimal strategies.
    // 0 = backend default.
    int           target_length;

    // strategy: match strategy.
    zstd_strategy strategy;

    // long_distance_matching: enable the long-distance matcher (helps large,
    // redundant inputs).
    bool          long_distance_matching;

    // ldm_hash_log: long-distance-matcher hash-table log. 0 = backend default;
    // consulted only when long_distance_matching is set.
    int           ldm_hash_log;

    // ldm_min_match: long-distance-matcher minimum match. 0 = backend default.
    int           ldm_min_match;

    // ldm_bucket_size_log: long-distance-matcher bucket-size log.
    // 0 = backend default.
    int           ldm_bucket_size_log;

    // ldm_hash_rate_log: long-distance-matcher hash-rate log. 0 = backend
    // default.
    int           ldm_hash_rate_log;

    // content_size_flag: write the decompressed size into the frame header.
    bool          content_size_flag;

    // checksum_flag: append a content checksum to the frame.
    bool          checksum_flag;

    // dict_id_flag: write the dictionary id into the frame header.
    bool          dict_id_flag;

    // workers: worker threads for multi-threaded compression. 0 = single
    // threaded.
    int           workers;

    // job_size: per-job size in bytes for multi-threaded compression.
    // 0 = backend default.
    std::size_t   job_size;

    // overlap_log: inter-job overlap for multi-threaded compression.
    // 0 = backend default.
    int           overlap_log;

    zstd_options()
        : level(0),
          window_log(0),
          hash_log(0),
          chain_log(0),
          search_log(0),
          min_match(0),
          target_length(0),
          strategy(zstd_strategy_default),
          long_distance_matching(false),
          ldm_hash_log(0),
          ldm_min_match(0),
          ldm_bucket_size_log(0),
          ldm_hash_rate_log(0),
          content_size_flag(true),
          checksum_flag(false),
          dict_id_flag(true),
          workers(0),
          job_size(0),
          overlap_log(0)
    {}
};

// lz4_options
//   struct: advanced tuning for the LZ4 frame format (the descriptor plus the
// encoder level).
struct lz4_options
{
    // level: LZ4 frame level. 0 is the fast codec; values >= 3 engage LZ4HC up
    // to 12 (maximum ratio). 0 means "unset" - the generic
    // compress_options::level is consulted first.
    int            level;

    // block_size: maximum block size within the frame.
    lz4_block_size block_size;

    // block_mode: linked (better ratio) or independent (parallel / seekable)
    // blocks.
    lz4_block_mode block_mode;

    // content_checksum: append a whole-frame checksum.
    bool           content_checksum;

    // block_checksum: append a per-block checksum.
    bool           block_checksum;

    // store_content_size: record the decompressed size in the frame header.
    bool           store_content_size;

    // favor_dec_speed: bias the HC encoder toward decompression speed.
    bool           favor_dec_speed;

    lz4_options()
        : level(0),
          block_size(lz4_block_size_default),
          block_mode(lz4_block_mode_linked),
          content_checksum(false),
          block_checksum(false),
          store_content_size(false),
          favor_dec_speed(false)
    {}
};

// brotli_options
//   struct: advanced tuning for Brotli.
struct brotli_options
{
    // quality: Brotli quality (0..11). -1 derives it from the generic level
    // (else the backend default, 11). higher is smaller and slower.
    int         quality;

    // window_bits: base-2 log of the sliding window (10..24). 0 = backend
    // default (22). with large_window set, up to 30 is permitted.
    int         window_bits;

    // block_bits: base-2 log of the input block (16..24). 0 lets the backend
    // choose automatically.
    int         block_bits;

    // mode: content-mode hint.
    brotli_mode mode;

    // large_window: enable the large-window extension (non-standard window
    // sizes; encoder and decoder must agree).
    bool        large_window;

    brotli_options()
        : quality(-1),
          window_bits(0),
          block_bits(0),
          mode(brotli_mode_generic),
          large_window(false)
    {}
};


// =============================================================================
// III. AGGREGATE
// =============================================================================

// compress_options
//   struct: the complete set of tuning knobs passed to a compression call.
// `level` is the codec-agnostic effort honoured by every codec; the per-codec
// blocks refine the chosen codec only. The store codec ignores all of it.
struct compress_options
{
    // level: generic, codec-agnostic effort. -1 selects the backend default;
    // otherwise it is mapped onto the chosen codec's native scale (commonly
    // 0..9 for DEFLATE / bzip2, wider ranges for zstd / xz / brotli). A
    // codec-specific effort field below, when moved off its sentinel,
    // overrides this for that codec.
    int             level;

    // advanced, per-codec tuning. consulted only for the selected codec;
    // fields left at their sentinels fall back to backend defaults, so the
    // common path needs to set nothing here.
    deflate_options deflate;
    bzip2_options   bzip2;
    lzma_options    lzma;
    zstd_options    zstd;
    lz4_options     lz4;
    brotli_options  brotli;

    compress_options()
        : level(-1)
    {}
};

}  // namespace djinterp


#endif  // DJINTERP_COMPRESS_OPTION_HPP_
