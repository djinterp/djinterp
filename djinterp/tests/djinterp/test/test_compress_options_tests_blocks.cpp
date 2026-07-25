#include "test_compress_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// co_check_block
//   function: the shared body of the six per-block field sweeps.  Walks the
// mutation table, and for every row belonging to _block asserts that the named
// block comparator SEES the change; rows outside the block must leave it
// reporting equal.  Returns false on the first disagreement, naming the field.
static bool
co_check_block(
    const char* _block,
    std::size_t _expect_fields
)
{
    const std::vector<co_field> t     = co_all_mutators();
    const dj::compress_options  fresh = dj::compress_options();
    std::size_t                 seen  = 0;
    std::size_t                 i     = 0;

    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);
        const bool                 mine  = (std::string(t[i].block) ==
                                            std::string(_block));
        const bool                 eq    =
            co_block_equal(_block, fresh, moved);

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
tests_compress_options_deflate_equal_reflexive
  the baseline every comparator must satisfy.
  Tests the following:
  - a block compares equal to itself
  - two independently default-constructed blocks compare equal, so equality
    is by value rather than by identity
  - a block compares equal to a copy of itself
*/
bool
tests_compress_options_deflate_equal_reflexive()
{
    const dj::compress_options a;
    const dj::compress_options b;

    D_CO_CHECK(dt::deflate_options_equal(a.deflate, a.deflate));
    D_CO_CHECK(dt::deflate_options_equal(a.deflate, b.deflate));

    const dj::deflate_options copy = a.deflate;

    D_CO_CHECK(dt::deflate_options_equal(a.deflate, copy));

    // the same holds for the other five blocks
    D_CO_CHECK(dt::bzip2_options_equal(a.bzip2, b.bzip2));
    D_CO_CHECK(dt::lzma_options_equal(a.lzma, b.lzma));
    D_CO_CHECK(dt::zstd_options_equal(a.zstd, b.zstd));
    D_CO_CHECK(dt::lz4_options_equal(a.lz4, b.lz4));
    D_CO_CHECK(dt::brotli_options_equal(a.brotli, b.brotli));

    return true;
}

/*
tests_compress_options_deflate_equal_all_fields
  the DEFLATE block's field coverage.
  Tests the following:
  - each of window_bits, mem_level and strategy is compared, so moving any
    one of them alone is detected
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the three documented DEFLATE fields
*/
bool
tests_compress_options_deflate_equal_all_fields()
{
    D_CO_CHECK(co_check_block("deflate", 3u));

    return true;
}

/*
tests_compress_options_bzip2_equal_all_fields
  the bzip2 block's field coverage.
  Tests the following:
  - each of block_size_100k, work_factor, verbosity and small_decompress is
    compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the four documented bzip2 fields
*/
bool
tests_compress_options_bzip2_equal_all_fields()
{
    D_CO_CHECK(co_check_block("bzip2", 4u));

    return true;
}

/*
tests_compress_options_lzma_equal_all_fields
  the lzma / xz block's field coverage.
  Tests the following:
  - all eleven lzma fields are compared, including the three that default to
    -1 and the unsigned thread count
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the eleven documented lzma fields
*/
bool
tests_compress_options_lzma_equal_all_fields()
{
    D_CO_CHECK(co_check_block("lzma", 11u));

    return true;
}

/*
tests_compress_options_zstd_equal_all_fields
  the Zstandard block's field coverage.
  Tests the following:
  - all nineteen zstd fields are compared - the widest block in the
    vocabulary, and the one a hand-written conjunction is likeliest to miss
    a member of
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the nineteen documented zstd fields
*/
bool
tests_compress_options_zstd_equal_all_fields()
{
    D_CO_CHECK(co_check_block("zstd", 19u));

    return true;
}

