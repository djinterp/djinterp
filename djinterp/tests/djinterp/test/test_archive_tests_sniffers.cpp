#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING


// ta_npos
//   constant: the not-found value zip_find_eocd returns.
static const std::size_t ta_npos = static_cast<std::size_t>(-1);


/*
tests_archive_zip_find_eocd_locates_record
  the backward scan for the end-of-central-directory signature.
  Tests the following:
  - a bare 22-byte record is found at offset zero
  - a record behind member data is found at its real offset
  - the scan runs backward, so with two candidate signatures the LATER one
    is returned
*/
bool
tests_archive_zip_find_eocd_locates_record()
{
    // a bare record, which is what an empty archive is
    const dj::byte_buffer bare = ta_eocd(0u);

    D_TA_CHECK(bare.size() == 22u);
    D_TA_CHECK(dt::zip_find_eocd(bare) == 0u);

    // a record behind one member
    std::vector<ta_zip_member> members;

    members.push_back(ta_zip_member("a.txt", dj::byte_buffer("hello"), 0u));

    const dj::byte_buffer one = ta_make_zip(members);

    D_TA_CHECK(one.size() > 22u);
    D_TA_CHECK(dt::zip_find_eocd(one) == (one.size() - 22u));

    // the scan is backward: the later of two signatures wins
    dj::byte_buffer twice = ta_eocd(1u);

    twice += ta_eocd(2u);

    D_TA_CHECK(twice.size() == 44u);
    D_TA_CHECK(dt::zip_find_eocd(twice) == 22u);

    // a signature buried in member DATA is still found if it is the last
    // one, which is why the sniffers are documented as lenient
    std::vector<ta_zip_member> tricky;

    tricky.push_back(ta_zip_member("t", ta_eocd(7u), 0u));

    const dj::byte_buffer buried = ta_make_zip(tricky);

    D_TA_CHECK(dt::zip_find_eocd(buried) == (buried.size() - 22u));

    return true;
}

/*
tests_archive_zip_find_eocd_rejects_absent_and_short
  the two ways the scan reports nothing.
  Tests the following:
  - a blob with no signature reports not-found
  - a blob shorter than the smallest possible record reports not-found
    without reading past its end
  - a 21-byte blob carrying the signature is still too short
*/
bool
tests_archive_zip_find_eocd_rejects_absent_and_short()
{
    // no signature anywhere
    const dj::byte_buffer plain(100u, 'x');

    D_TA_CHECK(dt::zip_find_eocd(plain) == ta_npos);

    // too short, including the empty blob
    D_TA_CHECK(dt::zip_find_eocd(dj::byte_buffer()) == ta_npos);

    std::size_t n = 0;

    for (n = 0; n < 22u; ++n)
    {
        const dj::byte_buffer shortb(n, 'x');

        D_TA_CHECK(dt::zip_find_eocd(shortb) == ta_npos);
    }

    // a 21-byte blob that DOES carry the signature is still too short
    const dj::byte_buffer truncated = ta_eocd(0u).substr(0, 21);

    D_TA_CHECK(truncated.size() == 21u);
    D_TA_CHECK(dt::zip_find_eocd(truncated) == ta_npos);

    // exactly 22 bytes is enough
    D_TA_CHECK(dt::zip_find_eocd(ta_eocd(0u)) == 0u);

    // a near-miss signature is not accepted
    dj::byte_buffer near = ta_eocd(0u);

    near[3] = static_cast<char>(0x07u);   // PK\5\7, not PK\5\6
    D_TA_CHECK(dt::zip_find_eocd(near) == ta_npos);

    near = ta_eocd(0u);
    near[0] = 'Q';
    D_TA_CHECK(dt::zip_find_eocd(near) == ta_npos);

    return true;
}

