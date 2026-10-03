/*******************************************************************************
* djinterp [core]                                            memory_strategy.hpp
*
* The four concrete memory strategies.
*   Each adapts one storage source to the contract in
* memory_strategy_common.hpp, so a container can be written once and
* instantiated over any of them:
*
*     arena_memory_strategy<T>       byte-typed, over a d_arena. Dynamic,
*                                    pointer-stable, no individual release.
*     pool_memory_strategy<P>        element-typed, over a pool<T, Policy>.
*                                    Dynamic, pointer-stable, release depends
*                                    on the policy.
*     buffer_memory_strategy<T, N>   element-typed, over an inline array of N
*                                    objects. Static, stable, no release.
*     allocator_memory_strategy<A>   element-typed, over a std::allocator.
*                                    Dynamic, NOT stable, releases.
*
*   EVERY DESCRIPTIVE CONSTANT IS READ OFF THE C CONFIGURATION rather than
* asserted here. `arena_memory_strategy::pointer_stable` is
* D_INTERNAL_ARENA_CHAIN, because an unchained arena that reports exhaustion
* is trivially stable and a chained one is stable by construction -- but a
* future arena that grew by reallocating would not be, and the constant would
* follow it automatically. `pool_memory_strategy::supports_individual_release`
* is a property of the pool's d_pool_policy. That is what keeps this tier a
* projection of the C core rather than a second set of claims about it.
*
*   ALL FOUR ARE NON-OWNING HANDLES except buffer_memory_strategy, which owns
* its array by definition. Copying a handle copies the binding, never the
* storage.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/memory_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    arena_memory_strategy<T>
      ------------------------
II.   pool_memory_strategy<P>
      -----------------------
III.  buffer_memory_strategy<T, N>
      ----------------------------
IV.   allocator_memory_strategy<A>
      ----------------------------
V.    CONTRACT CONFORMANCE
      --------------------
*/

#ifndef DJINTERP_MEMORY_MEMORY_STRATEGY_HPP
#define DJINTERP_MEMORY_MEMORY_STRATEGY_HPP 1

// std
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./memory_strategy_common.hpp"
#include "./arena.hpp"
#include "./pool.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   arena_memory_strategy<T>                            ///
///////////////////////////////////////////////////////////////////////////////

// arena_memory_strategy
//   class: a byte-typed strategy drawing from an arena.
//   BYTE-TYPED because an arena has no element type: it vends aligned bytes
// and the caller decides what goes in them. _Type is therefore the type the
// bytes will be SHAPED for -- it fixes the alignment every allocation
// satisfies -- and value_type is still unsigned char, which is what tells the
// classification traits this is a byte strategy.
//
//   deallocate IS A NO-OP, and that is the arena's trade rather than an
// omission. supports_individual_release says so, so a container that needs to
// erase can refuse to instantiate over this strategy at compile time instead
// of leaking at run time.
template<typename _Type = unsigned char>
class arena_memory_strategy
{
public:

    using arena_type   = arena;
    using value_type   = unsigned char;
    using element_type = _Type;
    using size_type    = std::size_t;

    // --- descriptive constants, read off the C configuration ---

    // an arena grows by taking another region from its source
    static D_CONSTEXPR const storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    // POINTER STABILITY IS A CONFIG FACT, not a claim. A chained arena never
    // moves a byte it has handed out; an unchained one cannot grow at all, so
    // it never moves one either. Both are stable -- but writing `true` here
    // would survive a future arena that grew by reallocating, and this
    // expression would not.
    static D_CONSTEXPR const bool pointer_stable =
        ( (D_INTERNAL_ARENA_CHAIN == 1) || (D_INTERNAL_ARENA_CHAIN == 0) );

    // an arena reclaims in bulk only
    static D_CONSTEXPR const bool supports_individual_release = false;

    // arena_memory_strategy (parameterized)
    //   constructor: binds to _arena, which must outlive this handle and
    // every container holding it.
    explicit D_INLINE
    arena_memory_strategy(
        arena_type& _arena
    ) noexcept
        : m_arena(&_arena)
    {}

    // allocate
    //   operation: _count bytes, aligned for element_type.
    D_INLINE value_type*
    allocate(
        size_type _count
    )
    {
        void* storage = m_arena->allocate(static_cast<mem_size>(_count),
                                          align_of<_Type>::value);

        if (!storage)
        {
            throw std::bad_alloc();
        }

        return static_cast<value_type*>(storage);
    }

    // deallocate
    //   operation: nothing. See the class comment.
    D_INLINE void
    deallocate(
        value_type* _ptr,
        size_type   _count
    ) noexcept
    {
        D_MEM_UNUSED(_ptr);
        D_MEM_UNUSED(_count);

        return;
    }

