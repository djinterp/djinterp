/*******************************************************************************
* djinterp [test]                               test_runner_tests_show_route.cpp
*
*   Sections II (show/routing) and III (free functions).  The show policy
* decides which leaves and which structural lines reach a sink: all emits every
* leaf plus the summary; failures_only keeps only failing leaves; failures_and_
* skipped adds skipped; summary_only drops the per-leaf and module lines but
* keeps the summary; silent drops everything.  Routing sends each line to the
* console sink (redirected here into an ostringstream) and/or the file sink
* (streamed to output_path, or - when packing is on - buffered and packed on
* finalize, falling back to an unpacked write when the codec is unavailable).
* Finally the run_tests free functions run a tree and return the exit code.
*
*
* path:      /tests/djinterp/test/test_runner_tests_show_route.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <iostream>    // std::cout (redirected for the default-console free function)
// djinterp
#include "test_runner_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_show_all
//   all emits every leaf line plus the summary.
bool
tests_show_all()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().show           = dt::test_show::all;
    r.options().format_summary = "SUMMARY {total}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "a"));
    t.push_back(make_leaf(dt::test_status::failed, "b"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(contains(out, "1. a"))      && ok;
    ok = D_TR_CHECK(contains(out, "2. b"))      && ok;
    ok = D_TR_CHECK(contains(out, "SUMMARY 2")) && ok;

    return ok;
}

// tests_show_failures_only
//   failures_only emits only failing leaves, but still the summary.
bool
tests_show_failures_only()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().show           = dt::test_show::failures_only;
    r.options().format_summary = "SUMMARY {total}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "aaa"));
    t.push_back(make_leaf(dt::test_status::failed, "bbb"));
    t.push_back(make_leaf(dt::test_status::error,  "ccc"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(!contains(out, "aaa"))      && ok;   // passed suppressed
    ok = D_TR_CHECK(contains(out, "bbb"))       && ok;   // failed shown
    ok = D_TR_CHECK(contains(out, "ccc"))       && ok;   // error shown
    ok = D_TR_CHECK(contains(out, "SUMMARY 3")) && ok;   // summary present

    return ok;
}

// tests_show_failures_and_skipped
//   failures_and_skipped adds skipped leaves to the failing set.
bool
tests_show_failures_and_skipped()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().show = dt::test_show::failures_and_skipped;

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed,  "aaa"));
    t.push_back(make_leaf(dt::test_status::failed,  "bbb"));
    t.push_back(make_leaf(dt::test_status::skipped, "ddd"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(!contains(out, "aaa")) && ok;   // passed suppressed
    ok = D_TR_CHECK(contains(out, "bbb"))  && ok;   // failed shown
    ok = D_TR_CHECK(contains(out, "ddd"))  && ok;   // skipped shown

    return ok;
}

// tests_show_summary_only
//   summary_only suppresses leaf and module lines but keeps the summary.
bool
tests_show_summary_only()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().show           = dt::test_show::summary_only;
    r.options().format_summary = "SUMMARY {total}";

    std::vector<dt::basic_test> t;
    t.push_back(make_interior("MOD"));
    t.push_back(make_leaf(dt::test_status::passed, "aaa"));
    t.push_back(make_leaf(dt::test_status::failed, "bbb"));
    r.run(t);

    const std::string out = oss.str();
    ok = D_TR_CHECK(!contains(out, "aaa"))      && ok;   // no leaf lines
    ok = D_TR_CHECK(!contains(out, "bbb"))      && ok;
    ok = D_TR_CHECK(!contains(out, "MOD"))      && ok;   // no module header
    ok = D_TR_CHECK(contains(out, "SUMMARY 2")) && ok;   // summary present

    return ok;
}

// tests_show_silent
//   silent suppresses everything, including the summary.
bool
tests_show_silent()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().show           = dt::test_show::silent;
    r.options().format_summary = "SUMMARY {total}";

    std::vector<dt::basic_test> t;
    t.push_back(make_interior("MOD"));
    t.push_back(make_leaf(dt::test_status::passed, "aaa"));
    r.run(t);

    ok = D_TR_CHECK(oss.str().empty()) && ok;

    return ok;
}

// tests_route_file_stream
//   the file sink streams the report to output_path (pack none), and a file-only
// sink leaves the console untouched.
bool
tests_route_file_stream()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    const std::string path = runner_temp_path();
    r.options().sinks       = dt::test_sink::file;
    r.options().output_path = path;
    r.options().pack        = dt::test_output_pack::none;
    r.options().format_test = "LEAF {name}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    const std::string body = slurp(path);
    std::remove(path.c_str());

    ok = D_TR_CHECK(contains(body, "LEAF alpha")) && ok;   // rendered line in the file
    ok = D_TR_CHECK(oss.str().empty())            && ok;   // console untouched

    return ok;
}

// tests_route_console_and_file
//   both sinks receive the report.
bool
tests_route_console_and_file()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    const std::string path = runner_temp_path();
    r.options().sinks       = dt::test_sink::console | dt::test_sink::file;
    r.options().output_path = path;
    r.options().pack        = dt::test_output_pack::none;
    r.options().format_test = "LEAF {name}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    const std::string body = slurp(path);
    std::remove(path.c_str());

    ok = D_TR_CHECK(contains(oss.str(), "LEAF alpha")) && ok;   // console got it
    ok = D_TR_CHECK(contains(body, "LEAF alpha"))      && ok;   // file got it

    return ok;
}

// tests_route_no_output_path
//   a file sink with no output_path writes nothing and does not crash.
bool
tests_route_no_output_path()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);
    r.options().sinks       = dt::test_sink::file;
    r.options().output_path = "";
    r.options().format_test = "LEAF {name}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);   // must complete without a file to open

    ok = D_TR_CHECK(oss.str().empty()) && ok;   // file-only sink, no console output
    ok = D_TR_CHECK(r.exit_code() == 0) && ok;  // and the run still resolves

    return ok;
}

// tests_route_pack_compress
//   a packing run buffers the report and writes the packed file.
bool
tests_route_pack_compress()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    const std::string path = runner_temp_path();
    r.options().sinks       = dt::test_sink::file;
    r.options().output_path = path;
    r.options().pack        = dt::test_output_pack::compress;
    r.options().format_test = "LEAF {name}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    const std::string body = slurp(path);
    std::remove(path.c_str());

    ok = D_TR_CHECK(!body.empty()) && ok;   // packed (or, if unavailable, unpacked) - never lost

    return ok;
}

// tests_route_pack_archive_fallback
//   when packing fails, the report is written unpacked rather than lost.
bool
tests_route_pack_archive_fallback()
{
    bool ok = true;
    std::ostringstream oss;

    dt::test_runner r;
    r.set_console(oss);

    const std::string path = runner_temp_path();
    r.options().sinks       = dt::test_sink::file;
    r.options().output_path = path;
    r.options().pack        = dt::test_output_pack::archive;
    r.options().format_test = "LEAF {name}";

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed, "alpha"));
    r.run(t);

    const std::string body = slurp(path);
    std::remove(path.c_str());

    ok = D_TR_CHECK(!body.empty()) && ok;   // fallback still writes the report

    return ok;
}

// tests_free_run_tests
//   the run_tests free functions run a tree and return its exit code.
bool
tests_free_run_tests()
{
    bool ok = true;

    {   // explicit (silent) options -> no output, all-passed exit 0
        dt::test_option_set opts;
        opts.show = dt::test_show::silent;
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed, "a"));
        t.push_back(make_leaf(dt::test_status::passed, "b"));
        const int code = dt::run_tests(t, opts);
        ok = D_TR_CHECK(code == 0) && ok;
    }
    {   // explicit (silent) options -> a failure yields exit 1
        dt::test_option_set opts;
        opts.show = dt::test_show::silent;
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::failed, "a"));
        const int code = dt::run_tests(t, opts);
        ok = D_TR_CHECK(code == 1) && ok;
    }
    {   // default-options overload writes to std::cout; redirect it while we run
        std::ostringstream sink;
        std::streambuf* saved = std::cout.rdbuf(sink.rdbuf());
        std::vector<dt::basic_test> t;
        t.push_back(make_leaf(dt::test_status::passed, "a"));
        const int code = dt::run_tests(t);
        std::cout.rdbuf(saved);
        ok = D_TR_CHECK(code == 0) && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
