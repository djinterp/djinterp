/******************************************************************************
* djinterp [container]                                threadsafe_container.hpp
*
* Foundation module for implementing thread-safe containers.
*   Provides the runtime building blocks that all threadsafe_<container>
* implementations compose from.  This sits between the raw locking
* primitives (threadsafe.hpp) and the concrete container types
* (threadsafe_array, threadsafe_tree, etc.):
*     threadsafe.hpp               - lock policies, guards, atomic utils,
*                                    hazard pointers, RCU, COW
*     threadsafe_container_traits  - compile-time detection (no runtime)
*     threadsafe_container.hpp     - THIS FILE: container-level runtime
*                                    abstractions
*     threadsafe_<type>.hpp        - concrete container implementations
*
*   Hazard pointers, epoch-based reclamation (RCU), and copy-on-write
* primitives all live in the threadsafe module proper (hazard_pointer.hpp,
* rcu.hpp, cow.hpp).  This file does NOT redefine them; concrete
* containers that need lock-free reclamation pull in those modules
* directly from threadsafe.
*
*   The module provides four layers of container-level concurrency:
*
*   LAYER 1: CRTP base (threadsafe_container_base)
*     Holds the mutex, provides RAII-scoped lock acquisition.
*     Every threadsafe container inherits from this.
*   LAYER 2: Locked accessors (locked_ref / const_locked_ref)
*     RAII handles that hold a lock and expose the underlying
*     container via operator-> / operator*.  The canonical
*     access pattern:
*       auto ref = ts_container.write_access();
*       ref->push_(42);
*       // lock released when ref goes out of scope
*   LAYER 3: Atomic container state (atomic_state)
*     Bundled atomic size + version counter for lock-free
*     metadata.  Enables optimistic reads: snapshot the
*     version, do the read, check the version hasn't changed.
*   LAYER 4: CAS retry infrastructure (exponential_off, cas_loop)
*     Exponential backoff and CAS-loop templates for building
*     lock-free and wait-free container operations.
*   Additionally: snapshot_view for safe iteration, batch_guard
*   for multi-operation transactions, and locked_range for
*   lock-held iteration.
*
* VERSIONING:
*   All features degrade gracefully across C++ standards:
*     C++98/03:  CRTP base, locked_ref (no move), batch_guard
*     C++11:     + atomic_state, exponential_off, move semantics
*     C++14:     + generic lambda locked_apply
*     C++17:     + if constexpr policy dispatch, shared_mutex,
*                  std::optional for try-lock results
*     C++20:     + concepts, std::atomic_ref, jthread-aware
*
* DEPENDENCIES:
*   threadsafe.hpp                  - lock policies, guards, atomics,
*                                     hazard pointers, RCU, COW
*   threadsafe_container_traits.hpp - compile-time detection
*   container_traits.hpp            - container classification
*
*
* path:      /inc/djinterp/core/container/threadsafe_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.29
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      threadsafe_container_base (CRTP)
II.     locked accessors (locked_ref, const_locked_ref)
III.    atomic container state
IV.     optimistic read protocol
V.      CAS retry infrastructure
VI.     snapshot view
VII.    batch guard
VIII.   locked range (safe iterator wrapper)
*/

#ifndef DJINTERP_THREADSAFE_CONTAINER_
#define DJINTERP_THREADSAFE_CONTAINER_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../sync/threadsafe.hpp"
#include "../sync/concurrency_strategy_tags.hpp"
#include "./traits/threadsafe_container_traits.hpp"
#include "./traits/container_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <functional>
    #include <utility>
    #include <vector>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>
#endif


NS_DJINTERP

// =============================================================================
// I.   threadsafe_container_base (CRTP)
// =============================================================================
// Every threadsafe container inherits from this.  It holds
// the mutable mutex (so const methods can still lock) and
// provides RAII-scoped lock acquisition via the policy's
// guard types.
//
// The mutex is declared mutable: a const container must
// still be able to acquire a read lock for safe access.
//
// On null_lock_policy, every lock method inlines to nothing.

template<typename _Derived,
         typename _Policy = default_lock_policy>
