/*******************************************************************************
* djinterp [cpp]                                                         cow.hpp
*
* Copy-on-write and immutable snapshot primitives for the thread-safe
* framework.
*
*   Provides building blocks for containers that use value-sharing
* semantics (copy-on-write) or that need to produce consistent, immutable
* views of mutable data (snapshots).
*
* TYPES:
*   DCowPtr<T>             - intrusive reference-counted copy-on-write
*                            smart pointer. Shares a single allocation
*                            across readers; clones on the first write
*                            when refcount > 1.
*   DImmutableSnapshot<T>  - a frozen, reference-counted view of a
*                            container state. Cheap to create (one
*                            atomic increment), cheap to copy (shared),
*                            never mutated after construction.
*   DCowState<T, Policy>   - combines DCowPtr with a lock policy and
*                            version stamp. This is the canonical
*                            building block for copy-on-write containers.
*
* DESIGN NOTES:
*   - DCowPtr uses an intrusive control block (embedded refcount)
*     to avoid the double-allocation of std::shared_ptr.
*   - All refcount operations use acquire/release memory ordering.
*   - DImmutableSnapshot holds a DCowPtr and exposes only const
*     access. It is trivially copyable in the sense that copies
*     share the same underlying data.
*   - DCowState provides the mutation protocol: read_access()
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
* path:      /inc/djinterp/cpp/sync/cow.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    COW CONTROL BLOCK
      -----------------

II.   COW_PTR
      -------

III.  IMMUTABLE SNAPSHOT
      ------------------

IV.   COW STATE
      ---------
*/

#ifndef DJINTERP_SYNC_COW_HPP
#define DJINTERP_SYNC_COW_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std
#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
// djinterp
#include "./lock_policy.hpp"
#include "./lock_guard.hpp"
#include "./atomic.hpp"
#include "./concurrency_strategy_tags.hpp"
#include "./sync_common.hpp"
// re_std
#include "../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint64_t
NS_DJINTERP


// I.    Cow control block
// Intrusive reference-counted control block. Embedded
// directly in the cow allocation alongside the managed
// object, avoiding the double allocation of shared_ptr.

