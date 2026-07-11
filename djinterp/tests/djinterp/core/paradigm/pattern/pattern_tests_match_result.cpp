// djinterp [test] : pattern_tests_match_result.cpp
//   The match result (section III): pattern_match_result carries the matched
// flag, a status code, and the capture map. Covers its three constructors
// (default, success, failure) and the explicit bool conversion.

// std
#include <string>
#include <utility>
// djinterp
#include "pattern_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    using cmap = pattern_capture_map<std::string, int>;
    using mres = pattern_match_result<std::string, int>;
}


/*
tests_match_result_default
  Verifies the default-constructed result.
  Tests the following:
  - matched is false, status is NoMatch, captures are empty
*/
bool
tests_match_result_default()
{
    mres r;

    bool ok = true;

    ok = ok && (!r.matched);
    ok = ok && (r.status == DPatternStatusNoMatch);
    ok = ok && (r.captures.empty());

    return ok;
}

/*
tests_match_result_success
  Verifies the capture-map (success) constructor.
  Tests the following:
  - matched is true and status is Ok
*/
bool
tests_match_result_success()
{
    cmap c;
    c.set("k", 1);

    mres r(std::move(c));

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.status == DPatternStatusOk);

    return ok;
}

/*
tests_match_result_failure
  Verifies the status (failure) constructor.
  Tests the following:
  - matched is false and the supplied status code is stored
  - captures are empty
*/
bool
tests_match_result_failure()
{
    mres r(DPatternStatusMalformed);

    bool ok = true;

    ok = ok && (!r.matched);
    ok = ok && (r.status == DPatternStatusMalformed);
    ok = ok && (r.captures.empty());

    return ok;
}

/*
tests_match_result_operator_bool
  Verifies the explicit bool conversion.
  Tests the following:
  - a matched result converts to true
  - an unmatched result converts to false
*/
bool
tests_match_result_operator_bool()
{
    mres good(cmap{});
    mres bad(DPatternStatusNoMatch);

    bool ok = true;

    ok = ok && (static_cast<bool>(good));
    ok = ok && (!static_cast<bool>(bad));

    // usable directly in a boolean context
    if (good) { ok = ok && true; } else { ok = false; }
    if (bad)  { ok = false; }

    return ok;
}

/*
tests_match_result_captures
  Verifies the capture map is carried by a success result.
  Tests the following:
  - the bindings placed in the success map are retrievable from the result
*/
bool
tests_match_result_captures()
{
    cmap c;
    c.set("a", 11).set("b", 22);

    mres r(std::move(c));

    bool ok = true;

    ok = ok && (r.captures.size() == 2);
    ok = ok && (*r.captures.find("a") == 11);
    ok = ok && (*r.captures.find("b") == 22);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
