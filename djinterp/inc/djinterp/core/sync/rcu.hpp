/*******************************************************************************
* djinterp [core]                                                        rcu.hpp
*
* Read-Copy-Update (RCU) and epoch-based reclamation primitives.
*   Provides building blocks for lock-free containers that use read-side
* critical sections and deferred memory reclamation.
*
* RCU PROTOCOL:
*   Readers:  enter a critical section (record the current epoch), read
*             shared data freely, exit the critical section. No locks,
*             no atomics on the fast path beyond the epoch load/store.
*   Writers:  copy the shared data, modify the copy, atomically swap the
*             pointer, advance the epoch, retire the old copy. The old
*             copy is reclaimed only after all readers from the previous
*             epoch have exited.
*
* TYPES:
*   epoch_counter          - global epoch tracker (advance / current)
*   epoch_guard            - RAII reader critical section marker
*   epoch_registry         - thread registry for determining safe
*                            reclamation epochs
*   deferred_reclaimer<T>  - combines epoch_counter, epoch_registry,
*                            and retired_list into a single reclamation
*                            engine
*   rcu_protected<T,Policy>- complete RCU-protected value: atomic data
*                            pointer + reclamation engine, providing
*                            read() / update() / snapshot() operations
*
* VERSIONING:
*   C++98/03:  unavailable (requires <atomic>)
*   C++11:     all custom types available
*   C++23:     rcu_protected delegates to std::rcu_domain when available
*
*
* path:      /inc/djinterp/core/sync/rcu.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STANDARD LIBRARY DELEGATION (C++23)
      -----------------------------------

II.   EPOCH COUNTER
      -------------

III.  EPOCH GUARD
      -----------

IV.   EPOCH REGISTRY
      --------------

V.    DEFERRED RECLAIMER
      ------------------

VI.   RCU PROTECTED VALUE
      -------------------
*/

#ifndef DJINTERP_SYNC_RCU_HPP
#define DJINTERP_SYNC_RCU_HPP 1

// env.h first: the language checks below and the gates after them read
// its D_ENV_* results (it used to be a precondition, an #error if absent)
#include "../../env/env.h"  // D_ENV_LANG_*, D_ENV_CPP_FEATURE_*

#ifndef __cplusplus
    #error "rcu.hpp can only be used in C++ compilation mode"
#endif

// C++23 standard RCU
#if D_ENV_LANG_IS_CPP23_OR_HIGHER
    #if __has_include(<rcu>)
        // std
        #include <rcu>
        #ifndef D_HAS_STD_RCU
            #define D_HAS_STD_RCU 1
        #endif
    #else
        #ifndef D_HAS_STD_RCU
            #define D_HAS_STD_RCU 0
        #endif
    #endif
#else
    #ifndef D_HAS_STD_RCU
        #define D_HAS_STD_RCU 0
    #endif
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <atomic>
#include <cstddef>
#include <functional>
#include <new>
#include <vector>
// djinterp
#include "./atomic.hpp"
#include "./concurrency_strategy_tags.hpp"
#include "./sync_common.hpp"
#include "./reclamation_common.hpp"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t


NS_DJINTERP

// I.    Standard library delegation (C++23)
#if D_HAS_STD_RCU

    using rcu_domain = std::rcu_default_domain;

    template<typename Type>
    void rcu_retire(Type* _ptr)
    {
        std::rcu_retire(_ptr);
    }

#endif  // D_HAS_STD_RCU


// II.   Epoch counter
// Global epoch tracker. Writers call advance() to move
// to a new epoch after completing a modification.
// Readers snapshot the current epoch to mark the start
// of their critical section.
//
//   The counter itself is atomic_version (./atomic.hpp):
// a monotonic atomic uint64 whose bump() returns the
// PREVIOUS value, which is exactly advance()'s contract.
// This class keeps the epoch vocabulary - current(),
// advance(), and the `inactive` sentinel, none of which
// belong on a general-purpose version stamp - and lets
// atomic_version carry the atomic plumbing that was
// otherwise spelled out twice in the module.
//
//   nonmovable (./sync_common.hpp) states the ownership
// contract that was previously hand-written as a pair of
// deleted copy operations. It is stricter on purpose:
// the registry hands out references INTO the counter, so
// relocating one would strand them.  (The atomic_version
// member is itself immovable, so the traits are unchanged
// either way - the base makes the intent explicit rather
// than leaving it as a side effect.)

