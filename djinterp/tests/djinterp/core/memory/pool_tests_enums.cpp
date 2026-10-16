/******************************************************************************
* djinterp [test]                                          pool_tests_enums.cpp
*
*   Section I of the pool suite - the two classification enums.
*
*   pool_release and pool_block_layout are scoped enums carrying no public
* numeric contract, so the checks here verify distinctness, constexpr
* reachability, and scoped-enum hygiene rather than specific values.
*
*
* path:      /tests/djinterp/core/memory/pool_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_pool_release_enumerators_are_distinct
  Verifies the three release enumerators are pairwise distinct, compare
  equal to themselves, survive copy and assignment, and are reachable in
  a constant expression.
*/
bool
tests_pool_release_enumerators_are_distinct()
{
    static_assert(pool_release::monotonic != pool_release::free_list,
                  "monotonic and free_list must differ.");
    static_assert(pool_release::monotonic != pool_release::generational,
                  "monotonic and generational must differ.");
    static_assert(pool_release::free_list != pool_release::generational,
                  "free_list and generational must differ.");

    D_PR_CHECK(pool_release::monotonic != pool_release::free_list);
    D_PR_CHECK(pool_release::monotonic != pool_release::generational);
    D_PR_CHECK(pool_release::free_list != pool_release::generational);

    D_PR_CHECK(pool_release::monotonic    == pool_release::monotonic);
    D_PR_CHECK(pool_release::free_list    == pool_release::free_list);
    D_PR_CHECK(pool_release::generational == pool_release::generational);

    // copy and assignment preserve the enumerator
    pool_release value = pool_release::generational;
    D_PR_CHECK(value == pool_release::generational);

    value = pool_release::monotonic;
    D_PR_CHECK(value == pool_release::monotonic);

    // reachable in a constant expression
    constexpr pool_release folded = pool_release::free_list;
    D_PR_CHECK(folded == pool_release::free_list);

    return true;
}

/*
tests_pool_block_layout_enumerators_are_distinct
  Verifies the two block-layout enumerators are distinct, self-equal,
  assignable, and constexpr-reachable.
*/
bool
tests_pool_block_layout_enumerators_are_distinct()
{
    static_assert(pool_block_layout::contiguous !=
                      pool_block_layout::chunked,
                  "contiguous and chunked must differ.");

    D_PR_CHECK(pool_block_layout::contiguous !=
               pool_block_layout::chunked);

    D_PR_CHECK(pool_block_layout::contiguous ==
               pool_block_layout::contiguous);
    D_PR_CHECK(pool_block_layout::chunked ==
               pool_block_layout::chunked);

    pool_block_layout value = pool_block_layout::chunked;
    D_PR_CHECK(value == pool_block_layout::chunked);

    value = pool_block_layout::contiguous;
    D_PR_CHECK(value == pool_block_layout::contiguous);

    constexpr pool_block_layout folded = pool_block_layout::chunked;
    D_PR_CHECK(folded == pool_block_layout::chunked);

    return true;
}

/*
tests_pool_enums_are_scoped_and_distinct_types
  Verifies both enums are declared `enum class` - they are their own
  types, are not the same as their underlying type, and do not convert
  implicitly to int - and that they are distinct from one another.
*/
bool
tests_pool_enums_are_scoped_and_distinct_types()
{
    D_PR_CHECK(!std::is_same<pool_release, int>::value);
    D_PR_CHECK(!std::is_same<pool_block_layout, int>::value);
    D_PR_CHECK(!std::is_same<pool_release, pool_block_layout>::value);

    // scoped enums do not convert implicitly in either direction
    D_PR_CHECK(!std::is_convertible<pool_release, int>::value);
    D_PR_CHECK(!std::is_convertible<int, pool_release>::value);
    D_PR_CHECK(!std::is_convertible<pool_block_layout, int>::value);
    D_PR_CHECK(!std::is_convertible<int, pool_block_layout>::value);

    // ... but remain explicitly convertible, and enum types at all
    D_PR_CHECK(std::is_enum<pool_release>::value);
    D_PR_CHECK(std::is_enum<pool_block_layout>::value);
    D_PR_CHECK(static_cast<int>(pool_release::monotonic) >= 0);
    D_PR_CHECK(static_cast<int>(pool_block_layout::contiguous) >= 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
