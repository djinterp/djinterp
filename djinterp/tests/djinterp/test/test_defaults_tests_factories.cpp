/*******************************************************************************
* djinterp [test]                              test_defaults_tests_factories.cpp
*
*   Sections V / VI - the node-identity helpers and the object factories that
* route through them.  set_identity stamps "name" and (when non-null)
* "descriptor"; node_name / node_descriptor read them back (empty when unset).
* The factories then build each kind: make_assert and make_test_fn are the two
* leaves and carry the boolean verdict as passed/failed (plus, for make_assert,
* the optional pass/fail message metadata); make_unit_test / make_test_block /
* make_test_module / make_test_suite are the interior nodes, born pending, each
* stamped with the right kind id and identity.
*
*
* path:      /tests/djinterp/test/test_defaults_tests_factories.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_defaults_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_factory_set_identity
//   set_identity writes name + descriptor; a null name/descriptor leaves that
// entry untouched.
bool
tests_factory_set_identity()
{
    bool ok = true;

    dt::basic_test n(dt::D_TEST_KIND_UNIT_TEST);
    dt::set_identity(n, "the_name", "the_desc");
    ok = D_TD_CHECK(dt::node_name(n)       == "the_name") && ok;
    ok = D_TD_CHECK(dt::node_descriptor(n) == "the_desc") && ok;

    dt::basic_test m(dt::D_TEST_KIND_UNIT_TEST);
    dt::set_identity(m, "only_name");                 // descriptor null
    ok = D_TD_CHECK(dt::node_name(m)       == "only_name") && ok;
    ok = D_TD_CHECK(dt::node_descriptor(m) == "")          && ok;

    return ok;
}

// tests_factory_node_readers
//   the readers return empty strings on a node with no identity stamped.
bool
tests_factory_node_readers()
{
    bool ok = true;
    dt::basic_test n(dt::D_TEST_KIND_ASSERT);

    ok = D_TD_CHECK(dt::node_name(n)       == "") && ok;
    ok = D_TD_CHECK(dt::node_descriptor(n) == "") && ok;

    return ok;
}

// tests_factory_make_assert
//   make_assert is the rank-0 leaf; its verdict is the boolean, and identity is
// stamped.
bool
tests_factory_make_assert()
{
    bool ok = true;

    dt::basic_test a = dt::make_assert(true, "a_name", "a_desc");
    ok = D_TD_CHECK(a.type_id() == dt::D_TEST_KIND_ASSERT)          && ok;
    ok = D_TD_CHECK(a.status()  == dt::basic_test::status_passed)   && ok;
    ok = D_TD_CHECK(dt::node_name(a)       == "a_name")             && ok;
    ok = D_TD_CHECK(dt::node_descriptor(a) == "a_desc")            && ok;

    dt::basic_test f = dt::make_assert(false, "f_name");
    ok = D_TD_CHECK(f.status() == dt::basic_test::status_failed)    && ok;

    return ok;
}

// tests_factory_make_assert_messages
//   the optional pass/fail messages land in metadata; omitted ones are absent.
bool
tests_factory_make_assert_messages()
{
    bool ok = true;

    dt::basic_test a = dt::make_assert(true, "n", "d", "on_pass", "on_fail");
    ok = D_TD_CHECK(a.metadata().contains("message_pass")) && ok;
    ok = D_TD_CHECK(a.metadata().contains("message_fail")) && ok;
    ok = D_TD_CHECK(a.metadata().get("message_pass") == "on_pass") && ok;
    ok = D_TD_CHECK(a.metadata().get("message_fail") == "on_fail") && ok;

    dt::basic_test b = dt::make_assert(true, "n");           // no messages
    ok = D_TD_CHECK(!b.metadata().contains("message_pass")) && ok;
    ok = D_TD_CHECK(!b.metadata().contains("message_fail")) && ok;

    return ok;
}

// tests_factory_make_test_fn
//   make_test_fn is the rank-1 leaf; its verdict is what the wrapped function
// returned.
bool
tests_factory_make_test_fn()
{
    bool ok = true;

    dt::basic_test t = dt::make_test_fn(true, "t_name", "t_desc");
    ok = D_TD_CHECK(t.type_id() == dt::D_TEST_KIND_TEST_FN)       && ok;
    ok = D_TD_CHECK(t.status()  == dt::basic_test::status_passed) && ok;
    ok = D_TD_CHECK(dt::node_name(t)       == "t_name")           && ok;
    ok = D_TD_CHECK(dt::node_descriptor(t) == "t_desc")          && ok;

    dt::basic_test f = dt::make_test_fn(false, "f");
    ok = D_TD_CHECK(f.status() == dt::basic_test::status_failed)  && ok;

    return ok;
}

// tests_factory_interior_nodes
//   the four interior factories stamp the right kind id, are born pending, and
// carry identity.
bool
tests_factory_interior_nodes()
{
    bool ok = true;

    dt::basic_test u = dt::make_unit_test("u", "ud");
    ok = D_TD_CHECK(u.type_id() == dt::D_TEST_KIND_UNIT_TEST)   && ok;
    ok = D_TD_CHECK(u.status()  == dt::basic_test::status_pending) && ok;
    ok = D_TD_CHECK(dt::node_name(u) == "u" && dt::node_descriptor(u) == "ud") && ok;

    dt::basic_test b = dt::make_test_block("b", "bd");
    ok = D_TD_CHECK(b.type_id() == dt::D_TEST_KIND_TEST_BLOCK)  && ok;
    ok = D_TD_CHECK(b.status()  == dt::basic_test::status_pending) && ok;

    dt::basic_test m = dt::make_test_module("m", "md");
    ok = D_TD_CHECK(m.type_id() == dt::D_TEST_KIND_TEST_MODULE) && ok;

    dt::basic_test s = dt::make_test_suite("s", "sd");
    ok = D_TD_CHECK(s.type_id() == dt::D_TEST_KIND_TEST_SUITE)  && ok;
    ok = D_TD_CHECK(dt::node_name(s) == "s" && dt::node_descriptor(s) == "sd") && ok;

    return ok;
}

// tests_factory_leaf_verdicts
//   the leaves reflect their boolean verdict; the interior nodes ignore any
// (they take no result and stay pending).
bool
tests_factory_leaf_verdicts()
{
    bool ok = true;

    ok = D_TD_CHECK(dt::make_assert(true).status()   == dt::basic_test::status_passed) && ok;
    ok = D_TD_CHECK(dt::make_assert(false).status()  == dt::basic_test::status_failed) && ok;
    ok = D_TD_CHECK(dt::make_test_fn(true).status()  == dt::basic_test::status_passed) && ok;
    ok = D_TD_CHECK(dt::make_test_fn(false).status() == dt::basic_test::status_failed) && ok;
    ok = D_TD_CHECK(dt::make_unit_test("u").status() == dt::basic_test::status_pending) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
