/*******************************************************************************
* djinterp [core]                                                       pool.hpp
*
* C++ face of the fixed-slot pool.
*   `raw_pool` DERIVES from d_pool and adds no data members. `pool<T>` derives
* from raw_pool and adds no data members either -- it supplies sizeof(T) and
* alignof(T) as compile-time constants and layers object lifetime on top of
* the kernel's raw slots. Both sizes are asserted equal to sizeof(d_pool)
* below.
*
*   THE TEMPLATE IS NOTATION, NOT SEMANTICS. pool<T> computes its slot
* geometry at translation time; d_pool computes the same geometry at run time
* from the same functions. That is the framework's phase distinction working
* as designed (AGENT_README.md section 8): C++ computes EARLIER, never
* DIFFERENTLY. One compiled kernel serves every element type in the program,
* which is exactly what a template-per-type implementation cannot do.
*
* WHAT THE TYPED FACE ADDS THAT THE KERNEL CANNOT:
*   Object lifetime. The kernel deals in raw aligned storage and will never
* run a constructor or a destructor, because it does not know what a slot
* holds. pool<T> does, so create() placement-news and destroy() calls the
* destructor before returning the slot. That is the one genuine capability
* difference between the two faces, and it is why this file exists at all.
*
* HANDLES:
*   pool_handle<T> is d_pool_handle with a phantom type parameter. It adds no
* bytes -- asserted -- and buys the thing an untyped handle cannot: a handle
* from one pool cannot be silently resolved against another pool of a
* different element type.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/pool.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    raw_pool
      --------
      a. construction / move / destruction
      b. acquire / release_slot / reset / release
      c. queries
II.   pool_handle<T>
      --------------
III.  pool<T>
      -------
      a. typed acquire / release
      b. create / destroy       (object lifetime)
      c. handles
IV.   THE COST LAW
*/

#ifndef DJINTERP_MEMORY_POOL_HPP
#define DJINTERP_MEMORY_POOL_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <new>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./mem_common.hpp"
#include "./mem_source.hpp"
#include "../../c/memory/pool.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                        I.   raw_pool                                    ///
///////////////////////////////////////////////////////////////////////////////

// raw_pool
//   class: an untyped fixed-slot allocator. Derives from d_pool and adds no
// state; every operation forwards to the C kernel.
//   Use this directly when the slot size is not known until run time -- a
// pool of records whose width comes from a schema, say. Use pool<T> when the
// type is known, which is most of the time.
class raw_pool : public ::d_pool
{
public:

    using size_type  = mem_size;
    using index_type = ::d_pool_index;

    // raw_pool (configured)
    //   constructor: a pool of _slot_size-byte slots at _slot_align.
    explicit D_INLINE
    raw_pool(
        size_type            _slot_size,
        size_type            _slot_align = 0,
        ::d_pool_policy     _release    = D_POOL_POLICY_FREE_LIST
    )
    {
        ::d_pool_config config = ::d_pool_config_default(_slot_size,
                                                         _slot_align);

        config.policy = _release;

        ::d_pool_init(this, &config);
    }

    // raw_pool (full config)
    //   constructor: every knob at once, for the caller who needs a source, a
    // block size, or a ceiling.
    explicit D_INLINE
    raw_pool(
        const ::d_pool_config& _config
    )
    {
        ::d_pool_init(this, &_config);
    }

    // ~raw_pool
    //   destructor: returns every block to the source. Runs NO destructors on
    // anything the pool held -- the kernel deals in raw storage. pool<T> is
    // the face that knows better.
    D_INLINE
    ~raw_pool()
    {
        ::d_pool_release(this);
    }

    raw_pool(const raw_pool&)            = delete;
    raw_pool& operator=(const raw_pool&) = delete;

    // raw_pool (move)
    //   constructor: takes over another pool's blocks, leaving it empty.
    //   A byte-wise steal is correct here for the same reason it is for the
    // arena: the kernel struct holds no pointers back to itself. Its blocks
    // point at each other and the block table points at them, and none of
    // that changes when the struct moves.
    D_INLINE
    raw_pool(
        raw_pool&& _other
    ) noexcept
    {
        static_cast< ::d_pool& >(*this) = static_cast< ::d_pool& >(_other);
        d_internal_clear(_other);
    }

    D_INLINE raw_pool&
    operator=(
        raw_pool&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            ::d_pool_release(this);
            static_cast< ::d_pool& >(*this) =
                static_cast< ::d_pool& >(_other);
            d_internal_clear(_other);
        }

