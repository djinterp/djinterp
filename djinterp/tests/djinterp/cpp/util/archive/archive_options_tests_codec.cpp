/*******************************************************************************
* djinterp [test]                                archive_options_tests_codec.cpp
*
*   Tests for the embedded codec (archive_options HAS-A compress_options):
* its fifty knobs are pristine until set; resolution tunes only the format's
* own codec, and gives its level the archive's unless the codec block set its
* own; and the codec surface and the container surface are separate data.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/cpp/util/archive/archive_options_tests_codec.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.11.11
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_codec_block_defaults
  Verifies the embedded codec is pristine, and how it resolves.
  Tests the following:
  - every one of the 50 codec knobs is unset, walked through the kernel's
    knob table, and the shared codec_is_default predicate agrees
  - resolved for zip, DEFLATE's own knobs are given values while another
    codec's (zstd's) stay unset: only the format's codec is tuned
  - the codec level is the archive level when the codec block leaves it
    unset, and its own when it sets one
*/
bool tests_codec_block_defaults()
{
    bool                  _ok = true;
    const archive_options o;
    int                   unset = 0;

    for (int k = 0; k < D_COMPRESS_KNOB_COUNT; ++k)
    {
        if (d_compress_options_knob_get(&o.codec, (enum dj_compress_knob)k)
            == D_COMPRESS_KNOB_UNSET)
        {
            ++unset;
        }
    }

    D_AO_CHECK(unset == D_COMPRESS_KNOB_COUNT);
    D_AO_CHECK(::djinterp::test::codec_is_default(o));

    {
        const dj_archive_options r = resolved_for(format_id_zip, o);

        D_AO_CHECK(r.codec.deflate.window_bits != D_COMPRESS_KNOB_UNSET);
        D_AO_CHECK(r.codec.zstd.window_log     == D_COMPRESS_KNOB_UNSET);
        D_AO_CHECK(r.codec.level               == r.level);
    }
    {
        archive_options a;
        a.level = 7;
        D_AO_CHECK(resolved_for(format_id_zip, a).codec.level == 7);

        a.codec.level = 2;
        D_AO_CHECK(resolved_for(format_id_zip, a).codec.level == 2);
        D_AO_CHECK(resolved_for(format_id_zip, a).level       == 7);
    }

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
  - the two effort fields (level and codec.level) are distinct storage:
    setting one does not move the other
*/
bool tests_codec_block_independence()
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
        o.zip.encryption = zip_encryption_aes_256;
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
        D_AO_CHECK(o.codec.level == archive_knob_unset);
    }
    {
        archive_options o;
        o.codec.level = 3;

        D_AO_CHECK(o.codec.level == 3);
        D_AO_CHECK(o.level       == archive_knob_unset);
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
