/******************************************************************************
* djinterp [test]                        test_builder_tests_kinds_summary.cpp
*
*   The builder's supporting vocabulary: the default kind set it seeds a tree
* with (module > block > test by rank, only test a leaf), the reserved kind
* ids, the run_summary tally type (its zero state and the all_passed /
* any_failed predicates), and the make_suite factory.
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests_kinds_summary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_default_kinds_records
//   default_kinds seeds exactly three records - module / block / test - with
// descending ranks 30 / 20 / 10 and only `test` a leaf.
bool
tests_default_kinds_records()
{
    bool ok = true;

    std::vector<dt::test_kind> k = dt::default_kinds();
    ok = D_TB_CHECK(k.size() == std::size_t(3))                     && ok;

    ok = D_TB_CHECK(k[0].id == dt::k_kind_module)                  && ok;
    ok = D_TB_CHECK(k[0].rank == 30)                               && ok;
    ok = D_TB_CHECK(!k[0].is_leaf)                                 && ok;
    ok = D_TB_CHECK(std::string(k[0].name) == "module")           && ok;

    ok = D_TB_CHECK(k[1].id == dt::k_kind_block)                   && ok;
    ok = D_TB_CHECK(k[1].rank == 20)                               && ok;
    ok = D_TB_CHECK(!k[1].is_leaf)                                 && ok;
    ok = D_TB_CHECK(std::string(k[1].name) == "block")            && ok;

    ok = D_TB_CHECK(k[2].id == dt::k_kind_test)                    && ok;
    ok = D_TB_CHECK(k[2].rank == 10)                               && ok;
    ok = D_TB_CHECK(k[2].is_leaf)                                  && ok;
    ok = D_TB_CHECK(std::string(k[2].name) == "test")             && ok;

    // module > block > test, the whole point of the seeding
    ok = D_TB_CHECK(k[0].rank > k[1].rank && k[1].rank > k[2].rank) && ok;

    return ok;
}

// tests_kind_id_constants
//   the reserved kind ids hold their documented values.
bool
tests_kind_id_constants()
{
    bool ok = true;

    ok = D_TB_CHECK(dt::k_kind_module == dt::test_type_id(1000)) && ok;
    ok = D_TB_CHECK(dt::k_kind_block  == dt::test_type_id(1001)) && ok;
    ok = D_TB_CHECK(dt::k_kind_test   == dt::test_type_id(1002)) && ok;

    return ok;
}

// tests_summary_default_zeroed
//   a default-constructed summary is all-zero, and with nothing failed /
// errored / pending it reads as all-passed.
bool
tests_summary_default_zeroed()
{
    bool ok = true;

    dt::test_summary s;
    ok = D_TB_CHECK(s.modules == 0 && s.blocks == 0 &&
                    s.tests == 0 && s.inline_checks == 0)          && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0 && s.skipped == 0 &&
                    s.errored == 0 && s.pending == 0)              && ok;
    ok = D_TB_CHECK(s.all_passed())                               && ok;
    ok = D_TB_CHECK(!s.any_failed())                              && ok;

    return ok;
}

// tests_summary_all_passed_any_failed
//   all_passed is (no failed, no errored, no pending); any_failed is (some
// failed or errored).  A skipped-only run still reads all-passed.
bool
tests_summary_all_passed_any_failed()
{
    bool ok = true;

    // passed only
    {
        dt::test_summary s; s.passed = 5;
        ok = D_TB_CHECK(s.all_passed())  && ok;
        ok = D_TB_CHECK(!s.any_failed()) && ok;
    }
    // a pending leaf blocks all_passed but is not a failure
    {
        dt::test_summary s; s.passed = 5; s.pending = 1;
        ok = D_TB_CHECK(!s.all_passed()) && ok;
        ok = D_TB_CHECK(!s.any_failed()) && ok;
    }
    // a failure
    {
        dt::test_summary s; s.failed = 1;
        ok = D_TB_CHECK(!s.all_passed()) && ok;
        ok = D_TB_CHECK(s.any_failed())  && ok;
    }
    // an error counts as a failure for any_failed, and blocks all_passed
    {
        dt::test_summary s; s.errored = 1;
        ok = D_TB_CHECK(!s.all_passed()) && ok;
        ok = D_TB_CHECK(s.any_failed())  && ok;
    }
    // skipped alone does not block all_passed
    {
        dt::test_summary s; s.passed = 2; s.skipped = 3;
        ok = D_TB_CHECK(s.all_passed())  && ok;
        ok = D_TB_CHECK(!s.any_failed()) && ok;
    }

    return ok;
}

// tests_make_suite_fresh
//   make_suite yields a fresh builder: empty until authored, then usable as
// the head of a chain.
bool
tests_make_suite_fresh()
{
    bool ok = true;

    // a fresh suite run is empty
    dt::test_summary e = dt::make_suite().run();
    ok = D_TB_CHECK(e.modules == 0 && e.blocks == 0 && e.tests == 0) && ok;
    ok = D_TB_CHECK(e.passed == 0 && e.failed == 0)                  && ok;

    // and it heads a working chain
    dt::test_summary s = dt::make_suite()
                             .test_module("m")
                             .test("t").assert_(true)
                             .run();
    ok = D_TB_CHECK(s.modules == 1 && s.tests == 1) && ok;
    ok = D_TB_CHECK(s.passed == 1 && s.failed == 0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
