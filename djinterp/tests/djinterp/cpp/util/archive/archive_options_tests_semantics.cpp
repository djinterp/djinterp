/*******************************************************************************
* djinterp [test]                            archive_options_tests_semantics.cpp
*
*   Tests for the value semantics archive_options relies on -- copy, assign, and
* the deep independence its std::string members require -- and for the shared
* comparators in test_archive_options.hpp that the rest of the suite trusts.
*
*   The comparator tests are deliberate: a deep comparator that silently forgets
* a field would make every "these options are identical" assertion elsewhere in
* the suite vacuous.  They walk every field of the aggregate, move it off its
* default one at a time, and require the comparator to notice.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_semantics.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.12
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // ao_moved
    //   function: whether the shared aggregate comparator reports a difference
    // between a mutated set and a pristine one.  Used by the comparator-
    // completeness tests below, one field per call.
    //
    // Parameter(s):
    //   _mutated: the option set with exactly one field moved.
    // Return:
    //   true when the comparator noticed the change.
    bool ao_moved(
        const archive_options& _mutated
    )
    {
        const archive_options fresh;

        return !::djinterp::test::archive_options_equal(_mutated, fresh);
    }
}

// D_AO_MOVES
//   macro: one comparator-completeness check: set _field to _value on a fresh
// aggregate and require the comparator to notice.
#define D_AO_MOVES(_field, _value)                                             \
    do                                                                         \
    {                                                                          \
        archive_options _o;                                                    \
        _o._field = (_value);                                                  \
        D_AO_CHECK(ao_moved(_o));                                              \
    }                                                                          \
    while (false)

/*
tests_options_copy_construction
  Verifies copy construction reproduces the whole tree, including the nested
blocks and the embedded codec.
  Tests the following:
  - a copy of a heavily customised set compares equal to its source
  - the copy's nested and codec fields carry the customised values, not defaults
  - a copy of a pristine set is itself pristine
*/
bool tests_options_copy_construction()
{
    bool            _ok = true;
    archive_options src;

    src.level                = 7;
    src.store_only           = 1;
    src.comment              = "built by the suite";
    src.preserve_permissions = 0;
    src.preserve_mtime       = 0;
    src.codec.level          = 4;
    src.codec.brotli.quality = 11;
    src.zip.method           = zip_method_lzma;
    src.zip.password         = "pw";
    src.tar.format           = tar_format_gnu;
    src.gz.original_name     = "orig";
    src.sevenzip.threads     = 8;
    src.rar.recovery_record  = 1;

    const archive_options copy(src);

    D_AO_CHECK_EQUAL_OPT(copy, src);

    D_AO_CHECK(copy.level                == 7);
    D_AO_CHECK(copy.comment              == "built by the suite");
    D_AO_CHECK(copy.preserve_permissions == 0);
    D_AO_CHECK(copy.codec.level          == 4);
    D_AO_CHECK(copy.codec.brotli.quality == 11);
    D_AO_CHECK(copy.zip.method           == zip_method_lzma);
    D_AO_CHECK(copy.zip.password         == "pw");
    D_AO_CHECK(copy.tar.format           == tar_format_gnu);
    D_AO_CHECK(copy.gz.original_name     == "orig");
    D_AO_CHECK(copy.sevenzip.threads     == 8);
    D_AO_CHECK(copy.rar.recovery_record  == 1);

    {
        const archive_options pristine;
        const archive_options pristine_copy(pristine);
        D_AO_CHECK(::djinterp::test::options_are_default(pristine_copy));
    }

    return _ok;
}

/*
tests_options_assignment
  Verifies assignment replaces the whole target tree, leaving nothing of the
target's previous state behind.
  Tests the following:
  - assigning a customised set over a differently customised one yields a set
    equal to the source, with none of the target's old values surviving
  - assigning a pristine set over a customised one restores pristine state
  - self-assignment is harmless
*/
bool tests_options_assignment()
{
    bool _ok = true;

    archive_options src;
    src.level        = 2;
    src.comment      = "source";
    src.zip.zip64    = 1;
    src.codec.lz4.level = 9;

    archive_options dst;
    dst.level            = 5;
    dst.comment          = "target";
    dst.tar.numeric_owner = 1;
    dst.sevenzip.solid    = 0;

    dst = src;

    D_AO_CHECK_EQUAL_OPT(dst, src);
    D_AO_CHECK(dst.level             == 2);
    D_AO_CHECK(dst.comment           == "source");
    D_AO_CHECK(dst.zip.zip64         == 1);
    D_AO_CHECK(dst.codec.lz4.level   == 9);
    // the target's former values are gone: the source left them unset
    D_AO_CHECK(dst.tar.numeric_owner == archive_knob_unset);
    D_AO_CHECK(dst.sevenzip.solid    == archive_knob_unset);

    // pristine over customised
    {
        archive_options       back;
        const archive_options pristine;

        back = pristine;
        D_AO_CHECK(::djinterp::test::options_are_default(back));
    }

    // self-assignment
    {
        archive_options       s;
        s.comment      = "self";
        s.zip.password = "keep";

        s = s;

        D_AO_CHECK(s.comment      == "self");
        D_AO_CHECK(s.zip.password == "keep");
    }

    return _ok;
}

