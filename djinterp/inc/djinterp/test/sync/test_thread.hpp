/******************************************************************************
* djinterp [test]                                              test_thread.hpp
*
*   Exception-capturing test thread: a managed thread wrapper for the
* DTest framework that captures exceptions, records timings, and
* preserves a thread identity (numeric id, optional name) for use in
* test reporting.
*   The class is intentionally thin - it adds three things to a raw
* std::thread:
*     1. EXCEPTION CAPTURE
*        Any exception thrown by the user-supplied callable is caught
*        and stored as a std::exception_ptr.  The harness can rethrow
*        on the main thread (rethrow_if_failed) or query the message
*        (exception_what).
*     2. TIMING
*        The thread records its start and stop time using a
*        std::chrono::steady_clock, exposing started_at(),
*        stopped_at(), and elapsed().
*     3. IDENTITY
*        Each thread carries a logical id (size_type) assigned by the
*        runner and an optional human-readable name.
*   PORTABILITY:
*   Requires C++11 or later for <thread> and <chrono>.  On C++98/03
* the class compiles to a single-threaded stub: every operation runs
* synchronously on the calling thread and start()/join() are no-ops.
*   This stub mode is intentional: it lets the same test code build
* against the null_lock_policy / single-threaded path without
* preprocessor branching at the call site.
*
*
* TABLE OF CONTENTS
* =================
* I.    THREAD STATE
* II.   TEST THREAD
* III.  THREAD GROUP
* IV.   FACTORY HELPERS
*
*
* path:      /inc/djinterp/test/sync/test_thread.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_THREAD_
#define DJINTERP_TEST_THREAD_ 1

// std
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <functional>
    #include <thread>
    #include <utility>
    #include <vector>
#endif
// djinterp
#include "../../core/djinterp.hpp"
#include "../../coresync/condvar.hpp"
#include "../test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   THREAD STATE                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_thread_state
//   enum: lifecycle state of a test_thread.
//
//   idle      - constructed but not yet started
//   running   - the worker function is currently executing
//   completed - the worker function returned normally
//   failed    - the worker function threw an uncaught exception
//   detached  - the std::thread was detached (no join possible)
enum class test_thread_state
{
    idle      = 0,
    running   = 1,
    completed = 2,
    failed    = 3,
    detached  = 4
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST THREAD                                         ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// test_thread
//   class: managed thread wrapper for the DTest framework.
// Captures exceptions, records timings, and carries a
// logical id and optional name.
//
//   The worker callable receives the thread's logical id
// as its first argument:
//
//     test_thread t(0, "writer", [](std::size_t _id) {
//         // ... work ...
//     });
//     t.start();
//     t.join();
//     if (t.failed()) {
//         std::rethrow_exception(t.exception());
//     }
class test_thread
{
public:
    using clock_type    = std::chrono::steady_clock;
    using time_point    = clock_type::time_point;
    using duration_type = clock_type::duration;
    using size_type     = std::size_t;
    using worker_fn     = std::function<void(size_type)>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    test_thread()
        : m_id(0),
          m_name(),
          m_worker(),
          m_thread(),
          m_state(test_thread_state::idle),
          m_exception(nullptr),
          m_started_at(),
          m_stopped_at()
    {}

    // from id, name, and worker
    test_thread(
        size_type    _id,
        std::string  _name,
        worker_fn    _worker
    )
        : m_id(_id),
          m_name(static_cast<std::string&&>(_name)),
          m_worker(static_cast<worker_fn&&>(_worker)),
          m_thread(),
          m_state(test_thread_state::idle),
          m_exception(nullptr),
          m_started_at(),
          m_stopped_at()
    {}

    // from id and worker (anonymous)
    test_thread(
        size_type _id,
        worker_fn _worker
    )
        : m_id(_id),
          m_name(),
          m_worker(static_cast<worker_fn&&>(_worker)),
          m_thread(),
          m_state(test_thread_state::idle),
          m_exception(nullptr),
          m_started_at(),
          m_stopped_at()
    {}

    // non-copyable: thread is non-copyable
    test_thread(const test_thread&)            = delete;
    test_thread& operator=(const test_thread&) = delete;

    // movable
    test_thread(test_thread&& _other) noexcept
        : m_id(_other.m_id),
          m_name(static_cast<std::string&&>(_other.m_name)),
          m_worker(static_cast<worker_fn&&>(_other.m_worker)),
          m_thread(static_cast<std::thread&&>(_other.m_thread)),
          m_state(_other.m_state.load(std::memory_order_acquire)),
          m_exception(_other.m_exception),
          m_started_at(_other.m_started_at),
          m_stopped_at(_other.m_stopped_at)
    {
        _other.m_state.store(test_thread_state::idle,
                             std::memory_order_release);
        _other.m_exception = nullptr;
    }

    // destructor: joins if joinable, ignores exceptions
    ~test_thread()
    {
        if (m_thread.joinable())
        {
            try
            {
                m_thread.join();
            }
            catch (...)
            {
                // swallow: destructor must not throw
            }
        }
    }

    // -----------------------------------------------------------------
    //  identity
    // -----------------------------------------------------------------

    // id
    //   returns the logical thread id assigned at construction.
    size_type
    id() const D_NOEXCEPT
    {
        return m_id;
    }

    // name
    //   returns the human-readable thread name (may be empty).
    const std::string&
    name() const D_NOEXCEPT
    {
        return m_name;
    }

    // set_name
    void
    set_name(
        std::string _name
    )
    {
        m_name = static_cast<std::string&&>(_name);

        return;
    }

    // -----------------------------------------------------------------
    //  state
    // -----------------------------------------------------------------

    // state
    //   returns the current lifecycle state.
    test_thread_state
    state() const D_NOEXCEPT
    {
        return m_state.load(std::memory_order_acquire);
    }

    bool
    is_idle() const D_NOEXCEPT
    {
        return (state() == test_thread_state::idle);
    }

    bool
    is_running() const D_NOEXCEPT
    {
        return (state() == test_thread_state::running);
    }

    bool
    completed() const D_NOEXCEPT
    {
        return (state() == test_thread_state::completed);
    }

    bool
    failed() const D_NOEXCEPT
    {
        return (state() == test_thread_state::failed);
    }

    bool
    joinable() const D_NOEXCEPT
    {
        return m_thread.joinable();
    }

    // -----------------------------------------------------------------
    //  lifecycle
    // -----------------------------------------------------------------

    // start
    //   spawns the worker thread.  Has no effect if not idle
    // or if no worker has been bound.
    void
    start()
    {
        if (m_state.load(std::memory_order_acquire) !=
            test_thread_state::idle)
        {
            return;
        }

        if (!m_worker)
        {
            return;
        }

        m_thread = std::thread(
            [this]()
            {
                this->run_worker_();
            });

        return;
    }

    // join
    //   blocks until the worker thread completes.  Has no
    // effect if the thread is not joinable.
    void
    join()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        return;
    }

    // detach
    //   detaches the underlying std::thread.  After detach,
    // the thread runs to completion independently and cannot
    // be joined.  State transitions to detached.
    void
    detach()
    {
        if (m_thread.joinable())
        {
            m_thread.detach();
            m_state.store(test_thread_state::detached,
                          std::memory_order_release);
        }

        return;
    }

    // try_join_for
    //   attempts to join the thread within _timeout.  Returns
    // true if the thread joined, false on timeout.  When false
    // is returned the thread is still running and must be
    // joined or detached separately.
    template<typename _Rep,
             typename _Period>
    bool
    try_join_for(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        if (!m_thread.joinable())
        {
            return true;
        }

        // poll the state at coarse intervals; std::thread
        // lacks a native timed_join, so we approximate.
        time_point start    = clock_type::now();
        time_point deadline = start + _timeout;

        while (clock_type::now() < deadline)
        {
            test_thread_state s =
                m_state.load(std::memory_order_acquire);

            if ( (s == test_thread_state::completed) ||
                 (s == test_thread_state::failed) )
            {
                m_thread.join();

                return true;
            }

            threadsafe::d_thread_yield();
            std::this_thread::sleep_for(
                std::chrono::microseconds(100));
        }

        return false;
    }

    // -----------------------------------------------------------------
    //  exception capture
    // -----------------------------------------------------------------

    // exception
    //   returns the captured exception_ptr, or nullptr if no
    // exception was thrown by the worker.
    std::exception_ptr
    exception() const D_NOEXCEPT
    {
        return m_exception;
    }

    // exception_what
    //   returns the message of the captured exception, or
    // an empty string if none.  Convenience wrapper for
    // reporting.
    std::string
    exception_what() const
    {
        if (!m_exception)
        {
            return std::string();
        }

        try
        {
            std::rethrow_exception(m_exception);
        }
        catch (const std::exception& e)
        {
            return std::string(e.what());
        }
        catch (...)
        {
            return std::string("unknown exception");
        }
    }

    // rethrow_if_failed
    //   if the worker threw, rethrows the captured
    // exception on the calling thread.  No-op if the worker
    // completed normally.
    void
    rethrow_if_failed() const
    {
        if (m_exception)
        {
            std::rethrow_exception(m_exception);
        }

        return;
    }

    // -----------------------------------------------------------------
    //  timing
    // -----------------------------------------------------------------

    // started_at
    //   returns the time_point at which the worker began.
    // Undefined if the thread has not yet started.
    time_point
    started_at() const D_NOEXCEPT
    {
        return m_started_at;
    }

    // stopped_at
    //   returns the time_point at which the worker finished.
    // Undefined if the thread is still running.
    time_point
    stopped_at() const D_NOEXCEPT
    {
        return m_stopped_at;
    }

    // elapsed
    //   returns the total wall-clock duration from start to
    // stop.  If the thread is still running, returns the
    // duration so far.
    duration_type
    elapsed() const D_NOEXCEPT
    {
        if (m_state.load(std::memory_order_acquire) ==
            test_thread_state::idle)
        {
            return duration_type::zero();
        }

        if (is_running())
        {
            return clock_type::now() - m_started_at;
        }

        return m_stopped_at - m_started_at;
    }

    // -----------------------------------------------------------------
    //  worker assignment
    // -----------------------------------------------------------------

    // set_worker
    //   replaces the worker function.  Must be called before
    // start().
    void
    set_worker(
        worker_fn _worker
    )
    {
        m_worker = static_cast<worker_fn&&>(_worker);

        return;
    }

private:
    // -----------------------------------------------------------------
    //  worker entry point
    // -----------------------------------------------------------------

    // run_worker_
    //   internal: dispatches the worker callable, capturing
    // any exception and recording timings.
    void
    run_worker_()
    {
        m_state.store(test_thread_state::running,
                      std::memory_order_release);
        m_started_at = clock_type::now();

        try
        {
            m_worker(m_id);

            m_stopped_at = clock_type::now();
            m_state.store(test_thread_state::completed,
                          std::memory_order_release);
        }
        catch (...)
        {
            m_exception  = std::current_exception();
            m_stopped_at = clock_type::now();
            m_state.store(test_thread_state::failed,
                          std::memory_order_release);
        }

        return;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    size_type                       m_id;
    std::string                     m_name;
    worker_fn                       m_worker;
    std::thread                     m_thread;
    std::atomic<test_thread_state>  m_state;
    std::exception_ptr              m_exception;
    time_point                      m_started_at;
    time_point                      m_stopped_at;
};


#else  // C++98/03 single-threaded stub


// test_thread
//   class: single-threaded stub.  All operations execute
// synchronously on the calling thread.  Provided so that
// test code targeting C++11+ can also build under C++98
// without preprocessor branches at the call site.
class test_thread
{
public:
    typedef std::size_t                            size_type;
    typedef void                                   (*worker_fn_ptr)(size_type);

    test_thread()
        : m_id(0),
          m_worker(0),
          m_state(test_thread_state::idle),
          m_failed(false)
    {}

    test_thread(size_type     _id,
                worker_fn_ptr _worker)
        : m_id(_id),
          m_worker(_worker),
          m_state(test_thread_state::idle),
          m_failed(false)
    {}

    size_type           id()        const { return m_id;    }
    test_thread_state   state()     const { return m_state; }
    bool                completed() const
    {
        return (m_state == test_thread_state::completed);
    }
    bool                failed()    const { return m_failed; }
    bool                joinable()  const { return false;    }

    void start()
    {
        if (!m_worker)
        {
            return;
        }

        m_state = test_thread_state::running;

        m_worker(m_id);

        m_state = test_thread_state::completed;
    }

    void join() {}

private:
    size_type         m_id;
    worker_fn_ptr     m_worker;
    test_thread_state m_state;
    bool              m_failed;
};


#endif  // C++11


///////////////////////////////////////////////////////////////////////////////
///                III. THREAD GROUP                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// test_thread_group
//   class: a group of test_threads launched and joined
// together.  The group enforces matched start/join calls
// and aggregates exception state.
//
// Usage:
//   test_thread_group group;
//   for (std::size_t i = 0; i < 4; ++i)
//   {
//       group.emplace(i, [](std::size_t _id) { ... });
//   }
//   group.start_all();
//   group.join_all();
//   if (group.any_failed()) { ... }
class test_thread_group
{
public:
    using size_type     = std::size_t;
    using worker_fn     = test_thread::worker_fn;
    using duration_type = test_thread::duration_type;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    test_thread_group()
        : m_threads()
    {}

    test_thread_group(const test_thread_group&)            = delete;
    test_thread_group& operator=(const test_thread_group&) = delete;

    test_thread_group(test_thread_group&&)                 = default;
    test_thread_group& operator=(test_thread_group&&)      = default;

    ~test_thread_group()
    {
        // best-effort join in destructor
        try
        {
            join_all();
        }
        catch (...)
        {}
    }

    // -----------------------------------------------------------------
    //  membership
    // -----------------------------------------------------------------

    // emplace
    //   constructs a new test_thread in place at the back of
    // the group with the given id, name, and worker.
    test_thread&
    emplace(
        size_type   _id,
        std::string _name,
        worker_fn   _worker
    )
    {
        m_threads.emplace_back(_id,
                               static_cast<std::string&&>(_name),
                               static_cast<worker_fn&&>(_worker));

        return m_threads.back();
    }

    // emplace (anonymous)
    test_thread&
    emplace(
        size_type _id,
        worker_fn _worker
    )
    {
        m_threads.emplace_back(_id,
                               static_cast<worker_fn&&>(_worker));

        return m_threads.back();
    }

    // emplace (auto-id)
    //   convenience: assigns the next sequential id.
    test_thread&
    emplace(
        worker_fn _worker
    )
    {
        size_type next_id = m_threads.size();

        m_threads.emplace_back(next_id,
                               static_cast<worker_fn&&>(_worker));

        return m_threads.back();
    }

    // size
    size_type
    size() const D_NOEXCEPT
    {
        return m_threads.size();
    }

    // empty
    bool
    empty() const D_NOEXCEPT
    {
        return m_threads.empty();
    }

    // at
    test_thread&
    at(
        size_type _index
    )
    {
        return m_threads.at(_index);
    }

    const test_thread&
    at(
        size_type _index
    ) const
    {
        return m_threads.at(_index);
    }

    // clear
    //   joins all threads (if any) and removes them from
    // the group.
    void
    clear()
    {
        join_all();
        m_threads.clear();

        return;
    }

    // -----------------------------------------------------------------
    //  bulk lifecycle
    // -----------------------------------------------------------------

    // start_all
    //   starts every thread in the group.
    void
    start_all()
    {
        for (auto& t : m_threads)
        {
            t.start();
        }

        return;
    }

    // join_all
    //   joins every thread in the group.
    void
    join_all()
    {
        for (auto& t : m_threads)
        {
            t.join();
        }

        return;
    }

    // try_join_all_for
    //   attempts to join all threads within _timeout.
    // Returns true if every thread joined.  Threads that
    // did not finish are left running.
    template<typename _Rep,
             typename _Period>
    bool
    try_join_all_for(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        bool all_joined = true;

        for (auto& t : m_threads)
        {
            if (!t.try_join_for(_timeout))
            {
                all_joined = false;
            }
        }

        return all_joined;
    }

    // -----------------------------------------------------------------
    //  aggregate state queries
    // -----------------------------------------------------------------

    // any_failed
    //   returns true if at least one thread captured an
    // exception.
    bool
    any_failed() const D_NOEXCEPT
    {
        for (const auto& t : m_threads)
        {
            if (t.failed())
            {
                return true;
            }
        }

        return false;
    }

    // all_completed
    //   returns true if every thread finished without
    // exception.
    bool
    all_completed() const D_NOEXCEPT
    {
        for (const auto& t : m_threads)
        {
            if (!t.completed())
            {
                return false;
            }
        }

        return !m_threads.empty();
    }

    // failure_count
    //   returns the number of threads that captured
    // exceptions.
    size_type
    failure_count() const D_NOEXCEPT
    {
        size_type count = 0;

        for (const auto& t : m_threads)
        {
            if (t.failed())
            {
                ++count;
            }
        }

        return count;
    }

    // first_exception
    //   returns the first captured exception_ptr in id
    // order, or nullptr if none.
    std::exception_ptr
    first_exception() const D_NOEXCEPT
    {
        for (const auto& t : m_threads)
        {
            if (t.exception())
            {
                return t.exception();
            }
        }

        return nullptr;
    }

    // rethrow_first_failure
    //   rethrows the first captured exception, if any.
    void
    rethrow_first_failure() const
    {
        std::exception_ptr ep = first_exception();

        if (ep)
        {
            std::rethrow_exception(ep);
        }

        return;
    }

    // -----------------------------------------------------------------
    //  iteration
    // -----------------------------------------------------------------

    using iterator       = std::vector<test_thread>::iterator;
    using const_iterator = std::vector<test_thread>::const_iterator;

    iterator       begin()        D_NOEXCEPT { return m_threads.begin();  }
    const_iterator begin()  const D_NOEXCEPT { return m_threads.begin();  }
    iterator       end()          D_NOEXCEPT { return m_threads.end();    }
    const_iterator end()    const D_NOEXCEPT { return m_threads.end();    }
    const_iterator cbegin() const D_NOEXCEPT { return m_threads.cbegin(); }
    const_iterator cend()   const D_NOEXCEPT { return m_threads.cend();   }

private:
    std::vector<test_thread> m_threads;
};

#endif  // C++11


///////////////////////////////////////////////////////////////////////////////
///                IV.  FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// make_test_thread
//   function: convenience constructor for an anonymous
// test_thread bound to a worker callable.
template<typename _Worker>
inline test_thread
make_test_thread(
    std::size_t _id,
    _Worker&&   _worker
)
{
    return test_thread(_id,
                       test_thread::worker_fn(
                           static_cast<_Worker&&>(_worker)));
}

// make_named_test_thread
//   function: convenience constructor for a named
// test_thread.
template<typename _Worker>
inline test_thread
make_named_test_thread(
    std::size_t _id,
    std::string _name,
    _Worker&&   _worker
)
{
    return test_thread(_id,
                       static_cast<std::string&&>(_name),
                       test_thread::worker_fn(
                           static_cast<_Worker&&>(_worker)));
}

#endif  // C++11


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_THREAD_