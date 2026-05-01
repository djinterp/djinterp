/******************************************************************************
* djinterp [container]                                              rcu_table.hpp
*
*   Read-Copy-Update threadsafe wrapper around the fundamental owning
* `table`.  Composes with `threadsafe::rcu_protected<table<...>>` to
* give:
*
*     - LOCK-FREE reads (one acquire load on a per-thread epoch slot)
*     - writers publish a new generation; the old one is retired and
*       reclaimed once all readers in its epoch have completed
*     - SINGLE-WRITER assumed; multiple writers require external
*       synchronization
*
*   COMPARED TO:
*     mutex_table  - reads serialize through a mutex (shared lock at
*                    best); writes are exclusive
*     cow_table    - reads take a shared lock; writes clone-on-write
*                    when refcounted snapshots exist
*     rcu_table    - reads are lock-free; writes always clone the
*                    full table and atomically swap the pointer
*
*   Use rcu_table when reads vastly outnumber writes AND the table is
* small enough that whole-table cloning on each write is acceptable
* (configuration tables, routing tables, dispatch tables).  Avoid
* rcu_table for large tables with frequent partial mutations - the
* per-write deep copy will dominate.
*
*   READER PATTERN:
*   ```cpp
*   {
*       auto guard = t.read_lock();
*       const auto& tab = t.read(guard);   // const ref valid for guard
*       // ... read tab ...
*   }   // guard destruction ends the read-side critical section
*   ```
*
*   Or via the visitor:
*   ```cpp
*   auto sz = t.with_read([](const auto& tab){ return tab.size(); });
*   ```
*
*   WRITER PATTERN:
*   ```cpp
*   t.update(new_table);          // wholesale replace
*   t.modify([](auto& tab){       // read-modify-publish
*       tab.push_back(42);
*   });
*   ```
*
* DEPENDENCIES:
*   djinterp.hpp        - NS_DJINTERP, D_CONSTEXPR
*   table.hpp           - container::table
*   threadsafe/rcu.hpp  - threadsafe::rcu_protected
*
* TABLE OF CONTENTS
* =================
* I.    rcu_table
* II.   make_rcu_table
*
*
* path:      /inc/djinterp/container/table/rcu_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RCU_TABLE_
#define DJINTERP_RCU_TABLE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`rcu_table.hpp` requires C++11 or later "                      \
           "(<atomic>, move semantics)."
#endif

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "../../threadsafe/rcu.hpp"
#include "./table.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   rcu_table
    // =========================================================================

    // rcu_table
    //   class: RCU-protected threadsafe wrapper around `table`.
    // Stores the wrapped table inside a `threadsafe::rcu_protected`
    // and exposes table-shaped operations on top.
    //
    //   Readers take a cheap epoch token (no mutex) and read the
    // current generation via const reference.  Writers publish a new
    // generation by replacing the underlying pointer; the old
    // generation is retired and reclaimed once no reader in its epoch
    // remains.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename... _Options>
    class rcu_table
    {
    private:
        using wrapped_t = table<_Type, _Allocator, _Options...>;
        using rcu_t     = threadsafe::rcu_protected<wrapped_t>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using wrapped_type = wrapped_t;
        using rcu_type     = rcu_t;
        using read_guard   = typename rcu_t::rcu_read_guard;

        // canonical surface from the wrapped type (for trait detection)
        using underlying_container_type = wrapped_t;
        using value_type      = typename wrapped_t::value_type;
        using size_type       = typename wrapped_t::size_type;
        using difference_type = typename wrapped_t::difference_type;

        // axis-8 marker: rcu_table is threadsafe even though there's
        // no mutex.  We surface ::lock_type as the read guard so the
        // structural detection (presence of lock_type) classifies us
        // as threadsafe; mutex_type points to a no-op marker.
        using lock_type  = read_guard;
        using mutex_type = threadsafe::no_op_mutex;

        using self_type =
            rcu_table<_Type, _Allocator, _Options...>;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // rcu_table()
        //   constructor: empty wrapped table is the initial generation.
        rcu_table()
            : m_rcu(wrapped_t{})
        {}

        // rcu_table(wrapped, lvalue)
        explicit rcu_table(
            const wrapped_t& _initial
        )
            : m_rcu(_initial)
        {}

        // rcu_table(wrapped, rvalue)
        explicit rcu_table(
            wrapped_t&& _initial
        )
            : m_rcu(static_cast<wrapped_t&&>(_initial))
        {}

        // non-copyable, non-movable (rcu_protected manages epoch
        // registration; wrapper instances are tied to a specific
        // domain registration)
        rcu_table(const rcu_table&)            = delete;
        rcu_table& operator=(const rcu_table&) = delete;
        rcu_table(rcu_table&&)                 = delete;
        rcu_table& operator=(rcu_table&&)      = delete;

        ~rcu_table() = default;


        // =================================================================
        //  READER API (lock-free)
        // =================================================================

        // read_lock
        //   function: enters a read-side critical section.  The
        // returned guard must outlive any references obtained via
        // read(guard).
        read_guard
        read_lock()
        {
            return m_rcu.read_lock();
        }

        // read
        //   function: returns a const reference to the current
        // generation.  The reference is valid only while _guard is
        // alive.
        const wrapped_t&
        read(const read_guard& _guard) const noexcept
        {
            return m_rcu.read(_guard);
        }

        // with_read
        //   function: convenience visitor.  Acquires a read guard,
        // invokes _fn with a const reference to the current
        // generation, and releases the guard on return.
        template<typename _Fn>
        auto
        with_read(_Fn&& _fn) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const wrapped_t&>()))
        {
            // const_cast because rcu_protected::read_lock() is a
            // mutating operation on the per-thread reader slot, even
            // though semantically the caller is a reader.
            auto& nc = const_cast<rcu_t&>(m_rcu);
            auto  g  = nc.read_lock();

            return static_cast<_Fn&&>(_fn)(nc.read(g));
        }


        // =================================================================
        //  WRITER API (single-writer assumed)
        // =================================================================

        // update
        //   function: publishes _new_table as the next generation.
        // The previous generation is retired for deferred reclamation.
        void
        update(const wrapped_t& _new_table)
        {
            m_rcu.update(_new_table);

            return;
        }

        // update (rvalue)
        void
        update(wrapped_t&& _new_table)
        {
            m_rcu.update(static_cast<wrapped_t&&>(_new_table));

            return;
        }

        // modify
        //   function: reads the current generation, applies _fn to a
        // mutable copy, and publishes the modified copy as the next
        // generation.  NOT atomic with respect to concurrent writers
        // - external synchronization required if multiple writers
        // exist.
        template<typename _Fn>
        void
        modify(_Fn&& _fn)
        {
            m_rcu.modify(static_cast<_Fn&&>(_fn));

            return;
        }


        // =================================================================
        //  CONVENIENCE READERS
        // =================================================================

        // size
        //   function: lock-free read of the current generation's size.
        size_type
        size() const
        {
            return with_read(
                [](const wrapped_t& _t)
                {
                    return _t.size();
                });
        }

        // empty
        //   function: lock-free read of the current generation's
        // emptiness.
        bool
        empty() const
        {
            return with_read(
                [](const wrapped_t& _t)
                {
                    return _t.empty();
                });
        }

        // at
        //   function: lock-free read of element _i from the current
        // generation.  Returns by value - the reader's view of the
        // generation is bounded by the read guard's lifetime.
        value_type
        at(size_type _i) const
        {
            return with_read(
                [_i](const wrapped_t& _t)
                {
                    return _t[_i];
                });
        }


        // =================================================================
        //  RECLAMATION QUERIES
        // =================================================================

        // pending_reclamation
        //   function: number of retired generations awaiting
        // reclamation.
        std::size_t
        pending_reclamation() const noexcept
        {
            return m_rcu.pending_reclamation();
        }

        // current_epoch
        //   function: the writer's current epoch counter.
        std::uint64_t
        current_epoch() const noexcept
        {
            return m_rcu.current_epoch();
        }


        // =================================================================
        //  RAW RCU ACCESS (advanced)
        // =================================================================

        // rcu
        //   function: direct access to the underlying rcu_protected.
        rcu_t&
        rcu() noexcept
        {
            return m_rcu;
        }

        const rcu_t&
        rcu() const noexcept
        {
            return m_rcu;
        }

    private:
        rcu_t m_rcu;
    };


    // =========================================================================
    // II.  make_rcu_table
    // =========================================================================

    // make_rcu_table
    //   function: factory.  Constructs a wrapped table from forwarded
    // args, then moves it into an rcu_table.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename... _Args>
    auto
    make_rcu_table(_Args&&... _args)
        -> rcu_table<_Type, _Allocator>
    {
        return rcu_table<_Type, _Allocator>(
            table<_Type, _Allocator>(
                static_cast<_Args&&>(_args)...));
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_RCU_TABLE_