class threadsafe_container_base
{
public:
    using lock_policy_type = _Policy;
    using mutex_type       = typename _Policy::mutex_type;
    using read_guard       = typename _Policy::read_lock_type;
    using write_guard      = typename _Policy::write_lock_type;

    // concurrency_strategy_tag
    //   alias: declares all types deriving from this base
    // as lock-based strategy.  Read by
    // concurrency_strategy_traits.hpp tag-alias fast path.
    // Derived containers using a mixed strategy (e.g.
    // locked metadata over a cow payload) should override
    // this with `hybrid_strategy_tag` in their own
    // class body.
    using concurrency_strategy_tag = locked_strategy_tag;

protected:
    threadsafe_container_base()  = default;
    ~threadsafe_container_base() = default;

    // non-copyable: mutex cannot be copied
    threadsafe_container_base(
        const threadsafe_container_base&)            = delete;
    threadsafe_container_base& operator=(
        const threadsafe_container_base&)            = delete;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // movable: mutex is default-initialized in the
    // moved-to object (no state to transfer)
    threadsafe_container_base(
        threadsafe_container_base&&)                 = default;
    threadsafe_container_base& operator=(
        threadsafe_container_base&&)                 = default;
#endif

public:
    // --- lock acquisition ---

    // read_lock
    //   acquires a shared (reader) lock if the policy
    // supports it, exclusive otherwise.  Returns an
    // RAII guard.
    read_guard read_lock() const
    {
        return read_guard(m_mutex);
    }

    // write_lock
    //   acquires an exclusive (writer) lock.
    write_guard write_lock() const
    {
        return write_guard(m_mutex);
    }

    // try_write_lock
    //   attempts a non-blocking exclusive lock.
    scoped_try_lock<_Policy>
    try_write_lock() const
    {
        return scoped_try_lock<_Policy>(m_mutex);
    }

    // --- mutex access ---

    // mutex
    //   direct access to the underlying mutex for
    // interop with external synchronization (e.g.
    // condition variables).
    mutex_type& mutex() const noexcept
    {
        return m_mutex;
    }

    // --- policy queries (constexpr) ---

    static constexpr thread_safety_level
    safety_level() noexcept
    {
        return _Policy::level;
    }

    static constexpr bool
    is_threadsafe() noexcept
    {
        return _Policy::is_threadsafe;
    }

    static constexpr bool
    supports_shared() noexcept
    {
        return _Policy::is_shared;
    }

    static constexpr bool
    supports_timed() noexcept
    {
        return _Policy::is_timed;
    }

private:
    mutable mutex_type m_mutex;
};


// =============================================================================
// II.  Locked Accessors
// =============================================================================
// RAII handles that hold a lock and expose the underlying
// container via operator->/operator*.
//
// Usage:
//   threadsafe_vector<int> ts_vec;
//   {
//       auto ref = make_locked_ref(ts_vec);
//       ref->push_(42);
//       ref->push_(99);
//   } // lock released
//
//   {
//       auto cref = make_const_locked_ref(ts_vec);
//       int first = (*cref)[0];
//   } // read lock released

// const_locked_ref
//   class: holds a read lock and provides const access
// to the container.
template<typename _Container,
         typename _Policy>
class const_locked_ref
{
public:
    using lock_type =
        typename _Policy::read_lock_type;

    explicit const_locked_ref(
        const _Container&          _c,
        typename _Policy::mutex_type& _mutex)
        : m_ref(_c)
        , m_lock(_mutex)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const_locked_ref(const_locked_ref&&) = default;
#endif

    const_locked_ref(const const_locked_ref&) = delete;
    const_locked_ref& operator=(
        const const_locked_ref&) = delete;

    const _Container*
    operator->() const noexcept
    {
        return &m_ref;
    }

    const _Container&
    operator*() const noexcept
    {
        return m_ref;
    }

    const _Container&
    get() const noexcept
    {
        return m_ref;
    }

private:
    const _Container& m_ref;
    lock_type         m_lock;
};

