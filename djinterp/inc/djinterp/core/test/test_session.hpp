/******************************************************************************
* djinterp [test]                                           test_session.hpp
*
*   Test session: the top-level runtime context for executing tests.
* A session owns a test tree, maintains quasi-global options,
* records statistical counters (pass/fail/skip/pending/error),
* and provides pause/resume lifecycle control.
*
*   CURRENT SCOPE:
*   This header defines the foundational session structure.  The
* following capabilities are stubbed for future implementation:
*   - save/load (serialization to/from binary or text)
*   - nested session composition
*   - parallel execution dispatch
*   - reporter/formatter attachment
*
*   SESSION STATE:
*   A session transitions through a linear state machine:
*     idle → running → paused → running → ... → finished
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
* Counters are incremented by the execution engine (not yet
* implemented) as each test element completes.
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
* TABLE OF CONTENTS
* =================
* I.    SESSION STATE
* II.   TEST SESSION
*
*
* path:      /inc/djinterp/test/test_session.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_SESSION_
#define DJINTERP_TEST_SESSION_ 1

#include <cstddef>
#include <chrono>
#include "../djinterp.hpp"
#include "./test_common.hpp"
#include "./test_tree.hpp"
#include "./test_counter.hpp"
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
//   // populate session.tree() ...
//   session.run();
//   // ... execution engine drives tests ...
//   session.finish();
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