/*
tests_options_deep_string_independence
  Verifies the four std::string members are deep-copied, not shared.  The blocks
are plain data with compiler-generated copies, so this pins that mutating a copy
never reaches back into its source (and vice versa) -- the property every
"forward these options onward" call site depends on.
  Tests the following:
  - comment, zip.password, gz.original_name, and sevenzip.password each stay
    independent after a copy is mutated
  - mutating the source after the copy likewise leaves the copy untouched
  - rar.password is independent too
*/
bool tests_options_deep_string_independence()
{
    bool            _ok = true;
    archive_options src;

    src.comment            = "original comment";
    src.zip.password       = "zip-secret";
    src.gz.original_name   = "original.name";
    src.sevenzip.password  = "7z-secret";
    src.rar.password       = "rar-secret";

    archive_options copy(src);

    // mutate the copy: the source must not move
    copy.comment           = "changed";
    copy.zip.password      = "other";
    copy.gz.original_name  = "other.name";
    copy.sevenzip.password = "other7z";
    copy.rar.password      = "otherrar";

    D_AO_CHECK(src.comment           == "original comment");
    D_AO_CHECK(src.zip.password      == "zip-secret");
    D_AO_CHECK(src.gz.original_name  == "original.name");
    D_AO_CHECK(src.sevenzip.password == "7z-secret");
    D_AO_CHECK(src.rar.password      == "rar-secret");

    // mutate the source: the copy must not move
    src.comment      = "mutated again";
    src.zip.password = "mutated";

    D_AO_CHECK(copy.comment      == "changed");
    D_AO_CHECK(copy.zip.password == "other");

    return _ok;
}

/*
tests_comparator_detects_archive_fields
  Verifies the shared aggregate comparator notices a change to each of the five
archive-level fields.  A comparator blind to one of these would make the
suite's equality assertions vacuous.
  Tests the following:
  - level, store_only, comment, preserve_permissions, and preserve_mtime are
    each detected when moved off their default, one at a time
*/
bool tests_comparator_detects_archive_fields()
{
    bool _ok = true;

    D_AO_MOVES(level, 5);
    D_AO_MOVES(store_only, 1);
    D_AO_MOVES(comment, "c");
    D_AO_MOVES(preserve_permissions, 0);
    D_AO_MOVES(preserve_mtime, 0);

    return _ok;
}

/*
tests_comparator_detects_format_fields
  Verifies the shared comparator notices a change to every field of every
per-format block -- all twenty of them across zip, tar, gz, sevenzip, and rar.
  Tests the following:
  - each DZipOptions field (method, encryption, password, zip64, utf8_names)
  - each DTarOptions field (format, numeric_owner)
  - each DGzOptions field (store_name, store_mtime, original_name)
  - each DSevenzipOptions field (method, solid, header_compression,
    header_encryption, password, threads)
  - each DRarOptions field (level, solid, recovery_record, password)
*/
bool tests_comparator_detects_format_fields()
{
    bool _ok = true;

    // zip
    D_AO_MOVES(zip.method, zip_method_store);
    D_AO_MOVES(zip.encryption, zip_encryption_aes_128);
    D_AO_MOVES(zip.password, "p");
    D_AO_MOVES(zip.zip64, 0);
    D_AO_MOVES(zip.utf8_names, 0);

    // tar
    D_AO_MOVES(tar.format, tar_format_v7);
    D_AO_MOVES(tar.numeric_owner, 1);

    // gz
    D_AO_MOVES(gz.store_name, 1);
    D_AO_MOVES(gz.store_mtime, 0);
    D_AO_MOVES(gz.original_name, "n");

    // sevenzip
    D_AO_MOVES(sevenzip.method, sevenzip_method_copy);
    D_AO_MOVES(sevenzip.solid, 0);
    D_AO_MOVES(sevenzip.header_compression, 0);
    D_AO_MOVES(sevenzip.header_encryption, 1);
    D_AO_MOVES(sevenzip.password, "s");
    D_AO_MOVES(sevenzip.threads, 2);

    // rar
    D_AO_MOVES(rar.level, 5);
    D_AO_MOVES(rar.solid, 1);
    D_AO_MOVES(rar.recovery_record, 1);
    D_AO_MOVES(rar.password, "r");

    return _ok;
}

