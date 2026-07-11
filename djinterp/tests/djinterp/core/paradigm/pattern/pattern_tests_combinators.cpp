// djinterp [test] : pattern_tests_combinators.cpp
//   The combinators (section VI): pattern_and, pattern_or, pattern_not and their
// factory functions. Each is exercised for its match rule, its extraction
// semantics (and-merge with right-wins collision, or-selection, not-empty), and
// its render / rewrite delegation, plus recursive nesting and factory decay.
// All are built from the shared tag_pattern fixture.

// std
#include <string>
#include <type_traits>
// djinterp
#include "pattern_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace tr = ::djinterp::traits;

namespace
{
    using cmap = pattern_capture_map<std::string, int>;
}


/*
tests_and_match
  Verifies pattern_and's match rule.
  Tests the following:
  - matches only when both sides match
*/
bool
tests_and_match()
{
    bool ok = true;

    ok = ok && ( pattern_and(tag_pattern("a", 1, true),  tag_pattern("b", 2, true)).match(0));
    ok = ok && (!pattern_and(tag_pattern("a", 1, true),  tag_pattern("b", 2, false)).match(0));
    ok = ok && (!pattern_and(tag_pattern("a", 1, false), tag_pattern("b", 2, true)).match(0));

    return ok;
}

/*
tests_and_extract_merge
  Verifies pattern_and merges both capture maps.
  Tests the following:
  - a successful and-extract carries both children's distinct captures
*/
bool
tests_and_extract_merge()
{
    auto p = pattern_and(tag_pattern("a", 1, true), tag_pattern("b", 2, true));

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.captures.size() == 2);
    ok = ok && (*r.captures.find("a") == 1);
    ok = ok && (*r.captures.find("b") == 2);

    return ok;
}

/*
tests_and_extract_collision
  Verifies right-hand keys win on collision in an and-merge.
  Tests the following:
  - when both sides bind the same key, the right-hand value survives
*/
bool
tests_and_extract_collision()
{
    auto p = pattern_and(tag_pattern("k", 1, true), tag_pattern("k", 2, true));

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.captures.size() == 1);
    ok = ok && (*r.captures.find("k") == 2);   // right wins

    return ok;
}

/*
tests_and_short_circuit
  Verifies a failing side yields no match on extract.
  Tests the following:
  - if either side fails to match, extract is unmatched with no captures
*/
bool
tests_and_short_circuit()
{
    auto left  = pattern_and(tag_pattern("a", 1, false), tag_pattern("b", 2, true));
    auto right = pattern_and(tag_pattern("a", 1, true),  tag_pattern("b", 2, false));

    auto rl = left.extract(0);
    auto rr = right.extract(0);

    bool ok = true;

    ok = ok && (!rl.matched && rl.captures.empty());
    ok = ok && (!rr.matched && rr.captures.empty());

    return ok;
}

/*
tests_and_render
  Verifies pattern_and render delegates to the right-hand pattern.
  Tests the following:
  - render resolves against the right child's key
*/
bool
tests_and_render()
{
    auto p = pattern_and(tag_pattern("a", 1, true), tag_pattern("b", 2, true));

    cmap c;
    c.set("b", 77);

    return (p.render(c) == 77);   // delegates to the right (key "b")
}

/*
tests_and_rewrite
  Verifies pattern_and rewrite delegates to the right-hand pattern.
  Tests the following:
  - rewriting the right child's key succeeds
  - rewriting the left child's key passes through (the right ignores it)
*/
bool
tests_and_rewrite()
{
    auto p = pattern_and(tag_pattern("a", 1, true), tag_pattern("b", 2, true));

    bool ok = true;

    ok = ok && (p.rewrite(0, "b", 55) == 55);   // right handles "b"
    ok = ok && (p.rewrite(3, "a", 55) == 3);    // right ignores "a" -> passthrough

    return ok;
}

/*
tests_and_introspection
  Verifies pattern_and exposes its children.
  Tests the following:
  - first() and second() reference the left and right patterns
*/
bool
tests_and_introspection()
{
    auto p = pattern_and(tag_pattern("left", 1, true), tag_pattern("right", 2, true));

    return (p.first().m_key == "left" && p.second().m_key == "right");
}

/*
tests_or_match
  Verifies pattern_or's match rule.
  Tests the following:
  - matches when either side matches; fails only when both fail
*/
bool
tests_or_match()
{
    bool ok = true;

    ok = ok && ( pattern_or(tag_pattern("a", 1, true),  tag_pattern("b", 2, false)).match(0));
    ok = ok && ( pattern_or(tag_pattern("a", 1, false), tag_pattern("b", 2, true)).match(0));
    ok = ok && ( pattern_or(tag_pattern("a", 1, true),  tag_pattern("b", 2, true)).match(0));
    ok = ok && (!pattern_or(tag_pattern("a", 1, false), tag_pattern("b", 2, false)).match(0));

    return ok;
}

/*
tests_or_extract_left
  Verifies pattern_or returns the left result when the left matches.
  Tests the following:
  - when both sides would match, the left-hand captures are the result
*/
bool
tests_or_extract_left()
{
    auto p = pattern_or(tag_pattern("a", 1, true), tag_pattern("b", 2, true));

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.captures.find("a") != nullptr);   // left wins
    ok = ok && (r.captures.find("b") == nullptr);

    return ok;
}

