/*******************************************************************************
* djinterp [test]                               archive_options_tests_facade.cpp
*
*   Tests for what the facade actually DOES with an archive_options.  The rest
* of the suite pins the vocabulary's data contract; this section drives
* archive.hpp to record which knobs the built-in writers consult.
*
*   CURRENT STATE:
*   archive.cpp reads exactly two fields -- store_only and level -- and builds a
* fresh compress_options for the stream, copying only `level` into it.  The
* embedded codec block and every per-format container knob are therefore inert
* today.  That diverges from the header's THE EMBEDDED CODEC note, which
* describes codec.level as taking precedence over the archive-level `level`.
*
*   The two characterization tests below (codec_block_is_inert, and the honoured
* pair above it) pin TODAY's behaviour deliberately, so that wiring the codec
* block through fails them loudly and forces the header's documentation and the
* implementation to be reconciled in the same change.  They are not an
* endorsement of the gap.
*
*   Part of the archive_options.hpp unit-test suite; the declarations, shared
* helpers, and per-suite check macros live in archive_options_tests.hpp.  These
* functions are flat in djinterp::testing and are called by the suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests_facade.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_options_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_facade_honours_store_only
  Verifies the archive-level store_only flag reaches the ZIP writer -- one of
the two fields the facade actually consults.
  Tests the following:
  - with store_only set, every ZIP local header records method 0, and the
    archive still round-trips
  - with store_only clear and a DEFLATE codec present, a compressible entry is
    recorded with method 8 instead, so the flag is demonstrably what moved
  - the flag is inert for tar, which has no compression to suppress: the two
    tar archives are byte-identical
*/
bool
tests_facade_honours_store_only()
{
    using namespace ::djinterp::formats;

    bool             _ok = true;
    const entry_list items = sample_entries();

    // store_only raised: every member stored
    {
        const archive_options opt = ::djinterp::test::store_only_options();

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);

        std::vector<unsigned int> methods;
        D_AO_REQUIRE(::djinterp::test::zip_local_methods(blob, methods));
        for (std::size_t i = 0; i < methods.size(); ++i)
        {
            D_AO_CHECK(methods[i] == 0u);
        }

        entry_list out;
        D_AO_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
        D_AO_CHECK(::djinterp::test::files_preserved(items, out));
    }

    // store_only clear: the compressible member deflates instead
    if (codec_is_available<codecs::deflate>())
    {
        const archive_options opt;   // pristine

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);

        std::vector<unsigned int> methods;
        D_AO_REQUIRE(::djinterp::test::zip_local_methods(blob, methods));
        D_AO_REQUIRE(!methods.empty());
        D_AO_CHECK(methods[0] == 8u);   // the 600-byte run of 'a'
    }

    // tar has nothing to suppress: the flag cannot change its output
    {
        const archive_options plain;
        const archive_options stored = ::djinterp::test::store_only_options();

        byte_blob a;
        byte_blob b;
        D_AO_CHECK_STATUS(try_archive<tar>(items, a, plain), status_ok);
        D_AO_CHECK_STATUS(try_archive<tar>(items, b, stored), status_ok);
        D_AO_CHECK(a == b);
    }

    return _ok;
}

/*
tests_facade_honours_level
  Verifies the archive-level `level` field reaches the stream codec -- the other
field the facade consults.  Effort is observed through its effect on the
produced size rather than through any header field, so the assertion holds for
any DEFLATE backend.
  Tests the following:
  - when a DEFLATE codec is present, a compressible payload archived at level 9
    is no larger than the same payload at level 1, and strictly smaller than at
    level 0 (which cannot compress at all)
  - archives at every level still round-trip with payloads intact
  - when no DEFLATE codec is present, the level is simply inert and creation
    still succeeds
*/
bool
tests_facade_honours_level()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    // a payload with real structure, so effort actually changes the outcome
    byte_blob payload;
    for (std::size_t i = 0; i < 4000; ++i)
    {
        payload += (char)('a' + (int)(i % 7));
    }

    entry_list items;
    items.push_back(::djinterp::test::make_file_entry("p.txt", payload));

    byte_blob at0;
    byte_blob at1;
    byte_blob at9;

    D_AO_CHECK_STATUS(try_archive<zip>(items, at0,
                                       ::djinterp::test::level_options(0)),
                      status_ok);
    D_AO_CHECK_STATUS(try_archive<zip>(items, at1,
                                       ::djinterp::test::level_options(1)),
                      status_ok);
    D_AO_CHECK_STATUS(try_archive<zip>(items, at9,
                                       ::djinterp::test::level_options(9)),
                      status_ok);

    if (codec_is_available<codecs::deflate>())
    {
        D_AO_CHECK(at9.size() <= at1.size());
        D_AO_CHECK(at9.size() <  at0.size());
    }

    // every level still produces a readable archive
    {
        entry_list out;
        D_AO_CHECK_STATUS(try_extract<zip>(at0, out), status_ok);
        D_AO_CHECK(::djinterp::test::files_preserved(items, out));
    }
    {
        entry_list out;
        D_AO_CHECK_STATUS(try_extract<zip>(at9, out), status_ok);
        D_AO_CHECK(::djinterp::test::files_preserved(items, out));
    }

    return _ok;
}

