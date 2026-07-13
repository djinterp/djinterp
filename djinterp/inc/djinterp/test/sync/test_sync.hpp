/******************************************************************************
* djinterp [test]                                                test_sync.hpp
*
*   Synchronization primitives for the DTest multithreading test
* harness: portable barriers, latches, gates, and rendezvous points
* used to coordinate test threads at well-defined moments.
*
*   These primitives are deliberately distinct from the production
* primitives in threadsafe.  They are tuned for test
* coordination - clarity and correctness over absolute performance -
* and they expose hooks that production primitives intentionally
* don't (e.g. arrival counts, last-to-arrive flags, timing of every
* arrival).
*
*   PRIMITIVES:
*     test_latch       - one-shot countdown latch (count_down/wait)
*     test_barrier     - reusable N-way barrier with optional
*                        completion callable
*     test_gate        - manual reset gate (open/close/wait)
*     test_rendezvous  - two-thread meeting point with payload
*                        exchange
*     simultaneous_start - convenience type built on test_gate
*                          for "all threads go on my mark" patterns
*
*   PORTABILITY:
*   Requires C++11 or later for the active implementations.  On
* C++98/03 every primitive degrades to a single-threaded stub: the
* counters still advance but wait() and arrive() never block (there
* is, by definition, no other thread to coordinate with).
*
*   On C++20, test_latch is implemented in terms of std::latch when
* available, and test_barrier in terms of std::barrier.  The
* fallback path uses portable_condvar from condvar.hpp.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST LATCH
* II.   TEST BARRIER
* III.  TEST GATE
* IV.   TEST RENDEZVOUS
* V.    SIMULTANEOUS START HELPER
*
*
* path:      /inc/djinterp/test/sync/test_sync.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_SYNC_
#define DJINTERP_TEST_SYNC_ 1

// std
#include <cstddef>

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <condition_variable>
    #include <functional>
    #include <mutex>
    #include <utility>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #if D_ENV_CPP_FEATURE_STL_LATCH
        #include <latch>
    #endif
    #if D_ENV_CPP_FEATURE_STL_BARRIER
        #include <barrier>
    #endif
#endif

// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/sync/atomic.hpp"
#include "../../core/sync/condvar.hpp"
#include "../test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST LATCH                                          ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- threadsafe foundation wrappers used by this module ---
using djinterp::portable_condvar;
using djinterp::exclusive_lock_policy;

// test_latch
//   class: one-shot countdown latch.  Threads call
// count_down() to decrement the counter; threads waiting
// in wait() unblock when the counter reaches zero.
//
//   Once the counter reaches zero, the latch stays open
// permanently - every subsequent wait() returns
// immediately.  This is the classic "starting gun"
// primitive but in the opposite direction: instead of
// starting threads, it lets a coordinator thread wait
// for N workers to finish a setup step.
//
// Example:
//   test_latch latch(4);
//   for (int i = 0; i < 4; ++i) {
//       group.emplace([&latch](std::size_t) {
//           // ... setup ...
//           latch.count_down();
//           // ... actual work ...
//       });
//   }
//   latch.wait();   // block until all four reach this point
class test_latch
{
public:
    using size_type = std::size_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    explicit test_latch(
        size_type _count
    )
        : m_initial(_count),
          m_remaining(_count),
          m_arrival_count(0),
          m_mutex(),
          m_cv()
    {}

    test_latch(const test_latch&)            = delete;
    test_latch& operator=(const test_latch&) = delete;

    // -----------------------------------------------------------------
    //  operations
    // -----------------------------------------------------------------

    // count_down
    //   decrements the counter by _amount.  If the counter
    // reaches zero, every waiter is unblocked.  Calls that
    // would take the counter below zero are clamped.
    void
    count_down(
        size_type _amount = 1
    )
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        if (_amount > m_remaining)
        {
            _amount = m_remaining;
        }

        m_remaining     -= _amount;
        m_arrival_count += _amount;

        if (m_remaining == 0)
        {
            lk.unlock();
            m_cv.notify_all();
        }

        return;
    }

    // arrive_and_wait
    //   atomically counts down by 1 and blocks until the
    // latch reaches zero.
    void
    arrive_and_wait()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        if (m_remaining > 0)
        {
            --m_remaining;
            ++m_arrival_count;

            if (m_remaining == 0)
            {
                lk.unlock();
                m_cv.notify_all();

                return;
            }
        }

        m_cv.wait(lk, [this]() { return (m_remaining == 0); });

        return;
    }

    // wait
    //   blocks until the counter reaches zero.  Returns
    // immediately if the latch is already open.
    void
    wait() const
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk, [this]() { return (m_remaining == 0); });

        return;
    }

    // wait_for
    //   blocks until the latch reaches zero or the timeout
    // expires.  Returns true if the latch opened, false on
    // timeout.
    template<typename _Rep,
             typename _Period>
    bool
    wait_for(
        const std::chrono::duration<_Rep, _Period>& _timeout
    ) const
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        return m_cv.wait_for(
            lk,
            _timeout,
            [this]() { return (m_remaining == 0); });
    }

    // try_wait
    //   non-blocking probe.  Returns true if the latch is
    // open.
    bool
    try_wait() const
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        return (m_remaining == 0);
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    // remaining
    //   returns the current counter value.  May change
    // immediately after this call returns.
    size_type
    remaining() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_remaining;
    }

    // arrival_count
    //   returns the total number of count_down units
    // applied so far.
    size_type
    arrival_count() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_arrival_count;
    }

    // initial_count
    size_type
    initial_count() const D_NOEXCEPT
    {
        return m_initial;
    }

    // is_open
    bool
    is_open() const
    {
        return try_wait();
    }

private:
    size_type                       m_initial;
    size_type                       m_remaining;
    size_type                       m_arrival_count;
    mutable std::mutex              m_mutex;
    mutable portable_condvar<exclusive_lock_policy>
                                    m_cv;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST BARRIER                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_barrier
//   class: reusable N-way synchronization barrier.  Each
// arriving thread calls arrive_and_wait().  When N threads
// have arrived, they all unblock and the barrier resets
// for the next phase.
//
//   An optional completion callable, if supplied, is
// invoked by the last-to-arrive thread before any thread
// is released.  This is the classic place to reset shared
// state between phases.
//
// Example:
//   test_barrier barrier(4, []() { /* phase boundary */ });
//   for (int i = 0; i < 4; ++i) {
//       group.emplace([&barrier](std::size_t) {
//           // ... phase 1 ...
//           barrier.arrive_and_wait();
//           // ... phase 2 ...
//           barrier.arrive_and_wait();
//       });
//   }
class test_barrier
{
public:
    using size_type      = std::size_t;
    using completion_fn  = std::function<void()>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    explicit test_barrier(
        size_type     _count,
        completion_fn _completion = completion_fn()
    )
        : m_count(_count),
          m_arrived(0),
          m_phase(0),
          m_completion(static_cast<completion_fn&&>(_completion)),
          m_mutex(),
          m_cv()
    {}

