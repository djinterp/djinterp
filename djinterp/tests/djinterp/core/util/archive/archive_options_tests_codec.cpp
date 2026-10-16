/*******************************************************************************
* djinterp [test]                                archive_options_tests_codec.cpp
*
*   Tests for the embedded codec surface -- the HAS-A relationship the header's
* THE EMBEDDED CODEC note describes, where archive_options carries a whole
* compress_options for tuning the compressed stream inside the container.  The
* per-codec blocks reached through it must arrive pristine, and moving a codec
* knob must not disturb the container knobs (or the reverse).
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_codec.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_codec_block_defaults
  Verifies the embedded codec and each of its six per-codec blocks default to
their documented sentinels, reached through the archive_options aggregate.  The
sentinel convention is deliberately mixed -- 0 for most integer knobs, -1 where
0 is itself a meaningful value -- so each family is spot-checked on the field
that carries its own sentinel.
  Tests the following:
  - codec.level defaults to -1 (backend default effort)
  - deflate window_bits / mem_level default to 0 and strategy to default
  - bzip2 block_size_100k / work_factor default to 0
  - lzma lc / lp / pb default to -1 (0 is meaningful for them) while dict_size
    and threads default to 0, and check defaults to crc64
  - zstd level defaults to 0 ("unset"), content_size_flag and dict_id_flag to
    true, checksum_flag to false
  - lz4 level defaults to 0 with linked blocks
  - brotli quality defaults to -1 while window_bits defaults to 0
  - the shared codec_is_default predicate agrees
*/
bool
tests_codec_block_defaults()
{
    bool                  _ok = true;
    const archive_options o;

    D_AO_CHECK(o.codec.level == -1);

    // deflate
    D_AO_CHECK(o.codec.deflate.window_bits == 0);
    D_AO_CHECK(o.codec.deflate.mem_level   == 0);
    D_AO_CHECK(o.codec.deflate.strategy    == deflate_strategy_default);

    // bzip2
    D_AO_CHECK(o.codec.bzip2.block_size_100k  == 0);
    D_AO_CHECK(o.codec.bzip2.work_factor      == 0);
    D_AO_CHECK(o.codec.bzip2.verbosity        == 0);
    D_AO_CHECK(o.codec.bzip2.small_decompress == false);

    // lzma: the -1 sentinels, where 0 is itself a legal setting
    D_AO_CHECK(o.codec.lzma.lc        == -1);
    D_AO_CHECK(o.codec.lzma.lp        == -1);
    D_AO_CHECK(o.codec.lzma.pb        == -1);
    D_AO_CHECK(o.codec.lzma.dict_size == 0);
    D_AO_CHECK(o.codec.lzma.threads   == 0);
    D_AO_CHECK(o.codec.lzma.extreme   == false);
    D_AO_CHECK(o.codec.lzma.check     == lzma_check_crc64);
    D_AO_CHECK(o.codec.lzma.mode      == lzma_mode_default);
    D_AO_CHECK(o.codec.lzma.mf        == lzma_mf_default);

    // zstd
    D_AO_CHECK(o.codec.zstd.level                  == 0);
    D_AO_CHECK(o.codec.zstd.window_log             == 0);
    D_AO_CHECK(o.codec.zstd.strategy               == zstd_strategy_default);
    D_AO_CHECK(o.codec.zstd.long_distance_matching == false);
    D_AO_CHECK(o.codec.zstd.content_size_flag      == true);
    D_AO_CHECK(o.codec.zstd.checksum_flag          == false);
    D_AO_CHECK(o.codec.zstd.dict_id_flag           == true);
    D_AO_CHECK(o.codec.zstd.workers                == 0);
    D_AO_CHECK(o.codec.zstd.job_size               == 0);

    // lz4
    D_AO_CHECK(o.codec.lz4.level      == 0);
    D_AO_CHECK(o.codec.lz4.block_size == lz4_block_size_default);
    D_AO_CHECK(o.codec.lz4.block_mode == lz4_block_mode_linked);

    // brotli: quality carries -1, window_bits carries 0
    D_AO_CHECK(o.codec.brotli.quality      == -1);
    D_AO_CHECK(o.codec.brotli.window_bits  == 0);
    D_AO_CHECK(o.codec.brotli.block_bits   == 0);
    D_AO_CHECK(o.codec.brotli.mode         == brotli_mode_generic);
    D_AO_CHECK(o.codec.brotli.large_window == false);

    D_AO_CHECK(::djinterp::test::codec_is_default(o));

    return _ok;
}

/*
tests_codec_block_independence
  Verifies the codec surface and the container surface are genuinely separate
data, so tuning a stream cannot silently alter a container choice.
  Tests the following:
  - moving a codec knob leaves every per-format block pristine, and the
    aggregate reports only that codec block as changed
  - moving a container knob leaves the whole codec block pristine
  - the two archive-level effort fields (level and codec.level) are distinct
    storage: setting one does not move the other
*/
bool
tests_codec_block_independence()
{
    bool                  _ok = true;
    const archive_options fresh;

    // codec knob moved: container blocks must not follow
    {
        archive_options o;
        o.codec.zstd.window_log = 27;

        D_AO_CHECK(::djinterp::test::zip_options_equal(o.zip, fresh.zip));
        D_AO_CHECK(::djinterp::test::tar_options_equal(o.tar, fresh.tar));
        D_AO_CHECK(::djinterp::test::gz_options_equal(o.gz, fresh.gz));
        D_AO_CHECK(::djinterp::test::sevenzip_options_equal(o.sevenzip,
                                                            fresh.sevenzip));
        D_AO_CHECK(::djinterp::test::rar_options_equal(o.rar, fresh.rar));

        D_AO_CHECK(!::djinterp::test::codec_is_default(o));
        D_AO_CHECK(::djinterp::test::describe_diff_from_default(o)
                       == std::string("codec.zstd"));
    }

    // container knob moved: the codec must not follow
    {
        archive_options o;
        o.zip.encryption = zip_encryption_aes256;
        o.zip.password   = "hunter2";

        D_AO_CHECK(::djinterp::test::codec_is_default(o));
        D_AO_CHECK(::djinterp::test::compress_options_equal(o.codec,
                                                             fresh.codec));
    }

    // the two effort fields are separate storage
    {
        archive_options o;
        o.level = 9;

        D_AO_CHECK(o.level       == 9);
        D_AO_CHECK(o.codec.level == -1);   // untouched
    }
    {
        archive_options o;
        o.codec.level = 3;

        D_AO_CHECK(o.codec.level == 3);
        D_AO_CHECK(o.level       == -1);   // untouched
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