class epoch_counter
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(epoch_counter)

    // sentinel: a thread not in a critical section
    // stores this value in its local epoch slot.
    static constexpr re_std::uint64_t inactive =
        UINT64_MAX;

    epoch_counter() noexcept
        : m_global_epoch(0)
    {}

    // current
    //   returns the current global epoch.
    re_std::uint64_t current() const noexcept
    {
        return m_global_epoch.load(
            std::memory_order_acquire);
    }

    // advance
    //   moves to the next epoch. Called by writers
    // after completing a modification. Returns the
    // previous epoch.
    re_std::uint64_t advance() noexcept
    {
        return m_global_epoch.bump();
    }

private:
    atomic_version m_global_epoch;
};


// III.  Epoch guard
// RAII critical section marker for epoch-based
// reclamation. The reader records the current epoch
// on construction and resets to inactive on destruction.
//
// The local epoch slot is passed by reference so that
// the registry can inspect it to determine whether any
// readers are still in an old epoch.

class epoch_guard
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(epoch_guard)

    explicit epoch_guard(
        const epoch_counter&           _counter,
        std::atomic<re_std::uint64_t>& _local_epoch)
        noexcept
        : m_local(_local_epoch)
    {
        m_local.store(
            _counter.current(),
            std::memory_order_release);
    }

    ~epoch_guard()
    {
        m_local.store(
            epoch_counter::inactive,
            std::memory_order_release);
    }


private:
    std::atomic<re_std::uint64_t>& m_local;
};


// IV.   Epoch registry
// Tracks per-thread local epoch values. Each thread
// that participates in RCU reads registers a slot.
// The writer scans all slots to determine the minimum
// active epoch - any retired node from an epoch strictly
// less than this minimum is safe to reclaim.
//
// Slot allocation is lock-free (CAS on the active flag).

class epoch_registry
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(epoch_registry)

    struct slot
    {
        std::atomic<re_std::uint64_t> local_epoch;
        std::atomic<bool>             active;

        slot() noexcept
            : local_epoch(epoch_counter::inactive)
            , active(false)
        {}
    };

    explicit epoch_registry(
        std::size_t _max_threads = 64) noexcept
        : m_slots(nullptr)
        , m_capacity(_max_threads)
    {
        m_slots = new (std::nothrow)
            slot[_max_threads];
    }

    ~epoch_registry()
    {
        delete[] m_slots;
    }


    // --- slot management ---

    // register_thread
    //   claims a slot for the calling thread. Returns
    // a pointer to the slot's local_epoch atomic, or
    // nullptr if all slots are in use.
    std::atomic<re_std::uint64_t>*
    register_thread() noexcept
    {
        if (!m_slots)
        {
            return nullptr;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            bool expected = false;

            if (m_slots[i].active
                    .compare_exchange_strong(
                        expected, true,
                        std::memory_order_acq_rel))
            {
                m_slots[i].local_epoch.store(
                    epoch_counter::inactive,
                    std::memory_order_release);

                return &m_slots[i].local_epoch;
            }
        }

        return nullptr;
    }

    // unregister_thread
    //   releases a slot back to the pool.
    void unregister_thread(
        std::atomic<re_std::uint64_t>* _slot) noexcept
    {
        if ( (!_slot) ||
             (!m_slots) )
        {
            return;
        }

        // find the slot index
        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            if (&m_slots[i].local_epoch == _slot)
            {
                m_slots[i].local_epoch.store(
                    epoch_counter::inactive,
                    std::memory_order_release);

                m_slots[i].active.store(
                    false,
                    std::memory_order_release);

                return;
            }
        }
    }

    // --- scanning ---

    // min_active_epoch
    //   returns the minimum epoch among all active
    // reader slots. Retired nodes from epochs strictly
    // less than this value are safe to reclaim.
    // Returns epoch_counter::inactive if no readers
    // are active (all retired nodes are safe).
    re_std::uint64_t min_active_epoch() const noexcept
    {
        re_std::uint64_t min_epoch =
            epoch_counter::inactive;

        if (!m_slots)
        {
            return min_epoch;
        }

        for (std::size_t i = 0;
             i < m_capacity; ++i)
        {
            if (m_slots[i].active.load(
                    std::memory_order_acquire))
            {
                re_std::uint64_t e =
                    m_slots[i].local_epoch.load(
                        std::memory_order_acquire);

                if (e < min_epoch)
                {
                    min_epoch = e;
                }
            }
        }

        return min_epoch;
    }

    // --- queries ---

    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

