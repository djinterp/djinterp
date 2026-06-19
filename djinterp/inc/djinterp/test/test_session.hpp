/******************************************************************************
* djinterp [test]                                             test_session.hpp
*
*   Test session: the top-level runtime context for executing tests.
* A session owns a test tree, maintains quasi-global options,
* records statistical counters (pass/fail/skip/pending/error),
* and provides pause/resume lifecycle control.
*
*   CURRENT SCOPE:
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
*   SUITE COMPOSITION (REMOVED):
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
*   SESSION STATE:
*   A session transitions through a linear state machine:
*     idle --> running --> paused --> running --> ... --> finished
*   The state is tracked as a simple enum.  Transitions are
*   validated; invalid transitions are no-ops.
*
*   OPTIONS:
*   The session carries a test_option_set for quasi-global
* configuration.  Individual test elements may consult the
* session's options for defaults (max failures, verbosity,
* handler).
*
*   COUNTERS:
*   The session owns a test_counter for each status category.
* When run(test_handler&) returns, every counter reflects the
* most recent run's tally - synced from the handler's
* session_result via on_reset / on_increment events so any
* listener bound to those gets a faithful event stream even
* though the underlying tally came from outside the session.
*
*   TIMER:
*   The session owns a test_timer measuring wall-clock
* execution time.  The timer starts on run(), pauses on
* pause(), resumes on resume(), and stops on finish().
*
*   PORTABILITY:
*   C++11 minimum.  Uses djinterp.hpp for namespace macros
* and constexpr support.
*
*
* path:      /inc/djinterp/test/test_session.hpp
* link(s):   TBA
* Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Session state
II.   Test session
*/

#ifndef DJINTERP_TEST_SESSION_
#define DJINTERP_TEST_SESSION_ 1

// std
#include <cstddef>
#include <chrono>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_tree.hpp"
#include "./test_counter.hpp"
#include "./test_handler.hpp"
#include "./test_timer.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   SESSION STATE                                       ///
///////////////////////////////////////////////////////////////////////////////

// session_state
//   enum: lifecycle state of a test session.
enum class session_state
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
//   _Element:    the test object element type.
//   _Underlying: the n-ary tree container type.
//
// Usage:
//   test_session<basic_test, my_tree<basic_test>> session;
//   default_test_handler                          handler;
//
//   // populate session.tree() directly (e.g. via a builder or a
//   // per-module subtree factory), then run it against a handler:
//   session_verdict v = session.run(handler);
//
//   auto passed = session.passed().value();
template<typename _Element,
         typename _Underlying>
class test_session
{
public:
    using element_type    = _Element;
    using underlying_type = _Underlying;
    using tree_type       = test_tree<_Element, _Underlying>;
    using counter_type    = test_counter<std::int64_t>;
    using timer_type      = test_timer<>;
    using size_type       = std::size_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    test_session()
        : m_tree(),
          m_state(session_state::idle),
          m_passed(0, 0),
          m_failed(0, 0),
          m_skipped(0, 0),
          m_pending(0, 0),
          m_errors(0, 0),
          m_timer()
    {}

    // -----------------------------------------------------------------
    //  state
    // -----------------------------------------------------------------

    // state
    //   returns the current session state.
    session_state
    state() const noexcept
    {
        return m_state;
    }

    // is_idle
    bool
    is_idle() const noexcept
    {
        return (m_state == session_state::idle);
    }

    // is_running
    bool
    is_running() const noexcept
    {
        return (m_state == session_state::running);
    }

    // is_paused
    bool
    is_paused() const noexcept
    {
        return (m_state == session_state::paused);
    }

    // is_finished
    bool
    is_finished() const noexcept
    {
        return (m_state == session_state::finished);
    }

    // -----------------------------------------------------------------
    //  lifecycle
    // -----------------------------------------------------------------

