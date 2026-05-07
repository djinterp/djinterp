/******************************************************************************
* djinterp [test]                                           test_deadlock.hpp
*
*   Deadlock detection and timeout watchdog primitives for the DTest
* multithreading harness.  Provides two complementary mechanisms:
*
*     1. DEADLOCK WATCHDOG
*        A timer-based fail-fast guard.  Arm the watchdog with a
*        timeout; if disarm() is not called within the deadline the
*        watchdog fires its on-fire callback (default: records a
*        violation and sets a stop flag).  Useful for wrapping any
*        join, lock, or condition-variable wait that should never
*        block indefinitely.
*
*     2. LOCK ORDER TRACKER
*        Records the order in which locks are acquired by each
*        thread (using thread-local-storage keyed by lock id),
*        builds an acquisition order graph, and detects cycles
*        indicative of potential AB-BA deadlocks.  Detection is
*        offline - call analyze() after the test completes and
*        the tracker reports any cycles in the global ordering.
*
*   Both mechanisms produce a deadlock_report convertible to a
* basic_test for the DTest tree via to_test_object().
*
*   USE WITH CARE:
*   Watchdog firing does NOT abort the program by default - it
* records the event and signals the SUT to stop cooperatively
* via an atomic stop flag.  An optional fatal mode can be enabled
* to call std::abort() when a deadlock is detected, but this is
* not the default because tests should fail gracefully.
*
*   PORTABILITY:
*   Requires C++11 or later.  C++98 builds receive degenerate
* stubs (single-threaded; watchdog never fires; tracker is a no-op).
*
*
* TABLE OF CONTENTS
* =================
* I.    DEADLOCK REPORT
* II.   DEADLOCK WATCHDOG
* III.  LOCK ORDER TRACKER
* IV.   SCOPED WATCHDOG (RAII)
* V.    FACTORY HELPERS
*
*
* path:      /inc/djinterp/test/test_deadlock.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_DEADLOCK_
#define DJINTERP_TEST_DEADLOCK_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <condition_variable>
    #include <functional>
    #include <mutex>
    #include <thread>
    #include <utility>
    #include <vector>
#else
    #include <vector>
#endif

// djinterp
#include "../core/djinterp.hpp"
#include "../sync/atomic.hpp"
#include "../sync/condvar.hpp"
#include "./test_common.hpp"
#include "./test_object.hpp"


NS_DJINTERP
NS_TEST


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- threadsafe foundation wrappers used by this module ---
using ::portable_condvar;
using ::exclusive_lock_policy;

///////////////////////////////////////////////////////////////////////////////
///                I.   DEADLOCK REPORT                                     ///
///////////////////////////////////////////////////////////////////////////////

// deadlock_event_kind
//   enum: classification of a recorded deadlock event.
enum class deadlock_event_kind
{
    timeout       = 0,   // watchdog fired
    cycle         = 1,   // lock-order cycle detected
    abandoned     = 2,   // join failed with timeout
    user_reported = 3    // explicit report() call
};

// deadlock_event
//   struct: a single recorded deadlock-related occurrence.
struct deadlock_event
{
    deadlock_event_kind                                    kind;
    std::string                                            description;
    std::chrono::steady_clock::time_point                  when;
    std::size_t                                            thread_id;
    std::size_t                                            lock_id_a;
    std::size_t                                            lock_id_b;

    deadlock_event()
        : kind(deadlock_event_kind::user_reported),
          description(),
          when(std::chrono::steady_clock::now()),
          thread_id(0),
          lock_id_a(0),
          lock_id_b(0)
    {}
};

// deadlock_report
//   struct: aggregate result from a deadlock_watchdog or
// lock_order_tracker run.
struct deadlock_report
{
    bool                          fired;
    std::size_t                   timeout_count;
    std::size_t                   cycle_count;
    std::vector<deadlock_event>   events;
    std::chrono::nanoseconds      configured_timeout;

    deadlock_report()
        : fired(false),
          timeout_count(0),
          cycle_count(0),
          events(),
          configured_timeout(std::chrono::nanoseconds(0))
    {}

    // ok
    //   returns true if no deadlock condition was detected.
    bool
    ok() const D_NOEXCEPT
    {
        return ( (!fired)            &&
                 (timeout_count == 0) &&
                 (cycle_count   == 0) );
    }

    // event_count
    //   returns the total number of recorded events.
    std::size_t
    event_count() const D_NOEXCEPT
    {
        return events.size();
    }

    // to_test_object
    //   converts this report into a basic_test instance.
    // The test passes iff no deadlock condition was detected.
    basic_test
    to_test_object(
        test_type_id _kind = 0,
        const char*  _name = "deadlock_test"
    ) const
    {
        return basic_test(_kind, ok(), _name);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  DEADLOCK WATCHDOG                                   ///
///////////////////////////////////////////////////////////////////////////////

// deadlock_watchdog
//   class: a one-shot timer that fires if not disarmed in
// time.  Spawns a watcher thread on arm(); the watcher
// sleeps for the configured timeout and then checks the
// disarm flag.  If still armed, it records a timeout
// event, sets the stop flag, and invokes the on-fire
// callback if one is set.
//
// Usage:
//   deadlock_watchdog wd;
//   wd.set_timeout(std::chrono::seconds(5));
//   wd.set_description("waiting for lock acquisition");
//   wd.arm();
//   // ... operation that should complete within 5s ...
//   wd.disarm();
//   auto report = wd.report();
//   suite.adopt(report.to_test_object(0, "lock acquisition"));
//
// Re-arming:
//   A watchdog may be reused.  Call arm() again after disarm()
// to start a new countdown.  Each fire is appended to the
// internal report.
class deadlock_watchdog
{
public:
    using duration_type    = std::chrono::nanoseconds;
    using on_fire_callback = std::function<void()>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    deadlock_watchdog()
        : m_timeout(std::chrono::seconds(30)),
          m_description(),
          m_on_fire(),
          m_fatal_on_fire(false),
          m_armed(),
          m_disarmed(),
          m_stop_requested(),
          m_mutex(),
          m_cv(),
          m_report(),
          m_watcher()
    {
        m_armed.store(false);
        m_disarmed.store(false);
        m_stop_requested.store(false);

        m_report.configured_timeout = m_timeout;

        return;
    }

    ~deadlock_watchdog()
    {
        // best-effort cleanup: signal disarm and join
        if (m_armed.load())
        {
            disarm();
        }

        return;
    }

    // disable copy
    deadlock_watchdog(const deadlock_watchdog&) D_DELETE;
    deadlock_watchdog&
    operator=(const deadlock_watchdog&) D_DELETE;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    // set_timeout
    //   sets the watchdog deadline.  Must be called before
    // arm() - changes after arming are ignored until the
    // next arm cycle.
    template<typename _Rep,
             typename _Period>
    void
    set_timeout(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        m_timeout = std::chrono::duration_cast<duration_type>(
                        _timeout);

        m_report.configured_timeout = m_timeout;

        return;
    }

    // set_description
    //   sets an optional human-readable description for
    // events recorded by this watchdog.
    void
    set_description(
        std::string _description
    )
    {
        m_description = std::move(_description);

        return;
    }

    // set_on_fire
    //   installs a callback to invoke when the watchdog
    // fires.  The callback runs on the watcher thread.
    void
    set_on_fire(
        on_fire_callback _callback
    )
    {
        m_on_fire = std::move(_callback);

        return;
    }

    // set_fatal_on_fire
    //   if true, std::abort() is called when the watchdog
    // fires.  Default: false (graceful failure).
    void
    set_fatal_on_fire(
        bool _fatal
    ) D_NOEXCEPT
    {
        m_fatal_on_fire = _fatal;

        return;
    }

    // -----------------------------------------------------------------
    //  arm / disarm
    // -----------------------------------------------------------------

    // arm
    //   starts the watchdog.  Spawns a watcher thread that
    // will fire after the configured timeout unless disarm()
    // is called first.  No-op if already armed.
    void
    arm()
    {
        if (m_armed.load())
        {
            return;
        }

        m_disarmed.store(false);
        m_armed.store(true);

        m_watcher = std::thread(&deadlock_watchdog::watcher_loop,
                                this);

        return;
    }

    // disarm
    //   cancels the watchdog.  If the watcher is still
    // sleeping, it wakes up and exits without firing.
    // Joins the watcher thread before returning.
    void
    disarm()
    {
        if (!m_armed.load())
        {
            return;
        }

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_disarmed.store(true);

            m_cv.notify_all();
        }

        if (m_watcher.joinable())
        {
            m_watcher.join();
        }

        m_armed.store(false);

        return;
    }

    // -----------------------------------------------------------------
    //  status
    // -----------------------------------------------------------------

    // armed
    //   returns true if the watchdog is currently counting
    // down.
    bool
    armed() const D_NOEXCEPT
    {
        return m_armed.load();
    }

    // fired
    //   returns true if the watchdog has fired at least once
    // since construction.
    bool
    fired() const D_NOEXCEPT
    {
        return m_report.fired;
    }

    // stop_requested
    //   returns true if the watchdog has signalled the SUT
    // to stop.  Threads under test should poll this flag
    // periodically and exit cooperatively.
    bool
    stop_requested() const D_NOEXCEPT
    {
        return m_stop_requested.load();
    }

    // -----------------------------------------------------------------
    //  reporting
    // -----------------------------------------------------------------

    // report
    //   returns the accumulated deadlock_report.  Safe to
    // call after disarm().
    const deadlock_report&
    report() const D_NOEXCEPT
    {
        return m_report;
    }

    // reset
    //   clears the report and stop flag.  Watchdog must be
    // disarmed before calling.
    void
    reset()
    {
        if (m_armed.load())
        {
            return;
        }

        m_report = deadlock_report();
        m_report.configured_timeout = m_timeout;
        m_stop_requested.store(false);

        return;
    }

private:
    // -----------------------------------------------------------------
    //  watcher loop
    // -----------------------------------------------------------------

    // watcher_loop
    //   runs on the watcher thread.  Sleeps until either
    // disarmed or the timeout expires.  On timeout, records
    // a fire event, sets the stop flag, invokes the
    // callback, and (if fatal) aborts.
    void
    watcher_loop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        bool disarmed_in_time = m_cv.wait_for(
            lock,
            m_timeout,
            [this]()
            {
                return m_disarmed.load();
            });

        // exit cleanly if disarmed
        if (disarmed_in_time)
        {
            return;
        }

        // fire path
        deadlock_event ev;
        ev.kind        = deadlock_event_kind::timeout;
        ev.description = m_description.empty()
                       ? std::string("watchdog timeout")
                       : m_description;
        ev.when        = std::chrono::steady_clock::now();

        m_report.events.push_back(std::move(ev));
        m_report.timeout_count += 1;
        m_report.fired = true;

        m_stop_requested.store(true);

        // unlock before invoking the callback to avoid
        // deadlocking the user's code if it tries to
        // disarm or query us.
        lock.unlock();

        if (m_on_fire)
        {
            m_on_fire();
        }

        if (m_fatal_on_fire)
        {
            std::abort();
        }

        return;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    duration_type           m_timeout;
    std::string             m_description;
    on_fire_callback        m_on_fire;
    bool                    m_fatal_on_fire;
    std::atomic<bool>       m_armed;
    std::atomic<bool>       m_disarmed;
    std::atomic<bool>       m_stop_requested;
    std::mutex              m_mutex;
    portable_condvar<exclusive_lock_policy>
                            m_cv;
    deadlock_report         m_report;
    std::thread             m_watcher;
};


///////////////////////////////////////////////////////////////////////////////
///                III. LOCK ORDER TRACKER                                  ///
///////////////////////////////////////////////////////////////////////////////

// lock_order_tracker
//   class: records per-thread lock acquisition orders and
// detects cycles in the global lock-acquisition order graph.
//
//   When a thread acquires lock A then lock B (without
// having released A), an edge A -> B is added to the graph.
// If at any point the reverse edge B -> A also exists in
// the graph (added by another thread), a potential AB-BA
// deadlock is reported.
//
// Usage:
//   lock_order_tracker tracker;
//   // ... in each thread, around each lock acquisition:
//   tracker.on_acquire(thread_id, lock_id_a);
//   // ... critical section ...
//   tracker.on_acquire(thread_id, lock_id_b);
//   // ... critical section ...
//   tracker.on_release(thread_id, lock_id_b);
//   tracker.on_release(thread_id, lock_id_a);
//
//   auto report = tracker.analyze();
//   if (!report.ok()) { /* deadlock potential detected */ }
class lock_order_tracker
{
public:
    using lock_id_type   = std::size_t;
    using thread_id_type = std::size_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    lock_order_tracker()
        : m_mutex(),
          m_thread_stacks(),
          m_edges(),
          m_report()
    {}