private:
    slot*       m_slots;
    std::size_t m_capacity;
};


// V.    Deferred reclaimer
// Combines epoch_counter + epoch_registry + retired list
// into a single reclamation engine. Writers retire nodes
// through this object; it handles epoch tracking and
// reclamation automatically.
//
//   The retired list itself is reclaim_list<Type>
// (./reclamation_common.hpp) - the shared engine that
// hazard_pointer.hpp also rides. The two schemes only
// ever differed in the RULE for calling a node safe, so
// this class contributes exactly that rule (a node is
// reclaimable once its retire epoch lies strictly below
// the oldest epoch any reader still occupies) and keeps
// the RCU vocabulary around it.
//
//   ON THE SCAN THRESHOLD: reclaim_list reports when a
// scan looks worthwhile via should_scan(), but never
// scans itself - only the owner holds the predicate. So
// the automatic scan on retire lives here, exactly as it
// did before; the engine merely counts. The threshold is
// still max_threads * 2.

template<typename Type>
class deferred_reclaimer
{
public:
    // non-copyable, non-movable: readers hold pointers INTO the
    // registry this object owns. Macro rather than the base, so
    // the empty base of a nested member can still collapse.
    D_NONMOVABLE(deferred_reclaimer)

    using deleter_fn =
        typename reclaim_list<Type>::deleter_fn;

    explicit deferred_reclaimer(
        std::size_t _max_threads = 64)
        : m_registry(_max_threads)
    {
        m_retired.set_scan_threshold(
            _max_threads * 2);
    }

    // --- reader API ---

    // enter
    //   registers the calling thread (if not already)
    // and returns a pointer to its local epoch slot.
    // The caller should use epoch_guard with this slot.
    std::atomic<re_std::uint64_t>*
    register_reader() noexcept
    {
        return m_registry.register_thread();
    }

    // unregister_reader
    //   releases the calling thread's epoch slot.
    void unregister_reader(
        std::atomic<re_std::uint64_t>* _slot) noexcept
    {
        m_registry.unregister_thread(_slot);
    }

    // epoch
    //   provides access to the epoch counter for
    // constructing epoch_guard objects.
    const epoch_counter& epoch() const noexcept
    {
        return m_epoch;
    }

    // --- writer API ---

    // retire
    //   retires _ptr for deferred deletion. Advances
    // the epoch and triggers a scan if the retired
    // count exceeds the threshold.
    void retire(Type* _ptr)
    {
        const re_std::uint64_t e = m_epoch.advance();

        m_retired.retire(_ptr, e);

        if (m_retired.should_scan())
        {
            scan();
        }
    }

    // retire (custom deleter)
    void retire(
        Type*        _ptr,
        deleter_fn    _deleter)
    {
        const re_std::uint64_t e = m_epoch.advance();

        m_retired.retire(_ptr, std::move(_deleter), e);

        if (m_retired.should_scan())
        {
            scan();
        }
    }

    // scan
    //   reclaims all retired nodes from epochs that no
    // active reader is still observing.
    // Returns the number of nodes reclaimed.
    std::size_t scan()
    {
        const re_std::uint64_t safe =
            m_registry.min_active_epoch();

        return m_retired.reclaim_if(
            [safe](
                const typename reclaim_list<Type>::entry&
                    _entry)
            {
                return (_entry.tag < safe);
            });
    }

    // force_reclaim
    //   reclaims ALL retired nodes regardless of active
    // readers. ONLY safe to call during shutdown when
    // no readers are active.
    void force_reclaim()
    {
        m_retired.force_reclaim();
    }

    // --- queries ---

    std::size_t pending() const noexcept
    {
        return m_retired.pending();
    }

    re_std::uint64_t current_epoch() const noexcept
    {
        return m_epoch.current();
    }

private:
    epoch_counter       m_epoch;
    epoch_registry      m_registry;
    reclaim_list<Type> m_retired;
};


// VI.   Rcu protected value
// Complete RCU-protected value with read / update /
// snapshot operations. The canonical RCU pattern:
//
//   rcu_protected<Config> cfg(initial_config);
//
//   // Reader (any thread):
//   auto guard = cfg.read_lock();
//   const Config& c = cfg.read(guard);
//   // use c ...
//   // guard destructor exits critical section
//
//   // Writer (single writer assumed):
//   Config new_cfg = /* ... */;
//   cfg.update(new ConfigValue);
//   // old value is reclaimed once all readers exit

#if !D_HAS_STD_RCU