/*
tests_compress_options_lz4_equal_all_fields
  the LZ4 frame block's field coverage.
  Tests the following:
  - all seven lz4 fields are compared, including the four independent flags
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the seven documented lz4 fields
*/
bool
tests_compress_options_lz4_equal_all_fields()
{
    D_CO_CHECK(co_check_block("lz4", 7u));

    return true;
}

/*
tests_compress_options_brotli_equal_all_fields
  the Brotli block's field coverage.
  Tests the following:
  - all five brotli fields are compared
  - a field outside the block leaves the comparator reporting equal
  - the table carries exactly the five documented brotli fields
*/
bool
tests_compress_options_brotli_equal_all_fields()
{
    D_CO_CHECK(co_check_block("brotli", 5u));

    return true;
}

/*
tests_compress_options_blocks_are_independent
  cross-block isolation.
  Tests the following:
  - moving one field of one block leaves the other five comparators
    reporting equal, so no comparator reaches outside its own block
  - this holds for a representative field of every block in turn
*/
bool
tests_compress_options_blocks_are_independent()
{
    const std::vector<co_field> t     = co_all_mutators();
    const dj::compress_options  fresh = dj::compress_options();
    std::size_t                 i     = 0;

    // for every row, exactly the owning block may disagree
    for (i = 0; i < t.size(); ++i)
    {
        const dj::compress_options moved = co_mutated(i);
        const std::string          owner(t[i].block);

        const bool d = dt::deflate_options_equal(fresh.deflate, moved.deflate);
        const bool b = dt::bzip2_options_equal(fresh.bzip2, moved.bzip2);
        const bool l = dt::lzma_options_equal(fresh.lzma, moved.lzma);
        const bool z = dt::zstd_options_equal(fresh.zstd, moved.zstd);
        const bool f = dt::lz4_options_equal(fresh.lz4, moved.lz4);
        const bool r = dt::brotli_options_equal(fresh.brotli, moved.brotli);

        // count how many blocks noticed; it must be at most one
        std::size_t disagreed = 0;

        if (!d) { ++disagreed; }
        if (!b) { ++disagreed; }
        if (!l) { ++disagreed; }
        if (!z) { ++disagreed; }
        if (!f) { ++disagreed; }
        if (!r) { ++disagreed; }

        if (owner.empty())
        {
            // the generic level belongs to no block
            D_CO_CHECK(disagreed == 0u);
        }
        else
        {
            D_CO_CHECK(disagreed == 1u);
            D_CO_CHECK(!co_block_equal(t[i].block, fresh, moved));
        }
    }

    return true;
}

/*
tests_compress_options_blocks_ignore_level
  the generic level's ownership.
  Tests the following:
  - compress_options::level sits outside every per-codec block, so moving it
    leaves all six block comparators reporting equal
  - the aggregate comparator does see it, which is what makes the generic
    level the aggregate's own responsibility
*/
bool
tests_compress_options_blocks_ignore_level()
{
    const dj::compress_options fresh;
    dj::compress_options       moved;

    moved.level = (fresh.level + 1);

    D_CO_CHECK(dt::deflate_options_equal(fresh.deflate, moved.deflate));
    D_CO_CHECK(dt::bzip2_options_equal(fresh.bzip2, moved.bzip2));
    D_CO_CHECK(dt::lzma_options_equal(fresh.lzma, moved.lzma));
    D_CO_CHECK(dt::zstd_options_equal(fresh.zstd, moved.zstd));
    D_CO_CHECK(dt::lz4_options_equal(fresh.lz4, moved.lz4));
    D_CO_CHECK(dt::brotli_options_equal(fresh.brotli, moved.brotli));

    // but the aggregate notices
    D_CO_CHECK(!dt::compress_options_equal(fresh, moved));

    // and zstd carries its OWN level, which the zstd comparator does own
    dj::compress_options zmoved;

    zmoved.zstd.level = (fresh.zstd.level + 1);

    D_CO_CHECK(!dt::zstd_options_equal(fresh.zstd, zmoved.zstd));

    return true;
}

NS_END  // testing
NS_END  // djinterp
