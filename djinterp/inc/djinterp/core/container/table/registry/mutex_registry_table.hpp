/******************************************************************************
* djinterp [container]                                  mutex_registry_table.hpp
*
*   Mutex-protected threadsafe wrapper around `registry_table`.  Following
* the registry_table redesign, the wrapped type is BOTH the lookup
* surface (find / contains / count) AND the optional cvar surface
* (get / get_or / has / set).  This wrapper exposes both:
*
*     ALWAYS AVAILABLE   (any registry_table, with or without a value column)
*       contains, count, find_copy_or, find_visit, find_visit_mut
*
*     SFINAE-GATED       (only when wrapped exposes them - i.e. when a
*                         value column was supplied)
*       get, get_or, has, set
*
*   This collapses what were previously mutex_lookup_table and
* mutex_registry_table into one wrapper.  Pure-lookup users wrap a
* registry_table whose value member defaults to nullptr; cvar users
* supply a value member.  Same wrapper type, same threadsafety
* contract; only the available method set differs.
*
*   THREAD SAFETY:
*   All public methods acquire the policy-driven lock for the duration
* of the operation.  Default policy (default_lock_policy) is
* shared_lock_policy on C++17+ - reads run concurrently, writes are
* exclusive.
*
*   By-value returns:
*   `find_copy_or`, `get`, and `get_or` return BY VALUE so the lock can
* be safely released before the caller uses the result.  A reference
* return would dangle the moment the guard releases.  For repeated
* same-row reads, prefer `find_visit` / `with_read` to amortize the
* lock acquisition cost.
*
* DEPENDENCIES:
*   djinterp.hpp                  - NS_DJINTERP, D_CONSTEXPR
*   mutex_table_base.hpp          - mutex_table_base
*
* TABLE OF CONTENTS
* =================
* I.    Internal helpers (SFINAE detectors, conditional cvar_type alias)
* II.   mutex_registry_table
* III.  make_mutex_registry_table
*
*
* path:      /inc/djinterp/container/table/mutex_registry_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_MUTEX_REGISTRY_TABLE_
#define DJINTERP_MUTEX_REGISTRY_TABLE_ 1

#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "./mutex_table_base.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   INTERNAL HELPERS
    // =========================================================================

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  SFINAE detectors
        //   Local to this header so the wrapper has no dependency on
        // registry_table_traits.hpp.  All probes use only members that
        // the registry_table contract guarantees.
        // -----------------------------------------------------------------

        // mrt_has_cvar_type
        //   trait: detects ::cvar_type alias on the wrapped type.
        // True iff the wrapped registry has a value column.
        template<typename _RT,
                 typename = void>
        struct mrt_has_cvar_type : std::false_type
        {};

        template<typename _RT>
        struct mrt_has_cvar_type<_RT,
            void_t<typename _RT::cvar_type>>
            : std::true_type
        {};

        // mrt_has_set
        //   trait: detects set(key, value) on the wrapped type.  True
        // iff the wrapped registry has a non-const value column.
        template<typename _RT,
                 typename = void>
        struct mrt_has_set : std::false_type
        {};

        template<typename _RT>
        struct mrt_has_set<_RT,
            void_t<
                decltype(
                    std::declval<_RT&>().set(
                        std::declval<const typename _RT::key_type&>(),
                        std::declval<const typename _RT::cvar_type&>()))
            >> : std::true_type
        {};


        // -----------------------------------------------------------------
        //  conditional cvar_type alias surfacing
        //   When the wrapped type has a cvar_type, propagate it to the
        // wrapper via inheritance.  When it doesn't, the wrapper has
        // no cvar_type either (consistent with the lookup-only mode).
        // -----------------------------------------------------------------

        template<typename _RT,
                 bool     _Has = mrt_has_cvar_type<_RT>::value>
        struct mrt_cvar_alias
        {};

        template<typename _RT>
        struct mrt_cvar_alias<_RT, true>
        {
            using cvar_type = typename _RT::cvar_type;
        };

    NS_END  // internal


    // =========================================================================
    // II.  mutex_registry_table
    // =========================================================================

    // mutex_registry_table
    //   class: locked wrapper around any type satisfying the
    // registry_table structural contract.  Always provides the lookup
    // interface; provides the cvar interface only when the wrapped
    // type does.
    template<typename _RegistryTable,
             typename _Policy = threadsafe::default_lock_policy>
    class mutex_registry_table
        : public mutex_table_base<_RegistryTable, _Policy>,
          public internal::mrt_cvar_alias<_RegistryTable>
    {
    private:
        using base = mutex_table_base<_RegistryTable, _Policy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases pulled in from base
        // -----------------------------------------------------------------
        using typename base::wrapped_type;
        using typename base::lock_policy_type;
        using typename base::mutex_type;
        using typename base::lock_type;
        using typename base::read_lock_type;
        using typename base::write_lock_type;
        using typename base::size_type;
        using typename base::underlying_container_type;

        // -----------------------------------------------------------------
        //  type aliases specific to registry_table
        // -----------------------------------------------------------------
        using key_type     = typename _RegistryTable::key_type;
        using element_type = typename _RegistryTable::element_type;

        // cvar_type is inherited from internal::mrt_cvar_alias when
        // the wrapped type has a value column; otherwise no cvar_type
        // alias exists on this class.

        // self_type
        //   type: convenience alias for the full wrapper type.
        using self_type =
            mutex_registry_table<_RegistryTable, _Policy>;

        // -----------------------------------------------------------------
        //  static mode flags
        // -----------------------------------------------------------------

        // has_value_column
        //   value: true when the wrapped registry exposes cvar_type
        // (i.e. has a value column).  Mirrors the wrapped type's flag
        // when present, computed structurally otherwise.
        static constexpr bool has_value_column =
            internal::mrt_has_cvar_type<_RegistryTable>::value;

        // is_writable
        //   value: true when the wrapped registry exposes a set()
        // method (i.e. has a non-const value column).
        static constexpr bool is_writable =
            internal::mrt_has_set<_RegistryTable>::value;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        using base::base;


        // =================================================================
        //  KEY-LEVEL QUERIES (locked, always available)
        // =================================================================

        // contains
        //   function: locked existence check.
        bool
        contains(const key_type& _key) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped.contains(_key);
        }

        // count
        //   function: locked count query.
        size_type
        count(const key_type& _key) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped.count(_key);
        }


        // =================================================================
        //  ELEMENT-LEVEL QUERIES (locked, always available)
        // =================================================================
        //   The wrapped registry's `find()` returns a raw pointer that
        // would dangle past the lock release.  Three safe alternatives:
        //
        //     find_copy_or  - copy the row out under the lock
        //     find_visit    - invoke a visitor under the read lock
        //     find_visit_mut- invoke a visitor under the write lock

        // find_copy_or
        //   function: returns a copy of the row, or _fallback if the
        // key is absent.
        element_type
        find_copy_or(
            const key_type&     _key,
            const element_type& _fallback
        ) const
        {
            read_lock_type guard(this->m_mutex);

            const element_type* p = this->m_wrapped.find(_key);

            if (p)
            {
                return *p;
            }

            return _fallback;
        }

        // find_visit
        //   function: invokes _fn under a read lock with a pointer to
        // the row (or nullptr).  Forwards _fn's return value out.
        template<typename _Fn>
        auto
        find_visit(
            const key_type& _key,
            _Fn&&           _fn
        ) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const element_type*>()))
        {
            read_lock_type guard(this->m_mutex);

            return static_cast<_Fn&&>(_fn)(
                this->m_wrapped.find(_key));
        }

        // find_visit_mut
        //   function: mutable counterpart - the visitor receives a
        // mutable row pointer under a write lock.  Useful for in-place
        // updates that bypass the cvar interface.
        template<typename _Fn>
        auto
        find_visit_mut(
            const key_type& _key,
            _Fn&&           _fn
        )
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<element_type*>()))
        {
            write_lock_type guard(this->m_mutex);

            return static_cast<_Fn&&>(_fn)(
                this->m_wrapped.find(_key));
        }


        // =================================================================
        //  CVAR ACCESS (locked, SFINAE-gated on has_value_column)
        // =================================================================
        //   Returns BY VALUE so the lock can be released safely.

        // get
        //   function: returns a copy of the cvar value for _key.
        // Behavior is undefined if the key is absent (mirrors the
        // wrapped registry's get contract).
        template<typename _R = _RegistryTable,
                 typename _Cv = typename _R::cvar_type,
                 typename = decltype(
                     std::declval<const _R&>().get(
                         std::declval<const key_type&>()))>
        _Cv
        get(const key_type& _key) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped.get(_key);
        }

        // get_or
        //   function: returns a copy of the cvar value for _key, or
        // _fallback if absent.
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
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped.get_or(_key, _fallback);
        }

        // has
        //   function: cvar-interface existence check.  Distinct from
        // contains() at the type-surface level: `has` is part of the
        // cvar interface (absent on lookup-only registries); `contains`
        // is part of the lookup interface (always present).
        // Semantically equivalent on registries that expose both.
        template<typename _R = _RegistryTable,
                 typename = decltype(
                     std::declval<const _R&>().has(
                         std::declval<const key_type&>()))>
        bool
        has(const key_type& _key) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped.has(_key);
        }


        // =================================================================
        //  CVAR MUTATION (locked, SFINAE-gated on is_writable)
        // =================================================================

        // set
        //   function: locked update of the cvar value.  Available only
        // when the wrapped registry has a non-const value column.
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
            write_lock_type guard(this->m_mutex);

            return this->m_wrapped.set(_key, _value);
        }
    };


    // =========================================================================
    // III. make_mutex_registry_table
    // =========================================================================

    // make_mutex_registry_table
    //   function: factory.  Wraps an existing registry_table-shaped
    // value (lookup-only or cvar-bearing) in a mutex_registry_table.
    template<typename _RegistryTable,
             typename _Policy = threadsafe::default_lock_policy>
    auto
    make_mutex_registry_table(_RegistryTable _registry)
        -> mutex_registry_table<
            typename std::decay<_RegistryTable>::type,
            _Policy>
    {
        using rt_t = typename std::decay<_RegistryTable>::type;

        return mutex_registry_table<rt_t, _Policy>(
            static_cast<_RegistryTable&&>(_registry));
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_MUTEX_REGISTRY_TABLE_