/*
tests_archive_looks_like_zip_front_signatures
  the two shapes a valid container can open with.
  Tests the following:
  - a blob opening with a local file header and carrying an EOCD is
    accepted
  - a bare EOCD is accepted, which is the empty archive
  - a multi-member container is accepted
*/
bool
tests_archive_looks_like_zip_front_signatures()
{
    // a local file header at the front
    std::vector<ta_zip_member> members;

    members.push_back(ta_zip_member("a.txt", dj::byte_buffer("hello"), 0u));

    D_TA_CHECK(dt::looks_like_zip(ta_make_zip(members)));

    // the empty archive: a bare EOCD, which is both the front and the back
    const dj::byte_buffer empty_zip = ta_eocd(0u);

    D_TA_CHECK(empty_zip.size() == 22u);
    D_TA_CHECK(dt::looks_like_zip(empty_zip));
    D_TA_CHECK(dt::zip_total_entries(empty_zip) == 0L);

    // several members
    members.push_back(ta_zip_member("b.bin",
                                    dj::byte_buffer("\0\1\2", 3), 8u));
    members.push_back(ta_zip_member("dir/", dj::byte_buffer(), 0u));

    const dj::byte_buffer three = ta_make_zip(members);

    D_TA_CHECK(dt::looks_like_zip(three));
    D_TA_CHECK(dt::zip_total_entries(three) == 3L);

    // a member with an empty payload is fine
    std::vector<ta_zip_member> hollow;

    hollow.push_back(ta_zip_member("e", dj::byte_buffer(), 0u));
    D_TA_CHECK(dt::looks_like_zip(ta_make_zip(hollow)));

    return true;
}

/*
tests_archive_looks_like_zip_rejects_malformed
  the ways the structural check fails.
  Tests the following:
  - a blob shorter than the smallest record is rejected
  - a blob opening with neither known signature is rejected, even when it
    carries an EOCD further in
  - a blob opening correctly but carrying no EOCD is rejected
  - both halves of the check are required, not either
*/
bool
tests_archive_looks_like_zip_rejects_malformed()
{
    // too short
    D_TA_CHECK(!dt::looks_like_zip(dj::byte_buffer()));
    D_TA_CHECK(!dt::looks_like_zip(dj::byte_buffer(21u, 'x')));
    D_TA_CHECK(!dt::looks_like_zip(ta_eocd(0u).substr(0, 21)));

    // the wrong front signature, even with a real EOCD behind it
    dj::byte_buffer bad_front(8u, 'Q');

    bad_front += ta_eocd(1u);

    D_TA_CHECK(bad_front.size() > 22u);
    D_TA_CHECK(dt::zip_find_eocd(bad_front) != ta_npos);
    D_TA_CHECK(!dt::looks_like_zip(bad_front));

    // a right front signature with NO EOCD behind it
    dj::byte_buffer no_eocd =
        ta_local_header("a.txt", dj::byte_buffer("hello"), 0u);

    no_eocd.append(64u, 'z');

    D_TA_CHECK(no_eocd.size() > 22u);
    D_TA_CHECK(dt::zip_find_eocd(no_eocd) == ta_npos);
    D_TA_CHECK(!dt::looks_like_zip(no_eocd));

    // each byte of the front signature matters
    std::vector<ta_zip_member> members;

    members.push_back(ta_zip_member("a", dj::byte_buffer("x"), 0u));

    const dj::byte_buffer good = ta_make_zip(members);
    std::size_t           i    = 0;

    D_TA_CHECK(dt::looks_like_zip(good));

    for (i = 0; i < 4u; ++i)
    {
        dj::byte_buffer broken = good;

        broken[i] = static_cast<char>(
            static_cast<unsigned char>(broken[i]) ^ 0xFFu);

        D_TA_CHECK(!dt::looks_like_zip(broken));
    }

    // other containers are not zips
    D_TA_CHECK(!dt::looks_like_zip(ta_make_gzip_header(64)));
    D_TA_CHECK(!dt::looks_like_zip(ta_make_tar_blocks(1)));

    return true;
}