    // disable copy
    lock_order_tracker(const lock_order_tracker&) D_DELETE;
    lock_order_tracker&
    operator=(const lock_order_tracker&) D_DELETE;

    // -----------------------------------------------------------------
    //  recording
    // -----------------------------------------------------------------

    // on_acquire
    //   records that _thread acquired _lock.  If the thread
    // already holds locks, edges are added from each held
    // lock to _lock.  If a reverse edge already exists, a
    // cycle event is recorded immediately.
    void
    on_acquire(
        thread_id_type _thread,
        lock_id_type   _lock
    )
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        // ensure this thread has a stack entry
        thread_stack* stack = ensure_stack_unlocked(_thread);

        // for every currently-held lock, record an edge
        for (std::size_t i = 0; i < stack->held.size(); ++i)
        {
            lock_id_type held = stack->held[i];

            add_edge_unlocked(held, _lock);

            // detect immediate cycle: reverse edge present
            if (has_edge_unlocked(_lock, held))
            {
                deadlock_event ev;
                ev.kind        = deadlock_event_kind::cycle;
                ev.description = std::string(
                    "AB-BA cycle: edge in both directions");
                ev.when        = std::chrono::steady_clock::now();
                ev.thread_id   = _thread;
                ev.lock_id_a   = held;
                ev.lock_id_b   = _lock;

                m_report.events.push_back(std::move(ev));
                m_report.cycle_count += 1;
                m_report.fired        = true;
            }
        }