/*
tests_or_extract_right
  Verifies pattern_or falls back to the right result on left failure.
  Tests the following:
  - when the left fails but the right matches, the right captures are returned
*/
bool
tests_or_extract_right()
{
    auto p = pattern_or(tag_pattern("a", 1, false), tag_pattern("b", 2, true));

    auto r = p.extract(0);

    bool ok = true;

    ok = ok && (r.matched);
    ok = ok && (r.captures.find("b") != nullptr);
    ok = ok && (r.captures.find("a") == nullptr);

    return ok;
}

/*
tests_or_render
  Verifies pattern_or render delegates to the left-hand pattern.
  Tests the following:
  - render resolves against the left child's key
*/
bool
tests_or_render()
{
    auto p = pattern_or(tag_pattern("a", 1, true), tag_pattern("b", 2, true));

    cmap c;
    c.set("a", 88);

    return (p.render(c) == 88);   // delegates to the left (key "a")
}

/*
tests_or_rewrite
  Verifies pattern_or rewrite delegates to whichever side matches.
  Tests the following:
  - when only the right matches, rewrite uses the right child
  - when the left matches, rewrite uses the left child
*/
bool
tests_or_rewrite()
{
    auto right_only = pattern_or(tag_pattern("a", 1, false), tag_pattern("b", 2, true));
    auto left_match = pattern_or(tag_pattern("a", 1, true),  tag_pattern("b", 2, true));

    bool ok = true;

    ok = ok && (right_only.rewrite(0, "b", 33) == 33);   // right side matches -> uses "b"
    ok = ok && (left_match.rewrite(0, "a", 44) == 44);   // left side matches -> uses "a"

    return ok;
}

/*
tests_not_match
  Verifies pattern_not negates the wrapped predicate.
  Tests the following:
  - not(matching) fails to match; not(non-matching) matches
*/
bool
tests_not_match()
{
    bool ok = true;

    ok = ok && (!pattern_not(tag_pattern("a", 1, true)).match(0));
    ok = ok && ( pattern_not(tag_pattern("a", 1, false)).match(0));

    return ok;
}

/*
tests_not_extract
  Verifies pattern_not extraction semantics.
  Tests the following:
  - when the inner matches, extract is unmatched with NoMatch status
  - when the inner does not match, extract succeeds with an empty capture map
*/
bool
tests_not_extract()
{
    auto inner_matches = pattern_not(tag_pattern("a", 1, true));
    auto inner_fails   = pattern_not(tag_pattern("a", 1, false));

    auto rm = inner_matches.extract(0);
    auto rf = inner_fails.extract(0);

    bool ok = true;

    ok = ok && (!rm.matched && rm.status == DPatternStatusNoMatch);
    ok = ok && (rf.matched && rf.captures.empty());

    return ok;
}

/*
tests_not_render
  Verifies pattern_not render returns a default input.
  Tests the following:
  - render ignores the captures and yields a value-initialized input
*/
bool
tests_not_render()
{
    auto n = pattern_not(tag_pattern("a", 1, true));

    cmap c;
    c.set("a", 5);

    return (n.render(c) == 0);   // default-constructed int
}

/*
tests_not_rewrite
  Verifies pattern_not rewrite returns the input unchanged.
  Tests the following:
  - rewrite passes the input through regardless of key or value
*/
bool
tests_not_rewrite()
{
    auto n = pattern_not(tag_pattern("a", 1, true));

    return (n.rewrite(123, "a", 9) == 123);
}

/*
tests_not_introspection
  Verifies pattern_not exposes the wrapped pattern.
  Tests the following:
  - inner() references the wrapped pattern
*/
bool
tests_not_introspection()
{
    auto n = pattern_not(tag_pattern("wrapped", 1, true));

    return (n.inner().m_key == "wrapped");
}

/*
tests_combinator_nesting
  Verifies combinators compose recursively.
  Tests the following:
  - and(or(...), not(...)) evaluates its sub-combinators correctly
  - the nested combinator is itself a conforming pattern
*/
bool
tests_combinator_nesting()
{
    auto nested = pattern_and(
        pattern_or(tag_pattern("a", 1, false), tag_pattern("a", 1, true)),   // matches (right)
        pattern_not(tag_pattern("z", 0, false)));                            // matches (inner fails)

    bool ok = true;

    ok = ok && (nested.match(0));
    ok = ok && (tr::is_pattern_v<decltype(nested)>);

    // a nested non-match: not(matching) inside an and -> whole fails
    auto nested_fail = pattern_and(
        tag_pattern("a", 1, true),
        pattern_not(tag_pattern("z", 0, true)));                             // inner matches -> not fails
    ok = ok && (!nested_fail.match(0));

    return ok;
}

/*
tests_combinator_factory_decay
  Verifies the combinator factories decay their forwarded pattern arguments.
  Tests the following:
  - passing lvalue patterns yields a combinator over the decayed value types
  - the resulting combinator behaves identically to one built from temporaries
*/
bool
tests_combinator_factory_decay()
{
    tag_pattern a("a", 1, true);
    tag_pattern b("b", 2, true);

    auto p = pattern_and(a, b);   // lvalues -> decayed to value members

    bool ok = true;

    ok = ok && (std::is_same<decltype(p),
                             pattern_and_combinator<tag_pattern, tag_pattern> >::value);
    ok = ok && (p.match(0));
    ok = ok && (p.extract(0).captures.size() == 2);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
