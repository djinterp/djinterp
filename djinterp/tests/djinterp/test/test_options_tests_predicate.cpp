/*******************************************************************************
* djinterp [test]                               test_options_tests_predicate.cpp
*
*   Sections A.II / A.III - match_context and the runtime predicate IR.  Every
* match kind is exercised through its value-level builder: any / none, exact
* name (name_eq), substring name (name_has, including the empty-needle case that
* matches anything), regex name (name_re, a search not a full match), type id,
* status, and tag membership.  The combinators are covered at their edges -
* all_of is a conjunction that is vacuously true when empty, any_of a
* disjunction that is false when empty, negate inverts and treats an empty child
* list as true - and a nested tree confirms they recurse correctly.
*
*
* path:      /tests/djinterp/test/test_options_tests_predicate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_pred_default_any
//   a default-constructed predicate has kind `any` and matches anything.
bool
tests_pred_default_any()
{
    bool ok = true;
    dt::test_predicate p;

    ok = D_TO_CHECK(p.evaluate(make_ctx("anything"))) && ok;
    ok = D_TO_CHECK(p.evaluate(make_ctx()))           && ok;   // even the empty context

    return ok;
}

// tests_pred_any_none
//   match_any is always true; match_none is always false.
bool
tests_pred_any_none()
{
    bool ok = true;

    ok = D_TO_CHECK( dt::match_any().evaluate(make_ctx("x")))  && ok;
    ok = D_TO_CHECK(!dt::match_none().evaluate(make_ctx("x"))) && ok;

    return ok;
}

// tests_pred_name_eq
//   match_name is exact-name equality (not substring).
bool
tests_pred_name_eq()
{
    bool ok = true;
    dt::test_predicate p = dt::match_name("login");

    ok = D_TO_CHECK( p.evaluate(make_ctx("login")))      && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("logout")))     && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("login_test"))) && ok;   // exact, not prefix

    return ok;
}

// tests_pred_name_contains
//   match_name_contains is a substring test; an empty needle matches anything.
bool
tests_pred_name_contains()
{
    bool ok = true;
    dt::test_predicate p = dt::match_name_contains("perf");

    ok = D_TO_CHECK( p.evaluate(make_ctx("perf_suite"))) && ok;
    ok = D_TO_CHECK( p.evaluate(make_ctx("my_perf")))    && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("speed")))      && ok;
    ok = D_TO_CHECK( dt::match_name_contains("").evaluate(make_ctx("whatever"))) && ok;

    return ok;
}

// tests_pred_name_regex
//   match_name_regex is a regex search (matches on any substring).
bool
tests_pred_name_regex()
{
    bool ok = true;
    dt::test_predicate p = dt::match_name_regex("^test_[0-9]+$");

    ok = D_TO_CHECK( p.evaluate(make_ctx("test_42"))) && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("test_x")))  && ok;
    ok = D_TO_CHECK( dt::match_name_regex("perf").evaluate(make_ctx("my_perf_run"))) && ok;

    return ok;
}

// tests_pred_kind
//   match_kind compares the node's type id.
bool
tests_pred_kind()
{
    bool ok = true;
    dt::test_predicate p = dt::match_kind(dt::test_type_id{7});

    ok = D_TO_CHECK( p.evaluate(make_ctx("n", dt::test_type_id{7}))) && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("n", dt::test_type_id{3}))) && ok;

    return ok;
}

// tests_pred_status
//   match_status compares the node's status.
bool
tests_pred_status()
{
    bool ok = true;
    dt::test_predicate p = dt::match_status(dt::test_status::failed);

    ok = D_TO_CHECK( p.evaluate(make_ctx("n", 0, dt::test_status::failed))) && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("n", 0, dt::test_status::passed))) && ok;

    return ok;
}

// tests_pred_tag
//   match_tag looks for the operand among the node's tags.
bool
tests_pred_tag()
{
    bool ok = true;

    dt::match_context c = make_ctx("n");
    c.tags.push_back("fast");
    c.tags.push_back("smoke");
    ok = D_TO_CHECK( dt::match_tag("smoke").evaluate(c)) && ok;   // present among several
    ok = D_TO_CHECK(!dt::match_tag("slow").evaluate(c))  && ok;   // absent

    ok = D_TO_CHECK(!dt::match_tag("fast").evaluate(make_ctx("n"))) && ok;   // no tags at all

    return ok;
}

// tests_pred_all_of
//   all_of is a conjunction; a single miss fails; empty is vacuously true.
bool
tests_pred_all_of()
{
    bool ok = true;

    std::vector<dt::test_predicate> kids;
    kids.push_back(dt::match_name_contains("perf"));
    kids.push_back(dt::match_status(dt::test_status::passed));
    dt::test_predicate p = dt::match_all(kids);

    ok = D_TO_CHECK( p.evaluate(make_ctx("perf_x", 0, dt::test_status::passed))) && ok;  // both
    ok = D_TO_CHECK(!p.evaluate(make_ctx("perf_x", 0, dt::test_status::failed))) && ok;  // status miss
    ok = D_TO_CHECK(!p.evaluate(make_ctx("other",  0, dt::test_status::passed))) && ok;  // name miss
    ok = D_TO_CHECK( dt::match_all(std::vector<dt::test_predicate>()).evaluate(make_ctx("z"))) && ok;

    return ok;
}

// tests_pred_any_of
//   any_of is a disjunction; a single hit satisfies; empty is false.
bool
tests_pred_any_of()
{
    bool ok = true;

    std::vector<dt::test_predicate> kids;
    kids.push_back(dt::match_name("a"));
    kids.push_back(dt::match_name("b"));
    dt::test_predicate p = dt::match_any_of(kids);

    ok = D_TO_CHECK( p.evaluate(make_ctx("a"))) && ok;
    ok = D_TO_CHECK( p.evaluate(make_ctx("b"))) && ok;
    ok = D_TO_CHECK(!p.evaluate(make_ctx("c"))) && ok;
    ok = D_TO_CHECK(!dt::match_any_of(std::vector<dt::test_predicate>()).evaluate(make_ctx("z"))) && ok;

    return ok;
}

// tests_pred_negate
//   negate inverts its child; an empty negate is true.
bool
tests_pred_negate()
{
    bool ok = true;

    ok = D_TO_CHECK(!dt::match_not(dt::match_any()).evaluate(make_ctx("x")))  && ok;  // not true
    ok = D_TO_CHECK( dt::match_not(dt::match_none()).evaluate(make_ctx("x"))) && ok;  // not false
    ok = D_TO_CHECK( dt::match_not(dt::match_name("a")).evaluate(make_ctx("b"))) && ok;

    dt::test_predicate neg;                       // negate with no child
    neg.kind = dt::test_match_kind::negate;
    ok = D_TO_CHECK(neg.evaluate(make_ctx("x"))) && ok;

    return ok;
}

// tests_pred_nested
//   combinators nest: all_of[ any_of[name, tag], not[status] ].
bool
tests_pred_nested()
{
    bool ok = true;

    std::vector<dt::test_predicate> inner;
    inner.push_back(dt::match_name("login"));
    inner.push_back(dt::match_tag("critical"));

    std::vector<dt::test_predicate> outer;
    outer.push_back(dt::match_any_of(inner));
    outer.push_back(dt::match_not(dt::match_status(dt::test_status::skipped)));
    dt::test_predicate p = dt::match_all(outer);

    // name login + not skipped -> inner true (name), outer conjunction true
    ok = D_TO_CHECK( p.evaluate(make_ctx("login", 0, dt::test_status::passed)))  && ok;
    // name login but skipped -> not(skipped) false -> whole false
    ok = D_TO_CHECK(!p.evaluate(make_ctx("login", 0, dt::test_status::skipped))) && ok;

    // tag critical (name differs) + passed -> inner true (tag) -> whole true
    dt::match_context c = make_ctx("other", 0, dt::test_status::passed);
    c.tags.push_back("critical");
    ok = D_TO_CHECK( p.evaluate(c)) && ok;

    // neither name nor tag -> inner false -> whole false
    ok = D_TO_CHECK(!p.evaluate(make_ctx("other", 0, dt::test_status::passed))) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