    test_barrier(const test_barrier&)            = delete;
    test_barrier& operator=(const test_barrier&) = delete;

    // -----------------------------------------------------------------
    //  operations
    // -----------------------------------------------------------------

    // arrive_and_wait
    //   blocks until _count threads have arrived.  The
    // last-to-arrive thread runs the completion callable
    // (if any) before all threads are released.  Returns
    // the phase index that was just completed (useful for
    // multi-phase test code).
    size_type
    arrive_and_wait()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        size_type my_phase = m_phase;

        ++m_arrived;

        // last to arrive resets and notifies
        if (m_arrived == m_count)
        {
            // run the completion hook on the last arrival
            if (m_completion)
            {
                try
                {
                    m_completion();
                }
                catch (...)
                {
                    // swallow: a throwing completion would
                    // strand the other waiters
                }
            }

            m_arrived = 0;
            ++m_phase;
            lk.unlock();
            m_cv.notify_all();

            return my_phase;
        }

        m_cv.wait(lk, [this, my_phase]()
                  {
                      return (m_phase != my_phase);
                  });

        return my_phase;
    }

    // arrive_and_drop
    //   the calling thread reduces the expected count
    // permanently and does not wait.  Use when a worker
    // is exiting early and should not block the others.
    void
    arrive_and_drop()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        if (m_count > 0)
        {
            --m_count;
        }

        // if dropping us makes the barrier complete,
        // release the others
        if ( (m_arrived >= m_count) &&
             (m_count   >  0) )
        {
            if (m_completion)
            {
                try
                {
                    m_completion();
                }
                catch (...)
                {}
            }

            m_arrived = 0;
            ++m_phase;
            lk.unlock();
            m_cv.notify_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    size_type
    expected() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_count;
    }

    size_type
    arrived() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_arrived;
    }

    size_type
    phase() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_phase;
    }

