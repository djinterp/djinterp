/******************************************************************************
* djinterp [container]                               threadsafe_container.hpp
*
* Foundation module for implementing thread-safe containers.
*   Provides the runtime building blocks that all threadsafe_<container>
* implementations compose from.  This sits between the raw locking
* primitives (threadsafe.hpp) and the concrete container types
* (threadsafe_array, threadsafe_tree, etc.):
*
*     threadsafe.hpp               — lock policies, guards, atomic utils
*     threadsafe_container_traits  — compile-time detection (no runtime)
*     threadsafe_container.hpp     — THIS FILE: runtime abstractions
*     threadsafe_<type>.hpp        — concrete container implementations
*
*   The module provides five layers of concurrency abstraction, from
* simple mutex-guarded access to advanced lock-free techniques.
* Concrete containers mix in only the layers they need.
*
*   LAYER 1: CRTP base (threadsafe_container_base)
*     Holds the mutex, provides RAII-scoped lock acquisition.
*     Every threadsafe container inherits from this.
*
*   LAYER 2: Locked accessors (locked_ref / const_locked_ref)
*     RAII handles that hold a lock and expose the underlying
*     container via operator-> / operator*.  The canonical
*     access pattern:
*       auto ref = ts_container.write_access();
*       ref->push_back(42);
*       // lock released when ref goes out of scope
*
*   LAYER 3: Atomic container state (atomic_state)
*     Bundled atomic size + version counter for lock-free
*     metadata.  Enables optimistic reads: snapshot the
*     version, do the read, check the version hasn't changed.
*
*   LAYER 4: CAS retry infrastructure (backoff, retry_loop)
*     Exponential backoff and CAS-loop templates for building
*     lock-free and wait-free container operations.
*
*   LAYER 5: Memory reclamation (hazard pointers, epoch-based)
*     Foundations for lock-free containers that need safe
*     memory reclamation: hazard pointer records, epoch
*     guards, deferred deletion queues.
*
*   Additionally: snapshot_view for safe iteration, and
*   batch_guard for multi-operation transactions.
*
* VERSIONING:
*   All features degrade gracefully across C++ standards:
*     C++98/03:  CRTP base, locked_ref (no move), batch_guard
*     C++11:     + atomic_state, backoff, hazard pointers,
*                  epoch reclamation, move semantics
*     C++14:     + generic lambda locked_apply
*     C++17:     + if constexpr policy dispatch, shared_mutex,
*                  std::optional for try-lock results
*     C++20:     + concepts, std::atomic_ref, jthread-aware
*     C++23/26:  + hazard_pointer (std), RCU (std::rcu_domain)
*
* DEPENDENCIES:
*   threadsafe.hpp                 — lock policies, guards
*   threadsafe_container_traits.hpp — compile-time detection
*   container_traits.hpp           — container classification
*
* TABLE OF CONTENTS
* =================
* I.      threadsafe_container_base (CRTP)
* II.     Locked Accessors (locked_ref, const_locked_ref)
* III.    Atomic Container State
* IV.     Optimistic Read Protocol
* V.      CAS Retry Infrastructure
* VI.     Snapshot View
* VII.    Batch Guard
* VIII.   Hazard Pointer Foundation (C++11+)
* IX.     Epoch-Based Reclamation (C++11+)
* X.      Safe Iterator Wrapper (C++11+)
*
*
* path:      \inc\container\threadsafe_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.28
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_CONTAINER_
#define DJINTERP_THREADSAFE_CONTAINER_ 1

#include <cstddef>
#include <type_traits>
#include "..\djinterp.hpp"
#include "threadsafe.hpp"
#include "meta\threadsafe_container_traits.hpp"
#include "meta\container_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <functional>
    #include <utility>
    #include <vector>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>
#endif

// C++23 standard hazard pointers / RCU
#if D_ENV_LANG_IS_CPP23_OR_HIGHER
    #if __has_include(<hazard_pointer>)
        #include <hazard_pointer>
        #define D_HAS_STD_HAZARD_POINTER 1
    #else
        #define D_HAS_STD_HAZARD_POINTER 0
    #endif

    #if __has_include(<rcu>)
        #include <rcu>
        #define D_HAS_STD_RCU 1
    #else
        #define D_HAS_STD_RCU 0
    #endif
#else
    #define D_HAS_STD_HAZARD_POINTER 0
    #define D_HAS_STD_RCU 0
#endif


NS_DJINTERP
NS_CONTAINER

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
    using mutex_type =
        typename _Policy::mutex_type;
    using read_guard =
        typename _Policy::read_lock_type;
    using write_guard =
        typename _Policy::write_lock_type;

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

    static constexpr DThreadSafetyLevel
    thread_safety_level() noexcept
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
        return _Policy::supports_shared;
    }

    static constexpr bool
    supports_timed() noexcept
    {
        return _Policy::supports_timed;
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
//       ref->push_back(42);
//       ref->push_back(99);
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
// All fields are independently atomic — no mutex needed
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
    // relative to each other (but NOT jointly atomic —
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
// This is a building block — concrete containers wrap this
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
// _mutex: the container's mutex (fallback path)
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

    // fallback: acquire a real read lock
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

// exponential_backoff
//   class: backs off exponentially between CAS retries.
// Starts at _initial_spins, doubles up to _max_spins,
// then yields.  Zero allocation, zero overhead when not
// spinning.
class exponential_backoff
{
public:
    explicit exponential_backoff(
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
// then attempts CAS.  Retries with backoff on failure.
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
    exponential_backoff backoff;

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

        backoff();
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
//       container.push_back_unsafe(1);
//       container.push_back_unsafe(2);
//       container.push_back_unsafe(3);
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
// VIII. Hazard Pointer Foundation (C++11+)
// =============================================================================
// Minimal hazard pointer infrastructure for lock-free
// containers.  Concrete containers (lock-free lists,
// lock-free queues, etc.) use these to safely reclaim
// nodes without locking.
//
// On C++23 with <hazard_pointer> available, these types
// are thin wrappers around the standard implementation.
// Otherwise, a minimal inline implementation is provided.
//
// Thread-local storage is used for per-thread hazard
// records.  The implementation is allocation-free on the
// fast path (protect / clear).

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_HAS_STD_HAZARD_POINTER

    // delegate to <hazard_pointer>
    using hazard_pointer_domain =
        std::hazard_pointer_default_domain;

    template<typename _T>
    using hazard_pointer = std::hazard_pointer<_T>;

#else

// hazard_record
//   struct: a single hazard pointer slot.
// Each thread can protect one pointer at a time per
// record.
struct hazard_record
{
    std::atomic<void*> protected_ptr;
    std::atomic<bool>  active;

    hazard_record() noexcept
        : protected_ptr(nullptr)
        , active(false)
    {}
};

// hazard_domain
//   class: collection of hazard records and a retired
// list.  One domain per container instance (or shared
// across containers of the same type).
//
// This is the minimal foundation — enough for a single-
// pointer-per-thread scheme.  Advanced multi-pointer
// schemes can extend this.
class hazard_domain
{
public:
    // max_threads
    //   the maximum number of concurrent threads.
    // Fixed at construction to avoid allocation on the
    // fast path.
    explicit hazard_domain(
        std::size_t _max_threads = 64) noexcept
        : m_records(nullptr)
        , m_capacity(_max_threads)
        , m_retired_count(0)
    {
        m_records = new (std::nothrow)
            hazard_record[_max_threads];
    }

    ~hazard_domain()
    {
        delete[] m_records;
    }

    hazard_domain(const hazard_domain&)            = delete;
    hazard_domain& operator=(const hazard_domain&) = delete;

    // acquire
    //   claims a hazard record for the calling thread.
    // Returns a pointer to the record, or nullptr if
    // all records are in use.
    hazard_record* acquire() noexcept
    {
        if (!m_records)
        {
            return nullptr;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            bool expected = false;

            if (m_records[i].active
                    .compare_exchange_strong(
                        expected, true,
                        std::memory_order_acq_rel))
            {
                return &m_records[i];
            }
        }

        return nullptr;
    }

    // release
    //   returns a hazard record to the pool.
    void release(hazard_record* _rec) noexcept
    {
        if (_rec)
        {
            _rec->protected_ptr.store(
                nullptr,
                std::memory_order_release);

            _rec->active.store(
                false,
                std::memory_order_release);
        }
    }

    // is_protected
    //   returns true if any active hazard record
    // currently protects _ptr.
    bool is_protected(void* _ptr) const noexcept
    {
        if (!m_records || !_ptr)
        {
            return false;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            if (m_records[i].active.load(
                    std::memory_order_acquire) &&
                m_records[i].protected_ptr.load(
                    std::memory_order_acquire) ==
                        _ptr)
            {
                return true;
            }
        }

        return false;
    }

    // retired_count
    //   approximate count of retired-but-not-reclaimed
    // nodes (for scan threshold decisions).
    std::size_t retired_count() const noexcept
    {
        return m_retired_count.load(
            std::memory_order_relaxed);
    }

    void increment_retired() noexcept
    {
        m_retired_count.fetch_add(
            1, std::memory_order_relaxed);
    }

    void decrement_retired(
        std::size_t _n = 1) noexcept
    {
        m_retired_count.fetch_sub(
            _n, std::memory_order_relaxed);
    }

    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

private:
    hazard_record*     m_records;
    std::size_t        m_capacity;
    std::atomic_size_t m_retired_count;
};

// scoped_hazard
//   class: RAII hazard pointer protector.  Protects a
// pointer for the lifetime of this object.
//
// Usage:
//   scoped_hazard guard(domain);
//   Node* node = head.load();
//   guard.protect(node);
//   // node is now safe to dereference
//   // ... use node ...
//   // guard destructor clears protection
class scoped_hazard
{
public:
    explicit scoped_hazard(
        hazard_domain& _domain) noexcept
        : m_domain(_domain)
        , m_record(_domain.acquire())
    {}

    ~scoped_hazard()
    {
        if (m_record)
        {
            m_domain.release(m_record);
        }
    }

    scoped_hazard(const scoped_hazard&)            = delete;
    scoped_hazard& operator=(const scoped_hazard&) = delete;

    // protect
    //   marks _ptr as protected.  Must be followed by
    // a re-read of the source pointer to confirm it
    // hasn't changed (standard hazard pointer protocol).
    void protect(void* _ptr) noexcept
    {
        if (m_record)
        {
            m_record->protected_ptr.store(
                _ptr,
                std::memory_order_release);
        }
    }

    // clear
    //   removes protection.
    void clear() noexcept
    {
        if (m_record)
        {
            m_record->protected_ptr.store(
                nullptr,
                std::memory_order_release);
        }
    }

    // valid
    //   true if a hazard record was successfully
    // acquired.
    bool valid() const noexcept
    {
        return (m_record != nullptr);
    }

private:
    hazard_domain& m_domain;
    hazard_record* m_record;
};

#endif  // D_HAS_STD_HAZARD_POINTER


// =============================================================================
// IX.  Epoch-Based Reclamation (C++11+)
// =============================================================================
// Alternative to hazard pointers for lock-free containers.
// Readers enter a "critical section" by recording the
// current epoch.  Writers advance the epoch and defer
// deletion until all readers from the old epoch have
// exited.
//
// Simpler than hazard pointers (no per-pointer protection),
// but requires that readers don't hold references across
// epoch boundaries.
//
// On C++23 with <rcu> available, prefer std::rcu_domain.

#if D_HAS_STD_RCU

    using rcu_domain = std::rcu_default_domain;

    template<typename _T>
    void rcu_retire(_T* _ptr)
    {
        std::rcu_retire(_ptr);
    }

#else

// epoch_counter
//   class: global epoch tracker.  Writers call advance()
// to move to a new epoch.  Readers call enter()/exit()
// to mark their critical section.
class epoch_counter
{
public:
    epoch_counter() noexcept
        : m_global_epoch(0)
    {}

    // current
    //   returns the current global epoch.
    std::uint64_t current() const noexcept
    {
        return m_global_epoch.load(
            std::memory_order_acquire);
    }

    // advance
    //   moves to the next epoch.  Called by writers
    // after completing a modification.
    std::uint64_t advance() noexcept
    {
        return m_global_epoch.fetch_add(
            1, std::memory_order_acq_rel);
    }

private:
    std::atomic<std::uint64_t> m_global_epoch;
};

// epoch_guard
//   class: RAII critical section marker for epoch-based
// reclamation.  The reader records the epoch on
// construction and releases on destruction.
//
// The active epoch is stored in a thread-local-like
// atomic (passed by reference).  The container's scan
// routine checks all active epochs to determine when
// it is safe to reclaim memory.
class epoch_guard
{
public:
    explicit epoch_guard(
        const epoch_counter&        _counter,
        std::atomic<std::uint64_t>& _local_epoch)
        noexcept
        : m_local(_local_epoch)
    {
        m_local.store(
            _counter.current(),
            std::memory_order_release);
    }

    ~epoch_guard()
    {
        // sentinel value: not in a critical section
        m_local.store(
            UINT64_MAX,
            std::memory_order_release);
    }

    epoch_guard(const epoch_guard&)            = delete;
    epoch_guard& operator=(const epoch_guard&) = delete;

private:
    std::atomic<std::uint64_t>& m_local;
};

// retired_list
//   class: a per-thread list of nodes awaiting
// reclamation.  Each entry records the node pointer and
// the epoch at which it was retired.  A scan pass
// deletes entries whose epoch is no longer active.
template<typename _T>
class retired_list
{
public:
    struct entry
    {
        _T*           ptr;
        std::uint64_t retire_epoch;
    };

    retired_list() = default;

    // retire
    //   adds a node to the retired list with its
    // retirement epoch.
    void retire(_T* _ptr,
                std::uint64_t _epoch)
    {
        m_entries.push_back({ _ptr, _epoch });
    }

    // scan
    //   reclaims all entries whose retire_epoch is
    // older than _safe_epoch (i.e. no reader is in
    // that epoch).  Calls delete on each reclaimed
    // pointer.
    // Returns the number of nodes reclaimed.
    std::size_t scan(std::uint64_t _safe_epoch)
    {
        std::size_t reclaimed = 0;
        std::size_t wr = 0;

        for (std::size_t rd = 0;
             rd < m_entries.size(); ++rd)
        {
            if (m_entries[rd].retire_epoch <
                _safe_epoch)
            {
                delete m_entries[rd].ptr;
                ++reclaimed;
            }
            else
            {
                if (wr != rd)
                {
                    m_entries[wr] =
                        m_entries[rd];
                }

                ++wr;
            }
        }

        m_entries.resize(wr);

        return reclaimed;
    }

    std::size_t pending() const noexcept
    {
        return m_entries.size();
    }

private:
    std::vector<entry> m_entries;
};

#endif  // D_HAS_STD_RCU


// =============================================================================
// X.   Safe Iterator Wrapper (C++11+)
// =============================================================================
// Wraps a container's iterator to hold a read lock for
// the lifetime of the iteration range.  The lock is
// released when the locked_range goes out of scope.
//
// WARNING: holding a read lock for the entire iteration
// can block writers.  For long iterations, prefer
// snapshot_view instead.

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
        : m_ref(_c)
        , m_lock(_mutex)
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


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_CONTAINER_