        return *this;
    }

    // ---------------------------------------------------------------
    //  acquire and release
    // ---------------------------------------------------------------

    // acquire
    //   operation: takes one slot's worth of raw storage, or returns null.
    D_INLINE void*
    acquire()
    {
        return ::d_pool_acquire(this);
    }

    // acquire_ex
    //   operation: takes one slot and reports why a failure failed.
    D_INLINE mem_status
    acquire_ex(
        mem_block& _out
    )
    {
        return ::d_pool_acquire_ex(this, &_out);
    }

    // release_slot
    //   operation: returns one slot. Reports D_MEM_ERR_UNSUPPORTED on a
    // monotonic pool rather than pretending to have freed anything.
    D_INLINE mem_status
    release_slot(
        void* _slot
    )
    {
        return ::d_pool_release_slot(this, _slot);
    }

    // reserve
    //   operation: ensures room for at least _slots slots in total.
    D_INLINE mem_status
    reserve(
        index_type _slots
    )
    {
        return ::d_pool_reserve(this, _slots);
    }

    // reset
    //   operation: reclaims every slot, keeping the blocks for reuse. On a
    // generational pool this also advances every counter, so every
    // outstanding handle becomes stale.
    D_INLINE void
    reset()
    {
        ::d_pool_reset(this);

        return;
    }

    // release
    //   operation: returns every block to the source.
    D_INLINE void
    release()
    {
        ::d_pool_release(this);

        return;
    }

    // ---------------------------------------------------------------
    //  queries
    // ---------------------------------------------------------------

    D_INLINE index_type size()      const { return ::d_pool_size(this); }
    D_INLINE index_type capacity()  const { return ::d_pool_capacity(this); }
    D_INLINE index_type available() const { return ::d_pool_available(this); }
    D_INLINE bool       empty()     const { return (::d_pool_size(this) == 0); }

    D_INLINE size_type  slot_size()  const
    {
        return ::d_pool_slot_size(this);
    }

    D_INLINE size_type  slot_align() const
    {
        return ::d_pool_slot_align(this);
    }

    D_INLINE size_type  block_count() const
    {
        return ::d_pool_block_count(this);
    }

    // bytes_allocated
    //   query: the honest cost of the pool, including per-block headers and
    // the block table -- as distinct from capacity times slot size, which
    // omits exactly the overhead a caller weighing the pool wants to see.
    D_INLINE size_type
    bytes_allocated() const
    {
        return ::d_pool_bytes_allocated(this);
    }

    D_INLINE double
    utilization() const
    {
        return ::d_pool_utilization(this);
    }

    D_INLINE mem_stats
    stats() const
    {
        mem_stats out;

        ::d_pool_stats(this, &out);

        return out;
    }

#if (D_INTERNAL_POOL_OWNS == 1)
    D_INLINE bool
    owns(
        const void* _slot
    ) const
    {
        return ::d_pool_owns(this, _slot);
    }
#endif

private:

    // d_internal_clear
    //   helper: leaves a moved-from pool in a state that is safe to destroy
    // and safe to reuse. Re-initializing through the kernel is deliberate --
    // zeroing the struct by hand here would be this file deciding what the
    // empty state is, which is the kernel's business.
    static D_INLINE void
    d_internal_clear(
        raw_pool& _pool
    )
    {
        ::d_pool_config config = ::d_pool_config_default(_pool.payload,
                                                         _pool.align);

        ::d_pool_init(&_pool, &config);

        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                     II.   pool_handle<T>                                ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_POOL_GENERATIONAL == 1)

// pool_handle
//   struct: a generation-checked reference to a slot holding a Type. The
// type parameter is PHANTOM: it adds no bytes -- asserted below -- and exists
// so that a handle into one pool cannot be resolved against a pool of a
// different element type without a diagnostic.
template<typename Type>
struct pool_handle : ::d_pool_handle
{
    using element_type = Type;

    // pool_handle (default)
    //   constructor: the null handle, which never resolves.
    D_INLINE
    pool_handle()
        : ::d_pool_handle(::d_pool_handle_null())
    {}

    // pool_handle (converting)
    //   constructor: wraps a handle produced by the kernel.
    D_INLINE
    pool_handle(
        const ::d_pool_handle& _handle
    )
        : ::d_pool_handle(_handle)
    {}

    // is_null
    //   query: whether this handle names no slot.
    D_INLINE bool
    is_null() const
    {
        return ::d_pool_handle_is_null(*this);
    }