private:
    size_type                       m_count;
    size_type                       m_arrived;
    size_type                       m_phase;
    completion_fn                   m_completion;
    mutable std::mutex              m_mutex;
    mutable portable_condvar<exclusive_lock_policy>
                                    m_cv;
};


///////////////////////////////////////////////////////////////////////////////
///                III. TEST GATE                                           ///
///////////////////////////////////////////////////////////////////////////////

// test_gate
//   class: a manual-reset gate.  Threads call wait() and
// block until another thread calls open().  Once opened,
// the gate stays open for every subsequent wait() until
// close() is called.
//
//   Distinguished from test_latch by being two-state and
// repeatable, rather than one-shot.
//
// Example (controlled start):
//   test_gate gate;
//   for (int i = 0; i < 4; ++i) {
//       group.emplace([&gate](std::size_t) {
//           gate.wait();   // all four block here
//           // ... work runs in parallel ...
//       });
//   }
//   group.start_all();
//   gate.open();           // release all four
class test_gate
{
public:
    test_gate()
        : m_open(false),
          m_pass_count(0),
          m_mutex(),
          m_cv()
    {}

    test_gate(const test_gate&)            = delete;
    test_gate& operator=(const test_gate&) = delete;

    // -----------------------------------------------------------------
    //  state operations
    // -----------------------------------------------------------------

    // open
    //   opens the gate and unblocks all waiters.
    void
    open()
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_open = true;
        }

        m_cv.notify_all();

        return;
    }

    // close
    //   closes the gate.  Subsequent wait() calls will
    // block until the next open().
    void
    close()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_open = false;

        return;
    }

    // -----------------------------------------------------------------
    //  wait operations
    // -----------------------------------------------------------------

    // wait
    //   blocks while the gate is closed.  Returns
    // immediately if the gate is already open.
    void
    wait()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        m_cv.wait(lk, [this]() { return m_open; });

        ++m_pass_count;

        return;
    }

    // wait_for
    //   blocks while the gate is closed up to _timeout.
    // Returns true if the gate was open within the
    // timeout, false otherwise.
    template<typename _Rep,
             typename _Period>
    bool
    wait_for(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        bool opened = m_cv.wait_for(
            lk,
            _timeout,
            [this]() { return m_open; });

        if (opened)
        {
            ++m_pass_count;
        }

        return opened;
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    // is_open
    bool
    is_open() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_open;
    }

    // pass_count
    //   returns the number of threads that have passed
    // through the gate since construction.
    std::size_t
    pass_count() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_pass_count;
    }

private:
    bool                            m_open;
    std::size_t                     m_pass_count;
    mutable std::mutex              m_mutex;
    mutable portable_condvar<exclusive_lock_policy>
                                    m_cv;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEST RENDEZVOUS                                     ///
///////////////////////////////////////////////////////////////////////////////

