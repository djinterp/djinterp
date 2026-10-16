/*******************************************************************************
* djinterp [test]                                          test_runner_tests.hpp
*
*   Declarations for the unit-test suite covering test_runner.hpp - the top-level
* RUN facade that composes a test_handler, a test_option_set, and an
* option-driven reporter bound to the event alphabet.  Each free function
* exercises one facet and returns true if every check inside it passed.  Tests
* are grouped into translation units by the section of the header they cover:
*
*   - test_runner_tests_helpers.cpp    -> I.   the internal reporter helpers
*                                            (status map / token, show policy,
*                                            substitution, tag splitting)
*   - test_runner_tests_facade.cpp     -> II.  construction, options / set_options,
*                                            set_console, handler(), and the
*                                            result / verdict / exit_code / counter
*                                            surface driven by run()
*   - test_runner_tests_render.cpp     -> II.  the reporter's rendered output:
*                                            per-leaf lines, module headers, the
*                                            summary, token interpolation, width
*                                            clamping, numbering
*   - test_runner_tests_show_route.cpp -> II/III. show policies, sink routing
*                                            (console / file / both / packing),
*                                            and the run_tests free functions
*
*   The lone shared check helper, test_runner_check, reports a failing check
* (with its stringized expression and source location) and forwards the boolean.
* The D_TR_CHECK macro is the intended call site.
*
*   OBSERVATION.  The internal helpers are pure and are called directly.  The
* runner's rendered output is observed by redirecting its console sink into an
* ostringstream (set_console) and reading the captured text; file output is
* observed by pointing output_path at a scratch file and reading it back.
* Counters / verdict / exit_code are read from the facade after run().  Nodes
* are built with make_leaf / make_interior (basic_test values whose type_id
* selects leaf vs interior and whose "name" metadata drives rendering).
*
*   NOTE: the runner lives in djinterp::test; the event vocabulary (verdict,
* handler_id) lives in djinterp; the tests live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/test/test_runner_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_RUNNER_TESTS_
#define DJINTERP_TEST_RUNNER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first, then the header under test.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_runner.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the runner surface (djinterp::test); the event vocabulary
// (verdict, handler_id) resolves unqualified from djinterp.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures + check helper


// =========================================================================
//  shared check helper
// =========================================================================

// test_runner_check
//   function: reports the result of a single check.  When _condition is false,
// prints the stringized expression with its source location to stdout; always
// returns _condition so the caller can fold it into a running result.
inline bool
test_runner_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  node fixtures
// =========================================================================

// make_leaf
//   fixture: a leaf node (type_id 0) with the requested status and a "name"
// metadata entry.
inline dt::basic_test
make_leaf(dt::test_status _status, const std::string& _name)
{
    dt::basic_test _n(dt::test_type_id{0});
    switch (_status)
    {
        case dt::test_status::passed:  _n.set_status(dt::basic_test::status_passed);  break;
        case dt::test_status::failed:  _n.set_status(dt::basic_test::status_failed);  break;
        case dt::test_status::skipped: _n.set_status(dt::basic_test::status_skipped); break;
        case dt::test_status::pending: _n.set_status(dt::basic_test::status_pending); break;
        case dt::test_status::error:   _n.set_status(dt::basic_test::status_error);   break;
    }
    _n.metadata().set("name", _name);
    return _n;
}

// make_interior
//   fixture: an interior node (type_id 2) with a "name" metadata entry.
inline dt::basic_test
make_interior(const std::string& _name)
{
    dt::basic_test _n(dt::test_type_id{2});
    _n.metadata().set("name", _name);
    return _n;
}


// =========================================================================
//  scratch-file fixtures
// =========================================================================

// runner_temp_path
//   fixture: a unique path in the system temp directory (not created; the
// runner opens it).  Unique per call within a process.
inline std::string
runner_temp_path()
{
    static unsigned s_counter = 0u;
    std::ostringstream _os;
    const char* _tmp =
    #if defined(_WIN32)
        ".";
    #else
        "/tmp";
    #endif
    _os << _tmp << "/dtest_runner_" << (++s_counter) << "_"
        << static_cast<long>(std::clock()) << ".out";
    return _os.str();
}

// slurp
//   fixture: read a file's entire contents into a string (empty if absent).
inline std::string
slurp(const std::string& _path)
{
    std::ifstream _in(_path.c_str(), std::ios::in | std::ios::binary);
    if (!_in.is_open()) { return std::string(); }
    std::ostringstream _ss;
    _ss << _in.rdbuf();
    return _ss.str();
}

// contains
//   fixture: does _hay contain _needle?
inline bool
contains(const std::string& _hay, const std::string& _needle)
{
    return _hay.find(_needle) != std::string::npos;
}

// count_occurrences
//   fixture: how many (non-overlapping) times _needle occurs in _hay.
inline std::size_t
count_occurrences(const std::string& _hay, const std::string& _needle)
{
    if (_needle.empty()) { return 0; }
    std::size_t _n = 0;
    std::string::size_type _p = 0;
    while ((_p = _hay.find(_needle, _p)) != std::string::npos)
    {
        ++_n;
        _p += _needle.size();
    }
    return _n;
}

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// internal reporter helpers
bool tests_helper_status_of();
bool tests_helper_status_str();
bool tests_helper_shown_all();
bool tests_helper_shown_failures_only();
bool tests_helper_shown_failures_and_skipped();
bool tests_helper_shown_summary_and_silent();
bool tests_helper_subst();
bool tests_helper_split_tags();

// facade
bool tests_facade_default_options();
bool tests_facade_explicit_options();
bool tests_facade_options_mutable();
bool tests_facade_set_options();
bool tests_facade_set_console();
bool tests_facade_handler_access();
bool tests_facade_run_counters();
bool tests_facade_verdict();
bool tests_facade_exit_code();
bool tests_facade_result_snapshot();

// rendering
bool tests_render_leaf_line();
bool tests_render_index_increments();
bool tests_render_status_tokens();
bool tests_render_name_and_message();
bool tests_render_numbering_off();
bool tests_render_module_header();
bool tests_render_summary();
bool tests_render_width_clamp();
bool tests_render_duration_timing_empty();
bool tests_render_tags_metadata_path();

// show policies / routing / free functions
bool tests_show_all();
bool tests_show_failures_only();
bool tests_show_failures_and_skipped();
bool tests_show_summary_only();
bool tests_show_silent();
bool tests_route_file_stream();
bool tests_route_console_and_file();
bool tests_route_no_output_path();
bool tests_route_pack_compress();
bool tests_route_pack_archive_fallback();
bool tests_free_run_tests();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
// =========================================================================

inline dt::module_spec
runner_spec()
{
    return dt::module_spec{
        "test_runner",
        "The top-level RUN facade: the internal reporter helpers, the facade "
        "(construction / options / handler / results), the reporter's rendered "
        "output, and the show policies + sink routing + free-function entry "
        "points.",
        {
            dt::block_spec{
                "helpers",
                "The internal reporter helpers: status mapping/token, the show "
                "policy, substitution, and tag splitting.",
                {
                    { "helper_status_of",
                      "the raw-status map covers every code and defaults to pending",
                      &tests_helper_status_of },
                    { "helper_status_str",
                      "each status has its fixed-width display token",
                      &tests_helper_status_str },
                    { "helper_shown_all",
                      "the all policy shows every status",
                      &tests_helper_shown_all },
                    { "helper_shown_failures_only",
                      "failures_only shows only failed/error",
                      &tests_helper_shown_failures_only },
                    { "helper_shown_failures_and_skipped",
                      "failures_and_skipped adds skipped",
                      &tests_helper_shown_failures_and_skipped },
                    { "helper_shown_summary_and_silent",
                      "summary_only and silent show nothing",
                      &tests_helper_shown_summary_and_silent },
                    { "helper_subst",
                      "substitution replaces every occurrence; empty key is a no-op",
                      &tests_helper_subst },
                    { "helper_split_tags",
                      "tag splitting trims, drops empties, and handles edges",
                      &tests_helper_split_tags },
                }
            },
            dt::block_spec{
                "facade",
                "Construction, options / set_options, set_console, handler(), and "
                "the result / verdict / exit_code / counter surface.",
                {
                    { "facade_default_options",
                      "a default runner carries the framework option defaults",
                      &tests_facade_default_options },
                    { "facade_explicit_options",
                      "an explicit option set is adopted",
                      &tests_facade_explicit_options },
                    { "facade_options_mutable",
                      "options() exposes a mutable option set",
                      &tests_facade_options_mutable },
                    { "facade_set_options",
                      "set_options replaces the option set",
                      &tests_facade_set_options },
                    { "facade_set_console",
                      "set_console redirects the console sink",
                      &tests_facade_set_console },
                    { "facade_handler_access",
                      "handler() exposes the underlying handler for extra listeners",
                      &tests_facade_handler_access },
                    { "facade_run_counters",
                      "run() tallies the tree into the counters",
                      &tests_facade_run_counters },
                    { "facade_verdict",
                      "verdict reflects the walked tree",
                      &tests_facade_verdict },
                    { "facade_exit_code",
                      "exit_code is 0 only for a non-empty all-passed run",
                      &tests_facade_exit_code },
                    { "facade_result_snapshot",
                      "result() returns the session snapshot",
                      &tests_facade_result_snapshot },
                }
            },
            dt::block_spec{
                "render",
                "The reporter's rendered output: leaf lines, module headers, the "
                "summary, token interpolation, width, and numbering.",
                {
                    { "render_leaf_line",
                      "a leaf renders its format with index/name/status filled",
                      &tests_render_leaf_line },
                    { "render_index_increments",
                      "the ordinal index advances across leaves",
                      &tests_render_index_increments },
                    { "render_status_tokens",
                      "each status renders its token",
                      &tests_render_status_tokens },
                    { "render_name_and_message",
                      "{name} and {message} come from node metadata",
                      &tests_render_name_and_message },
                    { "render_numbering_off",
                      "with numbering off {number} is empty but {index} still fills",
                      &tests_render_numbering_off },
                    { "render_module_header",
                      "an interior node renders format_module",
                      &tests_render_module_header },
                    { "render_summary",
                      "the summary interpolates the counter tokens",
                      &tests_render_summary },
                    { "render_width_clamp",
                      "a line longer than line_width is clamped",
                      &tests_render_width_clamp },
                    { "render_duration_timing_empty",
                      "{duration} and {timing} resolve empty in the pure walk",
                      &tests_render_duration_timing_empty },
                    { "render_tags_metadata_path",
                      "a node's tags metadata flows through context_of without error",
                      &tests_render_tags_metadata_path },
                }
            },
            dt::block_spec{
                "show_route",
                "Show policies, sink routing (console / file / packing), and the "
                "run_tests free functions.",
                {
                    { "show_all",
                      "all shows every leaf line plus the summary",
                      &tests_show_all },
                    { "show_failures_only",
                      "failures_only emits only failing leaves",
                      &tests_show_failures_only },
                    { "show_failures_and_skipped",
                      "failures_and_skipped adds skipped leaves",
                      &tests_show_failures_and_skipped },
                    { "show_summary_only",
                      "summary_only suppresses leaves and headers, keeps the summary",
                      &tests_show_summary_only },
                    { "show_silent",
                      "silent suppresses everything, including the summary",
                      &tests_show_silent },
                    { "route_file_stream",
                      "the file sink streams the report to output_path",
                      &tests_route_file_stream },
                    { "route_console_and_file",
                      "both sinks receive the report",
                      &tests_route_console_and_file },
                    { "route_no_output_path",
                      "a file sink with no path writes nothing and is safe",
                      &tests_route_no_output_path },
                    { "route_pack_compress",
                      "a packing run buffers and writes the packed file",
                      &tests_route_pack_compress },
                    { "route_pack_archive_fallback",
                      "a failed pack falls back to writing the report unpacked",
                      &tests_route_pack_archive_fallback },
                    { "free_run_tests",
                      "the run_tests free functions run a tree and return the exit code",
                      &tests_free_run_tests },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TR_CHECK
//   macro: evaluates its expression exactly once and routes the result through
// test_runner_check, capturing the expression text and source location.
// Variadic so expressions containing top-level commas need no defensive
// parentheses.
#define D_TR_CHECK(...)                                                        \
    ::djinterp::testing::test_runner_check(                                    \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_RUNNER_TESTS_