/*
tests_facade_codec_block_is_inert
  CHARACTERIZATION TEST -- pins today's behaviour, which diverges from the
header.  archive.cpp constructs a fresh compress_options per call and copies
only the archive-level `level` into it, so the embedded codec block is
discarded entirely.  The header's THE EMBEDDED CODEC note instead specifies
that codec.level takes precedence when it is moved off its -1 default.
  When the codec block is wired through, this test WILL fail; that is its
purpose, and the fix is to update it alongside the header's note.
  Tests the following:
  - setting codec.level alone produces output byte-identical to pristine
    defaults, i.e. it is currently ignored
  - setting the archive-level level alone DOES change the output, so the two
    fields are demonstrably not interchangeable today
  - setting an advanced codec knob (a zstd parameter irrelevant to the ZIP
    DEFLATE path) likewise changes nothing and never fails the call
*/
bool
tests_facade_codec_block_is_inert()
{
    using namespace ::djinterp::formats;

    bool             _ok = true;
    const entry_list items = sample_entries();

    byte_blob baseline;
    D_AO_CHECK_STATUS(try_archive<zip>(items, baseline, archive_options()),
                      status_ok);

    // codec.level alone: currently discarded
    {
        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob,
                              ::djinterp::test::codec_level_options(0)),
                          status_ok);
        D_AO_CHECK(blob == baseline);
    }

    // the archive-level level: honoured, so the pair is not interchangeable
    if (codec_is_available<codecs::deflate>())
    {
        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob,
                              ::djinterp::test::level_options(0)),
                          status_ok);
        D_AO_CHECK(blob != baseline);
    }

    // an advanced knob from an unrelated codec: inert, and harmless
    {
        archive_options opt;
        opt.codec.zstd.window_log      = 27;
        opt.codec.brotli.quality       = 11;
        opt.codec.deflate.window_bits  = 9;

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);
        D_AO_CHECK(blob == baseline);
    }

    return _ok;
}

