/*******************************************************************************
* djinterp [test]                                   test_runner_tests_render.cpp
*
*   Section II (rendering) - the reporter's rendered output, observed by
* redirecting the console sink into an ostringstream.  A leaf renders its format
* with {index}/{number}/{name}/{status}/{message} interpolated (and {duration}/
* {timing} resolving empty in the pure walk); the ordinal {index} advances
* across leaves; each status renders its token; an interior node renders
* format_module; the closing summary interpolates the counter tokens; a line
* longer than line_width is clamped; and a node's "tags" metadata flows through
* context_of without disturbing the line.
*
*
* path:      /tests/djinterp/test/test_runner_tests_render.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_render_leaf_line
//   a leaf renders the default format with index / name / status filled.
bool
tests_render_leaf_line()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "1. alpha [PASS]")) && ok;

    return ok;
}

// tests_render_index_increments
//   the ordinal index advances across successive leaves.
bool
tests_render_index_increments()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    t.push_back(make_leaf(dt::test_status::passed, "b"));
    t.push_back(make_leaf(dt::test_status::passed, "c"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(contains(out, "1. a")) && ok;
    ok = D_TR_CHECK(contains(out, "2. b")) && ok;
    ok = D_TR_CHECK(contains(out, "3. c")) && ok;

    return ok;
}

// tests_render_status_tokens
//   each status renders its display token.
bool
tests_render_status_tokens()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed,  "p"));
    t.push_back(make_leaf(dt::test_status::failed,  "f"));
    t.push_back(make_leaf(dt::test_status::skipped, "s"));
    t.push_back(make_leaf(dt::test_status::pending, "n"));
    t.push_back(make_leaf(dt::test_status::error,   "e"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(contains(out, "[PASS]")) && ok;
    ok = D_TR_CHECK(contains(out, "[FAIL]")) && ok;
    ok = D_TR_CHECK(contains(out, "[SKIP]")) && ok;
    ok = D_TR_CHECK(contains(out, "[PEND]")) && ok;
    ok = D_TR_CHECK(contains(out, "[ERR ]")) && ok;

    return ok;
}

// tests_render_name_and_message
//   {name} and {message} come from the node's metadata.
bool
tests_render_name_and_message()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().format_test = "{name}::{message}";

    std::vector<dt::basic_test> t;
    dt::basic_test leaf = make_leaf(dt::test_status::failed, "alpha");
    leaf.metadata().set("message", "boom");
    t.push_back(leaf);
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "alpha::boom")) && ok;

    return ok;
}

// tests_render_numbering_off
//   with numbering off {number} resolves empty, but {index} still fills.
bool
tests_render_numbering_off()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().number_tests = false;
    r.options().format_test  = "[{number}][{index}]";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "[][1]")) && ok;

    return ok;
}

// tests_render_module_header
//   an interior node renders format_module.
bool
tests_render_module_header()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().format_module = "== {name} ==";

    std::vector<dt::basic_test> t;
    t.push_back(make_interior("MyModule"));
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "== MyModule ==")) && ok;

    return ok;
}

// tests_render_summary
//   the closing summary interpolates every counter token.
bool
tests_render_summary()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().format_summary =
        "P={passed} F={failed} S={skipped} E={errors} N={pending} T={total}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed,  "a"));
    t.push_back(make_leaf(dt::test_status::passed,  "b"));
    t.push_back(make_leaf(dt::test_status::failed,  "c"));
    t.push_back(make_leaf(dt::test_status::skipped, "d"));
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "P=2 F=1 S=1 E=0 N=0 T=4")) && ok;

    return ok;
}

// tests_render_width_clamp
//   a rendered leaf line longer than line_width is clamped to it.
bool
tests_render_width_clamp()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().line_width  = static_cast<std::size_t>(5);
    r.options().format_test = "ABCDEFGHIJ";   // 10 chars, no tokens

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(contains(out, "ABCDE"))   && ok;   // clamped prefix present
    ok = D_TR_CHECK(!contains(out, "ABCDEF")) && ok;   // the 6th char was cut

    return ok;
}

// tests_render_duration_timing_empty
//   {duration} and {timing} resolve empty in the pure-walk model.
bool
tests_render_duration_timing_empty()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().format_test = "{name}|{duration}|{timing}|";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "alpha|||")) && ok;

    return ok;
}

// tests_render_tags_metadata_path
//   a node's "tags" metadata flows through context_of (which splits it) without
// disturbing the rendered line.
bool
tests_render_tags_metadata_path()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    std::vector<dt::basic_test> t;
    dt::basic_test leaf = make_leaf(dt::test_status::passed, "alpha");
    leaf.metadata().set("tags", "fast, smoke, unit");
    t.push_back(leaf);
    r.run(t);

    ok = D_TR_CHECK(contains(oss.str(), "1. alpha [PASS]")) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
