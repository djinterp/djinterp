/*******************************************************************************
* djinterp [test]                                            test_emit_tests.hpp
*
*   Declarations for the unit-test suite covering test_emit.hpp - the FP +
* fluent OUTPUT surface over a finished run.  Each free function exercises one
* facet of the emit algebra and returns true if every check inside it passed.
* Tests are grouped into translation units by the section of the header they
* cover:
*
*   - test_emit_tests_tunes.cpp    -> I.   option transforms (tune::* + compose,
*                                         the with() origin)
*   - test_emit_tests_targets.cpp  -> II.  targets: as_* / to / with / to_*_file,
*                                         operator+ fan-out, per_module
*   - test_emit_tests_sinks.cpp    -> III. terminals: into_7z / into_zip /
*                                         compress / to_dir / to_console factories
*   - test_emit_tests_builder.cpp  -> IV+V the builder, pipe operators, run_sink
*                                         dispatch, build() bundling, the RAII
*                                         default, moved-from inertness, origins
*
*   The lone shared check helper, test_emit_check, reports a failing check (with
* its stringized expression and source location) and forwards the boolean.  The
* D_TE_CHECK macro is the intended call site.
*
*   OBSERVING THE ENGINE.  The plan pieces (targets, groups, sinks, tunes) are
* value types with public fields / observable option effects, so those tests
* inspect them directly.  The builder's terminals actually drive the real
* bundle/packaging engine, which this suite observes at its edges - the write
* success bool, files appearing on disk under a scratch directory (loose names
* are fully determined by test_emit: dir + stem|"report" + format_extension),
* and console bytes captured by swapping std::cout's streambuf - none of which
* assumes the internal renderer's exact output.
*
*   Fixtures.  temp_dir is a unique scratch directory (removed on scope exit);
* cout_capture redirects std::cout into a buffer for the run of its lifetime;
* make_report builds a test_report carrying N named modules (for per_module
* fan-out).
*
*   PORTABILITY: test_emit.hpp self-suppresses below C++17, so every test body
* (and the C++17-only fixtures) is guarded; under an older standard each unit
* passes vacuously.
*
*   NOTE: the emit surface lives in djinterp::test; the tests live, flat, in
* djinterp::testing (hence the `dt` alias below).
*
*
* path:      /tests/djinterp/test/test_emit_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_EMIT_TESTS_
#define DJINTERP_TEST_EMIT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first (defines the language gates), then the
// header under test.
#include <djinterp/djinterp.hpp>
#if !defined(DTEST_SPEC_MODE) && D_ENV_LANG_IS_CPP17_OR_HIGHER
// fixtures only (global scope): filesystem for on-disk observation, streams for
// console capture.  Must precede the namespace blocks below.
// std
#include <filesystem>
#include <iostream>
#include <sstream>
#endif
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_emit.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the emit surface, which lives in djinterp::test; the tests live
// here in djinterp::testing.  Declared once for the whole suite.
namespace dt = ::djinterp::test;

#if !defined(DTEST_SPEC_MODE) && D_ENV_LANG_IS_CPP17_OR_HIGHER  // fixtures + check helper


// =========================================================================
//  shared check helper
// =========================================================================

// test_emit_check
//   function: reports the result of a single check.  When _condition is false,
// prints the stringized expression with its source location to stdout; always
// returns _condition so the caller can fold it into a running result.
inline bool
test_emit_check(
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
//  fixtures
// =========================================================================

// temp_dir
//   fixture: a unique scratch directory under the system temp path, created on
// construction and removed (recursively) on destruction.  with(leaf) yields an
// absolute path inside it (for a pinned target destination); dir_slash() yields
// the directory as a trailing-slash prefix (for to_dir); has(leaf) tests
// existence; file_count() counts the regular files written into it.
struct temp_dir
{
    std::filesystem::path root;

    temp_dir()
    {
        static unsigned s_counter = 0u;
        const std::filesystem::path base = std::filesystem::temp_directory_path();
        root = base / (std::string("dtest_emit_") + std::to_string(++s_counter)
                       + "_" + std::to_string(static_cast<long>(std::clock())));
        std::error_code _ec;
        std::filesystem::create_directories(root, _ec);
    }

    ~temp_dir()
    {
        std::error_code _ec;
        std::filesystem::remove_all(root, _ec);
    }

    temp_dir(const temp_dir&)            = delete;
    temp_dir& operator=(const temp_dir&) = delete;

    // an absolute path to a leaf inside the directory (a pinned destination).
    std::string with(const std::string& _leaf) const
    {
        return (root / _leaf).string();
    }

    // the directory as a to_dir prefix (trailing separator).
    std::string dir_slash() const
    {
        return root.string() + "/";
    }

    // does a leaf (relative to the directory) exist?
    bool has(const std::string& _leaf) const
    {
        std::error_code _ec;
        return std::filesystem::exists(root / _leaf, _ec);
    }

    // how many regular files were written into the directory (recursively)?
    std::size_t file_count() const
    {
        std::size_t   _n = 0;
        std::error_code _ec;
        for (std::filesystem::recursive_directory_iterator _it(root, _ec), _end;
             _it != _end; _it.increment(_ec))
        {
            if (_it->is_regular_file(_ec)) { ++_n; }
        }
        return _n;
    }
};


// cout_capture
//   fixture: redirects std::cout into an internal buffer for the duration of
// its lifetime (restoring the original streambuf on destruction).  str()
// returns what was written.  Used to observe console emits without a terminal
// spy.
struct cout_capture
{
    std::ostringstream buffer;
    std::streambuf*    previous;

    cout_capture()
        : buffer(),
          previous(std::cout.rdbuf(buffer.rdbuf()))
    {}

    ~cout_capture()
    {
        std::cout.rdbuf(previous);
    }

    cout_capture(const cout_capture&)            = delete;
    cout_capture& operator=(const cout_capture&) = delete;

    std::string str() const { return buffer.str(); }
};


// make_report
//   fixture: a test_report carrying one named module per entry (the per_module
// fan-out reads modules by name).  Returned by value; callers keep it in a
// named local so its address stays stable for emit().
inline dt::test_report
make_report(const std::vector<std::string>& _module_names)
{
    dt::test_report _r;
    std::size_t     _i = 0;
    for (_i = 0; _i < _module_names.size(); ++_i)
    {
        _r.add_module(_module_names[_i], std::string());
    }
    return _r;
}

#endif  // !DTEST_SPEC_MODE && C++17


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// tunes (option transforms)
bool tests_tune_width_effect();
bool tests_tune_builders_wellformed();
bool tests_tune_compose_order();
bool tests_tune_compose_null_safety();
bool tests_tune_with_origin_identity();

// targets
bool tests_target_bare_defaults();
bool tests_target_to_pins_dest();
bool tests_target_with_folds_local();
bool tests_target_to_file_shorthands();
bool tests_target_plus_two_targets();
bool tests_target_plus_group_append_prepend();
bool tests_target_per_module_forms();

// sinks
bool tests_sink_defaults();
bool tests_sink_into_7z_zip();
bool tests_sink_compress();
bool tests_sink_to_dir_console();

// builder / pipe / dispatch / origins
bool tests_builder_loose_names();
bool tests_builder_stem_of();
bool tests_builder_per_module_fanout();
bool tests_builder_multiple_targets();
bool tests_builder_console_default();
bool tests_builder_explicit_console();
bool tests_builder_archive_dispatch();
bool tests_builder_compress_dispatch();
bool tests_builder_pipe_operators();
bool tests_builder_fluent_dest_less();
bool tests_builder_fluent_dest_ful_and_style();
bool tests_builder_fluent_per_module_method();
bool tests_builder_raii_loose();
bool tests_builder_moved_from_inert();
bool tests_builder_null_report();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
// =========================================================================

inline dt::module_spec
emit_spec()
{
    return dt::module_spec{
        "test_emit",
        "The output surface over a finished run: option tunes, the target "
        "algebra, the terminal sinks, and the builder that lowers both faces "
        "into the one bundle/packaging engine.",
        {
            dt::block_spec{
                "tunes",
                "Option transforms: the tune builders, compose (order + "
                "null-safety), and the with() origin.",
                {
                    { "tune_width_effect",
                      "tune::width folds line_width onto the option set",
                      &tests_tune_width_effect },
                    { "tune_builders_wellformed",
                      "every tune builder yields a non-null, invocable transform",
                      &tests_tune_builders_wellformed },
                    { "tune_compose_order",
                      "compose applies the left tune then the right",
                      &tests_tune_compose_order },
                    { "tune_compose_null_safety",
                      "compose treats a null operand as the identity",
                      &tests_tune_compose_null_safety },
                    { "tune_with_origin_identity",
                      "with(tune) returns the tune unchanged",
                      &tests_tune_with_origin_identity },
                }
            },
            dt::block_spec{
                "targets",
                "Render specs: the bare formats, to / with, the to_*_file "
                "shorthands, the + fan-out, and per_module.",
                {
                    { "target_bare_defaults",
                      "the bare as_* targets carry their format and no dest/tune",
                      &tests_target_bare_defaults },
                    { "target_to_pins_dest",
                      "to() pins the destination and leaves the original intact",
                      &tests_target_to_pins_dest },
                    { "target_with_folds_local",
                      "with() folds a target-scoped tune",
                      &tests_target_with_folds_local },
                    { "target_to_file_shorthands",
                      "to_*_file == as_<fmt>.to(name)",
                      &tests_target_to_file_shorthands },
                    { "target_plus_two_targets",
                      "target + target fans into an ordered group",
                      &tests_target_plus_two_targets },
                    { "target_plus_group_append_prepend",
                      "group + target appends; target + group prepends",
                      &tests_target_plus_group_append_prepend },
                    { "target_per_module_forms",
                      "per_module flips the group's per-module bit",
                      &tests_target_per_module_forms },
                }
            },
            dt::block_spec{
                "sinks",
                "Terminals: the sink factories and their defaults.",
                {
                    { "sink_defaults",
                      "a default sink is loose with the zip/gzip fallbacks",
                      &tests_sink_defaults },
                    { "sink_into_7z_zip",
                      "into_7z / into_zip select archive mode + container",
                      &tests_sink_into_7z_zip },
                    { "sink_compress",
                      "compress selects deflate-each with the given codec",
                      &tests_sink_compress },
                    { "sink_to_dir_console",
                      "to_dir is loose with a prefix; to_console is console",
                      &tests_sink_to_dir_console },
                }
            },
            dt::block_spec{
                "builder",
                "The builder, pipe operators, run_sink dispatch, bundling, the "
                "RAII default, moved-from inertness, and the origins.",
                {
                    { "builder_loose_names",
                      "a loose write lands report.<ext> in the target dir",
                      &tests_builder_loose_names },
                    { "builder_stem_of",
                      "the pinned dest is stemmed (dir + name, extension dropped)",
                      &tests_builder_stem_of },
                    { "builder_per_module_fanout",
                      "per_module emits one file per module, named by module",
                      &tests_builder_per_module_fanout },
                    { "builder_multiple_targets",
                      "a two-format group emits both documents",
                      &tests_builder_multiple_targets },
                    { "builder_console_default",
                      "a dest-less builder with no terminal flushes to console",
                      &tests_builder_console_default },
                    { "builder_explicit_console",
                      "to_console writes the report to std::cout",
                      &tests_builder_explicit_console },
                    { "builder_archive_dispatch",
                      "into_zip drives the archive sink and reports success",
                      &tests_builder_archive_dispatch },
                    { "builder_compress_dispatch",
                      "compress drives the deflate-each sink and reports success",
                      &tests_builder_compress_dispatch },
                    { "builder_pipe_operators",
                      "the four operator| overloads wire tune/target/group/sink",
                      &tests_builder_pipe_operators },
                    { "builder_fluent_dest_less",
                      "the dest-less fluent format methods each add a target",
                      &tests_builder_fluent_dest_less },
                    { "builder_fluent_dest_ful_and_style",
                      "the dest-ful fluent methods and style() accumulate",
                      &tests_builder_fluent_dest_ful_and_style },
                    { "builder_fluent_per_module_method",
                      "the fluent per_module() method fans across modules",
                      &tests_builder_fluent_per_module_method },
                    { "builder_raii_loose",
                      "a dest-ful builder with no terminal writes loose on destruction",
                      &tests_builder_raii_loose },
                    { "builder_moved_from_inert",
                      "a moved-from builder does not also write",
                      &tests_builder_moved_from_inert },
                    { "builder_null_report",
                      "a builder with no report writes empty content and succeeds",
                      &tests_builder_null_report },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TE_CHECK
//   macro: evaluates its expression exactly once and routes the result through
// test_emit_check, capturing the expression text and source location.
// Variadic so expressions containing top-level commas need no defensive
// parentheses.
#define D_TE_CHECK(...)                                                        \
    ::djinterp::testing::test_emit_check(                                      \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_EMIT_TESTS_