    // operator==
    //   compare: two handles are equal when they name the same slot on the
    // same turn. A stale handle therefore never equals the live one that
    // replaced it, which is the comparison a caller wants.
    D_INLINE bool
    operator==(
        const pool_handle& _other
    ) const
    {
        return ( (index == _other.index) &&
                 (generation == _other.generation) );
    }

    D_INLINE bool
    operator!=(
        const pool_handle& _other
    ) const
    {
        return !(*this == _other);
    }
};

#endif  // D_INTERNAL_POOL_GENERATIONAL


///////////////////////////////////////////////////////////////////////////////
///                        III.   pool<T>                                   ///
///////////////////////////////////////////////////////////////////////////////

// pool
//   class: a fixed-slot allocator for objects of Type under release policy
// Policy. Derives from raw_pool and adds no state; it supplies the geometry
// and the policy at translation time and layers object lifetime on the
// kernel's raw slots.
//
//   WHY THE POLICY IS A TEMPLATE PARAMETER AND NOT A CONSTRUCTOR ARGUMENT.
// The kernel carries the policy as a runtime field, which is right for C: one
// compiled implementation has to serve every policy. But the facts that follow
// from the policy -- whether a slot can be released individually, whether the
// pool sweeps generations -- are what the strategy layer above publishes as
// `static constexpr`, and a runtime field cannot produce a constant.
//   Lifting it to a template parameter is the phase distinction again, and it
// costs nothing: the parameter is not a member, the kernel field is still set
// from it at construction, and both languages arrive at the same value. C++
// simply arrives EARLIER.
template<typename Type,
         ::d_pool_policy Policy = D_POOL_POLICY_FREE_LIST>
class pool : public raw_pool
{
public:

    using element_type = Type;
    using pointer      = Type*;

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    using handle_type  = pool_handle<Type>;
#endif

    // slot_bytes / slot_alignment
    //   constant: the geometry the kernel will compute, computed HERE at
    // translation time. Identical numbers, earlier -- which is the whole
    // claim this face makes.
    static D_CONSTEXPR const mem_size slot_bytes     = size_of<Type>::value;
    static D_CONSTEXPR const mem_size slot_alignment = align_of<Type>::value;

    // policy
    //   constant: the release policy, available at translation time. The
    // strategy layer reads this rather than the kernel's field.
    static D_CONSTEXPR const ::d_pool_policy policy = Policy;

    // pool (default)
    //   constructor: a pool of Type over the configured default source.
    D_INLINE
    pool()
        : raw_pool(slot_bytes, slot_alignment, Policy)
    {}

    // pool (sourced)
    //   constructor: a pool of Type over a named source, with an optional
    // block size and slot ceiling.
    explicit D_INLINE
    pool(
        const ::d_mem_source& _source,
        ::d_pool_index        _slots_per_block = 0,
        ::d_pool_index        _max_slots       = 0
    )
        : raw_pool(make_config(_source,
                               _slots_per_block,
                               _max_slots,
                               Policy))
    {}

    // ---------------------------------------------------------------
    //  raw slots
    // ---------------------------------------------------------------

    // acquire
    //   operation: raw, UNCONSTRUCTED storage for one Type.
    D_INLINE pointer
    acquire()
    {
        return static_cast<pointer>(raw_pool::acquire());
    }

    // ---------------------------------------------------------------
    //  object lifetime -- the capability the kernel cannot have
    // ---------------------------------------------------------------

    // create
    //   operation: takes a slot and CONSTRUCTS a Type in it from _args.
    // Returns null when the pool is full, in which case nothing was
    // constructed.
    template<typename... Args>
    D_INLINE pointer
    create(
        Args&&... _args
    )
    {
        void* storage = raw_pool::acquire();

        if (!storage)
        {
            return nullptr;
        }

        return new (storage) Type(std::forward<Args>(_args)...);
    }

    // destroy
    //   operation: DESTRUCTS the object and returns its slot.
    //   The destructor runs BEFORE the slot is recycled, which is the whole
    // reason this is not just release_slot: the kernel cannot know a slot
    // holds an object, so if this face did not run the destructor nothing
    // would.
    D_INLINE mem_status
    destroy(
        pointer _object
    )
    {
        if (!_object)
        {
            return D_MEM_OK;
        }

        _object->~Type();

        return raw_pool::release_slot(static_cast<void*>(_object));
    }

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    // ---------------------------------------------------------------
    //  handles  (generational pools only)
    // ---------------------------------------------------------------

