/*******************************************************************************
* djinterp [core]                                                     atomic.hpp
*
* Atomic utilities for the thread-safe framework.
*   Provides semantic wrappers around std::atomic for common metadata
* patterns (element counts, version stamps). These types add no
* overhead beyond the underlying atomic - they exist to clarify intent
* and prevent mixing up unrelated atomic variables.
*
* TYPES:
*   atomic_size       - atomic std::size_t for lock-free element counts
*   atomic_version    - atomic re_std::uint64_t for version/generation stamps
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
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ATOMIC SIZE
      -----------

II.   ATOMIC VERSION
      --------------

III.  ATOMIC FLAG GUARD
      -----------------

IV.   ATOMIC STAMPED POINTER (C++11+)
      -------------------------------
*/

#ifndef DJINTERP_SYNC_ATOMIC_HPP
#define DJINTERP_SYNC_ATOMIC_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std
#include <atomic>
#include <cstddef>
// djinterp
#include "../../djinterp.hpp"
#include "./concurrency_strategy_tags.hpp"
#include "./sync_common.hpp"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t, uint64_t,
                                                // uintptr_t


NS_DJINTERP

// I.    Atomic size
// Semantic wrapper for an atomic element count. Provides
// the standard atomic interface plus convenience methods
// for increment / decrement.

class atomic_size
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(atomic_size)

    // --- type aliases ---

    // value_type
    //   alias: the underlying value held atomically.
    // Mirrors std::atomic<T>::value_type so that generic
    // code (including the test trait surface) can probe
    // store/CAS overloads via T::value_type.
    using value_type = std::size_t;

    // concurrency_strategy_tag
    //   alias: declares this type as lock-free atomic
    // strategy. Read by concurrency_strategy_traits.hpp
    // tag-alias fast path.
    using concurrency_strategy_tag = atomic_strategy_tag;

    atomic_size() noexcept
        : m_value(0)
    {}

    explicit atomic_size(std::size_t _initial) noexcept
        : m_value(_initial)
    {}


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


// II.   Atomic version
// Semantic wrapper for an atomic version / generation
// counter. Used for optimistic concurrency control and
// ABA prevention.

class atomic_version
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(atomic_version)

    // --- type aliases ---

    // value_type
    //   alias: the underlying value held atomically.
    // Mirrors std::atomic<T>::value_type so that generic
    // code (including the test trait surface) can probe
    // store/CAS overloads via T::value_type.
    using value_type = re_std::uint64_t;

    // concurrency_strategy_tag
    //   alias: declares this type as lock-free atomic
    // strategy. Read by concurrency_strategy_traits.hpp
    // tag-alias fast path.
    using concurrency_strategy_tag = atomic_strategy_tag;

    atomic_version() noexcept
        : m_value(0)
    {}

    explicit atomic_version(re_std::uint64_t _initial) noexcept
        : m_value(_initial)
    {}


    // --- load / store ---

    re_std::uint64_t load(
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        return m_value.load(_order);
    }

    void store(
        re_std::uint64_t  _v,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        m_value.store(_v, _order);
    }

    // --- fetch operations ---

    re_std::uint64_t fetch_add(
        re_std::uint64_t  _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_value.fetch_add(_n, _order);
    }

    // --- convenience ---

    // bump
    //   increments the version and returns the previous
    // value. The standard mutation sequence is:
    //   uint64_t old = ver.bump();
    //   // ... perform mutation ...
    //   // readers comparing against old see a stale snapshot
    re_std::uint64_t bump(
        std::memory_order _order =
            std::memory_order_acq_rel) noexcept
    {
        return m_value.fetch_add(1, _order);
    }

    // --- CAS ---

    bool compare_exchange_weak(
        re_std::uint64_t& _expected,
        re_std::uint64_t  _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    bool compare_exchange_strong(
        re_std::uint64_t& _expected,
        re_std::uint64_t  _desired,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        return m_value.compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // --- conversion ---

    operator re_std::uint64_t() const noexcept
    {
        return m_value.load(
            std::memory_order_seq_cst);
    }

    // --- C++20 wait / notify ---

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    void wait(
        re_std::uint64_t  _old,
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
    std::atomic<re_std::uint64_t> m_value;
};


// III. Atomic flag guard
// RAII guard for std::atomic_flag. Sets the flag on
// construction (via test_and_set), clears on destruction.
//
// Primary use: one-shot initialization guards and
// simple spinlock patterns.

class atomic_flag_guard
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(atomic_flag_guard)

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


    // was_set
    //   returns true if the flag was already set before
    // this guard was constructed. Use to detect
    // re-entrancy or contention.
    bool was_set() const noexcept
    {
        return m_was_set;
    }

private:
    std::atomic_flag& m_flag;
    bool              m_was_set;
};


// IV.   Atomic stamped pointer (C++11+)
// Combines a pointer and a version stamp into a single
// atomically-updated unit. Used to solve the ABA problem
// in lock-free data structures.
//
// On 64-bit platforms, packs the stamp into the upper 16
// bits of the pointer (assumes 48-bit virtual addresses).
// On 32-bit platforms, uses a 64-bit CAS with separate
// fields.

template<typename Type>
class atomic_stamped_ptr
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(atomic_stamped_ptr)

    using stamp_type = re_std::uint16_t;

    atomic_stamped_ptr() noexcept
        : m_packed(0)
    {}

    explicit atomic_stamped_ptr(
        Type*        _ptr,
        stamp_type _stamp = 0) noexcept
        : m_packed(pack(_ptr, _stamp))
    {}


    // --- accessors ---

    Type* load_ptr(
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
        Type*               _ptr,
        stamp_type        _stamp,
        std::memory_order _order =
            std::memory_order_release) noexcept
    {
        m_packed.store(pack(_ptr, _stamp), _order);
    }

    // --- CAS ---

    bool compare_exchange_weak(
        Type*&              _expected_ptr,
        stamp_type&       _expected_stamp,
        Type*               _desired_ptr,
        stamp_type        _desired_stamp,
        std::memory_order _success =
            std::memory_order_acq_rel,
        std::memory_order _failure =
            std::memory_order_acquire) noexcept
    {
        re_std::uintptr_t expected =
            pack(_expected_ptr, _expected_stamp);
        re_std::uintptr_t desired =
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
    static re_std::uintptr_t pack(
        Type*        _ptr,
        stamp_type _stamp) noexcept
    {
        re_std::uintptr_t raw =
            reinterpret_cast<re_std::uintptr_t>(_ptr);

        // upper 16 bits for stamp (48-bit VA assumption)
        return (raw & 0x0000FFFFFFFFFFFF) |
               (static_cast<re_std::uintptr_t>(_stamp)
                   << 48);
    }

    static Type* unpack_ptr(
        re_std::uintptr_t _packed) noexcept
    {
        // sign-extend from 48 bits for canonical form
        re_std::uintptr_t raw = _packed & 0x0000FFFFFFFFFFFF;

        if (raw & (static_cast<re_std::uintptr_t>(1) << 47))
        {
            raw |= 0xFFFF000000000000;
        }

        return reinterpret_cast<Type*>(raw);
    }

    static stamp_type unpack_stamp(
        re_std::uintptr_t _packed) noexcept
    {
        return static_cast<stamp_type>(
            _packed >> 48);
    }

    std::atomic<re_std::uintptr_t> m_packed;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_SYNC_ATOMIC_HPP
