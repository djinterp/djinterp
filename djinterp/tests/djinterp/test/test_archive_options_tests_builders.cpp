#include "test_archive_options_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_options_store_only_builder
  the store-only preset.
  Tests the following:
  - the archive-level store_only flag is raised
  - nothing else moves, so the diff names store_only alone
  - the embedded codec and every per-format block stay pristine
*/
bool
tests_archive_options_store_only_builder()
{
    const dj::archive_options fresh;
    const dj::archive_options o = dt::store_only_options();

    D_AO_CHECK(o.store_only);
    D_AO_CHECK(!dt::options_are_default(o));
    D_AO_CHECK(dt::describe_diff_from_default(o) == "store_only");

    // the codec is untouched, which is what a container-only preset promises
    D_AO_CHECK(dt::codec_is_default(o));

    // as is every per-format block
    D_AO_CHECK(dt::zip_options_equal(fresh.zip, o.zip));
    D_AO_CHECK(dt::tar_options_equal(fresh.tar, o.tar));
    D_AO_CHECK(dt::gz_options_equal(fresh.gz, o.gz));
    D_AO_CHECK(dt::sevenzip_options_equal(fresh.sevenzip, o.sevenzip));
    D_AO_CHECK(dt::rar_options_equal(fresh.rar, o.rar));

    // and the archive level itself did not move
    D_AO_CHECK(o.level == fresh.level);

    // the builder is deterministic
    D_AO_CHECK(dt::archive_options_equal(dt::store_only_options(),
                                         dt::store_only_options()));

    return true;
}

/*
tests_archive_options_level_builder
  the archive-effort preset.
  Tests the following:
  - the requested effort reaches the archive-level field unclamped
  - nothing else moves, and the codec stays pristine
  - requesting the default effort produces a still-pristine set
*/
bool
tests_archive_options_level_builder()
{
    const dj::archive_options fresh;
    const dj::archive_options o = dt::level_options(6);

    D_AO_CHECK(o.level == 6);
    D_AO_CHECK(dt::describe_diff_from_default(o) == "level");
    D_AO_CHECK(dt::codec_is_default(o));
    D_AO_CHECK(!o.store_only);

    // any value passes through
    D_AO_CHECK(dt::level_options(0).level == 0);
    D_AO_CHECK(dt::level_options(9).level == 9);
    D_AO_CHECK(dt::level_options(-7).level == -7);
    D_AO_CHECK(dt::level_options(100).level == 100);

    // -1 IS the default, so that set is pristine
    const dj::archive_options at_default = dt::level_options(-1);

    D_AO_CHECK(dt::options_are_default(at_default));
    D_AO_CHECK(dt::describe_diff_from_default(at_default).empty());

    // and the codec's own level did not follow
    D_AO_CHECK(o.codec.level == fresh.codec.level);

    return true;
}

/*
tests_archive_options_codec_level_builder
  the embedded-effort preset.
  Tests the following:
  - the requested effort reaches codec.level
  - the archive-level effort is left at its default
  - the diff names codec.level alone, and the codec predicate reports false
*/
bool
tests_archive_options_codec_level_builder()
{
    const dj::archive_options fresh;
    const dj::archive_options o = dt::codec_level_options(19);

    D_AO_CHECK(o.codec.level == 19);
    D_AO_CHECK(o.level == fresh.level);
    D_AO_CHECK(dt::describe_diff_from_default(o) == "codec.level");

    // the codec moved, so its predicate reports false
    D_AO_CHECK(!dt::codec_is_default(o));
    D_AO_CHECK(!dt::options_are_default(o));

    // the rest of the codec is pristine
    D_AO_CHECK(dt::deflate_options_equal(fresh.codec.deflate,
                                         o.codec.deflate));
    D_AO_CHECK(dt::zstd_options_equal(fresh.codec.zstd, o.codec.zstd));
    D_AO_CHECK(dt::brotli_options_equal(fresh.codec.brotli, o.codec.brotli));

    // as is every container block
    D_AO_CHECK(dt::zip_options_equal(fresh.zip, o.zip));
    D_AO_CHECK(dt::rar_options_equal(fresh.rar, o.rar));

    // -1 is the codec default, so that set is pristine
    D_AO_CHECK(dt::options_are_default(dt::codec_level_options(-1)));

    return true;
}

/*
tests_archive_options_level_builders_are_distinct
  the pairing the two level presets exist to test.
  Tests the following:
  - the same effort through the two builders produces different sets
  - each names a different field in its diff
  - only the codec builder disturbs the codec-only predicate
*/
bool
tests_archive_options_level_builders_are_distinct()
{
    const dj::archive_options arch  = dt::level_options(9);
    const dj::archive_options codec = dt::codec_level_options(9);

    D_AO_CHECK(!dt::archive_options_equal(arch, codec));
    D_AO_CHECK(dt::describe_diff_from_default(arch) == "level");
    D_AO_CHECK(dt::describe_diff_from_default(codec) == "codec.level");

    // each leaves the other's field alone
    D_AO_CHECK(arch.codec.level != 9);
    D_AO_CHECK(codec.level != 9);

    // only the codec builder moves the codec
    D_AO_CHECK(dt::codec_is_default(arch));
    D_AO_CHECK(!dt::codec_is_default(codec));

    // a set carrying both names both
    dj::archive_options both = dt::level_options(9);

    both.codec.level = 9;

    D_AO_CHECK(dt::describe_diff_from_default(both) == "level, codec.level");
    D_AO_CHECK(!dt::archive_options_equal(both, arch));
    D_AO_CHECK(!dt::archive_options_equal(both, codec));

    return true;
}