    // create_handle
    //   operation: takes a slot, constructs a Type in it, and returns a
    // generation-checked handle. UNLIKE A POINTER, the result is safe to
    // store across a destroy: resolving it afterwards yields null rather than
    // the next occupant.
    template<typename... Args>
    D_INLINE handle_type
    create_handle(
        Args&&... _args
    )
    {
        ::d_pool_handle raw;
        void*           storage;

        if (::d_pool_acquire_handle(this, &raw) != D_MEM_OK)
        {
            return handle_type();
        }

        storage = ::d_pool_resolve(this, raw);

        if (!storage)
        {
            return handle_type();
        }

        new (storage) Type(std::forward<Args>(_args)...);

        return handle_type(raw);
    }

    // resolve
    //   query: the object a handle names, or null when the handle is null,
    // foreign, or stale.
    D_INLINE pointer
    resolve(
        handle_type _handle
    ) const
    {
        return static_cast<pointer>(::d_pool_resolve(this, _handle));
    }

    // is_live
    //   query: whether a handle still names the object it was issued for.
    D_INLINE bool
    is_live(
        handle_type _handle
    ) const
    {
        return ::d_pool_handle_is_live(this, _handle);
    }

    // destroy_handle
    //   operation: destructs the object a handle names and returns its slot.
    // Reports D_MEM_ERR_STALE rather than double-destroying when the handle
    // has already been spent.
    D_INLINE mem_status
    destroy_handle(
        handle_type _handle
    )
    {
        pointer object = resolve(_handle);

        if (!object)
        {
            return D_MEM_ERR_STALE;
        }

        object->~Type();

        return ::d_pool_release_handle(this, _handle);
    }
#endif  // D_INTERNAL_POOL_GENERATIONAL

private:

    // make_config
    //   helper: assembles a kernel configuration for this element type.
    static D_INLINE ::d_pool_config
    make_config(
        const ::d_mem_source& _source,
        ::d_pool_index        _slots_per_block,
        ::d_pool_index        _max_slots,
        ::d_pool_policy      _release
    )
    {
        ::d_pool_config config = ::d_pool_config_default(slot_bytes,
                                                         slot_alignment);

        config.source          = _source;
        config.slots_per_block = _slots_per_block;
        config.max_slots       = _max_slots;
        config.policy         = _release;

        return config;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                      IV.   THE COST LAW                                 ///
///////////////////////////////////////////////////////////////////////////////
//   Every wrapper in this file adds zero bytes, and says so in a form the
// compiler checks. The framework's own history is the argument for making
// these assertions rather than conventions: a wrapper that silently stopped
// deriving from its kernel once shipped, and only a test would have caught it.

D_STATIC_ASSERT(sizeof(raw_pool) == sizeof(::d_pool),
                "raw_pool must add nothing to d_pool");

D_STATIC_ASSERT((std::is_base_of< ::d_pool, raw_pool>::value),
                "raw_pool must derive from the C kernel, not mirror it");

D_STATIC_ASSERT(std::is_standard_layout< ::d_pool>::value,
                "d_pool must remain standard-layout for the wrapper to be "
                "layout-compatible with it");

//   The typed face is checked over several unrelated element types, because a
// template that added a member would do so per instantiation and a single
// witness could miss it.
D_STATIC_ASSERT(sizeof(pool<char>) == sizeof(::d_pool),
                "pool<char> must add nothing to d_pool");
D_STATIC_ASSERT(sizeof(pool<double>) == sizeof(::d_pool),
                "pool<double> must add nothing to d_pool");
D_STATIC_ASSERT(sizeof(pool<void*>) == sizeof(::d_pool),
                "pool<void*> must add nothing to d_pool");

//   The policy parameter must not cost bytes either: it selects behaviour at
// translation time and is written into the kernel's existing field, never
// stored a second time.
D_STATIC_ASSERT((sizeof(pool<char, D_POOL_POLICY_MONOTONIC>) ==
                 sizeof(::d_pool)),
                "the policy parameter must not add a member");
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
D_STATIC_ASSERT((sizeof(pool<char, D_POOL_POLICY_GENERATIONAL>) ==
                 sizeof(::d_pool)),
                "the policy parameter must not add a member");
#endif

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
D_STATIC_ASSERT(sizeof(pool_handle<char>) == sizeof(::d_pool_handle),
                "pool_handle must be a phantom type: no bytes over the kernel "
                "handle");
D_STATIC_ASSERT(sizeof(pool_handle<double>) == sizeof(::d_pool_handle),
                "pool_handle must be a phantom type: no bytes over the kernel "
                "handle");
#endif


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_MEMORY_POOL_HPP