// locked_ref
//   class: holds a write lock and provides mutable
// access to the container.
template<typename _Container,
         typename _Policy>
class locked_ref
{
public:
    using lock_type =
        typename _Policy::write_lock_type;

    explicit locked_ref(
        _Container&                   _c,
        typename _Policy::mutex_type& _mutex)
        : m_ref(_c),
          m_lock(_mutex)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    locked_ref(locked_ref&&) = default;
#endif

    locked_ref(const locked_ref&) = delete;
    locked_ref& operator=(
        const locked_ref&) = delete;

    _Container*
    operator->() noexcept
    {
        return &m_ref;
    }

    _Container&
    operator*() noexcept
    {
        return m_ref;
    }

    _Container&
    get() noexcept
    {
        return m_ref;
    }

    // const access is also available through a
    // write lock (it's a superset of read)
    const _Container*
    operator->() const noexcept
    {
        return &m_ref;
    }

    const _Container&
    operator*() const noexcept
    {
        return m_ref;
    }

private:
    _Container& m_ref;
    lock_type   m_lock;
};

// --- locked_apply ---
// Acquires a lock, invokes a callable on the container,
// and returns the result.  The lock is held for exactly
// the duration of the callable.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// locked_apply (write)
//   acquires a write lock and invokes _fn with a mutable
// reference to the container.
template<typename _Container,
         typename _Policy,
         typename _Fn>
auto
locked_apply(
    _Container&                   _c,
    typename _Policy::mutex_type& _mutex,
    _Fn&&                         _fn)
    -> decltype(_fn(_c))
{
    typename _Policy::write_lock_type guard(_mutex);

    return std::forward<_Fn>(_fn)(_c);
}

// locked_apply (read)
//   acquires a read lock and invokes _fn with a const
// reference.
template<typename _Container,
         typename _Policy,
         typename _Fn>
auto
locked_apply_read(
    const _Container&             _c,
    typename _Policy::mutex_type& _mutex,
    _Fn&&                         _fn)
    -> decltype(_fn(_c))
{
    typename _Policy::read_lock_type guard(_mutex);

    return std::forward<_Fn>(_fn)(_c);
}

#endif  // C++11


// =============================================================================
// III. Atomic Container State
// =============================================================================
// Bundled atomic metadata for containers that need lock-
// free size tracking, version stamping, or generation
// counting.
//
// Used by:
//   - Optimistic reads (version check)
//   - ABA prevention (generation counter)
//   - Lock-free size queries (atomic_size)
//
// On C++98/03 this section is unavailable (requires
// <atomic>).

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// atomic_state
//   struct: aggregated atomic container metadata.
// All fields are independently atomic - no mutex needed
// for reading individual fields.
struct atomic_state
{
    atomic_size    size;
    atomic_version version;

    atomic_state() noexcept
        : size(0)
        , version(0)
    {}

    // --- size operations ---

    std::size_t load_size(
        std::memory_order _order =
            std::memory_order_acquire) const noexcept
    {
        return size.load(_order);
    }

    void store_size(
        std::size_t _n,
        std::memory_order _order =
            std::memory_order_release) noexcept
    {
        size.store(_n, _order);
    }

    std::size_t fetch_add_size(
        std::size_t _n,
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return size.fetch_add(_n, _order);
    }

    std::size_t fetch_sub_size(
        std::size_t _n,
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return size.fetch_sub(_n, _order);
    }

    // --- version operations ---

    std::uint64_t load_version(
        std::memory_order _order =
            std::memory_order_acquire) const noexcept
    {
        return version.load(_order);
    }

    std::uint64_t increment_version(
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return version.fetch_add(1, _order);
    }

    // snapshot
    //   captures both size and version atomically
    // relative to each other (but NOT jointly atomic -
    // the two loads are sequentially consistent).
    struct snapshot
    {
        std::size_t   size;
        std::uint64_t version;
    };

    snapshot take_snapshot() const noexcept
    {
        return {
            size.load(std::memory_order_seq_cst),
            version.load(std::memory_order_seq_cst)
        };
    }

