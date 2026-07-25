#include "test_archive_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// ao_check_block
//   function: the shared body of the five per-block field sweeps.  Walks the
// mutation table, and for every row belonging to _block asserts that the named
// block comparator SEES the change; rows outside the block must leave it
// reporting equal.  Returns false on the first disagreement, naming the field.
static bool
ao_check_block(
    const char* _block,
    std::size_t _expect_fields
)
{
    const std::vector<ao_field> t     = ao_all_mutators();
    const dj::archive_options   fresh = dj::archive_options();
    std::size_t                 seen  = 0;
    std::size_t                 i     = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);
        const bool                mine  = (std::string(t[i].block) ==
                                           std::string(_block));
        const bool                eq    = ao_block_equal(_block, fresh, moved);

        if (mine)
        {
            ++seen;

            // the block owning this field must notice it moved
            if (eq)
            {
                std::printf("    [FAIL] %s comparator missed field %s\n",
                            _block, t[i].label);

                return false;
            }
        }
        else if (!eq)
        {
            // a field outside this block must not disturb it
            std::printf("    [FAIL] %s comparator reacted to foreign %s\n",
                        _block, t[i].label);

            return false;
        }
    }

    if (seen != _expect_fields)
    {
        std::printf("    [FAIL] %s: table carries %u fields, expected %u\n",
                    _block,
                    static_cast<unsigned int>(seen),
                    static_cast<unsigned int>(_expect_fields));

        return false;
    }

    return true;
}


/*
tests_archive_options_blocks_reflexive
  the baseline every comparator must satisfy.
  Tests the following:
  - each per-format block compares equal to itself
  - two independently default-constructed blocks compare equal, so equality
    is by value rather than by identity
  - a block compares equal to a copy of itself
*/
bool
tests_archive_options_blocks_reflexive()
{
    const dj::archive_options a;
    const dj::archive_options b;

    D_AO_CHECK(dt::zip_options_equal(a.zip, a.zip));
    D_AO_CHECK(dt::zip_options_equal(a.zip, b.zip));
    D_AO_CHECK(dt::tar_options_equal(a.tar, b.tar));
    D_AO_CHECK(dt::gz_options_equal(a.gz, b.gz));
    D_AO_CHECK(dt::sevenzip_options_equal(a.sevenzip, b.sevenzip));
    D_AO_CHECK(dt::rar_options_equal(a.rar, b.rar));

    // and each compares equal to a copy
    const dj::zip_options      zc = a.zip;
    const dj::sevenzip_options sc = a.sevenzip;

    D_AO_CHECK(dt::zip_options_equal(a.zip, zc));
    D_AO_CHECK(dt::sevenzip_options_equal(a.sevenzip, sc));

    return true;
}

/*
tests_archive_options_zip_equal_all_fields
  the ZIP block's field coverage.
  Tests the following:
  - each of method, encryption, password, zip64 and utf8_names is compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the five documented ZIP fields
*/
bool
tests_archive_options_zip_equal_all_fields()
{
    D_AO_CHECK(ao_check_block("zip", 5u));

    return true;
}

/*
tests_archive_options_tar_equal_all_fields
  the tar block's field coverage.
  Tests the following:
  - both format and numeric_owner are compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the two documented tar fields
*/
bool
tests_archive_options_tar_equal_all_fields()
{
    D_AO_CHECK(ao_check_block("tar", 2u));

    return true;
}

/*
tests_archive_options_gz_equal_all_fields
  the gzip header block's field coverage.
  Tests the following:
  - each of store_name, store_mtime and original_name is compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the three documented gz fields
*/
bool
tests_archive_options_gz_equal_all_fields()
{
    D_AO_CHECK(ao_check_block("gz", 3u));

    return true;
}

/*
tests_archive_options_sevenzip_equal_all_fields
  the 7z block's field coverage.
  Tests the following:
  - all six 7z fields are compared, including the two independent header
    flags a hand-written conjunction easily conflates
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the six documented 7z fields
*/
bool
tests_archive_options_sevenzip_equal_all_fields()
{
    D_AO_CHECK(ao_check_block("sevenzip", 6u));

    return true;
}

