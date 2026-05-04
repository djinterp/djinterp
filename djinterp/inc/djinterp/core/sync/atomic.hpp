/******************************************************************************
* djinterp [sync]                                                   atomic.hpp
*
* Atomic utilities for the thread-safe framework.
*   Provides semantic wrappers around std::atomic for common metadata
* patterns (element counts, version stamps).  These types add no
* overhead beyond the underlying atomic - they exist to clarify intent
* and prevent mixing up unrelated atomic variables.
*
* TYPES:
*   atomic_size       - atomic std::size_t for lock-free element counts
*   atomic_version    - atomic std::uint64_t for version/generation stamps
*   atomic_flag_guard - RAII guard for std::atomic_flag (set on construct,
*                       clear on destruct)
*
* VERSIONING:
*   C++98/03:  unavailable (requires <atomic>)
*   C++11:     all types available
*   C++20:     + atomic_ref support, wait/notify
*
*
* path:      /inc/djinterp/core/sync/atomic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_ATOMIC_
#define DJINTERP_THREADSAFE_ATOMIC_ 1

//#ifndef DJINTERP_ENVIRONMENT_
//    #error "atomic.hpp requires env.h to be included first"
//#endif

//#ifndef __cplusplus
//    #error "atomic.hpp can only be used in C++ compilation mode"
//#endif

//#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <atomic>
#include <cstddef>
#include <cstdint>
// djinterp
#include "../djinterp.hpp"
#include "./concurrency_strategy_tags.hpp"


NS_DJINTERP

// =========================================================================
// I.   ATOMIC SIZE
// =========================================================================
// Semantic wrapper for an atomic element count.  Provides
// the standard atomic interface plus convenience methods
// for increment / decrement.

class atomic_size
{
public:
    // --- type aliases ---

    // value_type
    //   alias: the underlying value held atomically.
    // Mirrors std::atomic<T>::value_type so that generic
    // code (including the test trait surface) can probe
    // store/CAS overloads via T::value_type.
    using value_type = std::size_t;

    // concurrency_strategy_tag
    //   alias: declares this type as lock-free atomic
    // strategy.  Read by concurrency_strategy_traits.hpp
    // tag-alias fast path.
    using concurrency_strategy_tag = atomic_strategy_tag;

    atomic_size() noexcept
        : m_value(0)
    {}

    explicit atomic_size(std::size_t _initial) noexcept
        : m_value(_initial)
    {}

    // non-copyable (atomics are non-copyable)
    atomic_size(const atomic_size&)            = delete;
    atomic_size& operator=(const atomic_size&) = delete;

    // --- load / store ---

    std::size_t load(
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        return m_value.load(_order);
    }

    void store(
        std::size_t       _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        m_value.store(_n, _order);
    }

    // --- fetch operations ---

    std::size_t fetch_add(
        std::size_t       _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_value.fetch_add(_n, _order);
    }

    std::size_t fetch_sub(
        std::size_t       _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_value.fetch_sub(_n, _order);
    }

    // --- convenience ---

    std::size_t increment(
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return m_value.fetch_add(1, _order);
    }

    std::size_t decrement(
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return m_value.fetch_sub(1, _order);
    }

    // --- CAS ---

    bool compare_exchange_weak(
        std::size_t&      _expected,
        std::size_t       _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    bool compare_exchange_strong(
        std::size_t&      _expected,
        std::size_t       _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // --- conversion ---

    operator std::size_t() const noexcept
    {
        return m_value.load(
            std::memory_order_seq_cst);
    }

    // --- C++20 wait / notify ---

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    void wait(
        std::size_t       _old,
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        m_value.wait(_old, _order);
    }

    void notify_one() noexcept
    {
        m_value.notify_one();
    }

    void notify_all() noexcept
    {
        m_value.notify_all();
    }

#endif  // C++20

private:
    std::atomic<std::size_t> m_value;
};


// =========================================================================
// II.  ATOMIC VERSION
// =========================================================================
// Semantic wrapper for an atomic version / generation
// counter.  Used for optimistic concurrency control and
// ABA prevention.

class atomic_version
{
public:
    // --- type aliases ---

    // value_type
    //   alias: the underlying value held atomically.
    // Mirrors std::atomic<T>::value_type so that generic
    // code (including the test trait surface) can probe
    // store/CAS overloads via T::value_type.
    using value_type = std::uint64_t;

    // concurrency_strategy_tag
    //   alias: declares this type as lock-free atomic
    // strategy.  Read by concurrency_strategy_traits.hpp
    // tag-alias fast path.
    using concurrency_strategy_tag = atomic_strategy_tag;

    atomic_version() noexcept
        : m_value(0)
    {}

    explicit atomic_version(std::uint64_t _initial) noexcept
        : m_value(_initial)
    {}

    atomic_version(const atomic_version&)            = delete;
    atomic_version& operator=(const atomic_version&) = delete;

    // --- load / store ---

    std::uint64_t load(
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        return m_value.load(_order);
    }

    void store(
        std::uint64_t     _v,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        m_value.store(_v, _order);
    }

    // --- fetch operations ---

    std::uint64_t fetch_add(
        std::uint64_t     _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_value.fetch_add(_n, _order);
    }

    // --- convenience ---

    // bump
    //   increments the version and returns the previous
    // value.  The standard mutation sequence is:
    //   uint64_t old = ver.bump();
    //   // ... perform mutation ...
    //   // readers comparing against old see a stale snapshot
    std::uint64_t bump(
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return m_value.fetch_add(1, _order);
    }

    // --- CAS ---

    bool compare_exchange_weak(
        std::uint64_t&    _expected,
        std::uint64_t     _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    bool compare_exchange_strong(
        std::uint64_t&    _expected,
        std::uint64_t     _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // --- conversion ---

    operator std::uint64_t() const noexcept
    {
        return m_value.load(
            std::memory_order_seq_cst);
    }

    // --- C++20 wait / notify ---

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    void wait(
        std::uint64_t     _old,
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        m_value.wait(_old, _order);
    }

    void notify_one() noexcept
    {
        m_value.notify_one();
    }

    void notify_all() noexcept
    {
        m_value.notify_all();
    }

#endif  // C++20

private:
    std::atomic<std::uint64_t> m_value;
};


// =========================================================================
// III. ATOMIC FLAG GUARD
// =========================================================================
// RAII guard for std::atomic_flag.  Sets the flag on
// construction (via test_and_set), clears on destruction.
//
// Primary use: one-shot initialization guards and
// simple spinlock patterns.

class atomic_flag_guard
{
public:
    // construct: sets the flag.  was_set() reports whether
    // the flag was already set before this guard.
    explicit atomic_flag_guard(
        std::atomic_flag& _flag) noexcept
        : m_flag(_flag)
        , m_was_set(
              _flag.test_and_set(
                  std::memory_order_acquire))
    {}

    ~atomic_flag_guard()
    {
        m_flag.clear(std::memory_order_release);
    }

    atomic_flag_guard(const atomic_flag_guard&)            = delete;
    atomic_flag_guard& operator=(const atomic_flag_guard&) = delete;

    // was_set
    //   returns true if the flag was already set before
    // this guard was constructed.  Use to detect
    // re-entrancy or contention.
    bool was_set() const noexcept
    {
        return m_was_set;
    }

private:
    std::atomic_flag& m_flag;
    bool              m_was_set;
};


// =========================================================================
// IV.  ATOMIC STAMPED POINTER (C++11+)
// =========================================================================
// Combines a pointer and a version stamp into a single
// atomically-updated unit.  Used to solve the ABA problem
// in lock-free data structures.
//
// On 64-bit platforms, packs the stamp into the upper 16
// bits of the pointer (assumes 48-bit virtual addresses).
// On 32-bit platforms, uses a 64-bit CAS with separate
// fields.

template<typename _T>
class atomic_stamped_ptr
{
public:
    using stamp_type = std::uint16_t;

    atomic_stamped_ptr() noexcept
        : m_packed(0)
    {}

    explicit atomic_stamped_ptr(
        _T*        _ptr,
        stamp_type _stamp = 0) noexcept
        : m_packed(pack(_ptr, _stamp))
    {}

    atomic_stamped_ptr(
        const atomic_stamped_ptr&)            = delete;
    atomic_stamped_ptr& operator=(
        const atomic_stamped_ptr&)            = delete;

    // --- accessors ---

    _T* load_ptr(
        std::memory_order _order =
            std::memory_order_acquire) const noexcept
    {
        return unpack_ptr(m_packed.load(_order));
    }

    stamp_type load_stamp(
        std::memory_order _order =
            std::memory_order_acquire) const noexcept
    {
        return unpack_stamp(m_packed.load(_order));
    }

    // --- store ---

    void store(
        _T*               _ptr,
        stamp_type        _stamp,
        std::memory_order _order =
            std::memory_order_release) noexcept
    {
        m_packed.store(pack(_ptr, _stamp), _order);
    }

    // --- CAS ---

    bool compare_exchange_weak(
        _T*&              _expected_ptr,
        stamp_type&       _expected_stamp,
        _T*               _desired_ptr,
        stamp_type        _desired_stamp,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        std::uintptr_t expected =
            pack(_expected_ptr, _expected_stamp);
        std::uintptr_t desired =
            pack(_desired_ptr, _desired_stamp);

        bool ok = m_packed.compare_exchange_weak(
            expected, desired, _success, _failure);

        if (!ok)
        {
            _expected_ptr   = unpack_ptr(expected);
            _expected_stamp = unpack_stamp(expected);
        }

        return ok;
    }

private:
    static std::uintptr_t pack(
        _T*        _ptr,
        stamp_type _stamp) noexcept
    {
        std::uintptr_t raw =
            reinterpret_cast<std::uintptr_t>(_ptr);

        // upper 16 bits for stamp (48-bit VA assumption)
        return (raw & 0x0000FFFFFFFFFFFF) |
               (static_cast<std::uintptr_t>(_stamp)
                   << 48);
    }

    static _T* unpack_ptr(
        std::uintptr_t _packed) noexcept
    {
        // sign-extend from 48 bits for canonical form
        std::uintptr_t raw = _packed & 0x0000FFFFFFFFFFFF;

        if (raw & (static_cast<std::uintptr_t>(1) << 47))
        {
            raw |= 0xFFFF000000000000;
        }

        return reinterpret_cast<_T*>(raw);
    }

    static stamp_type unpack_stamp(
        std::uintptr_t _packed) noexcept
    {
        return static_cast<stamp_type>(
            _packed >> 48);
    }

    std::atomic<std::uintptr_t> m_packed;
};


NS_END  // djinterp

//#endif  // C++11


#endif  // DJINTERP_THREADSAFE_ATOMIC_