    // validate_snapshot
    //   returns true if the version has not changed
    // since the snapshot was taken.
    bool validate_snapshot(
        const snapshot& _snap) const noexcept
    {
        return (version.load(
            std::memory_order_acquire) ==
                _snap.version);
    }
};

#endif  // C++11


// =============================================================================
// IV.  Optimistic Read Protocol
// =============================================================================
// Pattern for reading container data without acquiring a
// full lock.  The reader snapshots the version, performs
// its read, then validates that the version hasn't changed.
// If it has changed, the read is retried.
//
// This is a building block - concrete containers wrap this
// in type-specific read operations.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// optimistic_read
//   function: executes _fn under optimistic concurrency.
// _fn receives a const& to the container and must produce
// a copyable result.  If the version changes during the
// read, _fn is retried up to _max_retries times, after
// which a read lock is acquired for a guaranteed-safe read.
//
// _state: the container's atomic_state
// _c:     const reference to the underlying container
// _mutex: the container's mutex (fall path)
// _fn:    callable taking const Container&, returning R
template<typename _Container,
         typename _Policy,
         typename _Fn>
auto
optimistic_read(
    const atomic_state&           _state,
    const _Container&             _c,
    typename _Policy::mutex_type& _mutex,
    _Fn&&                         _fn,
    unsigned                      _max_retries = 3)
    -> decltype(_fn(_c))
{
    for (unsigned attempt = 0;
         attempt < _max_retries; ++attempt)
    {
        auto snap = _state.take_snapshot();

        auto result = _fn(_c);

        if (_state.validate_snapshot(snap))
        {
            return result;
        }
    }

    // fall: acquire a real read lock
    typename _Policy::read_lock_type guard(_mutex);

    return std::forward<_Fn>(_fn)(_c);
}

#endif  // C++11


// =============================================================================
// V.   CAS Retry Infrastructure
// =============================================================================
// Templates for building lock-free container operations
// that use compare-and-exchange loops.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// exponential_off
//   class: s off exponentially between CAS retries.
// Starts at _initial_spins, doubles up to _max_spins,
// then yields.  Zero allocation, zero overhead when not
// spinning.
class exponential_off
{
public:
    explicit exponential_off(
        unsigned _initial_spins = 4,
        unsigned _max_spins     = 1024) noexcept
        : m_current(_initial_spins)
        , m_max(_max_spins)
    {}

    void operator()() noexcept
    {
        if (m_current <= m_max)
        {
            for (unsigned i = 0;
                 i < m_current; ++i)
            {
                spin_pause();
            }

            m_current *= 2;
        }
        else
        {
            d_thread_yield();
        }
    }

    void reset() noexcept
    {
        m_current = m_max / 256;

        if (m_current == 0)
        {
            m_current = 1;
        }
    }

private:
    // spin_pause
    //   platform-specific pause hint for spin loops.
    static void spin_pause() noexcept
    {
    #if defined(__x86_64__) || defined(_M_X64) || \
        defined(__i386__)   || defined(_M_IX86)
        #if defined(_MSC_VER)
            _mm_pause();
        #else
            __builtin_ia32_pause();
        #endif
    #elif defined(__aarch64__) || defined(_M_ARM64)
        #if defined(_MSC_VER)
            __yield();
        #else
            __asm__ volatile("yield");
        #endif
    #else
        // no-op on other architectures
    #endif
    }

    unsigned m_current;
    unsigned m_max;
};

// cas_loop
//   function: generic CAS retry loop.  Calls _update_fn
// with the current value to produce the desired value,
// then attempts CAS.  Retries with off on failure.
// Returns true if the CAS succeeded, false if
// _max_attempts was reached.
//
// _target:      atomic variable to update
// _update_fn:   callable (T current) -> T desired
// _max_attempts: 0 = unlimited
template<typename _T,
         typename _Fn>
