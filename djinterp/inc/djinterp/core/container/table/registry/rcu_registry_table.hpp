/******************************************************************************
* djinterp [container]                                    rcu_registry_table.hpp
*
*   Read-Copy-Update threadsafe wrapper around `registry_table`.  Following
* the registry_table redesign, this wrapper handles both modes of the
* unified type:
*
*     ALWAYS AVAILABLE   (any registry_table)
*       contains, count, find_copy_or, find_visit,
*       get<MP> (column projection, C++17+)
*
*     SFINAE-GATED       (only when wrapped exposes them)
*       get, get_or, has, set
*
*   Composes with `threadsafe::rcu_protected<registry_table>` to give
* lock-free reads against a stable generation; writers atomically swap
* to a new generation and the old one is retired for deferred
* reclamation.
*
*   COMPARED TO:
*     mutex_registry_table  - reads serialize through shared lock
*     cow_registry_table    - reads take shared lock; writes clone
*     rcu_registry_table    - reads are lock-free; writes deep-copy
*                             the entire registry and atomically swap
*     atomic_registry_table - lock-free per-cvar atomics
*
*   USE CASE:
*   Hot read paths against a registry that mutates infrequently
* (dispatch tables, routing tables, mostly-static cvar configurations
* loaded once at startup and rarely reloaded).  SINGLE-WRITER assumed -
* concurrent writers require external synchronization.
*
*   For a NON-RCU pure lookup wrapper, use this with a value-column-
* less registry; the cvar methods are SFINAE-absent and the wrapper
* serves the read-mostly DNS-like map use case.
*
* DEPENDENCIES:
*   djinterp.hpp        - NS_DJINTERP, D_CONSTEXPR
*   threadsafe/rcu.hpp  - threadsafe::rcu_protected
*
* TABLE OF CONTENTS
* =================
* I.    Internal helpers (SFINAE detectors, conditional cvar_type alias)
* II.   rcu_registry_table
* III.  make_rcu_registry_table
*
*
* path:      /inc/djinterp/container/table/rcu_registry_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RCU_REGISTRY_TABLE_
#define DJINTERP_RCU_REGISTRY_TABLE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`rcu_registry_table.hpp` requires C++11 or later."
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "../../threadsafe/lock_policy.hpp"
#include "../../threadsafe/rcu.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   INTERNAL HELPERS
    // =========================================================================

    NS_INTERNAL

        // rrt_has_cvar_type
        //   trait: detects ::cvar_type alias on the wrapped type.
        template<typename _RT,
                 typename = void>
        struct rrt_has_cvar_type : std::false_type
        {};

        template<typename _RT>
        struct rrt_has_cvar_type<_RT,
            void_t<typename _RT::cvar_type>>
            : std::true_type
        {};

        // rrt_has_set
        //   trait: detects set(key, value) on the wrapped type.
        template<typename _RT,
                 typename = void>
        struct rrt_has_set : std::false_type
        {};

        template<typename _RT>
        struct rrt_has_set<_RT,
            void_t<
                decltype(
                    std::declval<_RT&>().set(
                        std::declval<const typename _RT::key_type&>(),
                        std::declval<const typename _RT::cvar_type&>()))
            >> : std::true_type
        {};

        // rrt_cvar_alias
        //   trait: surfaces ::cvar_type only when wrapped has one.
        template<typename _RT,
                 bool     _Has = rrt_has_cvar_type<_RT>::value>
        struct rrt_cvar_alias
        {};

        template<typename _RT>
        struct rrt_cvar_alias<_RT, true>
        {
            using cvar_type = typename _RT::cvar_type;
        };

    NS_END  // internal


    // =========================================================================
    // II.  rcu_registry_table
    // =========================================================================

    // rcu_registry_table
    //   class: RCU-protected threadsafe wrapper around any type
    // satisfying the registry_table structural contract.  Readers are
    // lock-free; writers replace the entire generation.
    template<typename _RegistryTable>
    class rcu_registry_table
        : public internal::rrt_cvar_alias<_RegistryTable>
    {
    private:
        using rcu_t = threadsafe::rcu_protected<_RegistryTable>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using wrapped_type = _RegistryTable;
        using rcu_type     = rcu_t;
        using read_guard   = typename rcu_t::rcu_read_guard;

        using underlying_container_type = _RegistryTable;
        using key_type     = typename _RegistryTable::key_type;
        using element_type = typename _RegistryTable::element_type;
        using size_type    = typename _RegistryTable::size_type;

        // axis-8 marker (see rcu_table for rationale): rcu_registry_table
        // is threadsafe via epoch tracking, no mutex.  Surface
        // ::lock_type and ::mutex_type so structural detection
        // classifies us as threadsafe.
        using lock_type  = read_guard;
        using mutex_type = threadsafe::no_op_mutex;

        // cvar_type comes from internal::rrt_cvar_alias when present.

        using self_type = rcu_registry_table<_RegistryTable>;

        // -----------------------------------------------------------------
        //  static mode flags
        // -----------------------------------------------------------------

        static constexpr bool has_value_column =
            internal::rrt_has_cvar_type<_RegistryTable>::value;

        static constexpr bool is_writable =
            internal::rrt_has_set<_RegistryTable>::value;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        explicit rcu_registry_table(
            const _RegistryTable& _initial
        )
            : m_rcu(_initial)
        {}

        explicit rcu_registry_table(
            _RegistryTable&& _initial
        )
            : m_rcu(static_cast<_RegistryTable&&>(_initial))
        {}

        rcu_registry_table(
            const rcu_registry_table&)                       = delete;
        rcu_registry_table& operator=(
            const rcu_registry_table&)                       = delete;
        rcu_registry_table(rcu_registry_table&&)             = delete;
        rcu_registry_table& operator=(rcu_registry_table&&)  = delete;

        ~rcu_registry_table() = default;


        // =================================================================
        //  READER API (lock-free)
        // =================================================================

        read_guard
        read_lock()
        {
            return m_rcu.read_lock();
        }

        const _RegistryTable&
        read(const read_guard& _guard) const noexcept
        {
            return m_rcu.read(_guard);
        }

        // with_read
        //   function: convenience visitor.  Acquires a read guard,
        // invokes _fn, and releases the guard on return.
        template<typename _Fn>
        auto
        with_read(_Fn&& _fn) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const _RegistryTable&>()))
        {
            // const_cast: rcu_protected::read_lock() mutates per-thread
            // reader slot state, even though the caller is semantically
            // a reader.
            auto& nc = const_cast<rcu_t&>(m_rcu);
            auto  g  = nc.read_lock();

            return static_cast<_Fn&&>(_fn)(nc.read(g));
        }


        // =================================================================
        //  KEY-LEVEL QUERIES (lock-free, always available)
        // =================================================================

        bool
        contains(const key_type& _key) const
        {
            return with_read(
                [&_key](const _RegistryTable& _r)
                {
                    return _r.contains(_key);
                });
        }

        size_type
        count(const key_type& _key) const
        {
            return with_read(
                [&_key](const _RegistryTable& _r)
                {
                    return _r.count(_key);
                });
        }


        // =================================================================
        //  ELEMENT-LEVEL QUERIES (safe, always available)
        // =================================================================

        // find_copy_or
        //   function: returns a copy of the row under a transient
        // read guard; the copy is independent of the generation.
        element_type
        find_copy_or(
            const key_type&     _key,
            const element_type& _fallback
        ) const
        {
            return with_read(
                [&_key, &_fallback](const _RegistryTable& _r) -> element_type
                {
                    const element_type* p = _r.find(_key);

                    if (p)
                    {
                        return *p;
                    }

                    return _fallback;
                });
        }

        // find_visit
        //   function: invokes _fn under a read guard with a pointer
        // to the row (or nullptr).  The pointer is valid only for the
        // duration of the call.
        template<typename _Fn>
        auto
        find_visit(
            const key_type& _key,
            _Fn&&           _fn
        ) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const element_type*>()))
        {
            return with_read(
                [&_key, &_fn](const _RegistryTable& _r)
                {
                    return static_cast<_Fn&&>(_fn)(_r.find(_key));
                });
        }


        // =================================================================
        //  COLUMN PROJECTION (lock-free, by value, C++17+)
        // =================================================================
        //   get<MP>(key) is a compile-time-named column accessor that
        // works on ANY registry.  Mirrors the underlying registry's
        // project<MP>(key), but returns BY VALUE under a transient
        // RCU read guard so the caller can safely use the result
        // outside the read-side critical section.
        //
        //   Distinguished from the non-template get(key) below: get(key)
        // reads the registry's DESIGNATED value column; get<MP>(key)
        // reads any column the user names.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

        // get<_Member>
        //   function: returns a copy of the value of the named column
        // for the row keyed by _key.  Behavior is undefined if the
        // key is absent (mirrors project<MP>'s contract).
        template<auto _Member>
        auto
        get(const key_type& _key) const
            -> typename std::decay<decltype(
                std::declval<const _RegistryTable&>()
                    .template project<_Member>(_key))>::type
        {
            return with_read(
                [&_key](const _RegistryTable& _r)
                {
                    return _r.template project<_Member>(_key);
                });
        }

#endif  // C++17+


        // =================================================================
        //  CVAR ACCESS (lock-free, SFINAE-gated)
        // =================================================================

        // get
        //   function: returns a copy of the cvar value for _key under
        // a transient read guard.
        template<typename _R = _RegistryTable,
                 typename _Cv = typename _R::cvar_type,
                 typename = decltype(
                     std::declval<const _R&>().get(
                         std::declval<const key_type&>()))>
        _Cv
        get(const key_type& _key) const
        {
            return with_read(
                [&_key](const _R& _r)
                {
                    return _r.get(_key);
                });
        }

        // get_or
        //   function: returns a copy of the cvar value, or _fallback
        // if absent.
        template<typename _R = _RegistryTable,
                 typename _Cv = typename _R::cvar_type,
                 typename = decltype(
                     std::declval<const _R&>().get_or(
                         std::declval<const key_type&>(),
                         std::declval<const _Cv&>()))>
        _Cv
        get_or(
            const key_type& _key,
            const _Cv&      _fallback
        ) const
        {
            return with_read(
                [&_key, &_fallback](const _R& _r) -> _Cv
                {
                    return _r.get_or(_key, _fallback);
                });
        }

        // has
        //   function: cvar-interface existence check.
        template<typename _R = _RegistryTable,
                 typename = decltype(
                     std::declval<const _R&>().has(
                         std::declval<const key_type&>()))>
        bool
        has(const key_type& _key) const
        {
            return with_read(
                [&_key](const _R& _r)
                {
                    return _r.has(_key);
                });
        }


        // =================================================================
        //  CVAR MUTATION (publish-new-generation, SFINAE-gated)
        // =================================================================

        // set
        //   function: clones the registry, applies the set, publishes
        // the new generation.  The old generation is retired and
        // reclaimed once readers in its epoch complete.
        template<typename _R = _RegistryTable,
                 typename _Cv = typename _R::cvar_type,
                 typename = decltype(
                     std::declval<_R&>().set(
                         std::declval<const key_type&>(),
                         std::declval<const _Cv&>()))>
        bool
        set(
            const key_type& _key,
            const _Cv&      _value
        )
        {
            bool result = false;

            m_rcu.modify(
                [&_key, &_value, &result](_RegistryTable& _r)
                {
                    result = _r.set(_key, _value);
                });

            return result;
        }


        // =================================================================
        //  WRITER API (whole-generation replacement)
        // =================================================================

        void
        update(const _RegistryTable& _new_registry)
        {
            m_rcu.update(_new_registry);

            return;
        }

        void
        update(_RegistryTable&& _new_registry)
        {
            m_rcu.update(
                static_cast<_RegistryTable&&>(_new_registry));

            return;
        }

        template<typename _Fn>
        void
        modify(_Fn&& _fn)
        {
            m_rcu.modify(static_cast<_Fn&&>(_fn));

            return;
        }


        // =================================================================
        //  CAPACITY / RECLAMATION
        // =================================================================

        size_type
        size() const
        {
            return with_read(
                [](const _RegistryTable& _r)
                {
                    return _r.size();
                });
        }

        bool
        empty() const
        {
            return with_read(
                [](const _RegistryTable& _r)
                {
                    return _r.empty();
                });
        }

        std::size_t
        pending_reclamation() const noexcept
        {
            return m_rcu.pending_reclamation();
        }

        std::uint64_t
        current_epoch() const noexcept
        {
            return m_rcu.current_epoch();
        }

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
    // III. make_rcu_registry_table
    // =========================================================================

    // make_rcu_registry_table
    //   function: factory.  Wraps an existing registry_table-shaped
    // value (lookup-only or cvar-bearing) in an rcu_registry_table.
    template<typename _RegistryTable>
    auto
    make_rcu_registry_table(_RegistryTable _registry)
        -> rcu_registry_table<typename std::decay<_RegistryTable>::type>
    {
        using rt_t = typename std::decay<_RegistryTable>::type;

        return rcu_registry_table<rt_t>(
            static_cast<_RegistryTable&&>(_registry));
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_RCU_REGISTRY_TABLE_
