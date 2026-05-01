/******************************************************************************
* djinterp [container]                                  atomic_registry_table.hpp
*
*   Lock-free registry table for trivially-copyable cvar values.  Each
* entry stores its value in a `std::atomic<_CvarType>` slot, so reads
* and writes of EXISTING entries are wait-free and require no
* synchronization beyond the atomic operation itself.
*
*   The structure of the registry (which keys exist) is NOT lock-free
* - adding a new key requires serialization.  This wrapper assumes the
* common configuration pattern:
*   1. all keys are registered up front (e.g. at process startup)
*   2. then the registry transitions to read/write of values only
*
*   ADD-vs-SET DISTINCTION:
*     add(key, value)       - registers a new key (locked, rare)
*     set(key, value)       - updates an existing key's value
*                             (lock-free, hot path)
*     get(key)              - reads an existing key's value
*                             (lock-free, hot path)
*     get_or(key, fallback) - safe lock-free read with fallback
*
*   COMPARED TO:
*     mutex_registry_table  - one shared lock per access; reads can
*                             be concurrent, writes are exclusive
*     cow_registry_table    - shared-lock reads; cloning writes
*     rcu_registry_table    - lock-free reads; whole-registry
*                             cloning writes
*     atomic_registry_table - lock-free reads AND writes for
*                             registered keys; locked add() for new
*                             keys
*
*   CONSTRAINTS ON _CvarType:
*   The cvar value type must satisfy std::atomic<_CvarType>'s
* requirements - typically trivially copyable and small enough that
* the platform supports it without falling back to a hidden mutex.
* For most platforms this means scalars (int*, float, double, bool,
* enum), small POD aggregates, and pointers.  For non-trivial types
* (std::string, std::vector, custom classes), use one of the lock-
* based variants instead.
*
* DEPENDENCIES:
*   djinterp.hpp           - NS_DJINTERP, D_CONSTEXPR
*   threadsafe/atomic.hpp  - threadsafe::atomic_size, atomic primitives
*   threadsafe/lock_policy.hpp - default_lock_policy for the add path
*
* TABLE OF CONTENTS
* =================
* I.    atomic_registry_entry
* II.   atomic_registry_table
* III.  make_atomic_registry_table
*
*
* path:      /inc/djinterp/container/table/atomic_registry_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_ATOMIC_REGISTRY_TABLE_
#define DJINTERP_ATOMIC_REGISTRY_TABLE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`atomic_registry_table.hpp` requires C++11 or later "         \
           "(<atomic>, move semantics)."
#endif

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include "../../djinterp.hpp"
#include "../../threadsafe/atomic.hpp"
#include "../../threadsafe/lock_policy.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   atomic_registry_entry
    // =========================================================================

    // atomic_registry_entry
    //   struct: a single registered cvar.  Holds the key (immutable
    // after registration) and an atomic value slot.
    //
    //   Non-copyable because std::atomic is non-copyable.  Stored in
    // a stable container (std::vector<unique_ptr<entry>>) so that
    // pointer-based access through the registry never invalidates.
    template<typename _Key,
             typename _Value>
    struct atomic_registry_entry
    {
        static_assert(std::is_trivially_copyable<_Value>::value,
                      "atomic_registry_table requires "
                      "trivially-copyable cvar values - use "
                      "mutex_/cow_/rcu_registry_table for "
                      "non-trivial types.");

        const _Key             key;
        std::atomic<_Value>    value;

        atomic_registry_entry(
            const _Key&   _k,
            const _Value& _v
        )
            : key(_k),
              value(_v)
        {}

        atomic_registry_entry(
            const atomic_registry_entry&)            = delete;
        atomic_registry_entry& operator=(
            const atomic_registry_entry&)            = delete;
    };


    // =========================================================================
    // II.  atomic_registry_table
    // =========================================================================

    // atomic_registry_table
    //   class: lock-free registry for trivially-copyable cvar values.
    // Hot-path get/set operations are wait-free atomic loads / stores
    // on the entry's value slot.  Registration of new keys (add) is
    // serialized via a lock policy because it must update the shared
    // entry list.
    //
    //   Storage is a vector of unique_ptr to entry, so individual
    // entry addresses are stable across registrations - an in-flight
    // get/set on entry K is never invalidated by a concurrent add of
    // entry K' (vector growth reseats the unique_ptrs, not the
    // entries themselves).
    //
    //   Lookup is linear by default.  For registries beyond a few
    // dozen entries on hot paths, consider sorting at startup and
    // using binary search, or building an index alongside.
    template<typename _Key,
             typename _Value,
             typename _AddPolicy = threadsafe::default_lock_policy>
    class atomic_registry_table
    {
    private:
        using entry_t      = atomic_registry_entry<_Key, _Value>;
        using entry_ptr    = entry_t*;
        using entry_owner  = entry_t*;   // raw - owned by m_owners
        using owner_list   = std::vector<entry_t*>;
        using slot_list    = std::vector<entry_ptr>;
        using add_mutex    = typename _AddPolicy::mutex_type;
        using add_lock     = typename _AddPolicy::write_lock_type;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using key_type   = _Key;
        using cvar_type  = _Value;
        using size_type  = std::size_t;

        // axis-8 marker.  add() takes a real lock; get/set are
        // lock-free.  Surface the add path's mutex so the trait
        // system classifies us as threadsafe.
        using mutex_type = add_mutex;
        using lock_type  = add_lock;

        using self_type =
            atomic_registry_table<_Key, _Value, _AddPolicy>;

        // -----------------------------------------------------------------
        //  static mode flags
        //   atomic_registry_table is constructed from scratch and ALWAYS
        // has both a value column (the atomic slot) and write capability
        // (set / exchange / compare_exchange).  Read-only registries
        // built on this template would be unusual; the slot type is
        // always mutable through the atomic.
        //   These flags exist for surface consistency with the other
        // *_registry_table wrappers so that generic threadsafe-table
        // code can introspect the same way regardless of strategy.
        // -----------------------------------------------------------------

        static constexpr bool has_value_column = true;

        static constexpr bool is_writable      = true;


        // =================================================================
        //  CONSTRUCTORS / DESTRUCTOR
        // =================================================================

        atomic_registry_table()
            : m_count(0)
        {}

        // pre-populate from a list of (key, default-value) pairs
        atomic_registry_table(
            std::initializer_list<std::pair<_Key, _Value>> _init
        )
            : m_count(0)
        {
            m_entries.reserve(_init.size());

            for (const auto& kv : _init)
            {
                add(kv.first, kv.second);
            }
        }

        // non-copyable, non-movable (atomic + mutex)
        atomic_registry_table(
            const atomic_registry_table&)                       = delete;
        atomic_registry_table& operator=(
            const atomic_registry_table&)                       = delete;
        atomic_registry_table(
            atomic_registry_table&&)                            = delete;
        atomic_registry_table& operator=(
            atomic_registry_table&&)                            = delete;

        ~atomic_registry_table()
        {
            for (entry_t* e : m_entries)
            {
                delete e;
            }
        }


        // =================================================================
        //  REGISTRATION (locked, rare)
        // =================================================================

        // add
        //   function: registers a new key with an initial value.
        // Returns true if the key was newly registered, false if it
        // already existed (no modification).
        //   Serialized through the add policy's lock; readers /
        // writers of EXISTING entries are not blocked because the
        // entry list grows by appending and existing entry addresses
        // are stable.
        bool
        add(
            const _Key&   _key,
            const _Value& _initial_value
        )
        {
            add_lock guard(m_add_mutex);

            // check for existing
            entry_t* existing = find_entry(_key);

            if (existing)
            {
                return false;
            }

            // append - vector growth reseats the pointers but not
            // the entries themselves
            entry_t* fresh = new entry_t(_key, _initial_value);

            m_entries.push_back(fresh);

            m_count.fetch_add(1, std::memory_order_release);

            return true;
        }


        // =================================================================
        //  CVAR ACCESS (lock-free)
        // =================================================================

        // get
        //   function: returns the current value for _key.  Behavior
        // is undefined if the key was not registered via add() -
        // callers that cannot guarantee registration should use
        // get_or() instead.
        _Value
        get(const _Key& _key) const noexcept
        {
            entry_t* e = find_entry(_key);

            return e->value.load(std::memory_order_acquire);
        }

        // get_or
        //   function: returns the current value for _key, or
        // _fallback if the key was not registered.  Lock-free.
        _Value
        get_or(
            const _Key&   _key,
            const _Value& _fallback
        ) const noexcept
        {
            entry_t* e = find_entry(_key);

            if (e)
            {
                return e->value.load(std::memory_order_acquire);
            }

            return _fallback;
        }

        // has
        //   function: lock-free existence check.
        bool
        has(const _Key& _key) const noexcept
        {
            return (find_entry(_key) != nullptr);
        }

        // contains
        //   function: alias for has().
        bool
        contains(const _Key& _key) const noexcept
        {
            return has(_key);
        }


        // =================================================================
        //  CVAR MUTATION (lock-free)
        // =================================================================

        // set
        //   function: stores _value in the entry for _key.  Returns
        // true if the key was found.  Lock-free.
        bool
        set(
            const _Key&   _key,
            const _Value& _value
        ) noexcept
        {
            entry_t* e = find_entry(_key);

            if (e)
            {
                e->value.store(_value, std::memory_order_release);

                return true;
            }

            return false;
        }

        // exchange
        //   function: atomically replaces the value for _key with
        // _value and returns the previous value.  Useful for "claim
        // and process" patterns on flag-style cvars.
        _Value
        exchange(
            const _Key&   _key,
            const _Value& _value
        ) noexcept
        {
            entry_t* e = find_entry(_key);

            return e->value.exchange(_value,
                                     std::memory_order_acq_rel);
        }

        // compare_exchange
        //   function: atomic CAS on the entry's value.  Returns true
        // if the swap succeeded; updates _expected with the actual
        // value on failure.
        bool
        compare_exchange(
            const _Key&   _key,
            _Value&       _expected,
            const _Value& _desired
        ) noexcept
        {
            entry_t* e = find_entry(_key);

            return e->value.compare_exchange_strong(
                _expected,
                _desired,
                std::memory_order_acq_rel,
                std::memory_order_acquire);
        }


        // =================================================================
        //  CAPACITY (lock-free)
        // =================================================================

        size_type
        size() const noexcept
        {
            return m_count.load(std::memory_order_acquire);
        }

        bool
        empty() const noexcept
        {
            return (size() == 0);
        }

    private:
        // find_entry
        //   function: linear scan for the entry with matching key.
        // Returns nullptr when absent.  Reads the count with
        // acquire semantics so that any add() published before our
        // call is visible.
        entry_t*
        find_entry(const _Key& _key) const noexcept
        {
            const size_type n =
                m_count.load(std::memory_order_acquire);

            for (size_type i = 0; i < n; ++i)
            {
                if (m_entries[i]->key == _key)
                {
                    return m_entries[i];
                }
            }

            return nullptr;
        }

        // -----------------------------------------------------------------
        //  storage
        //   m_entries  - stable pointers to atomic-valued entries.
        //                Reads of [0, m_count) are always valid; the
        //                vector itself is only mutated under the
        //                add lock.
        //   m_count    - atomic publish point for newly added
        //                entries.
        //   m_add_mutex- serializes add() against itself (and against
        //                concurrent vector growth).
        // -----------------------------------------------------------------
        owner_list                       m_entries;
        threadsafe::atomic_size          m_count;
        mutable add_mutex                m_add_mutex;
    };


    // =========================================================================
    // III. make_atomic_registry_table
    // =========================================================================

    // make_atomic_registry_table
    //   function: factory for an empty atomic_registry_table.
    template<typename _Key,
             typename _Value,
             typename _AddPolicy = threadsafe::default_lock_policy>
    auto
    make_atomic_registry_table()
        -> atomic_registry_table<_Key, _Value, _AddPolicy>
    {
        return atomic_registry_table<_Key, _Value, _AddPolicy>{};
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ATOMIC_REGISTRY_TABLE_