/*
tests_comparator_detects_codec_fields
  Verifies the shared comparator reaches every field of the embedded codec --
the generic level plus all six per-codec blocks (49 advanced knobs).  This is
the part most easily left half-written, so every field is walked.
  Tests the following:
  - codec.level
  - all 3 deflate, 4 bzip2, 11 lzma, 19 zstd, 7 lz4, and 5 brotli fields are
    each detected when moved off their sentinel, one at a time
*/
bool tests_comparator_detects_codec_fields()
{
    bool _ok = true;

    D_AO_MOVES(codec.level, 1);

    // deflate (3)
    D_AO_MOVES(codec.deflate.window_bits, 15);
    D_AO_MOVES(codec.deflate.mem_level, 9);
    D_AO_MOVES(codec.deflate.strategy, deflate_strategy_rle);

    // bzip2 (4)
    D_AO_MOVES(codec.bzip2.block_size_100k, 9);
    D_AO_MOVES(codec.bzip2.work_factor, 30);
    D_AO_MOVES(codec.bzip2.verbosity, 1);
    D_AO_MOVES(codec.bzip2.small_decompress, true);

    // lzma (11)
    D_AO_MOVES(codec.lzma.extreme, true);
    D_AO_MOVES(codec.lzma.check, lzma_check_sha256);
    D_AO_MOVES(codec.lzma.dict_size, 1u << 20);
    D_AO_MOVES(codec.lzma.lc, 0);
    D_AO_MOVES(codec.lzma.lp, 0);
    D_AO_MOVES(codec.lzma.pb, 0);
    D_AO_MOVES(codec.lzma.mode, lzma_mode_fast);
    D_AO_MOVES(codec.lzma.nice_len, 64);
    D_AO_MOVES(codec.lzma.mf, lzma_mf_bt4);
    D_AO_MOVES(codec.lzma.depth, 10);
    D_AO_MOVES(codec.lzma.threads, 4);

    // zstd (19)
    D_AO_MOVES(codec.zstd.level, 19);
    D_AO_MOVES(codec.zstd.window_log, 27);
    D_AO_MOVES(codec.zstd.hash_log, 20);
    D_AO_MOVES(codec.zstd.chain_log, 20);
    D_AO_MOVES(codec.zstd.search_log, 5);
    D_AO_MOVES(codec.zstd.min_match, 4);
    D_AO_MOVES(codec.zstd.target_length, 64);
    D_AO_MOVES(codec.zstd.strategy, zstd_strategy_btopt);
    D_AO_MOVES(codec.zstd.long_distance_matching, true);
    D_AO_MOVES(codec.zstd.ldm_hash_log, 20);
    D_AO_MOVES(codec.zstd.ldm_min_match, 32);
    D_AO_MOVES(codec.zstd.ldm_bucket_size_log, 3);
    D_AO_MOVES(codec.zstd.ldm_hash_rate_log, 4);
    D_AO_MOVES(codec.zstd.content_size_flag, false);
    D_AO_MOVES(codec.zstd.checksum_flag, true);
    D_AO_MOVES(codec.zstd.dict_id_flag, false);
    D_AO_MOVES(codec.zstd.workers, 4);
    D_AO_MOVES(codec.zstd.job_size, 1u << 20);
    D_AO_MOVES(codec.zstd.overlap_log, 6);

    // lz4 (7)
    D_AO_MOVES(codec.lz4.level, 9);
    D_AO_MOVES(codec.lz4.block_size, lz4_block_size_4mb);
    D_AO_MOVES(codec.lz4.block_mode, lz4_block_mode_independent);
    D_AO_MOVES(codec.lz4.content_checksum, true);
    D_AO_MOVES(codec.lz4.block_checksum, true);
    D_AO_MOVES(codec.lz4.store_content_size, true);
    D_AO_MOVES(codec.lz4.favor_dec_speed, true);

    // brotli (5)
    D_AO_MOVES(codec.brotli.quality, 11);
    D_AO_MOVES(codec.brotli.window_bits, 22);
    D_AO_MOVES(codec.brotli.block_bits, 20);
    D_AO_MOVES(codec.brotli.mode, brotli_mode_text);
    D_AO_MOVES(codec.brotli.large_window, true);

    return _ok;
}

