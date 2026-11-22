/*******************************************************************************
* djinterp [cpp][text]                               text_core_tests_pattern.cpp
*
*   The groups for text_pattern.hpp: compiling a literal into segments, the
* escape, the four faces (predicate, extractor, renderer, rewrite), find,
* and composition by concat and the factories. Captures are lazy, and the
* groups hold the pattern to that: a capture stops at the first occurrence
* of the literal after it.
*
*
* path:      /tests/djinterp/cpp/text/text_core_tests_pattern.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "text_core_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // NULL, std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/text_pattern.hpp"  // DTextPattern


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// text_pattern, result, bindings
//   the pattern under test, its extractor's result and its capture map.
typedef djinterp::DTextPattern                    text_pattern;
typedef djinterp::DTextPattern::match_result_type result;
typedef djinterp::DTextPattern::capture_map_type  bindings;


/*
d_tests_text_core_capture
  A capture's value in a result, or "(none)" when the result holds no
capture of that name.
*/
static std::string
d_tests_text_core_capture(
    const result& _result,
    const char*   _key
)
{
    const std::string* value = _result.captures.find(_key);

    return (value != NULL) ? *value : std::string("(none)");
}


/*
d_tests_text_core_pattern_compile
  Tests the following:
  - "revised: %year%.%month%.%day%" compiles to six segments, its keys in
    order, with its literal, the default markers and escape kept
  - DTextPattern meets the pattern protocol (is_pattern)
  - the empty pattern is well formed, and matches only ""
  - adjacent captures are malformed, and a malformed pattern matches
    nothing and says so
  - a prefix with no suffix after it is literal
  - set_markers recompiles; an empty marker is malformed, and good markers
    make the pattern well formed again
*/
void
d_tests_text_core_pattern_compile(void)
{
    const text_pattern             date("revised: %year%.%month%.%day%");
    const std::vector<std::string> keys = date.capture_keys();
    const text_pattern             none;
    const text_pattern             adjacent("%a%%b%");
    const text_pattern             unterminated("50% off");
    text_pattern                   angled("<a>-<b>");
    text_pattern                   unmarked("a%b%");

    D_TESTS_TEXT_CHECK(date.is_compiled_ok());
    D_TESTS_TEXT_CHECK(date.compile_status() ==
                       djinterp::DPatternStatusOk);
    D_TESTS_TEXT_CHECK(date.segment_count() == 6);
    D_TESTS_TEXT_CHECK((keys.size() == 3) && (keys[0] == "year") &&
                       (keys[1] == "month") && (keys[2] == "day"));
    D_TESTS_TEXT_CHECK(date.literal() == "revised: %year%.%month%.%day%");
    D_TESTS_TEXT_CHECK((date.prefix() == "%") && (date.suffix() == "%"));
    D_TESTS_TEXT_CHECK(date.escape() == '\\');
    D_TESTS_TEXT_CHECK((djinterp::is_pattern<text_pattern>::value));

    D_TESTS_TEXT_CHECK(none.is_compiled_ok() &&
                       (none.segment_count() == 0));
    D_TESTS_TEXT_CHECK(none("") && !none("x"));

    D_TESTS_TEXT_CHECK(adjacent.compile_status() ==
                       djinterp::DPatternStatusMalformed);
    D_TESTS_TEXT_CHECK(!adjacent.is_compiled_ok() && !adjacent("xy"));
    D_TESTS_TEXT_CHECK(adjacent.extract("xy").status ==
                       djinterp::DPatternStatusMalformed);

    D_TESTS_TEXT_CHECK((unterminated.segment_count() == 1) &&
                       unterminated.capture_keys().empty());
    D_TESTS_TEXT_CHECK(unterminated("50% off"));

    D_TESTS_TEXT_CHECK(angled.segment_count() == 1);
    angled.set_markers("<", ">");
    D_TESTS_TEXT_CHECK((angled.segment_count() == 3) &&
                       (angled.capture_keys().size() == 2));
    D_TESTS_TEXT_CHECK((angled.prefix() == "<") &&
                       (angled.suffix() == ">"));
    D_TESTS_TEXT_CHECK(angled("1-2"));

    unmarked.set_markers("", "%");
    D_TESTS_TEXT_CHECK(unmarked.compile_status() ==
                       djinterp::DPatternStatusMalformed);
    unmarked.set_markers("%", "%");
    D_TESTS_TEXT_CHECK(unmarked.is_compiled_ok() &&
                       (unmarked.segment_count() == 2));

    return;
}