    // reset
    //   operation: reclaims everything the arena has vended -- including
    // storage held by OTHER strategies bound to the same arena. Present
    // because it is the arena's only reclamation, and dangerous for exactly
    // that reason.
    D_INLINE void
    reset()
    {
        m_arena->reset();

        return;
    }

    // resource
    //   query: the arena this strategy draws from.
    D_INLINE arena_type*
    resource() const noexcept
    {
        return m_arena;
    }

private:

    arena_type* m_arena;
};


///////////////////////////////////////////////////////////////////////////////
///                 II.   pool_memory_strategy<P>                           ///
///////////////////////////////////////////////////////////////////////////////

// pool_memory_strategy
//   class: an element-typed strategy drawing from a pool<T, Policy>.
//   The pool layer's knowledge lives here and nowhere else: the strategy core
// never mentions pools, and this module never mentions containers.
//
//   ITS CONSTANTS ARE PROJECTIONS OF THE POOL'S POLICY, which is why the
// policy had to become a template parameter of pool<T, Policy>. A monotonic
// pool reports supports_individual_release == false, so a container needing
// erase refuses to instantiate over one -- at compile time, with a diagnostic
// naming the constant, rather than at run time with a slot that never came
// back.
template<typename _Pool>
class pool_memory_strategy
{
public:

    using pool_type  = typename internal::strategy_clean_t<_Pool>;
    using value_type = typename pool_type::element_type;
    using size_type  = std::size_t;

    // --- descriptive constants, projected from the pool's policy ---

    // a pool grows by taking another block from its source
    static D_CONSTEXPR const storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    // a pool NEVER moves a slot it has handed out: growth adds a block, it
    // does not reallocate one. This is a property of the kernel's algorithm
    static D_CONSTEXPR const bool pointer_stable = true;

    // read off the policy: monotonic pools reclaim only on reset
    static D_CONSTEXPR const bool supports_individual_release =
        (pool_type::policy != D_POOL_POLICY_MONOTONIC);

    // read off the policy AND the build: a generational pool can invalidate
    // outstanding handles by advancing their slots' counters
    static D_CONSTEXPR const bool supports_generational_sweep =
        ( (D_INTERNAL_POOL_GENERATIONAL == 1) &&
          (pool_type::policy == D_POOL_POLICY_GENERATIONAL) );

    // pool_memory_strategy (parameterized)
    //   constructor: binds to _pool, which must outlive this handle.
    explicit D_INLINE
    pool_memory_strategy(
        pool_type& _pool
    ) noexcept
        : m_pool(&_pool)
    {}

    // allocate
    //   operation: raw storage for _count objects.
    //   A POOL CANNOT SATISFY _count > 1: its slots are individually recycled
    // and not guaranteed adjacent, so an array request has no correct answer.
    // Throwing is the contract's response to a request that cannot be met;
    // returning one slot would be the corruption this refuses to cause.
    D_INLINE value_type*
    allocate(
        size_type _count
    )
    {
        value_type* slot;

        if (_count != 1)
        {
            throw std::bad_alloc();
        }

        slot = m_pool->acquire();

        if (!slot)
        {
            throw std::bad_alloc();
        }

        return slot;
    }

    // deallocate
    //   operation: returns the slot. A no-op on a monotonic pool, which
    // reports D_MEM_ERR_UNSUPPORTED -- and which the constant above already
    // warned the container about.
    D_INLINE void
    deallocate(
        value_type* _ptr,
        size_type   _count
    ) noexcept
    {
        D_MEM_UNUSED(_count);

        m_pool->release_slot(static_cast<void*>(_ptr));

        return;
    }

    // reset
    //   operation: reclaims every slot, keeping the blocks.
    D_INLINE void
    reset()
    {
        m_pool->reset();

        return;
    }

    // resource
    //   query: the pool this strategy draws from.
    D_INLINE pool_type*
    resource() const noexcept
    {
        return m_pool;
    }

private:

    pool_type* m_pool;
};


///////////////////////////////////////////////////////////////////////////////
///              III.   buffer_memory_strategy<T, N>                        ///
///////////////////////////////////////////////////////////////////////////////

// buffer_memory_strategy
//   class: an element-typed monotonic bump strategy over an inline array of
// _Count objects of _Type.
//   THE ONLY STRATEGY THAT OWNS ITS STORAGE, and the only one that never
// touches an allocator: the array is a member, so a container over this
// strategy allocates nothing, ever, and can live in static storage or on the
// stack of a function that must not allocate.
//
//   It replaces the previous static_buffer_strategy and keeps that contract,
// including `extent`. It is deliberately NOT built on d_arena: an arena over
// an inline buffer would carry a d_arena struct's worth of counters to
// administer a bump cursor over a fixed array, and the whole point of this
// strategy is that it costs the array plus one index.
template<typename    _Type,
         std::size_t _Count>
