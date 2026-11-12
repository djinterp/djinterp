/*******************************************************************************
* djinterp [cpp][test]                                          test_session.hpp
*
*   Test session: the top-level runtime context for executing tests.
* A session owns a test tree, maintains quasi-global options,
* records statistical counters (pass/fail/skip/pending/error),
* and provides pause/resume lifecycle control.
*
* CURRENT SCOPE
* -------------
*   The session ties together the framework's execution
* primitives: it owns a tree, drives a handler, manages timer
* and counter state, and exposes the verdict at the end.  The
* run(test_handler&) overload below is the main entry point.
*   The following capabilities are still stubbed for future
* implementation:
*   - save/load (serialization to/from binary or text)
*   - parallel execution dispatch
*   - reporter/formatter attachment beyond the handler's
*     own listener bundle
*
* SUITE COMPOSITION (REMOVED)
* ---------------------------
*   The previous compose / compose_and_run free functions grafted
* several independent module subtrees under one fresh root via the
* tree-level combine_subtrees() factory.  The refactored
* test_tree.hpp no longer ships that factory (its overlay/graft
* surface was dropped), so the compose helpers have been removed
* rather than left dangling.  To assemble a multi-module suite,
* populate session.tree() directly (a builder, or per-module
* subtree factories appended through the tree's add_root /
* append_child surface).  A standalone graft helper can be
* reintroduced - most naturally as a free function alongside the
* test_tree convenience aliases in test_defaults.hpp - once its
* shape is settled.
*
* SESSION STATE
* -------------
*   A session transitions through a linear state machine:
*     idle --> running --> paused --> running --> ... --> finished
*   The state is tracked as a simple enum.  Transitions are
*   validated; invalid transitions are no-ops.
*
* COUNTERS
* --------
*   The session keeps a test_tally keyed by the status event tags,
* used only as types (DOnTestPassed, DOnTestFailed,
* DOnTestSkipped, DOnTestError, and the session's own
* pending_key). When run(test_handler&) returns, every counter
* reflects the most recent run's tally, synced from the handler's
* DSessionResult. A caller who wants events to count into the
* tally binds test_counter_event.hpp's handlers.
*
* TIMER
* -----
*   The session owns a DTestTimer measuring wall-clock
* execution time.  The timer starts on run(), pauses on
* pause(), resumes on resume(), and stops on finish().
*
* PORTABILITY
* -----------
*   C++11 minimum.  Uses djinterp.hpp for namespace macros
* and constexpr support.
*
* HOW TO USE
* ----------
*   A session owns a tree and runs it through a handler; its tally and timer
* hold the run's counts and wall-clock time:
*
*     session.run(handler);
*     session.passed().value();
*
* OPTIONS
* -------
*   The session carries a test_option_set for quasi-global configuration; test
* elements may read their defaults (max failures, verbosity, handler) from it.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_session.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Session state
      -------------

II.   Test session
      ------------
*/

#ifndef DJINTERP_TEST_SESSION_HPP
#define DJINTERP_TEST_SESSION_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but
// core/container/tree/nary/nary_tree.hpp, which it reaches, needs C++17
// (lowered from C++20 by round 3's lane 2). The owner's ruling: compile at
// every level first; port down only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>
#include <cstddef>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./test_common.hpp"
#include "./test_tree.hpp"
#include "./test_counter.hpp"
#include "./test_handler.hpp"
#include "./test_timer.hpp"
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   SESSION STATE                                       ///
///////////////////////////////////////////////////////////////////////////////

// DSessionState
//   enum: lifecycle state of a test session.
enum class DSessionState
{
    idle     = 0,
    running  = 1,
    paused   = 2,
    finished = 3
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST SESSION                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_session
//   class: top-level runtime context for test execution.
// Owns the test tree, statistical counters, execution timer,
// and session-wide options.
//
// Template parameters:
//   Element:    the test object element type.
//   Underlying: the n-ary tree container type.
//
// Usage:
//   test_session<basic_test, my_tree<basic_test>> session;
//   DDefaultTestHandler                          handler;
//
//   // populate session.tree() directly (e.g. via a builder or a
//   // per-module subtree factory), then run it against a handler:
//   session_verdict v = session.run(handler);
//
//   auto passed = session.passed().value();
template<typename Element,
         typename Underlying>
class test_session
{
public:
    using element_type    = Element;
    using underlying_type = Underlying;
    using tree_type       = test_tree<Element, Underlying>;
    using counter_type    = test_counter<re_std::int64_t>;
    using tally_type      = test_tally<re_std::int64_t>;
    using timer_type      = DTestTimer<>;
    using size_type       = std::size_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // pending_key
    //   struct: the tally key the session's pending count is kept under. The
    // other four keys are the lifecycle event tags, used only as types.
    struct pending_key
    {
        static const char* name() noexcept
        {
            return "pending";
        }
    };

