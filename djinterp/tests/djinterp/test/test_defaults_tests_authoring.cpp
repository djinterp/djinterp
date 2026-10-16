/*******************************************************************************
* djinterp [test]                              test_defaults_tests_authoring.cpp
*
*   The authoring aggregates and the small pure helpers around them.  test_spec
* / block_spec / module_spec are the plain-data description a run is authored
* as - a name, a descriptor, and a nullary predicate at the leaf - and this
* section confirms the nesting holds and the stored predicate is callable.
* status_for maps a boolean to passed/failed; unit_test_tally rolls up
* total/passed/failed and computes a pass_rate on [0,100] (0 on an empty
* tally); status_word maps each status code to its label, with any out-of-range
* code falling to "error".
*
*
* path:      /tests/djinterp/test/test_defaults_tests_authoring.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_defaults_tests.hpp"

NS_DJINTERP
NS_TESTING

namespace
{
    // file-scope predicates for the authoring aggregates' test_fn leaves.
    bool spec_true()  { return true; }
    bool spec_false() { return false; }
}


// tests_authoring_spec_shape
//   test_spec / block_spec / module_spec nest as plain data, and the stored
// predicate runs.
bool
tests_authoring_spec_shape()
{
    bool ok = true;

    dt::test_spec ts;
    ts.name       = "unit";
    ts.descriptor = "checks the thing";
    ts.fn         = &spec_true;
    ok = D_TD_CHECK(std::string(ts.name) == "unit")             && ok;
    ok = D_TD_CHECK(std::string(ts.descriptor) == "checks the thing") && ok;
    ok = D_TD_CHECK(ts.fn != nullptr && ts.fn() == true)        && ok;

    dt::test_spec  tf; tf.name = "u2"; tf.descriptor = "d2"; tf.fn = &spec_false;
    dt::block_spec bs;
    bs.name       = "block";
    bs.descriptor = "a section";
    bs.tests.push_back(ts);
    bs.tests.push_back(tf);
    ok = D_TD_CHECK(bs.tests.size() == 2)          && ok;
    ok = D_TD_CHECK(bs.tests[1].fn() == false)     && ok;

    dt::module_spec ms;
    ms.name       = "module";
    ms.descriptor = "one header";
    ms.blocks.push_back(bs);
    ok = D_TD_CHECK(ms.blocks.size() == 1)                        && ok;
    ok = D_TD_CHECK(ms.blocks[0].tests.size() == 2)               && ok;
    ok = D_TD_CHECK(ms.blocks[0].tests[0].fn() == true)           && ok;
    ok = D_TD_CHECK(std::string(ms.blocks[0].name) == "block")    && ok;

    return ok;
}

// tests_authoring_status_for
//   status_for maps true -> passed, false -> failed.
bool
tests_authoring_status_for()
{
    bool ok = true;

    ok = D_TD_CHECK(dt::status_for(true)  == dt::test_status::passed) && ok;
    ok = D_TD_CHECK(dt::status_for(false) == dt::test_status::failed) && ok;

    return ok;
}

// tests_authoring_tally_default
//   a default tally is all zero and reports a 0 pass rate.
bool
tests_authoring_tally_default()
{
    bool ok = true;
    dt::unit_test_tally t;

    ok = D_TD_CHECK(t.total  == static_cast<std::size_t>(0)) && ok;
    ok = D_TD_CHECK(t.passed == static_cast<std::size_t>(0)) && ok;
    ok = D_TD_CHECK(t.failed == static_cast<std::size_t>(0)) && ok;
    ok = D_TD_CHECK(double_eq(t.pass_rate(), 0.0))           && ok;   // empty -> 0, not NaN

    return ok;
}

// tests_authoring_tally_pass_rate
//   pass_rate is 100 * passed / total on [0,100].
bool
tests_authoring_tally_pass_rate()
{
    bool ok = true;

    dt::unit_test_tally a; a.total = 4; a.passed = 3; a.failed = 1;
    ok = D_TD_CHECK(double_eq(a.pass_rate(), 75.0)) && ok;

    dt::unit_test_tally b; b.total = 5; b.passed = 5; b.failed = 0;
    ok = D_TD_CHECK(double_eq(b.pass_rate(), 100.0)) && ok;

    dt::unit_test_tally c; c.total = 4; c.passed = 1; c.failed = 3;
    ok = D_TD_CHECK(double_eq(c.pass_rate(), 25.0)) && ok;

    dt::unit_test_tally d; d.total = 4; d.passed = 0; d.failed = 4;
    ok = D_TD_CHECK(double_eq(d.pass_rate(), 0.0)) && ok;

    return ok;
}

// tests_authoring_status_word
//   status_word labels each status code; an unknown code falls to "error".
bool
tests_authoring_status_word()
{
    bool ok = true;

    ok = D_TD_CHECK(std::string(dt::status_word(dt::basic_test::status_passed))  == "passed")  && ok;
    ok = D_TD_CHECK(std::string(dt::status_word(dt::basic_test::status_failed))  == "failed")  && ok;
    ok = D_TD_CHECK(std::string(dt::status_word(dt::basic_test::status_skipped)) == "skipped") && ok;
    ok = D_TD_CHECK(std::string(dt::status_word(dt::basic_test::status_pending)) == "pending") && ok;
    ok = D_TD_CHECK(std::string(dt::status_word(dt::basic_test::status_error))   == "error")   && ok;
    // any out-of-range code also falls to the default label
    ok = D_TD_CHECK(std::string(dt::status_word(
        static_cast<dt::basic_test::status_type>(99))) == "error") && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