/*
tests_archive_zip_total_entries_reads_count
  the advertised member count.
  Tests the following:
  - the count is read from the total-entries field, ten bytes into the
    record
  - it is read as a 16-bit little-endian value, so a two-byte count decodes
    whole
  - a blob with no record reports minus one rather than zero, which is what
    distinguishes "no archive" from "empty archive"
*/
bool
tests_archive_zip_total_entries_reads_count()
{
    // the count follows the member list
    std::vector<ta_zip_member> members;
    std::size_t                i = 0;

    D_TA_CHECK(dt::zip_total_entries(ta_make_zip(members)) == 0L);

    for (i = 1; i <= 5u; ++i)
    {
        members.push_back(ta_zip_member("m", dj::byte_buffer("d"), 0u));

        D_TA_CHECK(dt::zip_total_entries(ta_make_zip(members)) ==
                   static_cast<long>(i));
    }

    // a two-byte count decodes whole
    D_TA_CHECK(dt::zip_total_entries(ta_eocd(300u)) == 300L);
    D_TA_CHECK(dt::zip_total_entries(ta_eocd(65535u)) == 65535L);
    D_TA_CHECK(dt::zip_total_entries(ta_eocd(256u)) == 256L);

    // the field really is the one at offset ten, not the one at eight
    dj::byte_buffer skewed = ta_eocd(0u);

    skewed[8]  = static_cast<char>(0x11u);   // entries on this disk
    skewed[9]  = static_cast<char>(0x00u);
    skewed[10] = static_cast<char>(0x22u);   // total entries
    skewed[11] = static_cast<char>(0x00u);

    D_TA_CHECK(dt::zip_total_entries(skewed) == 0x22L);

    // absent record
    D_TA_CHECK(dt::zip_total_entries(dj::byte_buffer()) == -1L);
    D_TA_CHECK(dt::zip_total_entries(dj::byte_buffer(100u, 'x')) == -1L);
    D_TA_CHECK(dt::zip_total_entries(ta_make_gzip_header(64)) == -1L);

    // which is distinguishable from a genuinely empty archive
    D_TA_CHECK(dt::zip_total_entries(ta_eocd(0u)) == 0L);

    return true;
}

/*
tests_archive_zip_local_methods_walks_chain
  the local-header walk.
  Tests the following:
  - one method code is appended per local header, in file order
  - the codes carried are the ones the headers declare
  - the walk stops when the signature no longer matches, so the EOCD ends
    the run rather than being parsed as a header
  - the output vector is cleared first, so a reused vector does not
    accumulate
*/
bool
tests_archive_zip_local_methods_walks_chain()
{
    std::vector<ta_zip_member> members;

    members.push_back(ta_zip_member("stored.txt",
                                    dj::byte_buffer("aaaa"), 0u));
    members.push_back(ta_zip_member("defl.txt", dj::byte_buffer("bbbb"), 8u));
    members.push_back(ta_zip_member("odd.bin", dj::byte_buffer("cc"), 12u));

    const dj::byte_buffer     blob = ta_make_zip(members);
    std::vector<unsigned int> out;

    D_TA_CHECK(dt::zip_local_methods(blob, out));
    D_TA_CHECK(out.size() == 3u);
    D_TA_CHECK(out[0] == 0u);
    D_TA_CHECK(out[1] == 8u);
    D_TA_CHECK(out[2] == 12u);

    // the walk stopped at the EOCD rather than parsing it
    D_TA_CHECK(out.size() == members.size());
    D_TA_CHECK(dt::zip_total_entries(blob) == 3L);

    // the output is cleared first
    out.push_back(999u);
    out.push_back(998u);
    D_TA_CHECK(dt::zip_local_methods(blob, out));
    D_TA_CHECK(out.size() == 3u);
    D_TA_CHECK(out[0] == 0u);

    // an empty archive has no local headers, but the walk still succeeds
    std::vector<ta_zip_member> none;

    D_TA_CHECK(dt::zip_local_methods(ta_make_zip(none), out));
    D_TA_CHECK(out.empty());

    // a blob with no headers at all: the first four bytes fail the
    // signature test and the walk ends immediately
    D_TA_CHECK(dt::zip_local_methods(dj::byte_buffer(100u, 'x'), out));
    D_TA_CHECK(out.empty());

    // and a blob too short to hold a signature
    D_TA_CHECK(dt::zip_local_methods(dj::byte_buffer(), out));
    D_TA_CHECK(out.empty());
    D_TA_CHECK(dt::zip_local_methods(dj::byte_buffer(3u, 'x'), out));
    D_TA_CHECK(out.empty());

    // a member with an empty payload still contributes a method
    std::vector<ta_zip_member> hollow;

    hollow.push_back(ta_zip_member("e", dj::byte_buffer(), 0u));
    D_TA_CHECK(dt::zip_local_methods(ta_make_zip(hollow), out));
    D_TA_CHECK(out.size() == 1u);
    D_TA_CHECK(out[0] == 0u);

    return true;
}