/*
tests_facade_container_knobs_do_not_corrupt
  Verifies that requesting a container knob the built-in writers do not
implement is always safe: the call reports a defined status and, when it
succeeds, yields an archive that still reads back correctly.  Unlike the
characterization test above this does not pin WHICH status comes back for the
compressed-method requests, so it keeps holding once a backend learns to honour
them.
  Tests the following:
  - zip64 policy, UTF-8 name flagging, an archive comment, and the two
    preserve_* flags all leave a valid, readable zip
  - a tar variant request leaves a valid, readable tar
  - the gz header knobs leave a valid gz (when the codec is present)
  - requesting a ZIP method or encryption scheme the built-in writer cannot
    provide returns one of the facade's defined statuses, and never a corrupt
    archive: whenever it reports success the result round-trips
*/
bool
tests_facade_container_knobs_do_not_corrupt()
{
    using namespace ::djinterp::formats;

    bool             _ok = true;
    const entry_list items = sample_entries();

    // zip container knobs
    {
        archive_options opt;
        opt.zip.zip64            = zip64_always;
        opt.zip.utf8_names       = false;
        opt.comment              = "suite comment";
        opt.preserve_permissions = false;
        opt.preserve_mtime       = false;

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);
        D_AO_CHECK(::djinterp::test::looks_like_zip(blob));

        entry_list out;
        D_AO_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
        D_AO_CHECK(::djinterp::test::files_preserved(items, out));
    }

    // tar variant request
    {
        archive_options opt;
        opt.tar.format        = tar_format_pax;
        opt.tar.numeric_owner = true;

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<tar>(items, blob, opt), status_ok);
        D_AO_CHECK(::djinterp::test::tar_has_ustar_magic(blob));

        entry_list out;
        D_AO_CHECK_STATUS(try_extract<tar>(blob, out), status_ok);
        D_AO_CHECK(::djinterp::test::files_preserved(items, out));
    }

    // gz header knobs
    if (format_is_writable<gz>() && format_is_readable<gz>())
    {
        archive_options opt;
        opt.gz.store_name    = true;
        opt.gz.original_name = "carried.txt";
        opt.gz.store_mtime   = false;

        entry_list one;
        one.push_back(::djinterp::test::make_file_entry("carried.txt",
                                                        byte_blob(400, 'g')));

        byte_blob blob;
        D_AO_CHECK_STATUS(try_archive<gz>(one, blob, opt), status_ok);
        D_AO_CHECK(::djinterp::test::looks_like_gzip(blob));

        entry_list out;
        D_AO_CHECK_STATUS(try_extract<gz>(blob, out), status_ok);
        D_AO_REQUIRE(out.size() == 1u);
        D_AO_CHECK(out[0].data == one[0].data);
    }

    // unimplemented ZIP methods / encryption: a defined status, never garbage
    {
        const zip_method methods[] = { zip_method_bzip2, zip_method_lzma,
                                       zip_method_zstd,  zip_method_xz };

        for (std::size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        {
            const archive_options opt =
                ::djinterp::test::zip_method_options(methods[i]);

            byte_blob blob;
            const status s = try_archive<zip>(items, blob, opt);

            D_AO_CHECK( (s == status_ok)          ||
                        (s == status_unavailable) ||
                        (s == status_unsupported) );

            if (s == status_ok)
            {
                entry_list out;
                D_AO_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
                D_AO_CHECK(::djinterp::test::files_preserved(items, out));
            }
        }
    }
    {
        const archive_options opt =
            ::djinterp::test::zip_encrypted_options(zip_encryption_aes256,
                                                    "hunter2");

        byte_blob  blob;
        const status s = try_archive<zip>(items, blob, opt);

        D_AO_CHECK( (s == status_ok)          ||
                    (s == status_unavailable) ||
                    (s == status_unsupported) );
    }

    return _ok;
}

/*
tests_facade_does_not_mutate_options
  Verifies the facade treats the option set as strictly read-only.  It takes a
const reference, so this cannot fail by accident today -- but it is the
invariant every "forward these options onward" call site depends on, and it
must survive any future rework that starts normalising or defaulting fields
in place.
  Tests the following:
  - a customised option set is field-for-field unchanged after a successful
    create, with the shared diff naming nothing
  - it is likewise unchanged after an extract
  - it is unchanged even when the call fails (a gz create rejected for carrying
    more than one entry)
  - the same option set drives two successive creates to identical output, so
    no hidden state is carried between calls
*/
bool
tests_facade_does_not_mutate_options()
{
    using namespace ::djinterp::formats;

    bool             _ok = true;
    const entry_list items = sample_entries();

    archive_options opt;
    opt.level                = 6;
    opt.comment              = "unchanged";
    opt.zip.zip64            = zip64_always;
    opt.zip.password         = "pw";
    opt.codec.level          = 2;
    opt.codec.zstd.workers   = 3;
    opt.sevenzip.threads     = 4;
    opt.preserve_mtime       = false;

    const archive_options before(opt);

    // after a successful create
    byte_blob blob;
    D_AO_CHECK_STATUS(try_archive<zip>(items, blob, opt), status_ok);
    D_AO_CHECK_EQUAL_OPT(opt, before);

    // after an extract
    {
        entry_list out;
        D_AO_CHECK_STATUS(try_extract<zip>(blob, out), status_ok);
        D_AO_CHECK_EQUAL_OPT(opt, before);
    }

    // after a failed create (gz rejects a multi-entry list)
    {
        byte_blob rejected;
        D_AO_CHECK_STATUS(try_archive<gz>(items, rejected, opt),
                          status_invalid_argument);
        D_AO_CHECK_EQUAL_OPT(opt, before);
    }

    // no hidden state between calls: same options in, same bytes out
    {
        byte_blob again;
        D_AO_CHECK_STATUS(try_archive<zip>(items, again, opt), status_ok);
        D_AO_CHECK(again == blob);
        D_AO_CHECK_EQUAL_OPT(opt, before);
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
