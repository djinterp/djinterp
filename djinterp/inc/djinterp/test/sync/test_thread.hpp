/******************************************************************************
* djinterp [test]                                              test_thread.hpp
*
*   Exception-capturing test thread: a managed thread wrapper for the
* DTest framework that captures exceptions, records timings, and
* preserves a thread identity (numeric id, optional name) for use in
* test reporting.
*
*   The class is intentionally thin - it adds three things to a raw
* std::thread:
*
*     1. EXCEPTION CAPTURE
*        Any exception thrown by the user-supplied callable is caught
*        and stored as a std::exception_ptr.  The harness can rethrow
*        on the main thread (rethrow_if_failed) or query the message
*        (exception_what).
*
*     2. TIMING
*        The thread records its start and stop time using a
*        std::chrono::steady_clock, exposing started_at(),
*        stopped_at(), and elapsed().
*
*     3. IDENTITY
*        Each thread carries a logical id (size_type) assigned by the
*        runner and an optional human-readable name.
*
*   PORTABILITY:
*   Requires C++11 or later for <thread> and <chrono>.  On C++98/03
* the class compiles to a single-threaded stub: every operation runs
* synchronously on the calling thread and start()/join() are no-ops.
*
*   This stub mode is intentional: it lets the same test code build
* against the null_lock_policy / single-threaded path without
* preprocessor branching at the call site.
*
*
*   FOLDED MODULES (2026.07):
*   The SFINAE trait surface (formerly test_thread_traits.hpp) and the
* C++20 concept layer (formerly test_thread_concepts.hpp) now live in this
* header under namespace djinterp::test::traits, guarded for C++11+ and
* C++20+ respectively.  The two standalone headers are retired - include
* this header instead.  test_thread itself is unchanged and still degrades
* to the single-threaded C++98 stub.
*
*
* TABLE OF CONTENTS
* =================
* I.    THREAD STATE
* II.   TEST THREAD
* III.  THREAD GROUP
* IV.   FACTORY HELPERS
* V.    THREADSAFE-TESTABLE TRAITS      (folded: test_thread_traits.hpp)
* VI.   THREADSAFE-TESTABLE CONCEPTS    (folded: test_thread_concepts.hpp)
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
    #include <type_traits>
#endif

// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/sync/condvar.hpp"
#include "../test_common.hpp"

// --- folded trait/concept dependencies (C++11+) ---
//   Pulled in only for the traits/concepts sections below; the C++98
// stub path needs none of them.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "../../core/meta/type_traits.hpp"
    #include "../../core/container/meta/threadsafe_container_traits.hpp"
    #include "../../core/container/meta/concurrency_strategy_traits.hpp"
#endif


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

            d_thread_yield();
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



///////////////////////////////////////////////////////////////////////////////
///        V.   THREADSAFE-TESTABLE TRAITS                                   ///
///             (folded from test_thread_traits.hpp; the internal            ///
///              I-IX numbering below is that module's own sub-TOC)          ///
///////////////////////////////////////////////////////////////////////////////
//   Structural SFINAE classification of threadsafe-testable types.
// Requires C++11 (void_t / declval / variadic templates); the
// single-threaded C++98 stub above intentionally carries none of it.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///                I.   RE-EXPORTED TRAITS FROM traits            ///
///////////////////////////////////////////////////////////////////////////////
//   These traits are defined canonically in
// concurrency_strategy_traits.hpp.  Bringing them into
// the test::traits namespace lets test-side code spell
// them with a short qualified name while guaranteeing
// the test classification stays in lockstep with the
// container classification.

using ::has_concurrency_strategy_tag;
using ::has_read_lock_method;
using ::has_write_lock_method;
using ::has_snapshot_method;
using ::has_cow_state_type;
using ::has_rcu_protected_type;
using ::has_epoch_type;
using ::has_hazard_domain_type;
using ::has_atomic_load_at;


///////////////////////////////////////////////////////////////////////////////
///                II.  RE-EXPORTED TRAITS FROM djinterp                    ///
///////////////////////////////////////////////////////////////////////////////
//   These traits live at namespace djinterp (no
// `traits` sub-namespace) in threadsafe_container_traits.hpp.
// They are re-exported here under their canonical names,
// with one rename: has_mutex_type_alias is exposed as
// has_mutex_type for symmetry with the rest of this
// header's nested-alias detection family.

using ::has_lock_policy_type;
using ::has_atomic_size_type;
using ::has_atomic_version_type;

// has_mutex_type
//   alias: presence-detection for a `mutex_type` nested
// alias.  Same predicate as upstream
// `has_mutex_type_alias`, exposed here under
// a shorter name.
template<typename _Type>
using has_mutex_type = ::has_mutex_type_alias<_Type>;


///////////////////////////////////////////////////////////////////////////////
///                III. TEST-SPECIFIC NESTED TYPE DETECTION                 ///
///////////////////////////////////////////////////////////////////////////////
//   These probes have no upstream counterpart.

// has_thread_safety_level
//   trait: true if _Type exposes a `thread_safety_level`
// nested alias.  Distinct from upstream
// `container_thread_safety_level<T>` which is a value
// extractor, not a presence probe.
template<typename _Type,
         typename = void>
struct has_thread_safety_level : std::false_type
{};

template<typename _Type>
struct has_thread_safety_level<_Type, void_t<
    typename _Type::thread_safety_level
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEST-SPECIFIC LOCK INTERFACE DETECTION              ///
///////////////////////////////////////////////////////////////////////////////
//   read_lock / write_lock are aliased from upstream;
// these add try_lock and the composite.

// has_try_lock_method
//   trait: true if _Type exposes try_lock() returning an
// optional / pointer guard.
template<typename _Type,
         typename = void>
struct has_try_lock_method : std::false_type
{};

template<typename _Type>
struct has_try_lock_method<_Type, void_t<
    decltype(std::declval<_Type&>().try_lock())
>> : std::true_type
{};

// has_full_lock_interface
//   trait: composite - both read_lock() and write_lock()
// are present.
template<typename _Type>
struct has_full_lock_interface
{
    static D_CONSTEXPR bool value =
        ( has_read_lock_method<_Type>::value &&
          has_write_lock_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST-SPECIFIC SNAPSHOT / COW DETECTION              ///
///////////////////////////////////////////////////////////////////////////////
//   snapshot() is aliased from upstream; this adds
// publish() and the composite.

// has_publish_method
//   trait: true if _Type exposes publish(...) for COW-style
// state replacement.
template<typename _Type,
         typename = void>
struct has_publish_method : std::false_type
{};

template<typename _Type>
struct has_publish_method<_Type, void_t<
    decltype(std::declval<_Type&>().publish())
>> : std::true_type
{};

// has_cow_interface
//   trait: composite - snapshot() and publish() both
// present, indicating COW-style lifecycle.
template<typename _Type>
struct has_cow_interface
{
    static D_CONSTEXPR bool value =
        ( has_snapshot_method<_Type>::value &&
          has_publish_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  TEST-SPECIFIC ATOMIC INTERFACE DETECTION            ///
///////////////////////////////////////////////////////////////////////////////
//   Upstream provides has_atomic_load_at (load by index);
// these probe the unindexed atomic interface used by
// scalar atomic wrappers.

// has_atomic_load_method
//   trait: true if _Type exposes load() as a const member.
template<typename _Type,
         typename = void>
struct has_atomic_load_method : std::false_type
{};

template<typename _Type>
struct has_atomic_load_method<_Type, void_t<
    decltype(std::declval<const _Type&>().load())
>> : std::true_type
{};

// has_atomic_store_method
//   trait: true if _Type exposes store(value) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_store_method : std::false_type
{};

template<typename _Type>
struct has_atomic_store_method<_Type, void_t<
    decltype(std::declval<_Type&>().store(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_compare_exchange_method
//   trait: true if _Type exposes
// compare_exchange_weak(expected, desired) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_compare_exchange_method : std::false_type
{};

template<typename _Type>
struct has_atomic_compare_exchange_method<_Type, void_t<
    decltype(std::declval<_Type&>().compare_exchange_weak(
        std::declval<typename _Type::value_type&>(),
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_interface
//   trait: composite - load() and store() both present.
template<typename _Type>
struct has_atomic_interface
{
    static D_CONSTEXPR bool value =
        ( has_atomic_load_method<_Type>::value &&
          has_atomic_store_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VII. STRATEGY-LEVEL COMPOSITES                           ///
///////////////////////////////////////////////////////////////////////////////
//   Test-suite-friendly composites built on the aliased
// upstream probes plus the test-specific atomic probes.

// is_locked_testable
//   trait: true if _Type appears to use lock-based
// synchronization (lock_policy_type alias OR full
// read_lock/write_lock interface).
template<typename _Type>
struct is_locked_testable
{
    static D_CONSTEXPR bool value =
        ( has_lock_policy_type<_Type>::value     ||
          has_full_lock_interface<_Type>::value );
};

// is_cow_testable
//   trait: true if _Type appears to use copy-on-write
// (cow_state_type alias OR snapshot()/publish() interface).
template<typename _Type>
struct is_cow_testable
{
    static D_CONSTEXPR bool value =
        ( has_cow_state_type<_Type>::value      ||
          has_cow_interface<_Type>::value );
};

// is_rcu_testable
//   trait: true if _Type exposes RCU markers.
template<typename _Type>
struct is_rcu_testable
{
    static D_CONSTEXPR bool value =
        has_rcu_protected_type<_Type>::value;
};

// is_hazard_testable
//   trait: true if _Type exposes hazard-pointer markers.
template<typename _Type>
struct is_hazard_testable
{
    static D_CONSTEXPR bool value =
        has_hazard_domain_type<_Type>::value;
};

// is_lock_free_testable
//   trait: true if _Type appears to be lock-free (atomic
// interface OR hazard / RCU strategy).
template<typename _Type>
struct is_lock_free_testable
{
    static D_CONSTEXPR bool value =
        ( has_atomic_interface<_Type>::value ||
          is_hazard_testable<_Type>::value   ||
          is_rcu_testable<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. COMBINED CLASSIFICATION                            ///
///////////////////////////////////////////////////////////////////////////////

// is_threadsafe_testable
//   trait: true if _Type is recognized as threadsafe under
// any supported strategy.
template<typename _Type>
struct is_threadsafe_testable
{
    static D_CONSTEXPR bool value =
        ( is_locked_testable<_Type>::value          ||
          is_cow_testable<_Type>::value             ||
          is_rcu_testable<_Type>::value             ||
          is_hazard_testable<_Type>::value          ||
          has_atomic_interface<_Type>::value        ||
          has_concurrency_strategy_tag<_Type>::value ||
          has_thread_safety_level<_Type>::value );
};

// thread_test_class
//   struct: comprehensive structural classification of a
// type for the threadsafe testing harness.  Every flag
// is independent and may be queried in isolation.
template<typename _Type>
struct thread_test_class
{
    // nested type aliases
    static D_CONSTEXPR bool has_safety_level =
        has_thread_safety_level<_Type>::value;
    static D_CONSTEXPR bool has_strategy_tag =
        has_concurrency_strategy_tag<_Type>::value;
    static D_CONSTEXPR bool has_lock_policy =
        has_lock_policy_type<_Type>::value;
    static D_CONSTEXPR bool has_mutex =
        has_mutex_type<_Type>::value;
    static D_CONSTEXPR bool has_cow_state =
        has_cow_state_type<_Type>::value;
    static D_CONSTEXPR bool has_rcu_protected =
        has_rcu_protected_type<_Type>::value;
    static D_CONSTEXPR bool has_hazard_domain =
        has_hazard_domain_type<_Type>::value;

    // lock interface
    static D_CONSTEXPR bool has_read_lock =
        has_read_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_write_lock =
        has_write_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_try_lock =
        has_try_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_full_lock =
        has_full_lock_interface<_Type>::value;

    // cow interface
    static D_CONSTEXPR bool has_snapshot =
        has_snapshot_method<_Type>::value;
    static D_CONSTEXPR bool has_publish =
        has_publish_method<_Type>::value;
    static D_CONSTEXPR bool has_cow =
        has_cow_interface<_Type>::value;

    // atomic interface
    static D_CONSTEXPR bool has_load =
        has_atomic_load_method<_Type>::value;
    static D_CONSTEXPR bool has_store =
        has_atomic_store_method<_Type>::value;
    static D_CONSTEXPR bool has_compare_exchange =
        has_atomic_compare_exchange_method<_Type>::value;
    static D_CONSTEXPR bool has_atomic =
        has_atomic_interface<_Type>::value;

    // strategy classification
    static D_CONSTEXPR bool is_locked =
        is_locked_testable<_Type>::value;
    static D_CONSTEXPR bool is_cow =
        is_cow_testable<_Type>::value;
    static D_CONSTEXPR bool is_rcu =
        is_rcu_testable<_Type>::value;
    static D_CONSTEXPR bool is_hazard =
        is_hazard_testable<_Type>::value;
    static D_CONSTEXPR bool is_lock_free =
        is_lock_free_testable<_Type>::value;
    static D_CONSTEXPR bool is_threadsafe =
        is_threadsafe_testable<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                IX.  VARIABLE TEMPLATES                                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // re-exported _v variants from traits
    using ::has_concurrency_strategy_tag_v;
    using ::has_read_lock_method_v;
    using ::has_write_lock_method_v;
    using ::has_snapshot_method_v;
    using ::has_cow_state_type_v;
    using ::has_rcu_protected_type_v;
    using ::has_epoch_type_v;
    using ::has_hazard_domain_type_v;
    using ::has_atomic_load_at_v;

    // re-exported _v variants from djinterp
    using ::has_lock_policy_type_v;
    using ::has_atomic_size_type_v;
    using ::has_atomic_version_type_v;

    // has_mutex_type_v: same predicate as upstream
    // has_mutex_type_alias_v.
    template<typename _Type>
    D_CONSTEXPR bool has_mutex_type_v =
        ::has_mutex_type_alias_v<_Type>;

    // test-specific
    template<typename _Type>
    D_CONSTEXPR bool has_thread_safety_level_v =
        has_thread_safety_level<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_try_lock_method_v =
        has_try_lock_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_full_lock_interface_v =
        has_full_lock_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_publish_method_v =
        has_publish_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_cow_interface_v =
        has_cow_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_load_method_v =
        has_atomic_load_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_store_method_v =
        has_atomic_store_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_compare_exchange_method_v =
        has_atomic_compare_exchange_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_interface_v =
        has_atomic_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_locked_testable_v =
        is_locked_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_cow_testable_v =
        is_cow_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_rcu_testable_v =
        is_rcu_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_hazard_testable_v =
        is_hazard_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_lock_free_testable_v =
        is_lock_free_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_threadsafe_testable_v =
        is_threadsafe_testable<_Type>::value;

#endif  // variable templates

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
///        VI.  THREADSAFE-TESTABLE CONCEPTS                                 ///
///             (folded from test_thread_concepts.hpp; C++20 concepts        ///
///              layered over the traits above)                             ///
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///                I.   NESTED TYPE ALIAS CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

// safety_level_aware_type
//   concept: the type exposes a thread_safety_level alias.
template<typename _Type>
concept safety_level_aware_type =
    has_thread_safety_level<_Type>::value;

// strategy_tagged_type
//   concept: the type exposes a concurrency_strategy_tag.
template<typename _Type>
concept strategy_tagged_type =
    has_concurrency_strategy_tag<_Type>::value;

// lock_policy_aware_type
//   concept: the type exposes a lock_policy_type alias.
template<typename _Type>
concept lock_policy_aware_type =
    has_lock_policy_type<_Type>::value;

// mutex_aware_type
//   concept: the type exposes a mutex_type alias.
template<typename _Type>
concept mutex_aware_type =
    has_mutex_type<_Type>::value;

// cow_state_aware_type
//   concept: the type exposes a cow_state_type alias.
template<typename _Type>
concept cow_state_aware_type =
    has_cow_state_type<_Type>::value;

// rcu_protected_aware_type
//   concept: the type exposes an rcu_protected_type alias.
template<typename _Type>
concept rcu_protected_aware_type =
    has_rcu_protected_type<_Type>::value;

// hazard_domain_aware_type
//   concept: the type exposes a hazard_domain_type alias.
template<typename _Type>
concept hazard_domain_aware_type =
    has_hazard_domain_type<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.  LOCK INTERFACE CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// read_lockable_type
//   concept: the type exposes read_lock() as a const member.
template<typename _Type>
concept read_lockable_type =
    has_read_lock_method<_Type>::value;

// write_lockable_type
//   concept: the type exposes write_lock() as a mutable member.
template<typename _Type>
concept write_lockable_type =
    has_write_lock_method<_Type>::value;

// try_lockable_type
//   concept: the type exposes try_lock().
template<typename _Type>
concept try_lockable_type =
    has_try_lock_method<_Type>::value;

// fully_lockable_type
//   concept: the type exposes both read_lock() and
// write_lock() - full reader/writer interface.
template<typename _Type>
concept fully_lockable_type =
    has_full_lock_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III. SNAPSHOT / COW CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// snapshottable_type
//   concept: the type exposes snapshot().
template<typename _Type>
concept snapshottable_type =
    has_snapshot_method<_Type>::value;

// publishable_type
//   concept: the type exposes publish().
template<typename _Type>
concept publishable_type =
    has_publish_method<_Type>::value;

// cow_capable_type
//   concept: the type exposes the full COW interface.
template<typename _Type>
concept cow_capable_type =
    has_cow_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IV.  ATOMIC INTERFACE CONCEPTS                           ///
///////////////////////////////////////////////////////////////////////////////

// atomic_loadable_type
//   concept: the type exposes load().
template<typename _Type>
concept atomic_loadable_type =
    has_atomic_load_method<_Type>::value;

// atomic_storable_type
//   concept: the type exposes store(value).
template<typename _Type>
concept atomic_storable_type =
    has_atomic_store_method<_Type>::value;

// atomic_cas_type
//   concept: the type exposes compare_exchange_weak.
template<typename _Type>
concept atomic_cas_type =
    has_atomic_compare_exchange_method<_Type>::value;

// atomic_capable_type
//   concept: the type exposes the full atomic interface
// (load + store).
template<typename _Type>
concept atomic_capable_type =
    has_atomic_interface<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                V.   STRATEGY CLASSIFICATION CONCEPTS                    ///
///////////////////////////////////////////////////////////////////////////////

// locked_testable_type
//   concept: the type uses lock-based synchronization.
template<typename _Type>
concept locked_testable_type =
    is_locked_testable<_Type>::value;

// cow_testable_type
//   concept: the type uses copy-on-write synchronization.
template<typename _Type>
concept cow_testable_type =
    is_cow_testable<_Type>::value;

// rcu_testable_type
//   concept: the type uses RCU / epoch-based reclamation.
template<typename _Type>
concept rcu_testable_type =
    is_rcu_testable<_Type>::value;

// hazard_testable_type
//   concept: the type uses hazard-pointer protection.
template<typename _Type>
concept hazard_testable_type =
    is_hazard_testable<_Type>::value;

// lock_free_testable_type
//   concept: the type appears lock-free (atomic OR hazard
// OR RCU).
template<typename _Type>
concept lock_free_testable_type =
    is_lock_free_testable<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.  AGGREGATE PROFILE CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

// threadsafe_testable_type
//   concept: shorthand for any type recognized as
// threadsafe by the trait system.
template<typename _Type>
concept threadsafe_testable_type =
    is_threadsafe_testable<_Type>::value;

// classified_thread_test_type
//   concept: shorthand for any type recognized by
// thread_test_class as threadsafe.
template<typename _Type>
concept classified_thread_test_type =
    thread_test_class<_Type>::is_threadsafe;

// fully_described_threadsafe_type
//   concept: a thread-safe type that exposes both type
// aliases AND a method-level interface - the richest
// classification suitable for full-spectrum tests.
template<typename _Type>
concept fully_described_threadsafe_type =
    threadsafe_testable_type<_Type>          &&
    ( safety_level_aware_type<_Type>         ||
      strategy_tagged_type<_Type> )          &&
    ( fully_lockable_type<_Type>             ||
      cow_capable_type<_Type>                ||
      atomic_capable_type<_Type> );

// race_testable_type
//   concept: a type that can meaningfully be subjected to
// race-probing - exposes either a reader/writer interface
// or atomic loads.
template<typename _Type>
concept race_testable_type =
    fully_lockable_type<_Type> ||
    snapshottable_type<_Type>  ||
    atomic_loadable_type<_Type>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // traits

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER  (folded traits + concepts)


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_THREAD_