/*
d_tests_text_core_pattern_escape
  Tests the following:
  - an escaped prefix is a literal prefix: "100\%: %v%" has one capture and
    matches "100%: x"
  - the escape escapes only a prefix; before anything else it is an
    ordinary character
  - set_escape changes the escape and recompiles
  - '\0' turns escaping off: a backslash before a prefix is then literal,
    and the prefix opens a capture
  - the four-argument constructor compiles with its escape at once
*/
void
d_tests_text_core_pattern_escape(void)
{
    const text_pattern percent("100\\%: %v%");
    const text_pattern backslash("a\\b%v%");
    const text_pattern four("#%v%", "%", "%", '#');
    text_pattern       hash("#%v%");
    text_pattern       off("\\%v%");

    D_TESTS_TEXT_CHECK((percent.segment_count() == 2) &&
                       (percent.capture_keys().size() == 1));
    D_TESTS_TEXT_CHECK(percent("100%: x"));
    D_TESTS_TEXT_CHECK(
        d_tests_text_core_capture(percent.extract("100%: x"), "v") == "x");

    D_TESTS_TEXT_CHECK(backslash.segment_count() == 2);
    D_TESTS_TEXT_CHECK(backslash("a\\bz"));

    D_TESTS_TEXT_CHECK(hash.segment_count() == 2);
    hash.set_escape('#');
    D_TESTS_TEXT_CHECK((hash.escape() == '#') &&
                       (hash.segment_count() == 1));
    D_TESTS_TEXT_CHECK(hash("%v%"));

    D_TESTS_TEXT_CHECK(off.segment_count() == 1);
    off.set_escape('\0');
    D_TESTS_TEXT_CHECK(off.segment_count() == 2);
    D_TESTS_TEXT_CHECK(
        d_tests_text_core_capture(off.extract("\\x"), "v") == "x");

    D_TESTS_TEXT_CHECK((four.escape() == '#') &&
                       (four.segment_count() == 1) && four("%v%"));

    return;
}


/*
d_tests_text_core_pattern_match
  Tests the following:
  - the predicate face, as operator() and as match()
  - the extractor face: matched, ok, and the three captures
  - the whole input must match: neither text before the pattern nor a
    wrong literal passes
  - a literal at the end must end the input; a capture at the end takes
    the rest
  - captures are lazy: "%dir%/%file%" splits "a/b/c" at its first '/', and
    "%a%-%b%." matches "x-y." but not "x-y.z."
  - a failed match: not matched, no match, no captures
*/
void
d_tests_text_core_pattern_match(void)
{
    const text_pattern date("revised: %year%.%month%.%day%");
    const result       read   = date.extract("revised: 2026.10.02");
    const result       tail   = date.extract("revised: 2026.10.02 UTC");
    const result       failed = date.extract("nope");
    const text_pattern tag("<%tag%>");
    const text_pattern path("%dir%/%file%");
    const result       split = path.extract("a/b/c");
    const text_pattern late("%a%-%b%.");

    D_TESTS_TEXT_CHECK(date("revised: 2026.10.02") &&
                       date.match("revised: 2026.10.02"));

    D_TESTS_TEXT_CHECK(read.matched && static_cast<bool>(read));
    D_TESTS_TEXT_CHECK(read.status == djinterp::DPatternStatusOk);
    D_TESTS_TEXT_CHECK(read.captures.size() == 3);
    D_TESTS_TEXT_CHECK(
        (d_tests_text_core_capture(read, "year") == "2026") &&
        (d_tests_text_core_capture(read, "month") == "10") &&
        (d_tests_text_core_capture(read, "day") == "02"));

    D_TESTS_TEXT_CHECK(!date("x revised: 2026.10.02"));
    D_TESTS_TEXT_CHECK(!date("revised: 2026-10-02"));

    D_TESTS_TEXT_CHECK(tag("<b>") && !tag("<b>x"));
    D_TESTS_TEXT_CHECK(tail.matched &&
        (d_tests_text_core_capture(tail, "day") == "02 UTC"));

    D_TESTS_TEXT_CHECK((d_tests_text_core_capture(split, "dir") == "a") &&
        (d_tests_text_core_capture(split, "file") == "b/c"));
    D_TESTS_TEXT_CHECK(late("x-y.") && !late("x-y.z."));

    D_TESTS_TEXT_CHECK(!failed.matched && !static_cast<bool>(failed));
    D_TESTS_TEXT_CHECK(failed.status == djinterp::DPatternStatusNoMatch);
    D_TESTS_TEXT_CHECK(failed.captures.empty());

    return;
}


/*
d_tests_text_core_pattern_find
  Tests the following:
  - find returns the first occurrence, with its position and captures
  - a pattern that starts with a capture is found at the start
  - with no occurrence, find returns false and its result says no match
  - a malformed pattern finds nothing, and its result says why
*/
void
d_tests_text_core_pattern_find(void)
{
    const text_pattern pair_of("[%k%=%v%]");
    const text_pattern leading("%k%=%v%;");
    const text_pattern adjacent("%a%%b%");
    std::size_t        position = 99;
    result             found;

    D_TESTS_TEXT_CHECK(pair_of.find("xx [a=1] yy", position, found));
    D_TESTS_TEXT_CHECK((position == 3) && found.matched);
    D_TESTS_TEXT_CHECK((d_tests_text_core_capture(found, "k") == "a") &&
                       (d_tests_text_core_capture(found, "v") == "1"));
    D_TESTS_TEXT_CHECK(pair_of.find("[a=1][b=2]", position, found) &&
                       (position == 0) &&
                       (d_tests_text_core_capture(found, "k") == "a"));

    D_TESTS_TEXT_CHECK(leading.find("a=1;b=2;", position, found) &&
                       (position == 0) &&
                       (d_tests_text_core_capture(found, "v") == "1"));

    D_TESTS_TEXT_CHECK(!pair_of.find("[a=1", position, found));
    D_TESTS_TEXT_CHECK(!found.matched &&
                       (found.status == djinterp::DPatternStatusNoMatch));

    D_TESTS_TEXT_CHECK(!adjacent.find("xy", position, found) &&
        (found.status == djinterp::DPatternStatusMalformed));

    return;
}


