// djinterp [test] : pattern_tests_base.cpp
//   The CRTP base (section IV): the four public faces - match / operator()
// (predicate), extract, render, rewrite - exercised through the shared
// conforming tag_pattern fixture. The base forwards each face to the derived
// do_* implementation after a deferred conformance check.

// std
#include <string>
// djinterp
#include "pattern_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    using cmap = pattern_capture_map<std::string, int>;
}


/*
tests_base_match
  Verifies the match face forwards to do_match.
  Tests the following:
  - match returns true for a matching pattern and false for a non-matching one
*/
bool
tests_base_match()
{
    tag_pattern yes("k", 1, true);
    tag_pattern no ("k", 1, false);

    return (yes.match(0) && !no.match(0));
}

/*
tests_base_operator_call
  Verifies the predicate face (operator()).
  Tests the following:
  - operator() yields the same result as match (its alias)
*/
bool
tests_base_operator_call()
{
    tag_pattern yes("k", 1, true);
    tag_pattern no ("k", 1, false);

    bool ok = true;

    ok = ok && (yes(0) == yes.match(0));
    ok = ok && (no(0) == no.match(0));
    ok = ok && (yes(0) && !no(0));

    return ok;
}

/*
tests_base_extract_matched
  Verifies extract on a matching input.
  Tests the following:
  - the result is matched with Ok status
  - the configured capture is present
*/
bool
tests_base_extract_matched()
{
    tag_pattern p("key", 5, true);

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.status == DPatternStatusOk);
    ok = ok && (r.captures.find("key") != nullptr && *r.captures.find("key") == 5);

    return ok;
}

/*
tests_base_extract_unmatched
  Verifies extract on a non-matching input.
  Tests the following:
  - the result is not matched and carries no captures
*/
bool
tests_base_extract_unmatched()
{
    tag_pattern p("key", 5, false);

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (!r.matched);
    ok = ok && (r.captures.empty());

    return ok;
}

/*
tests_base_render
  Verifies the render face forwards to do_render.
  Tests the following:
  - render returns the value bound to the pattern's key
  - a missing binding yields the pattern's sentinel
*/
bool
tests_base_render()
{
    tag_pattern p("key", 5, true);

    cmap bound;
    bound.set("key", 42);

    cmap empty;

    bool ok = true;

    ok = ok && (p.render(bound) == 42);
    ok = ok && (p.render(empty) == -99);   // sentinel from tag_pattern

    return ok;
}

/*
tests_base_rewrite_matching
  Verifies rewrite replaces the value for the pattern's key.
  Tests the following:
  - rewrite on the matching key returns the new value
*/
bool
tests_base_rewrite_matching()
{
    tag_pattern p("key", 5, true);

    return (p.rewrite(0, "key", 9) == 9);
}

/*
tests_base_rewrite_nonmatching
  Verifies rewrite passes through when the key does not match.
  Tests the following:
  - rewrite on a different key returns the input unchanged
*/
bool
tests_base_rewrite_nonmatching()
{
    tag_pattern p("key", 5, true);

    return (p.rewrite(7, "other", 9) == 7);
}


NS_END  // testing
NS_END  // djinterp
