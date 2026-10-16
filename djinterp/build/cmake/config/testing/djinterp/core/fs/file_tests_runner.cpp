/******************************************************************************
* djinterp [test]                                   file_tests_runner.cpp
*
*   Entry point for the core/fs C++ unit tests. Builds one forest from every
* section, evaluates it in a single pass, and reports the tally as the process
* exit code so CTest can gate on it.
*
*   PHASE ORDER, AND WHY IT MATTERS HERE. The native builder separates
* CONSTRUCTION from EVALUATION: a section function only registers predicates,
* and run() evaluates them afterwards. Two consequences shape this file.
*
*     The environment must exist BEFORE the sections are built, not merely
*   before run(), because add_if evaluates its condition at BUILD time -- the
*   symlink probe among them -- to decide whether a group of cases is admitted
*   at all. Setting up first is what lets an unavailable capability drop its
*   cases rather than fail them.
*
*     The environment must ALSO still exist during run(), since that is when
*   the predicates actually touch the filesystem. So teardown happens after
*   run() returns, and not one line earlier.
*
*   The link, pipe and expected sections are compiled out where their feature
* is absent, so their calls carry the same guards their definitions do. A
* mismatch there is a link error rather than a compile error, which is why the
* macros are spelled identically in both places.
*
* path:      \build\cmake\config\testing\djinterp\core\fs\file_tests_runner.cpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.22
******************************************************************************/

// std
#include <cstdio>
#include <string>

// djinterp
#include "file_tests.hpp"


namespace
{

// g_failures
//   variable: how many leaves the failure listener has reported, so the
// closing line can say whether anything above it was a failure notice.
int g_failures = 0;

} // anonymous namespace


// D_CF_SECTION
//   macro: splice one section into the suite. Unique letters per suite, so a
// co-compiled suite's runner macro can never collide with this one.
#define D_CF_SECTION(_fn) ::djinterp::testing::_fn(suite)


int
main(void)
{
    ::djinterp::test::test_builder<> suite;
    ::djinterp::test::test_summary   summary;

    std::printf(
        "==================================================================\n"
        " djinterp core/fs (C++ layer) -- unit tests via the DTest builder\n"
        " backend: %s\n"
        "==================================================================\n",
        d_file_backend_name());

    // the environment must exist before the sections are BUILT: add_if
    // decides at build time which capability-dependent cases are admitted
    if (!::djinterp::testing::fs_test_setup())
    {
        std::fprintf(stderr,
                     "fatal: could not create the test environment\n");

        return 2;
    }

    // name each failing leaf as it is evaluated; the summary alone would say
    // how many failed but not which
    suite.on_failed(
        [](const ::djinterp::test::basic_test* _node)
        {
            std::string name;

            if (_node)
            {
                name = _node->metadata().get("name");
            }

            std::printf("  [FAIL] %s\n",
                        name.empty() ? "(unnamed)" : name.c_str());
            ++g_failures;
        });

    // build every section, in the order the modules are numbered
    D_CF_SECTION(tests_fs_error);
    D_CF_SECTION(tests_fs_path);
    D_CF_SECTION(tests_fs_status);
    D_CF_SECTION(tests_fs_stream);
    D_CF_SECTION(tests_fs_dir);
    D_CF_SECTION(tests_fs_ops);

#if D_FILE_LINK_IS_AVAILABLE
    // defined only where the platform has symbolic links
    D_CF_SECTION(tests_fs_link);
#endif

    D_CF_SECTION(tests_fs_temp);
    D_CF_SECTION(tests_fs_space);
    D_CF_SECTION(tests_fs_recursive);

#if D_FILE_PIPE_IS_AVAILABLE
    // defined only where the platform has command pipes
    D_CF_SECTION(tests_fs_pipe);
#endif

#if defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
    // defined only when the standard library ships std::expected
    D_CF_SECTION(tests_fs_expected);
#endif

    // one pass over the finished forest: this is where the predicates run,
    // so the environment is still standing
    summary = suite.run();

    ::djinterp::testing::fs_test_teardown();

    std::printf(
        "------------------------------------------------------------------\n"
        " modules %zu   blocks %zu   tests %zu   inline %zu\n"
        " passed %zu   failed %zu   skipped %zu   errored %zu   pending %zu\n"
        "------------------------------------------------------------------\n",
        summary.modules,
        summary.blocks,
        summary.tests,
        summary.inline_checks,
        summary.passed,
        summary.failed,
        summary.skipped,
        summary.errored,
        summary.pending);

    // all_passed() is deliberately stricter than "nothing failed": a pending
    // leaf is a test that never ran, and a suite of those is not green
    return summary.all_passed() ? 0 : 1;
}

#undef D_CF_SECTION
