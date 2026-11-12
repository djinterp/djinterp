/*******************************************************************************
* djinterp [cpp][test]                              test_options_tests_enums.cpp
*
*   Section A.I - the value enums and, chiefly, DTestSink's bitwise algebra: the
* union (operator|), intersection (operator&), complement within the defined
* bits (operator~), in-place union (operator|=), and the contains-all query
* test_sink_has (which is false for the empty bit and false when the queried set
* is a strict subset of the requested bits).  The remaining enums are plain value
* lists; the sink bit values are checked for the powers-of-two contract the
* algebra relies on.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_options_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_enums_sink_or
//   operator| unions sink sets; none is the identity; union is idempotent.
bool tests_enums_sink_or()
{
    bool ok = true;

    const dt::DTestSink cf = dt::DTestSink::console | dt::DTestSink::file;
    ok = D_TO_CHECK(static_cast<unsigned>(cf) == 3u) && ok;
    ok = D_TO_CHECK((dt::DTestSink::console | dt::DTestSink::none) == dt::DTestSink::console) && ok;
    ok = D_TO_CHECK((dt::DTestSink::file    | dt::DTestSink::file) == dt::DTestSink::file)    && ok;

    return ok;
}

// tests_enums_sink_and
//   operator& intersects; disjoint sets meet in none.
bool tests_enums_sink_and()
{
    bool ok = true;

    const dt::DTestSink cf = dt::DTestSink::console | dt::DTestSink::file;
    ok = D_TO_CHECK((cf & dt::DTestSink::console) == dt::DTestSink::console) && ok;
    ok = D_TO_CHECK((cf & dt::DTestSink::file)    == dt::DTestSink::file)    && ok;
    ok = D_TO_CHECK((dt::DTestSink::console & dt::DTestSink::file) == dt::DTestSink::none) && ok;

    return ok;
}

// tests_enums_sink_not
//   operator~ complements within {console, file}.
bool tests_enums_sink_not()
{
    bool ok = true;

    ok = D_TO_CHECK((~dt::DTestSink::console) == dt::DTestSink::file)    && ok;
    ok = D_TO_CHECK((~dt::DTestSink::file)    == dt::DTestSink::console) && ok;
    ok = D_TO_CHECK((~dt::DTestSink::none) == (dt::DTestSink::console | dt::DTestSink::file)) && ok;
    ok = D_TO_CHECK((~(dt::DTestSink::console | dt::DTestSink::file)) == dt::DTestSink::none) && ok;

    return ok;
}

// tests_enums_sink_or_assign
//   operator|= unions in place and accumulates.
bool tests_enums_sink_or_assign()
{
    bool ok = true;

    dt::DTestSink s = dt::DTestSink::console;
    s |= dt::DTestSink::file;
    ok = D_TO_CHECK(s == (dt::DTestSink::console | dt::DTestSink::file)) && ok;

    dt::DTestSink t = dt::DTestSink::none;
    t |= dt::DTestSink::console;
    t |= dt::DTestSink::file;
    ok = D_TO_CHECK(dt::test_sink_has(t, dt::DTestSink::console) &&
                    dt::test_sink_has(t, dt::DTestSink::file)) && ok;

    return ok;
}

// tests_enums_sink_has
//   test_sink_has is contains-all; the empty bit is never contained; a strict
// subset does not contain a larger request.
bool tests_enums_sink_has()
{
    bool ok = true;

    const dt::DTestSink cf = dt::DTestSink::console | dt::DTestSink::file;
    ok = D_TO_CHECK( dt::test_sink_has(cf, dt::DTestSink::console)) && ok;
    ok = D_TO_CHECK( dt::test_sink_has(cf, dt::DTestSink::file))    && ok;
    ok = D_TO_CHECK( dt::test_sink_has(dt::DTestSink::console, dt::DTestSink::console)) && ok;
    ok = D_TO_CHECK(!dt::test_sink_has(dt::DTestSink::console, dt::DTestSink::file))    && ok;
    ok = D_TO_CHECK(!dt::test_sink_has(dt::DTestSink::console, cf))                     && ok;  // subset
    ok = D_TO_CHECK(!dt::test_sink_has(cf, dt::DTestSink::none))                        && ok;  // empty bit
    ok = D_TO_CHECK(!dt::test_sink_has(dt::DTestSink::none, dt::DTestSink::none))       && ok;

    return ok;
}

// tests_enums_sink_values
//   the sink bits are the powers of two the algebra relies on.
bool tests_enums_sink_values()
{
    bool ok = true;

    ok = D_TO_CHECK(static_cast<unsigned>(dt::DTestSink::none)    == 0u) && ok;
    ok = D_TO_CHECK(static_cast<unsigned>(dt::DTestSink::console) == 1u) && ok;
    ok = D_TO_CHECK(static_cast<unsigned>(dt::DTestSink::file)    == 2u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