/*
d_tests_text_core_pattern_render
  Tests the following:
  - the renderer face puts bound values in place, and an unbound capture
    renders empty
  - an escaped prefix renders as the prefix
  - render inverts extract
  - rewrite replaces one capture and keeps the others
  - input that doesn't match comes back unchanged, and a key the pattern
    lacks changes nothing it renders
*/
void
d_tests_text_core_pattern_render(void)
{
    const text_pattern date("revised: %year%.%month%.%day%");
    const text_pattern percent("100\\%: %v%");
    bindings           full;
    bindings           partial;
    bindings           answer;

    full.set("year", "2026").set("month", "11").set("day", "13");
    partial.set("year", "2026");
    answer.set("v", "yes");

    D_TESTS_TEXT_CHECK(date.render(full) == "revised: 2026.11.13");
    D_TESTS_TEXT_CHECK(date.render(partial) == "revised: 2026..");
    D_TESTS_TEXT_CHECK(percent.render(answer) == "100%: yes");
    D_TESTS_TEXT_CHECK(
        date.render(date.extract("revised: 2026.10.02").captures) ==
        "revised: 2026.10.02");

    D_TESTS_TEXT_CHECK(
        date.rewrite("revised: 2026.10.02", "day", "03") ==
        "revised: 2026.10.03");
    D_TESTS_TEXT_CHECK(
        date.rewrite("revised: 2026.10.02", "year", "2027") ==
        "revised: 2027.10.02");
    D_TESTS_TEXT_CHECK(
        date.rewrite("created: 2026.10.02", "day", "03") ==
        "created: 2026.10.02");
    D_TESTS_TEXT_CHECK(
        date.rewrite("revised: 2026.10.02", "hour", "9") ==
        "revised: 2026.10.02");

    return;
}


/*
d_tests_text_core_pattern_compose
  Tests the following:
  - concat joins two literals into one pattern
  - concat keeps the left's markers and escape: "#<x>" under '#' stays the
    literal "<x>" when joined to "<v>"
  - two patterns that meet capture to capture concatenate to a malformed
    one
  - the factories' markers: "%", "{" "}", "${" "}" and "{{" "}}"
*/
void
d_tests_text_core_pattern_compose(void)
{
    const text_pattern head("revised: %y%.");
    const text_pattern tail("%m%");
    const text_pattern both     = djinterp::concat(head, tail);
    const text_pattern left("#<x>", "<", ">", '#');
    const text_pattern right("<v>", "<", ">");
    const text_pattern joined   = djinterp::concat(left, right);
    const text_pattern touching = djinterp::concat(
                                      djinterp::make_pattern("%a%"),
                                      djinterp::make_pattern("%b%"));
    const text_pattern plain    = djinterp::make_pattern("%a%");
    const text_pattern curly    = djinterp::make_curly_pattern("{a}-{b}");
    const text_pattern dollar   = djinterp::make_dollar_pattern("${a}!");
    const text_pattern mustache = djinterp::make_mustache_pattern("{{a}}.");

    D_TESTS_TEXT_CHECK(both.literal() == "revised: %y%.%m%");
    D_TESTS_TEXT_CHECK(both("revised: 2026.11") &&
                       (both.capture_keys().size() == 2));

    D_TESTS_TEXT_CHECK((joined.prefix() == "<") &&
                       (joined.escape() == '#'));
    D_TESTS_TEXT_CHECK(joined.is_compiled_ok() &&
                       (joined.capture_keys().size() == 1));
    D_TESTS_TEXT_CHECK(joined("<x>7"));

    D_TESTS_TEXT_CHECK(touching.compile_status() ==
                       djinterp::DPatternStatusMalformed);

    D_TESTS_TEXT_CHECK((plain.prefix() == "%") &&
                       (plain.suffix() == "%"));
    D_TESTS_TEXT_CHECK((curly.prefix() == "{") &&
                       (curly.suffix() == "}") && curly("1-2"));
    D_TESTS_TEXT_CHECK((dollar.prefix() == "${") &&
        (d_tests_text_core_capture(dollar.extract("x!"), "a") == "x"));
    D_TESTS_TEXT_CHECK((mustache.prefix() == "{{") &&
                       (mustache.suffix() == "}}") &&
        (d_tests_text_core_capture(mustache.extract("y."), "a") == "y"));

    return;
}

#endif  // C++11