    // default
    //   tracks the five status counters, each at zero and held at or
    // above it, so the const accessors always find them. They count tests,
    // one at a time, whatever the knobs say.
    test_session()
        : m_tree(),
          m_state(DSessionState::idle),
          m_tally(session_options()),
          m_timer()
    {
        (void)m_tally.template track<DOnTestPassed>();
        (void)m_tally.template track<DOnTestFailed>();
        (void)m_tally.template track<DOnTestSkipped>();
        (void)m_tally.template track<pending_key>();
        (void)m_tally.template track<DOnTestError>();
    }

    // -----------------------------------------------------------------
    //  state
    // -----------------------------------------------------------------

    // state
    //   returns the current session state.
    DSessionState state() const noexcept
    {
        return m_state;
    }

    // is_idle
    bool is_idle() const noexcept
    {
        return (m_state == DSessionState::idle);
    }

    // is_running
    bool is_running() const noexcept
    {
        return (m_state == DSessionState::running);
    }

    // is_paused
    bool is_paused() const noexcept
    {
        return (m_state == DSessionState::paused);
    }

    // is_finished
    bool is_finished() const noexcept
    {
        return (m_state == DSessionState::finished);
    }

    // -----------------------------------------------------------------
    //  lifecycle
    // -----------------------------------------------------------------

    // run
    //   transitions from idle to running and starts the
    // timer.  No-op if not idle.
    void run()
    {
        if (m_state != DSessionState::idle)
        {
            return;
        }

        m_state = DSessionState::running;
        m_timer.start();

        return;
    }

    // run (with handler)
    //   Drives this session's owned tree against _handler.
    // Performs the full execution lifecycle: transitions the
    // session through idle -> running -> finished, brackets the
    // walk with timer start/stop, hands the underlying tree to
    // the handler (which fires its own session_start /
    // session_end events and walks the tree firing per-node
    // events), and finally syncs the session's per-status
    // counters from the handler's DSessionResult.
    //
    //   This is the standard "run a session and get a verdict"
    // entry point; the zero-argument run() above is the
    // state-only variant retained for cases where the caller
    // wants to manage walking themselves.
    //
    //   No-op if the session is not idle (returns the existing
    // verdict computed from current counters).
    //
    // Parameters:
    //   _handler: the test_handler driving the walk.  Listener
    //             bundle (printer, logger, etc.) must already be
    //             attached if any output is desired.  Passed by
    //             reference; the handler is not owned.
    //
    // Returns:
    //   The session_verdict for this run, equivalent to
    //   _handler.result().verdict() once the walk completes.
    session_verdict run(
        test_handler& _handler
    )
    {
        if (m_state != DSessionState::idle)
        {
            return current_verdict();
        }

        m_state = DSessionState::running;
        m_timer.start();

        // hand the underlying tree to the handler.  test_handler::run
        // brackets its own walk with start_session / end_session and
        // resets its DSessionResult up front, so we always end this
        // call with a fresh, reliable result snapshot.
        _handler.run(m_tree.underlying());

        // sync session-owned counters from the handler's totals so
        // that the session's passed/failed/skipped/pending/errors
        // accessors report the most recent run's tallies.
        sync_counters_from(_handler.result());

        m_state = DSessionState::finished;
        m_timer.stop();

        return current_verdict();
    }

    // run (with handler and tree)
    //   Replaces this session's owned tree with _tree (by move),
    // then drives the run as run(_handler) does above.
    //   Convenient when a caller has built a subtree separately
    // and wants the session to run it without first manually
    // moving it into m_tree.
    //
    //   No-op if the session is not idle.
    session_verdict run(
        test_handler& _handler,
        tree_type&&   _tree
    )
    {
        if (m_state != DSessionState::idle)
        {
            return current_verdict();
        }

        m_tree = static_cast<tree_type&&>(_tree);

        return run(_handler);
    }

    // pause
    //   transitions from running to paused and stops the
    // timer.  No-op if not running.
    void pause()
    {
        if (m_state != DSessionState::running)
        {
            return;
        }

        m_state = DSessionState::paused;
        m_timer.stop();

        return;
    }

    // resume
    //   transitions from paused to running and resumes
    // the timer.  No-op if not paused.
    void resume()
    {
        if (m_state != DSessionState::paused)
        {
            return;
        }

        m_state = DSessionState::running;
        m_timer.start();

        return;
    }

    // finish
    //   transitions to finished and stops the timer.
    // Valid from running or paused.  No-op if idle or
    // already finished.
    void finish()
    {
        if ( (m_state != DSessionState::running) &&
             (m_state != DSessionState::paused) )
        {
            return;
        }

        m_state = DSessionState::finished;
        m_timer.stop();

        return;
    }

    // reset
    //   returns the session to idle, clears the tree,
    // resets all counters and the timer.
    void reset()
    {
        m_state = DSessionState::idle;
        m_tree.clear();
        m_tally.reset();
        m_timer.reset();

        return;
    }

    // -----------------------------------------------------------------
    //  tree access
    // -----------------------------------------------------------------

    // tree
    //   returns a mutable reference to the test tree.
    tree_type& tree() noexcept
    {
        return m_tree;
    }

    // tree (const)
    const tree_type& tree() const noexcept
    {
        return m_tree;
    }