/*
tests_archive_options_zip_method_builder
  the ZIP method preset.
  Tests the following:
  - the requested per-entry method reaches zip.method
  - the other four ZIP fields stay pristine, as does everything outside the
    ZIP block
  - the diff names zip.method alone
*/
bool
tests_archive_options_zip_method_builder()
{
    const dj::archive_options fresh;
    const dj::zip_method      alt = ao_bump(fresh.zip.method);
    const dj::archive_options o   = dt::zip_method_options(alt);

    D_AO_CHECK(o.zip.method == alt);
    D_AO_CHECK(dt::describe_diff_from_default(o) == "zip.method");
    D_AO_CHECK(!dt::zip_options_equal(fresh.zip, o.zip));

    // the rest of the ZIP block is pristine
    D_AO_CHECK(o.zip.encryption == fresh.zip.encryption);
    D_AO_CHECK(o.zip.password == fresh.zip.password);
    D_AO_CHECK(o.zip.zip64 == fresh.zip.zip64);
    D_AO_CHECK(o.zip.utf8_names == fresh.zip.utf8_names);

    // and so is everything outside it
    D_AO_CHECK(dt::codec_is_default(o));
    D_AO_CHECK(dt::tar_options_equal(fresh.tar, o.tar));
    D_AO_CHECK(dt::gz_options_equal(fresh.gz, o.gz));
    D_AO_CHECK(dt::sevenzip_options_equal(fresh.sevenzip, o.sevenzip));
    D_AO_CHECK(dt::rar_options_equal(fresh.rar, o.rar));
    D_AO_CHECK(o.level == fresh.level);
    D_AO_CHECK(!o.store_only);

    // requesting the default method leaves the set pristine
    D_AO_CHECK(dt::options_are_default(
                   dt::zip_method_options(fresh.zip.method)));

    return true;
}

/*
tests_archive_options_zip_encrypted
  the ZIP encryption preset.
  Tests the following:
  - both the scheme and the passphrase reach their fields
  - exactly those two fields move, so the diff names both and nothing else
  - the passphrase is stored verbatim, including punctuation
*/
bool
tests_archive_options_zip_encrypted()
{
    const dj::archive_options fresh;
    const dj::zip_encryption  scheme = ao_bump(fresh.zip.encryption);
    const dj::archive_options o =
        dt::zip_encrypted_options(scheme, "s3cr3t!");

    D_AO_CHECK(o.zip.encryption == scheme);
    D_AO_CHECK(o.zip.password == "s3cr3t!");

    // exactly two fields moved, in declaration order
    const std::string d = dt::describe_diff_from_default(o);

    D_AO_CHECK(d == "zip.encryption, zip.password");
    D_AO_CHECK(ao_split_csv(d).size() == 2u);

    // the method and the remaining ZIP knobs are untouched
    D_AO_CHECK(o.zip.method == fresh.zip.method);
    D_AO_CHECK(o.zip.zip64 == fresh.zip.zip64);
    D_AO_CHECK(o.zip.utf8_names == fresh.zip.utf8_names);

    // nothing outside the ZIP block moved
    D_AO_CHECK(dt::codec_is_default(o));
    D_AO_CHECK(dt::sevenzip_options_equal(fresh.sevenzip, o.sevenzip));
    D_AO_CHECK(dt::rar_options_equal(fresh.rar, o.rar));

    // the passphrase does not leak into the other blocks' password fields
    D_AO_CHECK(o.sevenzip.password.empty());
    D_AO_CHECK(o.rar.password.empty());

    // a long passphrase with embedded spaces survives intact
    const dj::archive_options longpw =
        dt::zip_encrypted_options(scheme, "a long pass phrase with spaces");

    D_AO_CHECK(longpw.zip.password == "a long pass phrase with spaces");

    return true;
}

/*
tests_archive_options_zip_encrypted_empty_pw
  the empty-passphrase edge.
  Tests the following:
  - an empty passphrase is stored as given rather than being rejected
  - since the empty string IS the default, only the scheme is reported moved
  - requesting the default scheme with an empty passphrase leaves the set
    pristine
*/
bool
tests_archive_options_zip_encrypted_empty_pw()
{
    const dj::archive_options fresh;
    const dj::zip_encryption  scheme = ao_bump(fresh.zip.encryption);

    const dj::archive_options o =
        dt::zip_encrypted_options(scheme, std::string());

    D_AO_CHECK(o.zip.encryption == scheme);
    D_AO_CHECK(o.zip.password.empty());

    // the passphrase matches the default, so only the scheme is named
    D_AO_CHECK(dt::describe_diff_from_default(o) == "zip.encryption");

    // the default scheme with an empty passphrase moves nothing at all
    const dj::archive_options none =
        dt::zip_encrypted_options(fresh.zip.encryption, std::string());

    D_AO_CHECK(dt::options_are_default(none));
    D_AO_CHECK(dt::describe_diff_from_default(none).empty());
    D_AO_CHECK(dt::zip_options_equal(fresh.zip, none.zip));

    // a passphrase without a scheme change names only the passphrase
    const dj::archive_options pw_only =
        dt::zip_encrypted_options(fresh.zip.encryption, "pw");

    D_AO_CHECK(dt::describe_diff_from_default(pw_only) == "zip.password");
    D_AO_CHECK(pw_only.zip.encryption == fresh.zip.encryption);

    return true;
}

NS_END  // testing
NS_END  // djinterp