    // run
    //   transitions from idle to running and starts the
    // timer.  No-op if not idle.
    void
    run()
    {
        if (m_state != session_state::idle)
        {
            return;
        }

        m_state = session_state::running;
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
    // counters from the handler's session_result.
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
    session_verdict
    run(
        test_handler& _handler
    )
    {
        if (m_state != session_state::idle)
        {
            return current_verdict();
        }

        m_state = session_state::running;
        m_timer.start();

        // hand the underlying tree to the handler.  test_handler::run
        // brackets its own walk with start_session / end_session and
        // resets its session_result up front, so we always end this
        // call with a fresh, reliable result snapshot.
        _handler.run(m_tree.underlying());

        // sync session-owned counters from the handler's totals so
        // that the session's passed/failed/skipped/pending/errors
        // accessors report the most recent run's tallies.
        sync_counters_from(_handler.result());

        m_state = session_state::finished;
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
    session_verdict
    run(
        test_handler& _handler,
        tree_type&&   _tree
    )
    {
        if (m_state != session_state::idle)
        {
            return current_verdict();
        }

        m_tree = static_cast<tree_type&&>(_tree);

        return run(_handler);
    }

    // pause
    //   transitions from running to paused and stops the
    // timer.  No-op if not running.
    void
    pause()
    {
        if (m_state != session_state::running)
        {
            return;
        }

        m_state = session_state::paused;
        m_timer.stop();

        return;
    }

    // resume
    //   transitions from paused to running and resumes
    // the timer.  No-op if not paused.
    void
    resume()
    {
        if (m_state != session_state::paused)
        {
            return;
        }

        m_state = session_state::running;
        m_timer.start();

        return;
    }

    // finish
    //   transitions to finished and stops the timer.
    // Valid from running or paused.  No-op if idle or
    // already finished.
    void
    finish()
    {
        if ( (m_state != session_state::running) &&
             (m_state != session_state::paused) )
        {
            return;
        }

        m_state = session_state::finished;
        m_timer.stop();

        return;
    }

    // reset
    //   returns the session to idle, clears the tree,
    // resets all counters and the timer.
    void
    reset()
    {
        m_state = session_state::idle;
        m_tree.clear();
        m_passed.reset();
        m_failed.reset();
        m_skipped.reset();
        m_pending.reset();
        m_errors.reset();
        m_timer.reset();

        return;
    }

    // -----------------------------------------------------------------
    //  tree access
    // -----------------------------------------------------------------

    // tree
    //   returns a mutable reference to the test tree.
    tree_type&
    tree() noexcept
    {
        return m_tree;
    }

    // tree (const)
    const tree_type&
    tree() const noexcept
    {
        return m_tree;
    }

    // -----------------------------------------------------------------
    //  counters
    // -----------------------------------------------------------------

    // passed
    counter_type&
    passed() noexcept
    {
        return m_passed;
    }

    const counter_type&
    passed() const noexcept
    {
        return m_passed;
    }

    // failed
    counter_type&
    failed() noexcept
    {
        return m_failed;
    }

    const counter_type&
    failed() const noexcept
    {
        return m_failed;
    }

    // skipped
    counter_type&
    skipped() noexcept
    {
        return m_skipped;
    }

    const counter_type&
    skipped() const noexcept
    {
        return m_skipped;
    }

    // pending
    counter_type&
    pending() noexcept
    {
        return m_pending;
    }

    const counter_type&
    pending() const noexcept
    {
        return m_pending;
    }

    // errors
    counter_type&
    errors() noexcept
    {
        return m_errors;
    }

    const counter_type&
    errors() const noexcept
    {
        return m_errors;
    }

    // total
    //   returns the sum of all status counters.
    std::int64_t
    total() const noexcept
    {
        return m_passed.value()  +
               m_failed.value()  +
               m_skipped.value() +
               m_pending.value() +
               m_errors.value();
    }

    // -----------------------------------------------------------------
    //  timer
    // -----------------------------------------------------------------

    // timer
    timer_type&
    timer() noexcept
    {
        return m_timer;
    }

    const timer_type&
    timer() const noexcept
    {
        return m_timer;
    }

    // elapsed
    //   returns the total elapsed duration.
    typename timer_type::duration_type
    elapsed() const
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

    // sync_counters_from
    //   Copies the per-status totals from a session_result
    // (produced by test_handler) into this session's owned
    // counters.  Each counter is reset to its initial value
    // and then incremented to the result's tally; the counter
    // emits its on_reset and on_increment events, so any
    // listener bound to those gets a faithful event stream
    // even though the underlying tally came from outside the
    // session.
    void
    sync_counters_from(
        const session_result& _r
    )
    {
        m_passed.reset();
        m_failed.reset();
        m_skipped.reset();
        m_pending.reset();
        m_errors.reset();

        if (_r.passed  > 0) m_passed.increment(
                                static_cast<counter_value_type>(_r.passed));
        if (_r.failed  > 0) m_failed.increment(
                                static_cast<counter_value_type>(_r.failed));
        if (_r.skipped > 0) m_skipped.increment(
                                static_cast<counter_value_type>(_r.skipped));
        if (_r.pending > 0) m_pending.increment(
                                static_cast<counter_value_type>(_r.pending));
        if (_r.errors  > 0) m_errors.increment(
                                static_cast<counter_value_type>(_r.errors));

        return;
    }

    // current_verdict
    //   Computes a session_verdict from this session's owned
    // counters using the same decision order as
    // session_result::verdict():
    //     - empty when nothing observed
    //     - failed when any failure or error
    //     - pending when no failures but unfinished work
    //     - passed otherwise
    session_verdict
    current_verdict() const D_NOEXCEPT
    {
        const auto p = m_passed.value();
        const auto f = m_failed.value();
        const auto s = m_skipped.value();
        const auto pn = m_pending.value();
        const auto e = m_errors.value();

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
    // from the std::size_t fields of session_result.
    using counter_value_type = typename counter_type::value_type;

    tree_type     m_tree;
    session_state m_state;
    counter_type  m_passed;
    counter_type  m_failed;
    counter_type  m_skipped;
    counter_type  m_pending;
    counter_type  m_errors;
    timer_type    m_timer;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_SESSION_