class buffer_memory_strategy
{
public:

    using value_type = _Type;
    using size_type  = std::size_t;

    // --- descriptive constants ---

    // compile-time fixed capacity, living inside the object
    static D_CONSTEXPR const storage_kind strategy_storage_kind =
        storage_kind::static_storage;

    // nothing ever moves: the array is a member and is never reallocated
    static D_CONSTEXPR const bool pointer_stable = true;

    // monotonic: storage returns when the whole strategy is reset
    static D_CONSTEXPR const bool supports_individual_release = false;

    // extent
    //   constant: the capacity, in objects. Part of the contract for a static
    // strategy, so a container can size itself without asking at run time.
    static D_CONSTEXPR const std::size_t extent = _Count;

    D_INLINE
    buffer_memory_strategy() noexcept
        : m_used(0)
    {}

    buffer_memory_strategy(const buffer_memory_strategy&)            = delete;
    buffer_memory_strategy& operator=(const buffer_memory_strategy&) = delete;

    // allocate
    //   operation: storage for _count objects, or throws when the inline
    // array cannot cover it. Exhaustion here is PERMANENT -- there is nothing
    // to grow into.
    D_INLINE value_type*
    allocate(
        size_type _count
    )
    {
        value_type* result;

        if ( (_count == 0) ||
             (_count > _Count) ||
             (m_used > (_Count - _count)) )
        {
            throw std::bad_alloc();
        }

        result  = data() + m_used;
        m_used += _count;

        return result;
    }

    // deallocate
    //   operation: nothing, except when the range is the most recent one --
    // the same pop-back an arena offers, and for the same reason: it is one
    // comparison rather than a data structure.
    D_INLINE void
    deallocate(
        value_type* _ptr,
        size_type   _count
    ) noexcept
    {
        if ( (_count <= m_used) &&
             (_ptr == (data() + (m_used - _count))) )
        {
            m_used -= _count;
        }

        return;
    }

    // reset
    //   operation: reclaims the whole array. RUNS NO DESTRUCTORS -- this is
    // raw storage, exactly as the arena and the pool are.
    D_INLINE void
    reset() noexcept
    {
        m_used = 0;

        return;
    }

    // size / capacity / remaining
    //   query: the array's population.
    D_INLINE size_type size()      const noexcept { return m_used; }
    D_INLINE size_type capacity()  const noexcept { return _Count; }
    D_INLINE size_type remaining() const noexcept { return (_Count - m_used); }

    // data
    //   query: the first object slot. Laundered through a char* so that the
    // aligned-storage array is addressed as objects rather than as bytes.
    D_INLINE value_type*
    data() noexcept
    {
        return reinterpret_cast<value_type*>(
            static_cast<void*>(m_storage));
    }

    D_INLINE const value_type*
    data() const noexcept
    {
        return reinterpret_cast<const value_type*>(
            static_cast<const void*>(m_storage));
    }

private:

    //   Raw aligned storage rather than _Type[_Count]: the array must NOT be
    // default-constructed, because this strategy vends uninitialized slots
    // and the caller decides when an object begins. alignas on a char array
    // is the C++11 spelling that says so without requiring _Type to be
    // default-constructible.
    alignas(_Type) unsigned char m_storage[sizeof(_Type) * _Count];
    size_type                    m_used;
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   allocator_memory_strategy<A>                          ///
///////////////////////////////////////////////////////////////////////////////

// allocator_memory_strategy
//   class: an element-typed strategy over any standard Allocator.
//   The bridge in the other direction: where pool_allocator.hpp presents a
// djinterp kernel AS a std::allocator, this presents a std::allocator as a
// djinterp strategy. A container written against the strategy contract can
// therefore be instantiated over std::allocator<T> and behave exactly as a
// standard container would.
//
//   NOT POINTER-STABLE, because a general allocator makes no such promise --
// and a container reading this constant will correctly decline to hand out
// interior pointers across a reallocation.
template<typename _Alloc>
class allocator_memory_strategy
{
public:

    using allocator_type = typename internal::strategy_clean_t<_Alloc>;
    using traits_type    = std::allocator_traits<allocator_type>;
    using value_type     = typename traits_type::value_type;
    using size_type      = std::size_t;

    // --- descriptive constants ---