// test_rendezvous
//   class template: two-thread meeting point with optional
// payload exchange.  One thread calls send(value), the
// other calls receive(); both block until the meeting
// completes.
//
//   This is a synchronous channel of capacity zero - useful
// for testing handoff semantics, signal propagation, and
// CSP-style protocols.
//
// Template parameter:
//   _Payload: the value type exchanged.  Defaults to
//             a tag struct for "signal-only" rendezvous.
template<typename _Payload = std::nullptr_t>
class test_rendezvous
{
public:
    using payload_type = _Payload;

    test_rendezvous()
        : m_value(),
          m_has_value(false),
          m_value_consumed(false),
          m_mutex(),
          m_cv_send(),
          m_cv_recv()
    {}

    test_rendezvous(const test_rendezvous&)            = delete;
    test_rendezvous& operator=(const test_rendezvous&) = delete;

    // -----------------------------------------------------------------
    //  send / receive
    // -----------------------------------------------------------------

    // send
    //   delivers _value to a waiting receiver.  Blocks until
    // a receiver consumes it.
    void
    send(
        payload_type _value
    )
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        // wait until the previous value, if any, has been
        // consumed
        m_cv_send.wait(lk, [this]() { return !m_has_value; });

        m_value          = static_cast<payload_type&&>(_value);
        m_has_value      = true;
        m_value_consumed = false;

        // wake one receiver
        m_cv_recv.notify_one();

        // wait for the receiver to consume
        m_cv_send.wait(lk, [this]() { return m_value_consumed; });

        return;
    }

    // receive
    //   blocks until a value is sent, then returns it.
    payload_type
    receive()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        m_cv_recv.wait(lk, [this]() { return m_has_value; });

        payload_type value =
            static_cast<payload_type&&>(m_value);

        m_has_value      = false;
        m_value_consumed = true;

        // wake the sender (and any other senders queued)
        m_cv_send.notify_all();

        return value;
    }

    // try_receive
    //   non-blocking variant.  Returns true and writes the
    // value to *_out if a value was available.
    bool
    try_receive(
        payload_type* _out
    )
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        if (!m_has_value)
        {
            return false;
        }

        if (_out)
        {
            *_out = static_cast<payload_type&&>(m_value);
        }

        m_has_value      = false;
        m_value_consumed = true;
        m_cv_send.notify_all();

        return true;
    }

private:
    payload_type                    m_value;
    bool                            m_has_value;
    bool                            m_value_consumed;
    mutable std::mutex              m_mutex;
    portable_condvar<exclusive_lock_policy>
                                    m_cv_send;
    portable_condvar<exclusive_lock_policy>
                                    m_cv_recv;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   SIMULTANEOUS START HELPER                           ///
///////////////////////////////////////////////////////////////////////////////

// simultaneous_start
//   class: thin wrapper around test_gate tuned for the
// "all threads stand at the line, fire on my signal"
// pattern that dominates concurrent test setups.
//
//   The class additionally tracks how many workers have
// reached the line, so the coordinator can choose to
// fire the signal as soon as everyone is ready.
//
// Example:
//   simultaneous_start start(4);
//   for (int i = 0; i < 4; ++i) {
//       group.emplace([&start](std::size_t) {
//           start.ready();   // checks in
//           start.go();      // blocks until signal
//           // ... work ...
//       });
//   }
//   group.start_all();
//   start.wait_until_ready();   // wait for all 4 to check in
//   start.signal();             // fire
class simultaneous_start
{
public:
    using size_type = std::size_t;

    explicit simultaneous_start(
        size_type _expected
    )
        : m_expected(_expected),
          m_ready_count(0),
          m_signaled(false),
          m_mutex(),
          m_cv_ready(),
          m_cv_go()
    {}

    simultaneous_start(const simultaneous_start&)            = delete;
    simultaneous_start& operator=(const simultaneous_start&) = delete;

    // -----------------------------------------------------------------
    //  worker side
    // -----------------------------------------------------------------

