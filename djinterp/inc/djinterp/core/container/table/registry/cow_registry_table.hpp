/******************************************************************************
* djinterp [container]                                    cow_registry_table.hpp
*
*   Copy-on-write threadsafe wrapper around `registry_table`.  Following
* the registry_table redesign, this wrapper handles both modes of the
* unified type:
*
*     ALWAYS AVAILABLE   (any registry_table)
*       contains, count, find_copy_or, find_visit, find_visit_mut
*
*     SFINAE-GATED       (only when wrapped exposes them)
*       get, get_or, has, set
*
*   Composes with `threadsafe::cow_state<registry_table, _Policy>` to
* provide cheap snapshots (atomic refcount bump), shared-readers /
* exclusive-writer semantics under the policy lock, and clone-on-
* write when a snapshot is outstanding.
*
*   COMPARED TO:
*     mutex_registry_table  - direct in-place mutation; deep-copy snapshot
*     cow_registry_table    - clone-on-write mutation; refcounted snapshot
*     rcu_registry_table    - lock-free reads; whole-registry cloning
*                             writes
*     atomic_registry_table - lock-free per-cvar atomics; no whole-
*                             registry generation semantics
*
*   USE CASE:
*   The canonical "freeze the configuration at startup, hand snapshots
* to subsystems, mutate rarely" pattern.  Snapshots are essentially
* free; subsystems hold them as long as they like without disturbing
* live mutation.
*
* DEPENDENCIES:
*   djinterp.hpp        - NS_DJINTERP, D_CONSTEXPR
*   threadsafe/cow.hpp  - threadsafe::cow_state, immutable_snapshot
*
* TABLE OF CONTENTS
* =================
* I.    Internal helpers (SFINAE detectors, conditional cvar_type alias)
* II.   cow_registry_table
* III.  make_cow_registry_table
*
*
* path:      /inc/djinterp/container/table/cow_registry_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_COW_REGISTRY_TABLE_
#define DJINTERP_COW_REGISTRY_TABLE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`cow_registry_table.hpp` requires C++11 or later."
#endif

#include <cstdint>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "../../threadsafe/cow.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   INTERNAL HELPERS
    // =========================================================================

    NS_INTERNAL

        // crt_has_cvar_type
        //   trait: detects ::cvar_type alias on the wrapped type.
        template<typename _RT,
                 typename = void>
        struct crt_has_cvar_type : std::false_type
        {};

        template<typename _RT>
        struct crt_has_cvar_type<_RT,
            void_t<typename _RT::cvar_type>>
            : std::true_type
        {};

        // crt_has_set
        //   trait: detects set(key, value) on the wrapped type.
        template<typename _RT,
                 typename = void>
        struct crt_has_set : std::false_type
        {};

        template<typename _RT>
        struct crt_has_set<_RT,
            void_t<
                decltype(
                    std::declval<_RT&>().set(
                        std::declval<const typename _RT::key_type&>(),
                        std::declval<const typename _RT::cvar_type&>()))
            >> : std::true_type
        {};

        // crt_cvar_alias
        //   trait: surfaces ::cvar_type only when the wrapped type
        // has one.
        template<typename _RT,
                 bool     _Has = crt_has_cvar_type<_RT>::value>
        struct crt_cvar_alias
        {};

        template<typename _RT>
        struct crt_cvar_alias<_RT, true>
        {
            using cvar_type = typename _RT::cvar_type;
        };

    NS_END  // internal


    // =========================================================================
    // II.  cow_registry_table
    // =========================================================================

    // cow_registry_table
    //   class: copy-on-write threadsafe wrapper around any type
    // satisfying the registry_table structural contract.  Snapshots
    // are refcount-shared; writes clone storage when refcount > 1.
    template<typename _RegistryTable,
             typename _Policy = threadsafe::default_lock_policy>
    class cow_registry_table
        : public internal::crt_cvar_alias<_RegistryTable>
    {
    private:
        using state_t =
            threadsafe::cow_state<_RegistryTable, _Policy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using wrapped_type     = _RegistryTable;
        using lock_policy_type = _Policy;
        using mutex_type       = typename _Policy::mutex_type;
        using lock_type        = typename _Policy::write_lock_type;
        using read_lock_type   = typename _Policy::read_lock_type;
        using write_lock_type  = typename _Policy::write_lock_type;

        using underlying_container_type = _RegistryTable;
        using key_type     = typename _RegistryTable::key_type;
        using element_type = typename _RegistryTable::element_type;
        using size_type    = typename _RegistryTable::size_type;

        // cvar_type comes from internal::crt_cvar_alias (inherited)
        // when the wrapped type has a value column; otherwise absent.

        using snapshot_type =
            threadsafe::immutable_snapshot<_RegistryTable>;

        using self_type =
            cow_registry_table<_RegistryTable, _Policy>;

        // -----------------------------------------------------------------
        //  static mode flags
        // -----------------------------------------------------------------

        static constexpr bool has_value_column =
            internal::crt_has_cvar_type<_RegistryTable>::value;

        static constexpr bool is_writable =
            internal::crt_has_set<_RegistryTable>::value;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        cow_registry_table() = default;

        explicit cow_registry_table(
            const _RegistryTable& _wrapped
        )
            : m_state(_wrapped)
        {}

        explicit cow_registry_table(
            _RegistryTable&& _wrapped
        )
            : m_state(static_cast<_RegistryTable&&>(_wrapped))
        {}

        cow_registry_table(
            const cow_registry_table&)                       = delete;
        cow_registry_table& operator=(
            const cow_registry_table&)                       = delete;
        cow_registry_table(cow_registry_table&&)             = delete;
        cow_registry_table& operator=(cow_registry_table&&)  = delete;

        ~cow_registry_table() = default;


        // =================================================================
        //  GENERIC LOCKED VISITORS
        // =================================================================

        template<typename _Fn>
        auto
        with_read(_Fn&& _fn) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const _RegistryTable&>()))
        {
            return static_cast<_Fn&&>(_fn)(m_state.read());
        }

        template<typename _Fn>
        auto
        with_write(_Fn&& _fn)
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<_RegistryTable&>()))
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
        //  KEY-LEVEL QUERIES (locked, always available)
        // =================================================================

        bool
        contains(const key_type& _key) const
        {
            return m_state.read().contains(_key);
        }

        size_type
        count(const key_type& _key) const
        {
            return m_state.read().count(_key);
        }


        // =================================================================
        //  ELEMENT-LEVEL QUERIES (locked, always available)
        // =================================================================

        // find_copy_or
        //   function: returns a copy of the row, or _fallback if the
        // key is absent.
        element_type
        find_copy_or(
            const key_type&     _key,
            const element_type& _fallback
        ) const
        {
            const _RegistryTable& r = m_state.read();
            const element_type*   p = r.find(_key);

            if (p)
            {
                return *p;
            }

            return _fallback;
        }

        // find_visit
        //   function: invokes _fn under a read lock with a pointer to
        // the row (or nullptr).
        template<typename _Fn>
        auto
        find_visit(
            const key_type& _key,
            _Fn&&           _fn
        ) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const element_type*>()))
        {
            const _RegistryTable& r = m_state.read();

            return static_cast<_Fn&&>(_fn)(r.find(_key));
        }

        // find_visit_mut
        //   function: mutable counterpart - the visitor receives a
        // mutable row pointer; clones the storage if necessary.
        template<typename _Fn>
        auto
        find_visit_mut(
            const key_type& _key,
            _Fn&&           _fn
        )
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<element_type*>()))
        {
            return m_state.modify(
                [&_key, &_fn](_RegistryTable& _r)
                {
                    return static_cast<_Fn&&>(_fn)(_r.find(_key));
                });
        }


        // =================================================================
        //  CVAR ACCESS (locked, SFINAE-gated)
        // =================================================================

        // get
        //   function: returns a copy of the cvar value for _key.
        template<typename _R = _RegistryTable,
                 typename _Cv = typename _R::cvar_type,
                 typename = decltype(
                     std::declval<const _R&>().get(
                         std::declval<const key_type&>()))>
        _Cv
        get(const key_type& _key) const
        {
            return m_state.read().get(_key);
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
            return m_state.read().get_or(_key, _fallback);
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
            return m_state.read().has(_key);
        }


        // =================================================================
        //  CVAR MUTATION (clone-on-write, SFINAE-gated)
        // =================================================================

        // set
        //   function: clone-on-write update of the cvar value.
        // Available only when the wrapped registry has a non-const
        // value column.  Outstanding snapshots are unaffected.
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
            return m_state.modify(
                [&_key, &_value](_RegistryTable& _r)
                {
                    return _r.set(_key, _value);
                });
        }


        // =================================================================
        //  SNAPSHOT / REPLACE
        // =================================================================

        snapshot_type
        snapshot() const
        {
            return m_state.snapshot();
        }

        void
        replace(const _RegistryTable& _new_registry)
        {
            m_state.replace(_new_registry);

            return;
        }

        void
        replace(_RegistryTable&& _new_registry)
        {
            m_state.replace(
                static_cast<_RegistryTable&&>(_new_registry));

            return;
        }

        std::uint64_t
        version() const noexcept
        {
            return m_state.version();
        }

        mutex_type&
        mutex() const noexcept
        {
            return m_state.mutex();
        }

    private:
        state_t m_state;
    };


    // =========================================================================
    // III. make_cow_registry_table
    // =========================================================================

    // make_cow_registry_table
    //   function: factory.  Wraps an existing registry_table-shaped
    // value (lookup-only or cvar-bearing) in a cow_registry_table.
    template<typename _RegistryTable,
             typename _Policy = threadsafe::default_lock_policy>
    auto
    make_cow_registry_table(_RegistryTable _registry)
        -> cow_registry_table<
            typename std::decay<_RegistryTable>::type,
            _Policy>
    {
        using rt_t = typename std::decay<_RegistryTable>::type;

        return cow_registry_table<rt_t, _Policy>(
            static_cast<_RegistryTable&&>(_registry));
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_COW_REGISTRY_TABLE_