bool
cas_loop(
    std::atomic<_T>& _target,
    _Fn              _update_fn,
    unsigned         _max_attempts = 0)
{
    exponential_off off;

    _T current = _target.load(
        std::memory_order_acquire);

    unsigned attempt = 0;

    while (true)
    {
        _T desired = _update_fn(current);

        if (_target.compare_exchange_weak(
                current, desired,
                std::memory_order_acq_rel,
                std::memory_order_acquire))
        {
            return true;
        }

        ++attempt;

        if (_max_attempts > 0 &&
            attempt >= _max_attempts)
        {
            return false;
        }

        off();
    }
}

#endif  // C++11


// =============================================================================
// VI.  Snapshot View
// =============================================================================
// Takes a copy of the container's data under a read lock,
// then allows iteration over the copy without holding any
// lock.  Safe for containers where iteration must not
// block writers.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// snapshot_view
//   class: read-locked copy of a container's elements.
// The lock is held only during construction (copy).
// After construction, the snapshot is independent of the
// source container.
template<typename _Container,
         typename _Policy>
class snapshot_view
{
public:
    using value_type =
        typename _Container::value_type;
    using const_iterator =
        typename std::vector<value_type>
            ::const_iterator;

    snapshot_view(
        const _Container&             _c,
        typename _Policy::mutex_type& _mutex)
    {
        typename _Policy::read_lock_type guard(
            _mutex);

        m_data.assign(
            std::begin(_c), std::end(_c));
    }

    const_iterator begin() const noexcept
    {
        return m_data.begin();
    }

    const_iterator end() const noexcept
    {
        return m_data.end();
    }

    std::size_t size() const noexcept
    {
        return m_data.size();
    }

    bool empty() const noexcept
    {
        return m_data.empty();
    }

    const value_type&
    operator[](std::size_t _i) const
    {
        return m_data[_i];
    }

    const std::vector<value_type>&
    data() const noexcept
    {
        return m_data;
    }

private:
    std::vector<value_type> m_data;
};

#endif  // C++11


// =============================================================================
// VII. Batch Guard
// =============================================================================
// Holds a write lock for the duration of multiple
// operations, ensuring they appear atomic to other
// threads.  Works on all C++ versions.
//
// Usage:
//   {
//       batch_guard<Policy> batch(container.mutex());
//       container.push__unsafe(1);
//       container.push__unsafe(2);
//       container.push__unsafe(3);
//   } // all three insertions are visible atomically

template<typename _Policy>
class batch_guard
{
public:
    using lock_type =
        typename _Policy::write_lock_type;

    explicit batch_guard(
        typename _Policy::mutex_type& _mutex)
        : m_lock(_mutex)
        , m_count(0)
    {}

    ~batch_guard() = default;

    batch_guard(const batch_guard&) = delete;
    batch_guard& operator=(
        const batch_guard&) = delete;

    // record
    //   increments the operation count for diagnostics.
    void record() noexcept
    {
        ++m_count;
    }

    // count
    //   number of operations performed in this batch.
    std::size_t count() const noexcept
    {
        return m_count;
    }

private:
    lock_type   m_lock;
    std::size_t m_count;
};


// =============================================================================
// VIII. Locked Range (Safe Iterator Wrapper)
// =============================================================================
// Wraps a container's iterator to hold a read lock for
// the lifetime of the iteration range.  The lock is
// released when the locked_range goes out of scope.
//
// WARNING: holding a read lock for the entire iteration
// can block writers.  For long iterations, prefer
// snapshot_view instead.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// locked_range
//   class: holds a read lock and exposes begin()/end()
// from the underlying container.
template<typename _Container,
         typename _Policy>
class locked_range
{
public:
    using const_iterator =
        decltype(std::begin(
            std::declval<const _Container&>()));

    locked_range(
        const _Container&             _c,
        typename _Policy::mutex_type& _mutex)
        : m_ref(_c),
          m_lock(_mutex)
    {}

    locked_range(const locked_range&) = delete;
    locked_range& operator=(
        const locked_range&) = delete;

    const_iterator begin() const
    {
        return std::begin(m_ref);
    }

    const_iterator end() const
    {
        return std::end(m_ref);
    }

    std::size_t size() const
    {
        return m_ref.size();
    }

private:
    const _Container&                       m_ref;
    typename _Policy::read_lock_type m_lock;
};

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_CONTAINER_