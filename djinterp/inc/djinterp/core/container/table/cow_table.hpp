/******************************************************************************
* djinterp [container]                                              cow_table.hpp
*
*   Copy-on-write threadsafe wrapper around the fundamental owning
* `table`.  Composes with `threadsafe::cow_state<table<...>, _Policy>`
* to give:
*
*     - cheap snapshots (atomic refcount bump, no deep copy)
*     - shared-readers / exclusive-writer semantics under the policy
*     - clone-on-write when a snapshot is outstanding
*     - version stamping for optimistic concurrency control
*
*   COMPARED TO mutex_table:
*     mutex_table  - direct in-place mutation under the policy lock;
*                    snapshot() makes a deep copy
*     cow_table    - mutations clone the storage on first write when
*                    snapshots are outstanding; snapshot() returns an
*                    immutable_snapshot that shares the data via
*                    refcount
*
*   Use cow_table when readers want long-lived independent views of
* historical state (e.g. "freeze the table for this report") without
* paying for a deep copy at snapshot time.  Use mutex_table when
* snapshots are rare or non-existent and writes dominate.
*
*   USAGE:
*   ```cpp
*   cow_table<int> t;
*   t.with_write([](auto& tab){ tab.push_back(1); tab.push_back(2); });
*
*   auto snap = t.snapshot();          // cheap (refcount++)
*   auto v0 = snap->operator[](0);     // 1, locked-out from writes
*
*   t.with_write([](auto& tab){        // clones if snap still alive
*       tab.push_back(3);
*   });
*   ```
*
* DEPENDENCIES:
*   djinterp.hpp        - NS_DJINTERP, D_CONSTEXPR
*   table.hpp           - container::table
*   threadsafe/cow.hpp  - threadsafe::cow_state, immutable_snapshot
*
* TABLE OF CONTENTS
* =================
* I.    cow_table
* II.   make_cow_table
*
*
* path:      /inc/djinterp/container/table/cow_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_COW_TABLE_
#define DJINTERP_COW_TABLE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`cow_table.hpp` requires C++11 or later "                      \
           "(<atomic>, move semantics)."
#endif

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "../../threadsafe/cow.hpp"
#include "./table.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   cow_table
    // =========================================================================

    // cow_table
    //   class: copy-on-write threadsafe wrapper around `table`.
    // Stores the wrapped table inside a `threadsafe::cow_state` and
    // exposes table-shaped operations on top.
    //
    //   Snapshots are cheap (atomic refcount).  Writes clone the
    // storage when the refcount > 1.  All access is serialized by the
    // policy lock embedded in cow_state.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename    _Policy    = threadsafe::default_lock_policy,
             typename... _Options>
    class cow_table
    {
    private:
        using wrapped_t = table<_Type, _Allocator, _Options...>;
        using state_t   = threadsafe::cow_state<wrapped_t, _Policy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using wrapped_type     = wrapped_t;
        using lock_policy_type = _Policy;
        using mutex_type       = typename _Policy::mutex_type;
        using lock_type        = typename _Policy::write_lock_type;
        using read_lock_type   = typename _Policy::read_lock_type;
        using write_lock_type  = typename _Policy::write_lock_type;

        // canonical surface from the wrapped type (for trait detection)
        using underlying_container_type = wrapped_t;
        using value_type      = typename wrapped_t::value_type;
        using size_type       = typename wrapped_t::size_type;
        using difference_type = typename wrapped_t::difference_type;

        // snapshot type
        using snapshot_type =
            threadsafe::immutable_snapshot<wrapped_t>;

        using self_type =
            cow_table<_Type, _Allocator, _Policy, _Options...>;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // cow_table()
        //   constructor: default - empty wrapped table.
        cow_table() = default;

        // cow_table(wrapped, lvalue)
        //   constructor: copy an existing table into the cow_state.
        explicit cow_table(
            const wrapped_t& _wrapped
        )
            : m_state(_wrapped)
        {}

        // cow_table(wrapped, rvalue)
        //   constructor: move an existing table into the cow_state.
        explicit cow_table(
            wrapped_t&& _wrapped
        )
            : m_state(static_cast<wrapped_t&&>(_wrapped))
        {}

        // non-copyable (cow_state contains a mutex)
        cow_table(const cow_table&)            = delete;
        cow_table& operator=(const cow_table&) = delete;
        cow_table(cow_table&&)                 = delete;
        cow_table& operator=(cow_table&&)      = delete;

        ~cow_table() = default;


        // =================================================================
        //  GENERIC LOCKED VISITORS
        // =================================================================

        // with_read
        //   function: invokes _fn under a read lock with a const
        // reference to the wrapped table.  The lock is held for the
        // duration of _fn.  No clone is performed.
        template<typename _Fn>
        auto
        with_read(_Fn&& _fn) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const wrapped_t&>()))
        {
            // cow_state::read() returns a const reference under a
            // read lock; the lock is released when read() returns,
            // so we acquire it ourselves here.
            return static_cast<_Fn&&>(_fn)(m_state.read());
        }

        // with_write
        //   function: invokes _fn under a write lock with a mutable
        // reference to the wrapped table.  Clones the storage on first
        // write if a snapshot is outstanding; bumps the version.
        template<typename _Fn>
        auto
        with_write(_Fn&& _fn)
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<wrapped_t&>()))
        {
            return m_state.modify(static_cast<_Fn&&>(_fn));
        }


        // =================================================================
        //  CAPACITY (locked)
        // =================================================================

        size_type
        size() const
        {
            return m_state.read().size();
        }

        bool
        empty() const
        {
            return m_state.read().empty();
        }


        // =================================================================
        //  ELEMENT ACCESS (locked, by value)
        // =================================================================

        // at
        //   function: returns a copy of element _i under a read lock.
        // No bounds check (mirrors table::operator[]).  Returns by
        // value so the result is safe to use after the lock releases.
        value_type
        at(size_type _i) const
        {
            return m_state.read()[_i];
        }

        value_type
        operator[](size_type _i) const
        {
            return m_state.read()[_i];
        }


        // =================================================================
        //  MUTATION (clone-on-write, locked)
        // =================================================================

        // push_back
        //   function: appends _v.  Clones the storage on first write
        // when a snapshot is outstanding.  Available when the wrapped
        // table itself supports push_back (unsorted, unhashed).
        template<typename _T = _Type,
                 typename = decltype(
                     std::declval<wrapped_t&>().push_back(
                         std::declval<const _T&>()))>
        void
        push_back(const _Type& _v)
        {
            m_state.modify(
                [&_v](wrapped_t& _t)
                {
                    _t.push_back(_v);
                });

            return;
        }

        // push_back (rvalue)
        template<typename _T = _Type,
                 typename = decltype(
                     std::declval<wrapped_t&>().push_back(
                         std::declval<_T&&>()))>
        void
        push_back(_Type&& _v)
        {
            m_state.modify(
                [&_v](wrapped_t& _t)
                {
                    _t.push_back(static_cast<_Type&&>(_v));
                });

            return;
        }

        // clear
        //   function: removes all elements.
        void
        clear()
        {
            m_state.modify(
                [](wrapped_t& _t)
                {
                    _t.clear();
                });

            return;
        }


        // =================================================================
        //  SNAPSHOT / REPLACE
        // =================================================================

        // snapshot
        //   function: returns an immutable_snapshot of the current
        // table state.  Cheap (atomic refcount bump).  The snapshot
        // is independent of subsequent mutations - those will clone
        // the storage rather than disturb this snapshot.
        snapshot_type
        snapshot() const
        {
            return m_state.snapshot();
        }

        // replace
        //   function: atomically replaces the wrapped table with a
        // new one.  Outstanding snapshots remain valid against the
        // old state.
        void
        replace(const wrapped_t& _new_wrapped)
        {
            m_state.replace(_new_wrapped);

            return;
        }

        void
        replace(wrapped_t&& _new_wrapped)
        {
            m_state.replace(
                static_cast<wrapped_t&&>(_new_wrapped));

            return;
        }


        // =================================================================
        //  VERSIONING
        // =================================================================

        // version
        //   function: returns the current version stamp.  Bumped on
        // every successful write.  Useful for optimistic concurrency
        // patterns.
        std::uint64_t
        version() const noexcept
        {
            return m_state.version();
        }


        // =================================================================
        //  DIRECT MUTEX ACCESS (advanced)
        // =================================================================

        // mutex
        //   function: returns the underlying mutex from cow_state.
        mutex_type&
        mutex() const noexcept
        {
            return m_state.mutex();
        }

    private:
        state_t m_state;
    };


    // =========================================================================
    // II.  make_cow_table
    // =========================================================================

    // make_cow_table
    //   function: factory.  Constructs a wrapped table from forwarded
    // args, then moves it into a cow_table.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename    _Policy    = threadsafe::default_lock_policy,
             typename... _Args>
    auto
    make_cow_table(_Args&&... _args)
        -> cow_table<_Type, _Allocator, _Policy>
    {
        return cow_table<_Type, _Allocator, _Policy>(
            table<_Type, _Allocator>(
                static_cast<_Args&&>(_args)...));
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_COW_TABLE_