    // -----------------------------------------------------------------
    //  counters
    // -----------------------------------------------------------------

    // passed
    counter_type& passed() noexcept
    {
        return status<DOnTestPassed>();
    }

    const counter_type& passed() const noexcept
    {
        return status<DOnTestPassed>();
    }

    // failed
    counter_type& failed() noexcept
    {
        return status<DOnTestFailed>();
    }

    const counter_type& failed() const noexcept
    {
        return status<DOnTestFailed>();
    }

    // skipped
    counter_type& skipped() noexcept
    {
        return status<DOnTestSkipped>();
    }

    const counter_type& skipped() const noexcept
    {
        return status<DOnTestSkipped>();
    }

    // pending
    counter_type& pending() noexcept
    {
        return status<pending_key>();
    }

    const counter_type& pending() const noexcept
    {
        return status<pending_key>();
    }

    // errors
    counter_type& errors() noexcept
    {
        return status<DOnTestError>();
    }

    const counter_type& errors() const noexcept
    {
        return status<DOnTestError>();
    }

    // tally
    //   returns the session's tally: the five status counters, and any
    // other event a caller tracks in it.
    tally_type& tally() noexcept
    {
        return m_tally;
    }

    const tally_type& tally() const noexcept
    {
        return m_tally;
    }

    // total
    //   returns the sum of all status counters.
    re_std::int64_t total() const noexcept
    {
        return passed().value()  +
               failed().value()  +
               skipped().value() +
               pending().value() +
               errors().value();
    }

    // -----------------------------------------------------------------
    //  timer
    // -----------------------------------------------------------------

    // timer
    timer_type& timer() noexcept
    {
        return m_timer;
    }

    const timer_type& timer() const noexcept
    {
        return m_timer;
    }

    // elapsed
    //   returns the total elapsed duration.
    typename timer_type::duration_type elapsed() const
    {
        return m_timer.elapsed();
    }

    // -----------------------------------------------------------------
    //  save / load (stubs)
    // -----------------------------------------------------------------

    // save
    //   placeholder for future session serialization.
    // Currently a no-op.
    void save() const
    {
        return;
    }

    // load
    //   placeholder for future session deserialization.
    // Currently a no-op.
    void load()
    {
        return;
    }

private:
    // -----------------------------------------------------------------
    //  internal: counter sync and verdict
    // -----------------------------------------------------------------

    // session_options
    //   the options the status counters start from: a step of one and a
    // floor at zero, since they count tests and a count cannot go below it.
    static typename tally_type::options_type session_options() noexcept
    {
        typename tally_type::options_type options;

        options.step      = 1;
        options.has_floor = true;

        return options;
    }

    // status
    //   the status counter kept under `Key`; the constructor tracked all
    // five, so it is always there.
    template<typename Key>
    counter_type& status() noexcept
    {
        return *m_tally.template find<Key>();
    }

    template<typename Key>
    const counter_type& status() const noexcept
    {
        return *m_tally.template find<Key>();
    }

    // sync_counters_from
    //   Copies the per-status totals from a DSessionResult
    // (produced by test_handler) into this session's status
    // counters: each is reset to its start and then incremented
    // to the result's tally.
    void sync_counters_from(
        const DSessionResult& _r
    )
    {
        m_tally.reset();

        // each status the result counted
        if (_r.passed > 0)
        {
            (void)passed().increment(
                static_cast<counter_value_type>(_r.passed));
        }

        if (_r.failed > 0)
        {
            (void)failed().increment(
                static_cast<counter_value_type>(_r.failed));
        }

        if (_r.skipped > 0)
        {
            (void)skipped().increment(
                static_cast<counter_value_type>(_r.skipped));
        }

        if (_r.pending > 0)
        {
            (void)pending().increment(
                static_cast<counter_value_type>(_r.pending));
        }

        if (_r.errors > 0)
        {
            (void)errors().increment(
                static_cast<counter_value_type>(_r.errors));
        }

        return;
    }

    // current_verdict
    //   Computes a session_verdict from this session's owned
    // counters using the same decision order as
    // DSessionResult::verdict():
    //     - empty when nothing observed
    //     - failed when any failure or error
    //     - pending when no failures but unfinished work
    //     - passed otherwise
    session_verdict current_verdict() const D_NOEXCEPT
    {
        const auto p  = passed().value();
        const auto f  = failed().value();
        const auto s  = skipped().value();
        const auto pn = pending().value();
        const auto e  = errors().value();

        const auto t = p + f + s + pn + e;

        if (t == 0)
        {
            return session_verdict::empty;
        }

        if ((f > 0) || (e > 0))
        {
            return session_verdict::failed;
        }

        if (pn > 0)
        {
            return session_verdict::pending;
        }

        return session_verdict::passed;
    }

    // counter_value_type
    //   the underlying integer the session's counters store.
    // Aliased here so the increment calls above can cast cleanly
    // from the std::size_t fields of DSessionResult.
    using counter_value_type = typename counter_type::value_type;

    tree_type     m_tree;
    DSessionState m_state;
    tally_type    m_tally;
    timer_type    m_timer;
};


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_SESSION_HPP
