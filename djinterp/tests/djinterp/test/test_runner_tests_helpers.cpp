/*******************************************************************************
* djinterp [test]                                  test_runner_tests_helpers.cpp
*
*   Section I - the internal reporter helpers.  These are pure functions in
* djinterp::test::internal, so they are called directly: the raw-status map
* (every code plus the out-of-range default), the fixed-width status token, the
* show policy (all / failures_only / failures_and_skipped / summary_only /
* silent), the substitute-every-occurrence helper (including its empty-key guard
* and its advance-past-the-replacement behaviour), and the comma-separated tag
* splitter (trimming, dropped empties, edge cases).
*
*
* path:      /tests/djinterp/test/test_runner_tests_helpers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_helper_status_of
//   runner_status_of_helper maps each raw status code, defaulting an
// out-of-range code to pending.
bool
tests_helper_status_of()
{
    bool ok = true;
    typedef dt::basic_test bt;

    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(bt::status_passed)  == dt::test_status::passed)  && ok;
    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(bt::status_failed)  == dt::test_status::failed)  && ok;
    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(bt::status_skipped) == dt::test_status::skipped) && ok;
    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(bt::status_pending) == dt::test_status::pending) && ok;
    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(bt::status_error)   == dt::test_status::error)   && ok;
    ok = D_TR_CHECK(dt::internal::runner_status_of_helper(
                        static_cast<bt::status_type>(99)) == dt::test_status::pending) && ok;

    return ok;
}

// tests_helper_status_str
//   each status has its fixed-width display token.
bool
tests_helper_status_str()
{
    bool ok = true;

    ok = D_TR_CHECK(std::string(dt::internal::runner_status_str_helper(dt::test_status::passed))  == "PASS") && ok;
    ok = D_TR_CHECK(std::string(dt::internal::runner_status_str_helper(dt::test_status::failed))  == "FAIL") && ok;
    ok = D_TR_CHECK(std::string(dt::internal::runner_status_str_helper(dt::test_status::skipped)) == "SKIP") && ok;
    ok = D_TR_CHECK(std::string(dt::internal::runner_status_str_helper(dt::test_status::pending)) == "PEND") && ok;
    ok = D_TR_CHECK(std::string(dt::internal::runner_status_str_helper(dt::test_status::error))   == "ERR ") && ok;

    return ok;
}

// tests_helper_shown_all
//   the all policy shows every status.
bool
tests_helper_shown_all()
{
    bool ok = true;
    const dt::test_show a = dt::test_show::all;

    ok = D_TR_CHECK(dt::internal::runner_shown_helper(a, dt::test_status::passed))  && ok;
    ok = D_TR_CHECK(dt::internal::runner_shown_helper(a, dt::test_status::failed))  && ok;
    ok = D_TR_CHECK(dt::internal::runner_shown_helper(a, dt::test_status::skipped)) && ok;
    ok = D_TR_CHECK(dt::internal::runner_shown_helper(a, dt::test_status::pending)) && ok;
    ok = D_TR_CHECK(dt::internal::runner_shown_helper(a, dt::test_status::error))   && ok;

    return ok;
}

// tests_helper_shown_failures_only
//   failures_only shows only failed and error.
bool
tests_helper_shown_failures_only()
{
    bool ok = true;
    const dt::test_show f = dt::test_show::failures_only;

    ok = D_TR_CHECK( dt::internal::runner_shown_helper(f, dt::test_status::failed))  && ok;
    ok = D_TR_CHECK( dt::internal::runner_shown_helper(f, dt::test_status::error))   && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(f, dt::test_status::passed))  && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(f, dt::test_status::skipped)) && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(f, dt::test_status::pending)) && ok;

    return ok;
}

// tests_helper_shown_failures_and_skipped
//   failures_and_skipped adds skipped to the failing set.
bool
tests_helper_shown_failures_and_skipped()
{
    bool ok = true;
    const dt::test_show f = dt::test_show::failures_and_skipped;

    ok = D_TR_CHECK( dt::internal::runner_shown_helper(f, dt::test_status::failed))  && ok;
    ok = D_TR_CHECK( dt::internal::runner_shown_helper(f, dt::test_status::error))   && ok;
    ok = D_TR_CHECK( dt::internal::runner_shown_helper(f, dt::test_status::skipped)) && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(f, dt::test_status::passed))  && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(f, dt::test_status::pending)) && ok;

    return ok;
}

// tests_helper_shown_summary_and_silent
//   summary_only and silent show nothing.
bool
tests_helper_shown_summary_and_silent()
{
    bool ok = true;
    const dt::test_show su = dt::test_show::summary_only;
    const dt::test_show si = dt::test_show::silent;

    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(su, dt::test_status::failed))  && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(su, dt::test_status::passed))  && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(si, dt::test_status::failed))  && ok;
    ok = D_TR_CHECK(!dt::internal::runner_shown_helper(si, dt::test_status::error))   && ok;

    return ok;
}

// tests_helper_subst
//   substitution replaces every occurrence, guards an empty key, advances past
// the replacement (no infinite loop when the value contains the key), and can
// delete (empty value).
bool
tests_helper_subst()
{
    bool ok = true;

    std::string t1 = "hello {x} world {x}";
    dt::internal::runner_subst_helper(t1, "{x}", "Y");
    ok = D_TR_CHECK(t1 == "hello Y world Y") && ok;   // every occurrence

    std::string t2 = "abc";
    dt::internal::runner_subst_helper(t2, "{z}", "Q");
    ok = D_TR_CHECK(t2 == "abc") && ok;               // absent key -> unchanged

    std::string t3 = "abc";
    dt::internal::runner_subst_helper(t3, "", "Q");
    ok = D_TR_CHECK(t3 == "abc") && ok;               // empty key -> no-op

    std::string t4 = "{k}";
    dt::internal::runner_subst_helper(t4, "{k}", "{k}{k}");
    ok = D_TR_CHECK(t4 == "{k}{k}") && ok;            // advances past the insertion

    std::string t5 = "a{d}b{d}c";
    dt::internal::runner_subst_helper(t5, "{d}", "");
    ok = D_TR_CHECK(t5 == "abc") && ok;              // empty value deletes

    return ok;
}

// tests_helper_split_tags
//   tag splitting trims surrounding spaces, drops empty fields, and handles the
// empty string and a trailing comma.
bool
tests_helper_split_tags()
{
    bool ok = true;

    std::vector<std::string> a = dt::internal::runner_split_tags_helper("x,y,z");
    ok = D_TR_CHECK(a.size() == 3 && a[0] == "x" && a[1] == "y" && a[2] == "z") && ok;

    std::vector<std::string> b = dt::internal::runner_split_tags_helper("  a , b ,c  ");
    ok = D_TR_CHECK(b.size() == 3 && b[0] == "a" && b[1] == "b" && b[2] == "c") && ok;

    std::vector<std::string> c = dt::internal::runner_split_tags_helper("");
    ok = D_TR_CHECK(c.empty()) && ok;

    std::vector<std::string> d = dt::internal::runner_split_tags_helper("a,,b, ,c");
    ok = D_TR_CHECK(d.size() == 3 && d[0] == "a" && d[1] == "b" && d[2] == "c") && ok;

    std::vector<std::string> e = dt::internal::runner_split_tags_helper("solo,");
    ok = D_TR_CHECK(e.size() == 1 && e[0] == "solo") && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