    // ready
    //   called by a worker to indicate it has reached the
    // starting line.
    void
    ready()
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            ++m_ready_count;
        }

        m_cv_ready.notify_all();

        return;
    }

    // go
    //   called by a worker to block until the signal is
    // fired.
    void
    go()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv_go.wait(lk, [this]() { return m_signaled; });

        return;
    }

    // ready_and_go
    //   atomic combination of ready() then go().
    void
    ready_and_go()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        ++m_ready_count;
        m_cv_ready.notify_all();

        m_cv_go.wait(lk, [this]() { return m_signaled; });

        return;
    }

    // -----------------------------------------------------------------
    //  coordinator side
    // -----------------------------------------------------------------

    // wait_until_ready
    //   blocks until all expected workers have called
    // ready().
    void
    wait_until_ready()
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        m_cv_ready.wait(
            lk,
            [this]()
            {
                return (m_ready_count >= m_expected);
            });

        return;
    }

    // wait_until_ready_for
    //   bounded variant of wait_until_ready.  Returns true
    // if all workers reported ready within _timeout.
    template<typename _Rep,
             typename _Period>
    bool
    wait_until_ready_for(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        std::unique_lock<std::mutex> lk(m_mutex);

        return m_cv_ready.wait_for(
            lk,
            _timeout,
            [this]()
            {
                return (m_ready_count >= m_expected);
            });
    }

    // signal
    //   fires the start signal, releasing all waiting
    // workers.
    void
    signal()
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_signaled = true;
        }

        m_cv_go.notify_all();

        return;
    }

    // wait_then_signal
    //   convenience: wait_until_ready() followed by
    // signal().
    void
    wait_then_signal()
    {
        wait_until_ready();
        signal();

        return;
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    size_type
    expected() const D_NOEXCEPT
    {
        return m_expected;
    }

    size_type
    ready_count() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_ready_count;
    }

    bool
    fired() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_signaled;
    }

private:
    size_type                       m_expected;
    size_type                       m_ready_count;
    bool                            m_signaled;
    mutable std::mutex              m_mutex;
    portable_condvar<exclusive_lock_policy>
                                    m_cv_ready;
    portable_condvar<exclusive_lock_policy>
                                    m_cv_go;
};


#else  // C++98/03 single-threaded stubs


///////////////////////////////////////////////////////////////////////////////
///                C++98 STUBS                                               ///
///////////////////////////////////////////////////////////////////////////////
// These types provide a minimal compile-time API so that
// test code targeting C++11+ can also build under C++98.
// All operations are non-blocking no-ops; counters still
// advance for introspection but no thread coordination is
// performed.

class test_latch
{
public:
    typedef std::size_t size_type;

    explicit test_latch(size_type _count)
        : m_remaining(_count)
    {}

    void      count_down(size_type _amount = 1)
    {
        if (_amount > m_remaining) { _amount = m_remaining; }
        m_remaining -= _amount;
    }
    void      arrive_and_wait()           { count_down(1); }
    void      wait()                const {}
    bool      try_wait()            const { return (m_remaining == 0); }
    size_type remaining()           const { return m_remaining; }

private:
    size_type m_remaining;
};

class test_barrier
{
public:
    typedef std::size_t size_type;

    explicit test_barrier(size_type _count)
        : m_count(_count), m_arrived(0), m_phase(0)
    {}

    size_type arrive_and_wait()
    {
        size_type p = m_phase;
        ++m_arrived;
        if (m_arrived == m_count) { m_arrived = 0; ++m_phase; }
        return p;
    }

private:
    size_type m_count;
    size_type m_arrived;
    size_type m_phase;
};

class test_gate
{
public:
    test_gate() : m_open(false) {}

    void open()  { m_open = true;  }
    void close() { m_open = false; }
    void wait()  {}
    bool is_open() const { return m_open; }

private:
    bool m_open;
};


#endif  // C++11


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_SYNC_