/*
tests_archive_zip_local_methods_rejects_truncated
  the bounds guards on the walk.
  Tests the following:
  - a local header signature followed by fewer than thirty bytes fails
  - a header whose declared payload runs past the end of the blob fails
  - a failure is reported rather than read out of bounds
*/
bool
tests_archive_zip_local_methods_rejects_truncated()
{
    std::vector<ta_zip_member> members;

    members.push_back(ta_zip_member("a.txt", dj::byte_buffer("hello"), 0u));

    const dj::byte_buffer     blob = ta_make_zip(members);
    std::vector<unsigned int> out;

    D_TA_CHECK(dt::zip_local_methods(blob, out));

    // the signature present but the header incomplete
    std::size_t n = 0;

    for (n = 4u; n < 30u; ++n)
    {
        const dj::byte_buffer cut = blob.substr(0, n);

        D_TA_CHECK(!dt::zip_local_methods(cut, out));
    }

    // a full header whose declared payload runs past the end
    dj::byte_buffer header_only =
        ta_local_header("a.txt", dj::byte_buffer("hello"), 0u);

    // drop the payload but leave the declared size in place
    header_only = header_only.substr(0, header_only.size() - 3u);

    D_TA_CHECK(header_only.size() >= 30u);
    D_TA_CHECK(!dt::zip_local_methods(header_only, out));

    // a header declaring a name longer than the blob can hold
    dj::byte_buffer long_name =
        ta_local_header("a.txt", dj::byte_buffer(), 0u);

    long_name[26] = static_cast<char>(0xFFu);
    long_name[27] = static_cast<char>(0xFFu);

    D_TA_CHECK(!dt::zip_local_methods(long_name, out));

    // a header declaring an enormous payload
    dj::byte_buffer big_payload =
        ta_local_header("a.txt", dj::byte_buffer("hello"), 0u);

    big_payload[18] = static_cast<char>(0xFFu);
    big_payload[19] = static_cast<char>(0xFFu);
    big_payload[20] = static_cast<char>(0x00u);
    big_payload[21] = static_cast<char>(0x00u);

    D_TA_CHECK(!dt::zip_local_methods(big_payload, out));

    return true;
}

/*
tests_archive_looks_like_gzip_checks_three_bytes
  the gzip front check.
  Tests the following:
  - both magic bytes and the DEFLATE method byte are required
  - a bare two-byte magic is rejected, so the method byte really is checked
  - a wrong byte at any of the three positions is rejected
  - a buffer shorter than three bytes is rejected
*/
bool
tests_archive_looks_like_gzip_checks_three_bytes()
{
    const dj::byte_buffer good = ta_make_gzip_header(32);

    D_TA_CHECK(dt::looks_like_gzip(good));

    // exactly three bytes is enough
    D_TA_CHECK(dt::looks_like_gzip(ta_make_gzip_header(0)));

    // the bare two-byte magic is NOT enough
    D_TA_CHECK(!dt::looks_like_gzip(ta_make_gzip_header(0).substr(0, 2)));
    D_TA_CHECK(!dt::looks_like_gzip(ta_make_gzip_header(0).substr(0, 1)));
    D_TA_CHECK(!dt::looks_like_gzip(dj::byte_buffer()));

    // every one of the three bytes is checked
    std::size_t i = 0;

    for (i = 0; i < 3u; ++i)
    {
        dj::byte_buffer broken = good;

        broken[i] = static_cast<char>(
            static_cast<unsigned char>(broken[i]) ^ 0xFFu);

        D_TA_CHECK(!dt::looks_like_gzip(broken));
    }

    // a different compression method byte is rejected
    dj::byte_buffer other_method = good;

    other_method[2] = static_cast<char>(0x00u);
    D_TA_CHECK(!dt::looks_like_gzip(other_method));

    // other containers are not gzip
    D_TA_CHECK(!dt::looks_like_gzip(ta_eocd(0u)));
    D_TA_CHECK(!dt::looks_like_gzip(ta_make_tar_blocks(1)));

    return true;
}

