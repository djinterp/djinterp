/*******************************************************************************
* djinterp [test]                               test_defaults_tests_metadata.cpp
*
*   Section II - the sorted tag-metadata helpers.  metadata_insert keeps the
* vector sorted and drops duplicates; metadata_contains is a binary search;
* metadata_remove erases a present tag (reporting whether it was there) and is
* a no-op miss otherwise.  The sorted invariant is checked directly (the type
* is a std::vector<std::string>), and the edges - duplicate insert, absent
* contains, remove of an absent tag, remove from an empty vector - are covered.
*
*
* path:      /tests/djinterp/test/test_defaults_tests_metadata.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_defaults_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_metadata_insert_sorted
//   inserting out of order yields a sorted vector.
bool
tests_metadata_insert_sorted()
{
    bool ok = true;
    dt::test_metadata_type m;

    dt::metadata_insert(m, "charlie");
    dt::metadata_insert(m, "alpha");
    dt::metadata_insert(m, "bravo");

    ok = D_TD_CHECK(m.size() == 3) && ok;
    ok = D_TD_CHECK(m[0] == "alpha")   && ok;
    ok = D_TD_CHECK(m[1] == "bravo")   && ok;
    ok = D_TD_CHECK(m[2] == "charlie") && ok;

    return ok;
}

// tests_metadata_insert_duplicate
//   a duplicate insert is silently ignored.
bool
tests_metadata_insert_duplicate()
{
    bool ok = true;
    dt::test_metadata_type m;

    dt::metadata_insert(m, "tag");
    dt::metadata_insert(m, "tag");
    dt::metadata_insert(m, "tag");

    ok = D_TD_CHECK(m.size() == 1)    && ok;
    ok = D_TD_CHECK(m[0] == "tag")    && ok;

    return ok;
}

// tests_metadata_contains
//   contains finds a present tag and rejects an absent one (and the empty set).
bool
tests_metadata_contains()
{
    bool ok = true;
    dt::test_metadata_type m;

    ok = D_TD_CHECK(!dt::metadata_contains(m, "x")) && ok;   // empty

    dt::metadata_insert(m, "fast");
    dt::metadata_insert(m, "smoke");
    ok = D_TD_CHECK( dt::metadata_contains(m, "fast"))  && ok;
    ok = D_TD_CHECK( dt::metadata_contains(m, "smoke")) && ok;
    ok = D_TD_CHECK(!dt::metadata_contains(m, "slow"))  && ok;

    return ok;
}

// tests_metadata_remove
//   remove reports true and erases a present tag; false for an absent tag and
// for an empty vector.
bool
tests_metadata_remove()
{
    bool ok = true;
    dt::test_metadata_type m;

    ok = D_TD_CHECK(!dt::metadata_remove(m, "x")) && ok;   // remove from empty

    dt::metadata_insert(m, "a");
    dt::metadata_insert(m, "b");
    dt::metadata_insert(m, "c");

    ok = D_TD_CHECK( dt::metadata_remove(m, "b"))   && ok;  // present
    ok = D_TD_CHECK(!dt::metadata_contains(m, "b")) && ok;  // gone
    ok = D_TD_CHECK( m.size() == 2)                 && ok;
    ok = D_TD_CHECK( m[0] == "a" && m[1] == "c")    && ok;  // order preserved

    ok = D_TD_CHECK(!dt::metadata_remove(m, "z"))   && ok;  // absent

    return ok;
}

// tests_metadata_interleaved_order
//   interleaved inserts and removes keep the sorted invariant.
bool
tests_metadata_interleaved_order()
{
    bool ok = true;
    dt::test_metadata_type m;

    dt::metadata_insert(m, "m");
    dt::metadata_insert(m, "a");
    dt::metadata_insert(m, "z");
    dt::metadata_insert(m, "f");
    dt::metadata_remove(m, "a");
    dt::metadata_insert(m, "c");

    ok = D_TD_CHECK(m.size() == 4) && ok;
    ok = D_TD_CHECK(m[0] == "c" && m[1] == "f" && m[2] == "m" && m[3] == "z") && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