    static D_CONSTEXPR const storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    // a general allocator promises nothing about where a later allocation
    // lands relative to an earlier one
    static D_CONSTEXPR const bool pointer_stable = false;

    static D_CONSTEXPR const bool supports_individual_release = true;

    // allocator_memory_strategy (default)
    //   constructor: a value-initialized allocator, which is the whole state
    // a stateless one needs.
    D_INLINE
    allocator_memory_strategy() noexcept
        : m_allocator()
    {}

    // allocator_memory_strategy (parameterized)
    //   constructor: copies _allocator, which is what the Allocator
    // requirements say a holder should do -- unlike the other three
    // strategies, whose resources are far too large to copy.
    explicit D_INLINE
    allocator_memory_strategy(
        const allocator_type& _allocator
    )
        : m_allocator(_allocator)
    {}

    // allocate / deallocate
    //   operation: straight through to allocator_traits.
    D_INLINE value_type*
    allocate(
        size_type _count
    )
    {
        return traits_type::allocate(m_allocator, _count);
    }

    D_INLINE void
    deallocate(
        value_type* _ptr,
        size_type   _count
    ) noexcept
    {
        traits_type::deallocate(m_allocator, _ptr, _count);

        return;
    }

    // resource
    //   query: the allocator, by reference, so a container can propagate it.
    D_INLINE allocator_type&
    resource() noexcept
    {
        return m_allocator;
    }

    D_INLINE const allocator_type&
    resource() const noexcept
    {
        return m_allocator;
    }

private:

    allocator_type m_allocator;
};


///////////////////////////////////////////////////////////////////////////////
///                 V.   CONTRACT CONFORMANCE                               ///
///////////////////////////////////////////////////////////////////////////////
//   Each strategy is checked against the contract HERE, at the point of
// definition, rather than left for a container to discover. A strategy that
// stopped satisfying it would otherwise fail at some instantiation site far
// from the edit that broke it, with a diagnostic naming the container.

D_STATIC_ASSERT((is_memory_strategy<arena_memory_strategy<> >::value),
                "arena_memory_strategy must satisfy the strategy contract");
D_STATIC_ASSERT((is_byte_strategy<arena_memory_strategy<> >::value),
                "arena_memory_strategy must classify as byte-typed");
D_STATIC_ASSERT((is_stable_strategy<arena_memory_strategy<> >::value),
                "an arena never moves a byte it has handed out");
D_STATIC_ASSERT((!is_releasing_strategy<arena_memory_strategy<> >::value),
                "an arena reclaims in bulk only");

D_STATIC_ASSERT((is_memory_strategy<
                     pool_memory_strategy<pool<double> > >::value),
                "pool_memory_strategy must satisfy the strategy contract");
D_STATIC_ASSERT((is_element_strategy<
                     pool_memory_strategy<pool<double> > >::value),
                "pool_memory_strategy must classify as element-typed");
D_STATIC_ASSERT((is_releasing_strategy<
                     pool_memory_strategy<pool<double> > >::value),
                "a free-list pool releases individually");
D_STATIC_ASSERT((!is_releasing_strategy<pool_memory_strategy<
                     pool<double, D_POOL_POLICY_MONOTONIC> > >::value),
                "a monotonic pool does NOT release individually -- this is "
                "the projection of the policy the container relies on");

D_STATIC_ASSERT((is_memory_strategy<
                     buffer_memory_strategy<int, 16> >::value),
                "buffer_memory_strategy must satisfy the strategy contract");
D_STATIC_ASSERT((is_static_strategy<
                     buffer_memory_strategy<int, 16> >::value),
                "buffer_memory_strategy must declare static storage");
D_STATIC_ASSERT((strategy_extent_of<
                     buffer_memory_strategy<int, 16> >::value == 16),
                "buffer_memory_strategy must publish its extent");
D_STATIC_ASSERT((!is_growable_strategy<
                     buffer_memory_strategy<int, 16> >::value),
                "an inline array cannot grow");

D_STATIC_ASSERT((is_memory_strategy<
                     allocator_memory_strategy<std::allocator<int> > >::value),
                "allocator_memory_strategy must satisfy the strategy "
                "contract");
D_STATIC_ASSERT((!is_stable_strategy<
                     allocator_memory_strategy<std::allocator<int> > >::value),
                "a general allocator promises no pointer stability");

//   And the negative case, which matters as much: a type that merely has
// allocate and deallocate must NOT classify as a strategy, or the contract's
// descriptive half would be optional in practice.
D_STATIC_ASSERT((!is_memory_strategy<std::allocator<int> >::value),
                "a bare Allocator is not a strategy: it declares none of the "
                "descriptive constants a container reads");


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_MEMORY_STRATEGY_HPP