        // push the new lock
        stack->held.push_back(_lock);

        return;
    }

    // on_release
    //   records that _thread released _lock.  Removes the
    // lock from the thread's held stack.  No edges are
    // removed - the order graph is monotonic.
    void
    on_release(
        thread_id_type _thread,
        lock_id_type   _lock
    )
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        thread_stack* stack = find_stack_unlocked(_thread);

        if (stack == nullptr)
        {
            return;
        }

        // remove the lock from the held stack (most-recent first)
        for (std::size_t i = stack->held.size(); i > 0; --i)
        {
            std::size_t idx = i - 1;

            if (stack->held[idx] == _lock)
            {
                stack->held.erase(stack->held.begin() + idx);

                break;
            }
        }

        return;
    }

    // -----------------------------------------------------------------
    //  analysis
    // -----------------------------------------------------------------

    // analyze
    //   performs a full cycle search on the order graph and
    // returns an updated report.  Cycles already detected
    // by on_acquire are preserved; analyze() additionally
    // walks the graph to find longer cycles.
    deadlock_report
    analyze()
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        // collect all unique nodes
        std::vector<lock_id_type> nodes;

        for (std::size_t i = 0; i < m_edges.size(); ++i)
        {
            push_unique_unlocked(nodes, m_edges[i].from);
            push_unique_unlocked(nodes, m_edges[i].to);
        }

        // DFS-based cycle detection
        std::vector<int> color(nodes.size(), 0);
            // 0 = white, 1 = gray (on stack), 2 = black (done)

        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            if (color[i] == 0)
            {
                std::vector<lock_id_type> path;

                if (dfs_cycle_unlocked(i, nodes, color, path))
                {
                    deadlock_event ev;
                    ev.kind        = deadlock_event_kind::cycle;
                    ev.description = std::string(
                        "graph cycle detected during analysis");
                    ev.when        = std::chrono::steady_clock::now();

                    if (!path.empty())
                    {
                        ev.lock_id_a = path.front();
                        ev.lock_id_b = path.back();
                    }

                    m_report.events.push_back(std::move(ev));
                    m_report.cycle_count += 1;
                    m_report.fired        = true;
                }
            }
        }

        return m_report;
    }

    // report
    //   returns the most recent report without re-analyzing.
    const deadlock_report&
    report() const D_NOEXCEPT
    {
        return m_report;
    }

    // edge_count
    //   returns the number of unique acquisition-order edges
    // recorded.
    std::size_t
    edge_count() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        return m_edges.size();
    }

    // reset
    //   clears all recorded edges, thread stacks, and the
    // report.
    void
    reset()
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        m_thread_stacks.clear();
        m_edges.clear();
        m_report = deadlock_report();

        return;
    }

