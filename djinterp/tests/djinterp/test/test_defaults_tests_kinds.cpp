/*******************************************************************************
* djinterp [test]                                  test_defaults_tests_kinds.cpp
*
*   Sections III / IV - the six built-in kind constants (ranked 0..5, the two
* leaves lowest), their back-compat aliases, and default_test_kinds().  The
* default six-record set is checked through the resolved queries the tree uses:
* rank_of gives the ascending rank per id, is_leaf marks assert / test_fn as
* leaves and the four interior kinds as non-leaves, name_of returns each kind's
* spelling, and every record leaves default_options null.
*
*
* path:      /tests/djinterp/test/test_defaults_tests_kinds.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_defaults_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_kinds_constant_values
//   the six kind ids are 0..5 and the count is 6.
bool
tests_kinds_constant_values()
{
    bool ok = true;

    ok = D_TD_CHECK(dt::D_TEST_KIND_ASSERT      == 0) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_TEST_FN     == 1) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_UNIT_TEST   == 2) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_TEST_BLOCK  == 3) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_TEST_MODULE == 4) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_TEST_SUITE  == 5) && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_COUNT == static_cast<std::size_t>(6)) && ok;

    return ok;
}

// tests_kinds_aliases
//   the pre-rename spellings map to the new ids.
bool
tests_kinds_aliases()
{
    bool ok = true;

    ok = D_TD_CHECK(dt::D_TEST_KIND_TEST   == dt::D_TEST_KIND_UNIT_TEST)   && ok;
    ok = D_TD_CHECK(dt::D_TEST_KIND_MODULE == dt::D_TEST_KIND_TEST_MODULE) && ok;

    return ok;
}

// tests_kinds_default_set_size
//   the default kind set has exactly the six records.
bool
tests_kinds_default_set_size()
{
    bool ok = true;
    std::vector<dt::test_kind> k = dt::default_test_kinds();

    ok = D_TD_CHECK(k.size() == static_cast<std::size_t>(dt::D_TEST_KIND_COUNT)) && ok;

    return ok;
}

// tests_kinds_default_ranks
//   rank_of returns the ascending rank for each id.
bool
tests_kinds_default_ranks()
{
    bool ok = true;
    std::vector<dt::test_kind> k = dt::default_test_kinds();

    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_ASSERT)      == 0) && ok;
    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_TEST_FN)     == 1) && ok;
    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_UNIT_TEST)   == 2) && ok;
    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_TEST_BLOCK)  == 3) && ok;
    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_TEST_MODULE) == 4) && ok;
    ok = D_TD_CHECK(dt::rank_of(k, dt::D_TEST_KIND_TEST_SUITE)  == 5) && ok;

    return ok;
}

// tests_kinds_default_leaf_flags
//   the two leaves are leaves; the four interior kinds are not.
bool
tests_kinds_default_leaf_flags()
{
    bool ok = true;
    std::vector<dt::test_kind> k = dt::default_test_kinds();

    ok = D_TD_CHECK( dt::is_leaf(k, dt::D_TEST_KIND_ASSERT))      && ok;
    ok = D_TD_CHECK( dt::is_leaf(k, dt::D_TEST_KIND_TEST_FN))     && ok;
    ok = D_TD_CHECK(!dt::is_leaf(k, dt::D_TEST_KIND_UNIT_TEST))   && ok;
    ok = D_TD_CHECK(!dt::is_leaf(k, dt::D_TEST_KIND_TEST_BLOCK))  && ok;
    ok = D_TD_CHECK(!dt::is_leaf(k, dt::D_TEST_KIND_TEST_MODULE)) && ok;
    ok = D_TD_CHECK(!dt::is_leaf(k, dt::D_TEST_KIND_TEST_SUITE))  && ok;

    return ok;
}

// tests_kinds_default_names
//   name_of returns each kind's spelling.
bool
tests_kinds_default_names()
{
    bool ok = true;
    std::vector<dt::test_kind> k = dt::default_test_kinds();

    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_ASSERT))      == "assert")      && ok;
    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_TEST_FN))     == "test_fn")     && ok;
    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_UNIT_TEST))   == "unit_test")   && ok;
    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_TEST_BLOCK))  == "test_block")  && ok;
    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_TEST_MODULE)) == "test_module") && ok;
    ok = D_TD_CHECK(std::string(dt::name_of(k, dt::D_TEST_KIND_TEST_SUITE))  == "test_suite")  && ok;

    return ok;
}

// tests_kinds_default_options_null
//   every default record leaves default_options null.
bool
tests_kinds_default_options_null()
{
    bool ok = true;
    std::vector<dt::test_kind> k = dt::default_test_kinds();

    bool all_null = true;
    for (std::size_t i = 0; i < k.size(); ++i)
    {
        if (k[i].default_options != nullptr) { all_null = false; }
    }
    ok = D_TD_CHECK(all_null) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