/*
tests_option_diff_description
  Verifies the shared diff helper names the fields that moved, at the
granularity it documents: archive-level and per-format fields individually, the
codec's per-codec blocks by block with codec.level called out by name.
  Tests the following:
  - identical sets produce an empty description
  - a single moved field is named exactly
  - several moved fields are all named, comma-separated
  - codec.level is named individually while an advanced zstd knob is reported as
    the block "codec.zstd"
  - every block's coarse name is the prefix of its knobs' fine names, so the
    lzma.* knobs report as "codec.lzma"
  - the description is symmetric in its two arguments
*/
bool tests_option_diff_description()
{
    bool                  _ok = true;
    const archive_options fresh;

    // identical: no diff
    {
        const archive_options same;
        D_AO_CHECK(::djinterp::test::describe_option_diff(same, fresh).empty());
    }

    // one field
    {
        archive_options o;
        o.store_only = 1;
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("store_only"));
    }
    {
        archive_options o;
        o.zip.password = "p";
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("zip.password"));
    }

    // codec.level by name; an advanced knob by block
    {
        archive_options o;
        o.codec.level = 5;
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("codec.level"));
    }
    {
        archive_options o;
        o.codec.zstd.workers = 4;
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("codec.zstd"));
    }

    // a block is named as its knobs are: the coarse name is the prefix of
    // describe_compress_option_diff's fine one, for every block -- lzma's
    // included, whose knobs xz takes and which reports as codec.lzma
    {
        archive_options moved[6];
        size_t          k;

        moved[0].codec.deflate.window_bits = 12;
        moved[1].codec.bzip2.work_factor   = 30;
        moved[2].codec.lzma.nice_len       = 64;
        moved[3].codec.zstd.window_log     = 20;
        moved[4].codec.lz4.block_checksum  = 1;
        moved[5].codec.brotli.quality      = 5;

        for (k = 0; k < 6; ++k)
        {
            const std::string coarse =
                ::djinterp::test::describe_diff_from_default(moved[k]);
            const std::string fine =
                ::djinterp::describe_compress_option_diff(moved[k].codec,
                                                          fresh.codec);

            D_AO_CHECK(fine.find('.') != std::string::npos);
            D_AO_CHECK(coarse == ("codec." + fine.substr(0, fine.find('.'))));
        }

        D_AO_CHECK(::djinterp::test::describe_diff_from_default(moved[2])
                       == std::string("codec.lzma"));
    }

    // several fields, in the helper's documented order
    {
        archive_options o;
        o.level         = 3;
        o.comment       = "c";
        o.tar.format    = tar_format_pax;

        const std::string d = ::djinterp::test::describe_diff_from_default(o);
        D_AO_CHECK(d == std::string("level, comment, tar.format"));
    }

    // symmetry: the same fields are named whichever way round the call goes
    {
        archive_options o;
        o.gz.store_name = 1;

        D_AO_CHECK(::djinterp::test::describe_option_diff(o, fresh)
                       == ::djinterp::test::describe_option_diff(fresh, o));
    }

    return _ok;
}

/*
tests_option_builders
  Verifies each shared builder moves exactly the one thing it advertises and
leaves the rest of the tree pristine -- the property that makes them safe to use
as the starting point of a focused assertion.
  Tests the following:
  - store_only_options moves only store_only
  - level_options moves only level, and leaves codec.level alone
  - codec_level_options moves only codec.level, and leaves level alone
  - zip_method_options moves only zip.method
  - zip_encrypted_options moves exactly zip.encryption and zip.password
*/
bool tests_option_builders()
{
    bool _ok = true;

    {
        const archive_options o = ::djinterp::test::store_only_options();
        D_AO_CHECK(o.store_only == 1);
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("store_only"));
    }
    {
        const archive_options o = ::djinterp::test::level_options(9);
        D_AO_CHECK(o.level       == 9);
        D_AO_CHECK(o.codec.level == archive_knob_unset);
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("level"));
    }
    {
        const archive_options o = ::djinterp::test::codec_level_options(4);
        D_AO_CHECK(o.codec.level == 4);
        D_AO_CHECK(o.level       == archive_knob_unset);
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("codec.level"));
    }
    {
        const archive_options o =
            ::djinterp::test::zip_method_options(zip_method_bzip2);
        D_AO_CHECK(o.zip.method == zip_method_bzip2);
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("zip.method"));
    }
    {
        const archive_options o =
            ::djinterp::test::zip_encrypted_options(zip_encryption_aes_192,
                                                    "s3cret");
        D_AO_CHECK(o.zip.encryption == zip_encryption_aes_192);
        D_AO_CHECK(o.zip.password   == "s3cret");
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("zip.encryption, zip.password"));
    }

    // the default builder really is pristine
    {
        const archive_options o = ::djinterp::test::default_archive_options();
        D_AO_CHECK(::djinterp::test::options_are_default(o));
    }

    return _ok;
}

#undef D_AO_MOVES

NS_END  // testing
NS_END  // djinterp