private:
    // -----------------------------------------------------------------
    //  internal types
    // -----------------------------------------------------------------

    struct thread_stack
    {
        thread_id_type             id;
        std::vector<lock_id_type>  held;
    };

    struct edge
    {
        lock_id_type from;
        lock_id_type to;
    };

    // -----------------------------------------------------------------
    //  internal helpers (caller holds m_mutex)
    // -----------------------------------------------------------------

    thread_stack*
    find_stack_unlocked(
        thread_id_type _thread
    )
    {
        for (std::size_t i = 0; i < m_thread_stacks.size(); ++i)
        {
            if (m_thread_stacks[i].id == _thread)
            {
                return &m_thread_stacks[i];
            }
        }

        return nullptr;
    }

    thread_stack*
    ensure_stack_unlocked(
        thread_id_type _thread
    )
    {
        thread_stack* existing = find_stack_unlocked(_thread);

        if (existing != nullptr)
        {
            return existing;
        }

        thread_stack fresh;
        fresh.id = _thread;

        m_thread_stacks.push_back(std::move(fresh));

        return &m_thread_stacks.back();
    }

    bool
    has_edge_unlocked(
        lock_id_type _from,
        lock_id_type _to
    ) const
    {
        for (std::size_t i = 0; i < m_edges.size(); ++i)
        {
            if ( (m_edges[i].from == _from) &&
                 (m_edges[i].to   == _to) )
            {
                return true;
            }
        }

        return false;
    }

    void
    add_edge_unlocked(
        lock_id_type _from,
        lock_id_type _to
    )
    {
        if (has_edge_unlocked(_from, _to))
        {
            return;
        }

        edge e;
        e.from = _from;
        e.to   = _to;

        m_edges.push_back(e);

        return;
    }

    static void
    push_unique_unlocked(
        std::vector<lock_id_type>& _nodes,
        lock_id_type               _id
    )
    {
        for (std::size_t i = 0; i < _nodes.size(); ++i)
        {
            if (_nodes[i] == _id)
            {
                return;
            }
        }

        _nodes.push_back(_id);

        return;
    }

    static std::size_t
    index_of_unlocked(
        const std::vector<lock_id_type>& _nodes,
        lock_id_type                     _id
    )
    {
        for (std::size_t i = 0; i < _nodes.size(); ++i)
        {
            if (_nodes[i] == _id)
            {
                return i;
            }
        }

        return _nodes.size();
    }

    bool
    dfs_cycle_unlocked(
        std::size_t                       _node_index,
        const std::vector<lock_id_type>&  _nodes,
        std::vector<int>&                 _color,
        std::vector<lock_id_type>&        _path
    )
    {
        _color[_node_index] = 1;
        _path.push_back(_nodes[_node_index]);

        lock_id_type from = _nodes[_node_index];

        for (std::size_t i = 0; i < m_edges.size(); ++i)
        {
            if (m_edges[i].from != from)
            {
                continue;
            }

            std::size_t to_idx = index_of_unlocked(_nodes,
                                                    m_edges[i].to);

            if (to_idx >= _nodes.size())
            {
                continue;
            }

            // back-edge to a gray node => cycle
            if (_color[to_idx] == 1)
            {
                _path.push_back(_nodes[to_idx]);

                return true;
            }

            if (_color[to_idx] == 0)
            {
                if (dfs_cycle_unlocked(to_idx, _nodes,
                                        _color, _path))
                {
                    return true;
                }
            }
        }

        _color[_node_index] = 2;
        _path.pop_back();

        return false;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    mutable std::mutex          m_mutex;
    std::vector<thread_stack>   m_thread_stacks;
    std::vector<edge>           m_edges;
    deadlock_report             m_report;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SCOPED WATCHDOG (RAII)                              ///
///////////////////////////////////////////////////////////////////////////////

// scoped_deadlock_watchdog
//   class: RAII wrapper that arms a deadlock_watchdog on
// construction and disarms it on destruction.  Convenient
// for guarding a critical region:
//
//   {
//       scoped_deadlock_watchdog guard(wd, std::chrono::seconds(2));
//       // ... operation that should complete in 2s ...
//   }   // wd disarmed automatically
class scoped_deadlock_watchdog
{
public:
    template<typename _Rep,
             typename _Period>
    scoped_deadlock_watchdog(
        deadlock_watchdog&                          _wd,
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
        : m_wd(_wd)
    {
        m_wd.set_timeout(_timeout);
        m_wd.arm();

        return;
    }

    ~scoped_deadlock_watchdog()
    {
        m_wd.disarm();

        return;
    }

    scoped_deadlock_watchdog(const scoped_deadlock_watchdog&) D_DELETE;
    scoped_deadlock_watchdog&
    operator=(const scoped_deadlock_watchdog&) D_DELETE;

private:
    deadlock_watchdog& m_wd;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_deadlock_watchdog
//   function: convenience constructor for a configured
// deadlock_watchdog.  Returns a watchdog with the given
// timeout and description set; not yet armed.
template<typename _Rep,
         typename _Period>
D_INLINE deadlock_watchdog
make_deadlock_watchdog(
    const std::chrono::duration<_Rep, _Period>& _timeout,
    std::string                                  _description = std::string()
)
{
    deadlock_watchdog wd;

    wd.set_timeout(_timeout);
    wd.set_description(std::move(_description));

    return wd;
}

#else  // !D_ENV_LANG_IS_CPP11_OR_HIGHER

///////////////////////////////////////////////////////////////////////////////
///                C++98 DEGENERATE STUBS                                   ///
///////////////////////////////////////////////////////////////////////////////
//   Single-threaded execution: the watchdog never fires
// and the lock-order tracker is a no-op.

enum deadlock_event_kind_e
{
    DEADLOCK_EVENT_TIMEOUT       = 0,
    DEADLOCK_EVENT_CYCLE         = 1,
    DEADLOCK_EVENT_ABANDONED     = 2,
    DEADLOCK_EVENT_USER_REPORTED = 3
};

struct deadlock_event
{
    deadlock_event_kind_e kind;
    const char*           description;
    std::size_t           thread_id;
    std::size_t           lock_id_a;
    std::size_t           lock_id_b;

    deadlock_event()
        : kind(DEADLOCK_EVENT_USER_REPORTED),
          description(""),
          thread_id(0),
          lock_id_a(0),
          lock_id_b(0)
    {}
};

struct deadlock_report
{
    bool        fired;
    std::size_t timeout_count;
    std::size_t cycle_count;

    deadlock_report()
        : fired(false),
          timeout_count(0),
          cycle_count(0)
    {}

    bool
    ok() const
    {
        return !fired;
    }

    basic_test
    to_test_object(
        test_type_id _kind = 0,
        const char*  _name = "deadlock_test"
    ) const
    {
        return basic_test(_kind, ok(), _name);
    }
};

class deadlock_watchdog
{
public:
    deadlock_watchdog() : m_report() {}

    void arm()    {}
    void disarm() {}

    bool fired()          const { return false; }
    bool stop_requested() const { return false; }

    const deadlock_report&
    report() const { return m_report; }

private:
    deadlock_report m_report;
};

class lock_order_tracker
{
public:
    typedef std::size_t lock_id_type;
    typedef std::size_t thread_id_type;

    lock_order_tracker() : m_report() {}

    void on_acquire(thread_id_type, lock_id_type) {}
    void on_release(thread_id_type, lock_id_type) {}

    deadlock_report analyze() { return m_report; }

    const deadlock_report&
    report() const { return m_report; }

private:
    deadlock_report m_report;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_DEADLOCK_