/*
tests_archive_options_rar_equal_all_fields
  the RAR block's field coverage.
  Tests the following:
  - each of level, solid, recovery_record and password is compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the four documented RAR fields
*/
bool
tests_archive_options_rar_equal_all_fields()
{
    D_AO_CHECK(ao_check_block("rar", 4u));

    return true;
}

/*
tests_archive_options_blocks_are_independent
  cross-block isolation.
  Tests the following:
  - moving one field leaves every block it does not belong to reporting
    equal, so no comparator reaches outside its own block
  - an archive-level or codec change disturbs no per-format comparator at all
*/
bool
tests_archive_options_blocks_are_independent()
{
    const std::vector<ao_field> t     = ao_all_mutators();
    const dj::archive_options   fresh = dj::archive_options();
    std::size_t                 i     = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::archive_options moved = ao_mutated(i);
        const std::string         owner(t[i].block);

        std::size_t disagreed = 0;

        if (!dt::zip_options_equal(fresh.zip, moved.zip))
        {
            ++disagreed;
        }
        if (!dt::tar_options_equal(fresh.tar, moved.tar))
        {
            ++disagreed;
        }
        if (!dt::gz_options_equal(fresh.gz, moved.gz))
        {
            ++disagreed;
        }
        if (!dt::sevenzip_options_equal(fresh.sevenzip, moved.sevenzip))
        {
            ++disagreed;
        }
        if (!dt::rar_options_equal(fresh.rar, moved.rar))
        {
            ++disagreed;
        }

        if ( (owner == "archive") ||
             (owner == "codec") )
        {
            // neither owns a per-format block
            D_AO_CHECK(disagreed == 0u);
        }
        else
        {
            D_AO_CHECK(disagreed == 1u);
            D_AO_CHECK(!ao_block_equal(t[i].block, fresh, moved));
        }
    }

    return true;
}

/*
tests_archive_options_blocks_compare_strings
  the string-bearing fields.
  Tests the following:
  - the ZIP, 7z and RAR passphrases and the gzip original name compare by
    content, so two equal strings built differently still match
  - an empty string and a one-character string are told apart
  - the comparison is case sensitive
*/
bool
tests_archive_options_blocks_compare_strings()
{
    dj::archive_options a;
    dj::archive_options b;

    // the same passphrase assembled two different ways
    a.zip.password = "secret";
    b.zip.password = std::string("sec") + "ret";

    D_AO_CHECK(a.zip.password == b.zip.password);
    D_AO_CHECK(dt::zip_options_equal(a.zip, b.zip));

    // case matters
    b.zip.password = "Secret";
    D_AO_CHECK(!dt::zip_options_equal(a.zip, b.zip));

    // an empty passphrase differs from any non-empty one
    b.zip.password.clear();
    D_AO_CHECK(!dt::zip_options_equal(a.zip, b.zip));

    // and the pristine default is the empty string
    const dj::archive_options fresh;

    D_AO_CHECK(fresh.zip.password.empty());
    D_AO_CHECK(fresh.sevenzip.password.empty());
    D_AO_CHECK(fresh.rar.password.empty());
    D_AO_CHECK(fresh.gz.original_name.empty());
    D_AO_CHECK(fresh.comment.empty());

    // the same holds for the other string-bearing blocks
    dj::archive_options s1;
    dj::archive_options s2;

    s1.sevenzip.password = "pw";
    s2.sevenzip.password = "pw";
    D_AO_CHECK(dt::sevenzip_options_equal(s1.sevenzip, s2.sevenzip));

    s2.sevenzip.password = "pw2";
    D_AO_CHECK(!dt::sevenzip_options_equal(s1.sevenzip, s2.sevenzip));

    dj::archive_options g1;
    dj::archive_options g2;

    g1.gz.original_name = "report.txt";
    g2.gz.original_name = "report.txt";
    D_AO_CHECK(dt::gz_options_equal(g1.gz, g2.gz));

    g2.gz.original_name = "report.bin";
    D_AO_CHECK(!dt::gz_options_equal(g1.gz, g2.gz));

    dj::archive_options r1;
    dj::archive_options r2;

    r1.rar.password = "k";
    r2.rar.password = "k";
    D_AO_CHECK(dt::rar_options_equal(r1.rar, r2.rar));

    r2.rar.password = "";
    D_AO_CHECK(!dt::rar_options_equal(r1.rar, r2.rar));

    return true;
}

NS_END  // testing
NS_END  // djinterp
