/******************************************************************************
* djinterp [sync]                                          cow.hpp
*
* Copy-on-write and immutable snapshot primitives for the thread-safe
* framework.
*
*   Provides building blocks for containers that use value-sharing
* semantics (copy-on-write) or that need to produce consistent, immutable
* views of mutable data (snapshots).
*
* TYPES:
*   cow_ptr<T>             - intrusive reference-counted copy-on-write
*                            smart pointer.  Shares a single allocation
*                            across readers; clones on the first write
*                            when refcount > 1.
*   immutable_snapshot<T>  - a frozen, reference-counted view of a
*                            container state.  Cheap to create (one
*                            atomic increment), cheap to copy (shared),
*                            never mutated after construction.
*   cow_state<T, Policy>   - combines cow_ptr with a lock policy and
*                            version stamp.  This is the canonical
*                            building block for copy-on-write containers.
*
* DESIGN NOTES:
*   - cow_ptr uses an intrusive control block (embedded refcount)
*     to avoid the double-allocation of std::shared_ptr.
*   - All refcount operations use acquire/release memory ordering.
*   - immutable_snapshot holds a cow_ptr and exposes only const
*     access.  It is trivially copyable in the sense that copies
*     share the same underlying data.
*   - cow_state provides the mutation protocol: read_access()
*     returns a const view, write_access() clones-on-write and
*     bumps the version.
*
* VERSIONING:
*   C++98/03:  unavailable (requires atomics, move semantics)
*   C++11:     all types available
*   C++17:     + if constexpr optimizations
*   C++20:     + concepts constraints
*
*
* path:      /inc/djinterp/sync/cow.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_COW_
#define DJINTERP_THREADSAFE_COW_ 1

//#ifndef DJINTERP_ENVIRONMENT_
//    #error "cow.hpp requires env.h to be included first"
//#endif

//#ifndef __cplusplus
//    #error "cow.hpp can only be used in C++ compilation mode"
//#endif

//#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
// djinterp
#include "lock_policy.hpp"
#include "lock_guard.hpp"
#include "atomic.hpp"
#include "./concurrency_strategy_tags.hpp"


NS_DJINTERP


// =========================================================================
// I.   COW CONTROL BLOCK
// =========================================================================
// Intrusive reference-counted control block.  Embedded
// directly in the cow allocation alongside the managed
// object, avoiding the double allocation of shared_ptr.

NS_INTERNAL

    // cow_control_block
    //   struct: reference count and managed object storage.
    template<typename _Type>
    struct cow_control_block
    {
        std::atomic<std::size_t> refcount;
        _Type                    value;

        // construct from forwarded args
        template<typename... _Args>
        explicit cow_control_block(_Args&&... _args)
            : refcount(1),
              value(std::forward<_Args>(_args)...)
        {}

        // copy the value (refcount starts at 1)
        explicit cow_control_block(const _Type& _src)
            : refcount(1),
              value(_src)
        {}

        // move the value (refcount starts at 1)
        explicit cow_control_block(_Type&& _src)
            : refcount(1), 
              value(std::move(_src))
        {}

        void add_ref() noexcept
        {
            refcount.fetch_add(1, std::memory_order_relaxed);
        }

        // release
        //   decrements the refcount and returns true if
        // the caller should delete this block.
        bool release() noexcept
        {
            return (refcount.fetch_sub(1, std::memory_order_acq_rel) == 1);
        }

        bool is_unique() const noexcept
        {
            return (refcount.load(std::memory_order_acquire) == 1);
        }

        std::size_t use_count() const noexcept
        {
            return refcount.load(std::memory_order_acquire);
        }
    };

NS_END  // internal


// =========================================================================
// II.  COW_PTR
// =========================================================================
// Intrusive copy-on-write smart pointer.  Multiple
// cow_ptrs can share the same control block.  On the
// first write when the refcount > 1, the data is cloned
// into a new allocation and the writer detaches.
//
// THREAD SAFETY:
//   The refcount is atomic.  However, concurrent reads
//   and writes to the SAME cow_ptr instance are NOT safe.
//   Protect the cow_ptr itself with a lock (see
//   cow_state below) or use separate cow_ptrs per thread.

template<typename _Type>
class cow_ptr
{
public:
    using control_block =
        internal::cow_control_block<_Type>;

    // --- constructors ---

    cow_ptr() noexcept
        : m_block(nullptr)
    {}

    // construct with a new value
    template<typename... _Args>
    static cow_ptr make(_Args&&... _args)
    {
        cow_ptr result;
        result.m_block = new control_block(
            std::forward<_Args>(_args)...);
        return result;
    }

    // --- copy (shared ownership) ---

    cow_ptr(const cow_ptr& _other) noexcept
        : m_block(_other.m_block)
    {
        if (m_block)
        {
            m_block->add_ref();
        }
    }

    cow_ptr& operator=(const cow_ptr& _other) noexcept
    {
        if (this != &_other)
        {
            release();
            m_block = _other.m_block;

            if (m_block)
            {
                m_block->add_ref();
            }
        }

        return *this;
    }

    // --- move ---

    cow_ptr(cow_ptr&& _other) noexcept
        : m_block(_other.m_block)
    {
        _other.m_block = nullptr;
    }

    cow_ptr& operator=(cow_ptr&& _other) noexcept
    {
        if (this != &_other)
        {
            release();
            m_block = _other.m_block;
            _other.m_block = nullptr;
        }

        return *this;
    }

    // --- destructor ---

    ~cow_ptr()
    {
        release();
    }

    // --- const access (never clones) ---

    const _Type& read() const noexcept
    {
        return m_block->value;
    }

    const _Type* operator->() const noexcept
    {
        return &m_block->value;
    }

    const _Type& operator*() const noexcept
    {
        return m_block->value;
    }

    // --- mutable access (clones if shared) ---

    // write
    //   returns a mutable reference to the managed
    // object.  If the refcount > 1, clones the data
    // first so that other readers are not affected.
    _Type& write()
    {
        ensure_unique();
        return m_block->value;
    }

    // --- queries ---

    bool is_unique() const noexcept
    {
        return (m_block && m_block->is_unique());
    }

    std::size_t use_count() const noexcept
    {
        return m_block
            ? m_block->use_count()
            : 0;
    }

    explicit operator bool() const noexcept
    {
        return (m_block != nullptr);
    }

    // --- detach ---

    // detach
    //   forces a clone even if currently unique.
    // Useful to guarantee that subsequent writes
    // will not be visible through any snapshot taken
    // before detach().
    void detach()
    {
        if (m_block)
        {
            control_block* fresh =
                new control_block(m_block->value);

            release();

            m_block = fresh;
        }
    }

    // --- swap ---

    void swap(cow_ptr& _other) noexcept
    {
        control_block* tmp = m_block;
        m_block            = _other.m_block;
        _other.m_block     = tmp;
    }

private:
    void release() noexcept
    {
        if (m_block && m_block->release())
        {
            delete m_block;
        }

        m_block = nullptr;
    }

    void ensure_unique()
    {
        if (m_block && !m_block->is_unique())
        {
            control_block* fresh =
                new control_block(m_block->value);

            release();

            m_block = fresh;
        }
    }

    control_block* m_block;
};

// swap (ADL)
template<typename _Type>
void swap(cow_ptr<_Type>& _a,
          cow_ptr<_Type>& _b) noexcept
{
    _a.swap(_b);
}


// =========================================================================
// III. IMMUTABLE SNAPSHOT
// =========================================================================
// A frozen, read-only view of a container's state.
// Construction takes a copy under a lock; after that,
// the snapshot is independent and can be read without
// any synchronization.
//
// Snapshots share their data via cow_ptr - copying a
// snapshot is an atomic refcount increment, not a deep
// copy.

template<typename _Type>
class immutable_snapshot
{
public:
    // construct from a value (takes a copy)
    explicit immutable_snapshot(const _Type& _src)
        : m_data(cow_ptr<_Type>::make(_src))
        , m_version(0)
    {}

    // construct with version stamp
    immutable_snapshot(
        const _Type&     _src,
        std::uint64_t _version)
        : m_data(cow_ptr<_Type>::make(_src))
        , m_version(_version)
    {}

    // construct from an existing cow_ptr (zero-copy)
    explicit immutable_snapshot(
        const cow_ptr<_Type>& _cow)
        : m_data(_cow)
        , m_version(0)
    {}

    immutable_snapshot(
        const cow_ptr<_Type>& _cow,
        std::uint64_t      _version)
        : m_data(_cow)
        , m_version(_version)
    {}

    // copy / move: cheap (shared cow_ptr)
    immutable_snapshot(
        const immutable_snapshot&)               = default;
    immutable_snapshot& operator=(
        const immutable_snapshot&)               = default;
    immutable_snapshot(
        immutable_snapshot&&) noexcept           = default;
    immutable_snapshot& operator=(
        immutable_snapshot&&) noexcept           = default;

    // --- const access ---

    const _Type& get() const noexcept
    {
        return m_data.read();
    }

    const _Type* operator->() const noexcept
    {
        return &m_data.read();
    }

    const _Type& operator*() const noexcept
    {
        return m_data.read();
    }

    // --- version ---

    std::uint64_t version() const noexcept
    {
        return m_version;
    }

    // --- queries ---

    std::size_t use_count() const noexcept
    {
        return m_data.use_count();
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_data);
    }

private:
    cow_ptr<_Type>   m_data;
    std::uint64_t m_version;
};


// =========================================================================
// IV.  COW STATE
// =========================================================================
// Combines a cow_ptr, a lock policy, and a version stamp
// into a single building block for copy-on-write
// containers.
//
// read_access()  - returns a const cow_ptr (no clone)
//                  under a read lock.
// write_access() - clones the cow_ptr if shared, bumps
//                  the version, returns mutable ref
//                  under a write lock.
// snapshot()     - takes an immutable_snapshot under a
//                  read lock.

template<typename _Type,
         typename _Policy = default_lock_policy>
class cow_state
{
public:
    using lock_policy_type = _Policy;
    using mutex_type =
        typename _Policy::mutex_type;

    // cow_state_type
    //   alias: self-marker so that
    // `has_cow_state_type<cow_state<...>>` reports
    // true.  Containers built on top of cow_state typically
    // forward this alias to identify themselves as
    // copy-on-write.
    using cow_state_type = cow_state;

    // concurrency_strategy_tag
    //   alias: declares this type as copy-on-write strategy.
    // Read by concurrency_strategy_traits.hpp tag-alias
    // fast path.
    using concurrency_strategy_tag = cow_strategy_tag;

    // --- constructors ---

    cow_state()
        : m_data(cow_ptr<_Type>::make())
    {}

    explicit cow_state(const _Type& _initial)
        : m_data(cow_ptr<_Type>::make(_initial))
    {}

    explicit cow_state(_Type&& _initial)
        : m_data(cow_ptr<_Type>::make(
              std::move(_initial)))
    {}

    // non-copyable (contains mutex)
    cow_state(const cow_state&)            = delete;
    cow_state& operator=(const cow_state&) = delete;

    // --- read access ---

    // read
    //   returns a const reference to the managed object
    // under a read lock.  The lock is held only during
    // this call; callers must not retain references.
    // For persistent access, use snapshot() instead.
    const _Type& read() const
    {
        typename _Policy::read_lock_type guard(
            m_mutex);

        return m_data.read();
    }

    // --- write access ---

    // write
    //   returns a mutable reference to the managed
    // object under a write lock.  Clones if the cow_ptr
    // is shared (snapshots are holding references).
    // Bumps the version counter.
    _Type& write()
    {
        typename _Policy::write_lock_type guard(
            m_mutex);

        m_version.bump();

        return m_data.write();
    }

    // modify
    //   acquires a write lock, clones if necessary,
    // invokes _fn with a mutable reference, bumps the
    // version.  Returns the result of _fn.
    template<typename _Fn>
    auto modify(_Fn&& _fn)
        -> decltype(_fn(std::declval<_Type&>()))
    {
        typename _Policy::write_lock_type guard(
            m_mutex);

        m_version.bump();

        return std::forward<_Fn>(_fn)(
            m_data.write());
    }

    // --- snapshot ---

    // snapshot
    //   returns an immutable_snapshot of the current
    // state.  The read lock is held only during the
    // cow_ptr copy (atomic refcount bump).
    immutable_snapshot<_Type> 
    snapshot() const
    {
        typename _Policy::read_lock_type guard(
            m_mutex);

        return immutable_snapshot<_Type>(
            m_data,
            m_version.load(std::memory_order_acquire)
        );
    }

    // --- version query ---

    std::uint64_t version() const noexcept
    {
        return m_version.load(
            std::memory_order_acquire);
    }

    // --- replace ---

    // replace
    //   atomically replaces the entire managed object.
    // Previous snapshots remain valid (they hold their
    // own cow_ptr).
    void replace(
        const _Type& _new_value
    )
    {
        typename _Policy::write_lock_type guard(m_mutex);

        m_data = cow_ptr<_Type>::make(_new_value);
        m_version.bump();
    }

    void replace(
        _Type&& _new_value
    )
    {
        typename _Policy::write_lock_type guard(m_mutex);

        m_data = cow_ptr<_Type>::make(std::move(_new_value));
        m_version.bump();
    }

    // --- mutex access ---

    mutex_type& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    cow_ptr<_Type>     m_data;
    atomic_version     m_version;
    mutable mutex_type m_mutex;
};


NS_END  // djinterp

//#endif  // C++11


#endif  // DJINTERP_THREADSAFE_COW_