NS_INTERNAL

    // DCowControlBlock
    //   struct: reference count and managed object storage.
    template<typename Type>
    struct DCowControlBlock
    {
        std::atomic<std::size_t> refcount;
        Type                    value;

        // construct from forwarded args
        template<typename... Args>
        explicit DCowControlBlock(Args&&... _args)
            : refcount(1),
              value(std::forward<Args>(_args)...)
        {}

        // copy the value (refcount starts at 1)
        explicit DCowControlBlock(const Type& _src)
            : refcount(1),
              value(_src)
        {}

        // move the value (refcount starts at 1)
        explicit DCowControlBlock(Type&& _src)
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


// II.   Cow_ptr
// Intrusive copy-on-write smart pointer. Multiple
// cow_ptrs can share the same control block. On the
// first write when the refcount > 1, the data is cloned
// into a new allocation and the writer detaches.
//
// THREAD SAFETY:
//   The refcount is atomic. However, concurrent reads
//   and writes to the SAME DCowPtr instance are NOT safe.
//   Protect the DCowPtr itself with a lock (see
//   DCowState below) or use separate cow_ptrs per thread.

template<typename Type>
class DCowPtr
{
public:
    using control_block =
        internal::DCowControlBlock<Type>;

    // --- constructors ---

    DCowPtr() noexcept
        : m_block(nullptr)
    {}

    // construct with a new value
    template<typename... Args>
    static
    DCowPtr make(
        Args&&... _args
    )
    {
        DCowPtr result;
        result.m_block = new control_block(
            std::forward<Args>(_args)...);

        return result;
    }

    // --- copy (shared ownership) ---

    DCowPtr(const DCowPtr& _other) noexcept
        : m_block(_other.m_block)
    {
        if (m_block)
        {
            m_block->add_ref();
        }
    }

    DCowPtr& operator=(const DCowPtr& _other) noexcept
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

    DCowPtr(DCowPtr&& _other) noexcept
        : m_block(_other.m_block)
    {
        _other.m_block = nullptr;
    }

    DCowPtr& operator=(DCowPtr&& _other) noexcept
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

    ~DCowPtr()
    {
        release();
    }

    // --- const access (never clones) ---

    const Type& read() const noexcept
    {
        return m_block->value;
    }

    const Type* operator->() const noexcept
    {
        return &m_block->value;
    }

    const Type& operator*() const noexcept
    {
        return m_block->value;
    }

    // --- mutable access (clones if shared) ---

    // write
    //   returns a mutable reference to the managed
    // object. If the refcount > 1, clones the data
    // first so that other readers are not affected.
    Type& write()
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

    explicit
    operator bool() const noexcept
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

    void swap(
        DCowPtr& _other
    ) noexcept
    {
        control_block* tmp = m_block;
        m_block            = _other.m_block;
        _other.m_block     = tmp;
    }

private:
    void release() noexcept
    {
        if ( (m_block) &&
             (m_block->release()) )
        {
            delete m_block;
        }

        m_block = nullptr;
    }

    void ensure_unique()
    {
        if ( (m_block) &&
             (!m_block->is_unique()) )
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
template<typename Type>
void swap(
    DCowPtr<Type>& _a,
    DCowPtr<Type>& _b
) noexcept
{
    _a.swap(_b);
}


// III. Immutable snapshot
// A frozen, read-only view of a container's state.
// Construction takes a copy under a lock; after that,
// the snapshot is independent and can be read without
// any synchronization.
//
// Snapshots share their data via DCowPtr - copying a
// snapshot is an atomic refcount increment, not a deep
// copy.

template<typename Type>
class DImmutableSnapshot
{
public:
    // construct from a value (takes a copy)
    explicit DImmutableSnapshot(const Type& _src)
        : m_data(DCowPtr<Type>::make(_src))
        , m_version(0)
    {}

    // construct with version stamp
    DImmutableSnapshot(
        const Type&     _src,
        re_std::uint64_t _version)
        : m_data(DCowPtr<Type>::make(_src))
        , m_version(_version)
    {}

    // construct from an existing DCowPtr (zero-copy)
    explicit DImmutableSnapshot(
        const DCowPtr<Type>& _cow)
        : m_data(_cow)
        , m_version(0)
    {}

    DImmutableSnapshot(
        const DCowPtr<Type>& _cow,
        re_std::uint64_t   _version)
        : m_data(_cow)
        , m_version(_version)
    {}

    // copy / move: cheap (shared DCowPtr)
    DImmutableSnapshot(
        const DImmutableSnapshot&
    ) = default;
    DImmutableSnapshot& operator=(
        const DImmutableSnapshot&
    ) = default;
    DImmutableSnapshot(
        DImmutableSnapshot&&
    ) noexcept = default;
    DImmutableSnapshot& operator=(
        DImmutableSnapshot&&
    ) noexcept = default;

    // --- const access ---

    const Type& get() const noexcept
    {
        return m_data.read();
    }

    const Type* operator->() const noexcept
    {
        return &m_data.read();
    }

    const Type& operator*() const noexcept
    {
        return m_data.read();
    }

    // --- version ---

    re_std::uint64_t version() const noexcept
    {
        return m_version;
    }

    // --- queries ---

    std::size_t use_count() const noexcept
    {
        return m_data.use_count();
    }

    explicit
    operator bool() const noexcept
    {
        return static_cast<bool>(m_data);
    }

private:
    DCowPtr<Type>   m_data;
    re_std::uint64_t m_version;
};


// IV.   Cow state
// Combines a DCowPtr, a lock policy, and a version stamp
// into a single building block for copy-on-write
// containers.
//
// read_access()  - returns a const DCowPtr (no clone)
//                  under a read lock.
// write_access() - clones the DCowPtr if shared, bumps
//                  the version, returns mutable ref
//                  under a write lock.
// snapshot()     - takes an DImmutableSnapshot under a
//                  read lock.

template<typename Type,
         typename Policy = default_lock_policy>
class DCowState
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(DCowState)

    using lock_policy_type = Policy;
    using mutex_type =
        typename Policy::mutex_type;

    // cow_state_type
    //   alias: self-marker so that
    // `has_cow_state_type<DCowState<...>>` reports
    // true. Containers built on top of DCowState typically
    // forward this alias to identify themselves as
    // copy-on-write.
    using cow_state_type = DCowState;

    // concurrency_strategy_tag
    //   alias: declares this type as copy-on-write strategy.
    // Read by concurrency_strategy_traits.hpp tag-alias
    // fast path.
    using concurrency_strategy_tag = cow_strategy_tag;

    // --- constructors ---

    DCowState()
        : m_data(DCowPtr<Type>::make())
    {}

    explicit DCowState(const Type& _initial)
        : m_data(DCowPtr<Type>::make(_initial))
    {}

    explicit DCowState(Type&& _initial)
        : m_data(DCowPtr<Type>::make(
              std::move(_initial)))
    {}


    // --- read access ---

    // read
    //   returns a const reference to the managed object
    // under a read lock. The lock is held only during
    // this call; callers must not retain references.
    // For persistent access, use snapshot() instead.
    const Type& read() const
    {
        typename Policy::read_lock_type guard(
            m_mutex);

        return m_data.read();
    }

    // --- write access ---

    // write
    //   returns a mutable reference to the managed
    // object under a write lock. Clones if the DCowPtr
    // is shared (snapshots are holding references).
    // Bumps the version counter.
    Type& write()
    {
        typename Policy::write_lock_type guard(
            m_mutex);

        m_version.bump();

        return m_data.write();
    }

    // modify
    //   acquires a write lock, clones if necessary,
    // invokes _fn with a mutable reference, bumps the
    // version. Returns the result of _fn.
    template<typename Fn>
    auto modify(Fn&& _fn)
        -> decltype(_fn(std::declval<Type&>()))
    {
        typename Policy::write_lock_type guard(
            m_mutex);

        m_version.bump();

        return std::forward<Fn>(_fn)(
            m_data.write());
    }

    // --- snapshot ---

    // snapshot
    //   returns an DImmutableSnapshot of the current
    // state. The read lock is held only during the
    // DCowPtr copy (atomic refcount bump).
    DImmutableSnapshot<Type>
    snapshot() const
    {
        typename Policy::read_lock_type guard(
            m_mutex);

        return DImmutableSnapshot<Type>(
            m_data,
            m_version.load(std::memory_order_acquire)
        );
    }

    // --- version query ---

    re_std::uint64_t version() const noexcept
    {
        return m_version.load(
            std::memory_order_acquire);
    }

    // --- replace ---

    // replace
    //   atomically replaces the entire managed object.
    // Previous snapshots remain valid (they hold their
    // own DCowPtr).
    void replace(
        const Type& _new_value
    )
    {
        typename Policy::write_lock_type guard(m_mutex);

        m_data = DCowPtr<Type>::make(_new_value);
        m_version.bump();
    }

    void replace(
        Type&& _new_value
    )
    {
        typename Policy::write_lock_type guard(m_mutex);

        m_data = DCowPtr<Type>::make(std::move(_new_value));
        m_version.bump();
    }

    // --- mutex access ---

    mutex_type& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    DCowPtr<Type>     m_data;
    atomic_version     m_version;
    mutable mutex_type m_mutex;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_SYNC_COW_HPP