/*
tests_archive_tar_has_ustar_magic_at_offset
  the ustar marker.
  Tests the following:
  - the magic is read at offset 257 specifically, not merely found anywhere
  - a blob shorter than 263 bytes is rejected without reading past its end
  - a wrong byte at any of the five magic positions is rejected
*/
bool
tests_archive_tar_has_ustar_magic_at_offset()
{
    const dj::byte_buffer good = ta_make_tar_blocks(1);

    D_TA_CHECK(dt::tar_has_ustar_magic(good));

    // the offset is exact: the same text elsewhere does not count
    dj::byte_buffer misplaced(512u, static_cast<char>(0));

    misplaced[0] = 'u';
    misplaced[1] = 's';
    misplaced[2] = 't';
    misplaced[3] = 'a';
    misplaced[4] = 'r';

    D_TA_CHECK(!dt::tar_has_ustar_magic(misplaced));

    // one byte off in either direction fails
    dj::byte_buffer shifted(512u, static_cast<char>(0));

    shifted[256] = 'u';
    shifted[257] = 's';
    shifted[258] = 't';
    shifted[259] = 'a';
    shifted[260] = 'r';

    D_TA_CHECK(!dt::tar_has_ustar_magic(shifted));

    // the length guard
    D_TA_CHECK(!dt::tar_has_ustar_magic(dj::byte_buffer()));
    D_TA_CHECK(!dt::tar_has_ustar_magic(good.substr(0, 262)));
    D_TA_CHECK(dt::tar_has_ustar_magic(good.substr(0, 263)));

    // every magic byte is checked
    std::size_t i = 0;

    for (i = 257u; i <= 261u; ++i)
    {
        dj::byte_buffer broken = good;

        broken[i] = 'X';
        D_TA_CHECK(!dt::tar_has_ustar_magic(broken));
    }

    // other containers are not tar
    D_TA_CHECK(!dt::tar_has_ustar_magic(ta_eocd(0u)));
    D_TA_CHECK(!dt::tar_has_ustar_magic(ta_make_gzip_header(400)));

    return true;
}

/*
tests_archive_tar_is_terminated_checks_zero_tail
  the mandatory zero tail.
  Tests the following:
  - a stream ending in 1024 zero bytes is accepted
  - one non-zero byte anywhere in the tail fails, including at either end
  - a blob shorter than 1024 bytes is rejected
  - the check reads the LAST 1024 bytes, so leading content is irrelevant
*/
bool
tests_archive_tar_is_terminated_checks_zero_tail()
{
    const dj::byte_buffer good = ta_make_tar_blocks(1);

    D_TA_CHECK(good.size() == (512u + 1024u));
    D_TA_CHECK(dt::tar_is_terminated(good));

    // exactly 1024 zero bytes is enough
    const dj::byte_buffer bare(1024u, static_cast<char>(0));

    D_TA_CHECK(dt::tar_is_terminated(bare));

    // the length guard
    D_TA_CHECK(!dt::tar_is_terminated(dj::byte_buffer()));
    D_TA_CHECK(!dt::tar_is_terminated(dj::byte_buffer(1023u,
                                                      static_cast<char>(0))));

    // a non-zero byte at either end of the tail, and in the middle
    dj::byte_buffer first_bad = good;

    first_bad[good.size() - 1024u] = 'X';
    D_TA_CHECK(!dt::tar_is_terminated(first_bad));

    dj::byte_buffer last_bad = good;

    last_bad[good.size() - 1u] = 'X';
    D_TA_CHECK(!dt::tar_is_terminated(last_bad));

    dj::byte_buffer mid_bad = good;

    mid_bad[good.size() - 500u] = 'X';
    D_TA_CHECK(!dt::tar_is_terminated(mid_bad));

    // leading content is irrelevant: only the last 1024 bytes are read
    dj::byte_buffer noisy(2000u, 'N');

    noisy.append(1024u, static_cast<char>(0));
    D_TA_CHECK(dt::tar_is_terminated(noisy));

    // the byte just before the tail may be anything
    dj::byte_buffer edge(1u, 'N');

    edge.append(1024u, static_cast<char>(0));
    D_TA_CHECK(dt::tar_is_terminated(edge));

    // several header blocks then the tail
    D_TA_CHECK(dt::tar_is_terminated(ta_make_tar_blocks(3)));
    D_TA_CHECK(dt::tar_has_ustar_magic(ta_make_tar_blocks(3)));

    return true;
}