template<typename Type>
class rcu_protected
{
public:
    // non-copyable, non-movable: others hold references
    // or pointers INTO this object. The MACRO form is used
    // rather than the nonmovable base because these types
    // nest one another - two empty bases in one object need
    // distinct addresses, which defeats the empty base
    // optimization and would grow every one of them.
    D_NONMOVABLE(rcu_protected)

    // --- type aliases ---

    // rcu_protected_type
    //   alias: self-marker so that
    // `has_rcu_protected_type<rcu_protected<T>>`
    // reports true. Containers built on rcu_protected
    // typically forward this alias to identify themselves
    // as RCU-strategy.
    using rcu_protected_type = rcu_protected;

    // concurrency_strategy_tag
    //   alias: declares this type as RCU strategy.
    // Read by concurrency_strategy_traits.hpp tag-alias
    // fast path.
    using concurrency_strategy_tag = rcu_strategy_tag;

    // --- reader token ---

    // rcu_read_guard
    //   RAII token returned by read_lock(). Holds
    // the calling thread in the current epoch.
    class rcu_read_guard
    {
    public:
        rcu_read_guard(
            const epoch_counter&           _counter,
            std::atomic<re_std::uint64_t>& _slot)
            noexcept
            : m_guard(_counter, _slot)
        {}

        rcu_read_guard(const rcu_read_guard&)            = delete;
        rcu_read_guard& operator=(
            const rcu_read_guard&)                       = delete;
        rcu_read_guard(rcu_read_guard&&)                 = default;

    private:
        epoch_guard m_guard;
    };

    // --- constructors ---

    explicit rcu_protected(const Type& _initial)
        : m_reclaimer(64)
    {
        Type* p = new Type(_initial);
        m_data.store(p);

        m_reader_slot =
            m_reclaimer.register_reader();
    }

    explicit rcu_protected(Type&& _initial)
        : m_reclaimer(64)
    {
        Type* p = new Type(std::move(_initial));
        m_data.store(p);

        m_reader_slot =
            m_reclaimer.register_reader();
    }

    ~rcu_protected()
    {
        // reclaim the current value
        Type* p = m_data.load();

        delete p;

        // reclaim any pending retired values
        m_reclaimer.force_reclaim();

        if (m_reader_slot)
        {
            m_reclaimer.unregister_reader(
                m_reader_slot);
        }
    }


    // --- reader API ---

    // read_lock
    //   enters a read-side critical section. The
    // returned guard must be kept alive for the
    // duration of the read.
    rcu_read_guard read_lock()
    {
        return rcu_read_guard(
            m_reclaimer.epoch(),
            *m_reader_slot);
    }

    // read
    //   returns a const reference to the current value.
    // MUST be called while holding a rcu_read_guard.
    const Type& read(
        const rcu_read_guard& /*guard*/) const noexcept
    {
        return *m_data.load();
    }

    // --- writer API ---

    // update
    //   atomically replaces the current value with a
    // copy of _new_value. The old value is retired
    // for deferred reclamation.
    void update(const Type& _new_value)
    {
        Type* fresh = new Type(_new_value);
        Type* old   = m_data.publish(fresh);

        m_reclaimer.retire(old);
    }

    // update (move)
    void update(Type&& _new_value)
    {
        Type* fresh = new Type(std::move(_new_value));
        Type* old   = m_data.publish(fresh);

        m_reclaimer.retire(old);
    }

    // modify
    //   reads the current value, applies _fn to a copy,
    // and publishes the modified copy. NOT atomic with
    // respect to concurrent writers - use external
    // synchronization if multiple writers are possible.
    template<typename Fn>
    void modify(Fn&& _fn)
    {
        const Type* current = m_data.load();

        Type* fresh = new Type(*current);

        std::forward<Fn>(_fn)(*fresh);

        Type* old = m_data.publish(fresh);

        m_reclaimer.retire(old);
    }

    // --- queries ---

    std::size_t pending_reclamation() const noexcept
    {
        return m_reclaimer.pending();
    }

    re_std::uint64_t current_epoch() const noexcept
    {
        return m_reclaimer.current_epoch();
    }

private:
    published_ptr<Type>                m_data;
    deferred_reclaimer<Type>           m_reclaimer;
    std::atomic<re_std::uint64_t>*      m_reader_slot;
};

#endif  // !D_HAS_STD_RCU


NS_END  // djinterp

#endif  // C++11


#endif  // DJINTERP_SYNC_RCU_HPP
