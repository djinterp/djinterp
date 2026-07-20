/******************************************************************************
* djinterp [tests]                 test_report_runner_table_tests_convenience.cpp
*
*   The free-function tail: run_named_tests (pass / fail / empty exit codes),
* report_detail::to_report_string (each overload, including the null C-string),
* and the D_CHECK_EQ macro (capture on pass, on fail, and its once-only operand
* evaluation).
*
*   run_named_tests writes its console to the real stdout (the builder it
* creates defaults there and exposes no sink hook), so those cases run under a
* file-descriptor mute that redirects fd 1 to /dev/null and restores it - the
* assertions are on the returned exit code, which is fully deterministic.
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests_convenience.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#include "test_report_runner_table_tests.hpp"

// POSIX, for muting stdout at the descriptor level around run_named_tests
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>


NS_DJINTERP
NS_TESTING


namespace
{
    // scoped_stdout_mute
    //   redirects fd 1 to /dev/null for its lifetime, restoring it on scope
    // exit.  Used to silence run_named_tests, which prints to stdout.  We touch
    // only the underlying descriptor (never the stdout FILE), so the stream is
    // intact once restored; the fflush calls flush pending data to the right
    // target on each transition.
    struct scoped_stdout_mute
    {
        int saved;

        scoped_stdout_mute()
            : saved(-1)
        {
            std::fflush(stdout);
            saved = ::dup(1);

            const int _devnull = ::open("/dev/null", O_WRONLY);

            if (_devnull >= 0)
            {
                ::dup2(_devnull, 1);
                ::close(_devnull);
            }
        }

        ~scoped_stdout_mute()
        {
            std::fflush(stdout);

            if (saved >= 0)
            {
                ::dup2(saved, 1);
                ::close(saved);
            }
        }

        scoped_stdout_mute(const scoped_stdout_mute&)            = delete;
        scoped_stdout_mute& operator=(const scoped_stdout_mute&) = delete;
    };
}   // namespace


bool
tests_report_runner_table_run_named_tests_pass()
{
    const std::string        _names[3] = { "a", "b", "c" };
    const dt::test_predicate_fn _fns[3] = {
        &rr_pred_pass, &rr_pred_pass, &rr_pred_pass };

    int _code = 0;
    {
        scoped_stdout_mute _mute;
        _code = dt::run_named_tests(
            "mod", "desc", _names, _fns, 3, dt::default_test_options());
    }

    D_RRT_CHECK(_code == 0);

    return true;
}


bool
tests_report_runner_table_run_named_tests_fail()
{
    const std::string        _names[2] = { "a", "b" };
    const dt::test_predicate_fn _fns[2] = { &rr_pred_pass, &rr_pred_fail };

    int _code = 0;
    {
        scoped_stdout_mute _mute;
        _code = dt::run_named_tests(
            "mod", "", _names, _fns, 2, dt::default_test_options());
    }

    D_RRT_CHECK(_code == 1);

    return true;
}


bool
tests_report_runner_table_run_named_tests_empty()
{
    // zero tests: the lone module has no units, folds to the empty verdict,
    // and an empty verdict is not a pass -> non-zero exit code.
    int _code = 0;
    {
        scoped_stdout_mute _mute;
        _code = dt::run_named_tests(
            "mod", "", nullptr, nullptr, 0, dt::default_test_options());
    }

    D_RRT_CHECK(_code == 1);

    return true;
}


bool
tests_report_runner_table_to_report_string_overloads()
{
    // std::string and C-string pass through; bool prints true/false; anything
    // streamable goes through operator<<.
    D_RRT_CHECK(dt::report_detail::to_report_string(std::string("hi")) == "hi");
    D_RRT_CHECK(dt::report_detail::to_report_string("cstr")            == "cstr");
    D_RRT_CHECK(dt::report_detail::to_report_string(true)             == "true");
    D_RRT_CHECK(dt::report_detail::to_report_string(false)            == "false");
    D_RRT_CHECK(dt::report_detail::to_report_string(42)               == "42");
    D_RRT_CHECK(dt::report_detail::to_report_string(3.5)              == "3.5");

    return true;
}


bool
tests_report_runner_table_to_report_string_null_cstr()
{
    // a null C-string maps to the empty string, not a dereference
    const char* _null = nullptr;
    D_RRT_CHECK(dt::report_detail::to_report_string(_null) == "");

    return true;
}


bool
tests_report_runner_table_check_eq_pass()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");
    _rb.open_unit("case");

    const int _four = 4;
    D_CHECK_EQ(_rb, _four, 4);

    const dt::report_unit& _u = _rb.report().modules[0].units[0];
    D_RRT_CHECK(_u.checks.size() == std::size_t(1));
    D_RRT_CHECK(_u.checks[0].expression == "_four");   // stringized expr
    D_RRT_CHECK(_u.checks[0].expected   == "4");        // to_report_string(4)
    D_RRT_CHECK(_u.checks[0].actual     == "4");        // to_report_string(_four)
    D_RRT_CHECK(_u.checks[0].status     == dt::test_status::passed);

    return true;
}


bool
tests_report_runner_table_check_eq_fail()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");
    _rb.open_unit("case");

    const int _four = 4;
    D_CHECK_EQ(_rb, _four, 5);

    const dt::report_unit& _u = _rb.report().modules[0].units[0];
    D_RRT_CHECK(_u.checks[0].expected == "5");
    D_RRT_CHECK(_u.checks[0].actual   == "4");
    D_RRT_CHECK(_u.checks[0].status   == dt::test_status::failed);

    return true;
}


bool
tests_report_runner_table_check_eq_single_eval()
{
    dt::report_builder _rb;
    _rb.set_console(false);
    _rb.module("m", "");
    _rb.open_unit("case");

    // the expression has a side effect; the macro must evaluate it exactly once
    int _calls = 0;

    // a niladic callable so the expression _f() carries the side effect
    struct counter { int* n; int operator()() const { ++(*n); return 4; } };
    counter _f;
    _f.n = &_calls;

    D_CHECK_EQ(_rb, _f(), 4);

    D_RRT_CHECK(_calls == 1);

    const dt::report_unit& _u = _rb.report().modules[0].units[0];
    D_RRT_CHECK(_u.checks[0].status == dt::test_status::passed);

    return true;
}


NS_END  // testing
NS_END  // djinterp