/*
tests_archive_sniffers_agree_with_real_containers
  the hand-built bytes against genuine output.
  Tests the following:
  - wherever this build can actually write a format, the sniffers accept
    the real container, so the synthetic fixtures above are not testing a
    layout the writer never produces
  - the advertised member count matches what was handed to the writer
  - a format that cannot be written produces nothing to sniff, and the
    sniffers correctly reject the empty result
*/
bool
tests_archive_sniffers_agree_with_real_containers()
{
    const dj::entry_list items = ta_sample_entries();
    dj::byte_buffer      blob;

    // zip, if this build writes one
    if (dj::try_archive<dj::formats::zip>(items, blob,
                                          dj::archive_options()) ==
        dj::status_ok)
    {
        D_TA_CHECK(dt::looks_like_zip(blob));
        D_TA_CHECK(dt::zip_find_eocd(blob) != ta_npos);
        D_TA_CHECK(dt::zip_total_entries(blob) ==
                   static_cast<long>(items.size()));

        std::vector<unsigned int> methods;

        D_TA_CHECK(dt::zip_local_methods(blob, methods));
        D_TA_CHECK(methods.size() == items.size());

        // and it is not mistaken for another container
        D_TA_CHECK(!dt::looks_like_gzip(blob));
        D_TA_CHECK(!dt::tar_has_ustar_magic(blob));
    }
    else
    {
        // nothing was written, and the sniffers say so
        D_TA_CHECK(blob.empty());
        D_TA_CHECK(!dt::looks_like_zip(blob));
        D_TA_CHECK(dt::zip_total_entries(blob) == -1L);
    }

    // tar, if this build writes one
    blob.clear();

    if (dj::try_archive<dj::formats::tar>(items, blob,
                                          dj::archive_options()) ==
        dj::status_ok)
    {
        D_TA_CHECK(dt::tar_has_ustar_magic(blob));
        D_TA_CHECK(dt::tar_is_terminated(blob));
        D_TA_CHECK((blob.size() % 512u) == 0u);

        // and it is not mistaken for another container
        D_TA_CHECK(!dt::looks_like_zip(blob));
        D_TA_CHECK(!dt::looks_like_gzip(blob));
    }
    else
    {
        D_TA_CHECK(blob.empty());
        D_TA_CHECK(!dt::tar_has_ustar_magic(blob));
        D_TA_CHECK(!dt::tar_is_terminated(blob));
    }

    // gz, if this build writes one.  It takes exactly one member.
    dj::entry_list one;

    one.push_back(dt::make_text_entry("only.txt", "payload"));
    blob.clear();

    if (dj::try_archive<dj::formats::gz>(one, blob,
                                         dj::archive_options()) ==
        dj::status_ok)
    {
        D_TA_CHECK(dt::looks_like_gzip(blob));
        D_TA_CHECK(!dt::looks_like_zip(blob));
    }
    else
    {
        D_TA_CHECK(blob.empty());
        D_TA_CHECK(!dt::looks_like_gzip(blob));
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
