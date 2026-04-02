/******************************************************************************
* djinterp [container]                                     registry_table.hpp
*
* djinterp registry table header:
*   A registry_table is a subset of lookup_tables that provides configuration
* variable (cvar) semantics: named values obtained and (optionally) modified
* by key lookup. It composes a lookup_table and a value extractor, adding
* get/set/has operations on top of the lookup_table's find/contains.
*
*   No assumptions are made about the entry structure, key type, value type,
* or underlying table implementation. The key extractor (in the lookup_table)
* determines how elements map to keys; the value extractor determines how
* elements map to cvar values. Both are user-supplied callables.
*
*   CVAR ACCESS MODEL:
*     get(key)              — returns the cvar value for the key (const ref)
*     get_or(key, default)  — returns the cvar value or a fallback
*     set(key, value)       — modifies the cvar value in place (SFINAE-gated)
*     has(key)              — existence check (delegates to lookup_table)
*
*   SET AVAILABILITY:
*   set() is available only when the value extractor returns a mutable
*   reference from a non-const element. When the extractor returns by value
*   or const reference, set() is absent from the interface (SFINAE).
*
*   CONTAINER_TRAITS CLASSIFICATION:
*   The registry_table exposes backing_container_type (the wrapped
*   lookup_table), which in turn backs the actual storage table. This
*   creates a two-level backed chain: registry_table → lookup_table → table.
*   All twelve classification axes pass through from the innermost table.
*
*   OPTION/CLI INTEGRATION:
*   The get/set interface maps directly to the container_option_traits
*   detection probes (set_option / get_option). Generic option and CLI
*   algorithms can operate on any registry_table without knowing the
*   concrete entry type.
*
*   USAGE:
*   ```cpp
*   struct cvar { int id; float value; const char* name; };
*
*   auto reg = make_registry_table(
*       my_cvar_table,
*       [](const cvar& c) { return c.id; },       // key extractor
*       [](const cvar& c) -> const float&          // value extractor
*           { return c.value; });
*
*   float v = reg.get(42);
*   reg.set(42, 3.14f);
*   bool exists = reg.has(42);
*   ```
*
* path:      \inc\container\registry_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINER_REGISTRY_TABLE_
#define DJINTERP_CONTAINER_REGISTRY_TABLE_ 1

#include <cstddef>
#include <type_traits>
#include <utility>
#include "..\..\djinterp.h"
#include ".\lookup_table.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   INTERNAL SET-AVAILABILITY DETECTION
    // =========================================================================

    NS_INTERNAL

        // is_mutable_extractor
        //   trait: true if _Extract applied to a mutable _Element reference
        // returns a mutable (non-const) lvalue reference, enabling
        // assignment through the extractor.
        template<typename _Extract,
                 typename _Element,
                 typename = void>
        struct is_mutable_extractor : std::false_type
        {
        };

        // is_mutable_extractor (specialization)
        //   trait: SFINAE success — extract returns a non-const lvalue ref.
        template<typename _Extract,
                 typename _Element>
        struct is_mutable_extractor<_Extract, _Element,
            typename std::enable_if<
                std::is_lvalue_reference<decltype(
                    std::declval<_Extract>()(
                        std::declval<_Element&>()))>::value &&
                !std::is_const<typename std::remove_reference<decltype(
                    std::declval<_Extract>()(
                        std::declval<_Element&>()))>::type>::value
            >::type>
            : std::true_type
        {
        };

    NS_END  // internal


    // =========================================================================
    // II.  REGISTRY TABLE
    // =========================================================================

    // registry_table
    //   class: wraps a lookup_table and adds cvar (configuration variable)
    // get/set semantics via a user-supplied value extractor. The
    // lookup_table provides keyed element retrieval; the value extractor
    // projects each element to its cvar value.
    template<typename _LookupTable,
             typename _ValueExtract>
    class registry_table
    {
    private:
        using element_type = typename _LookupTable::element_type;
        using can_set      = internal::is_mutable_extractor<
            _ValueExtract, element_type>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using lookup_table_type      = _LookupTable;
        using value_extract_type     = _ValueExtract;
        using backing_container_type = _LookupTable;
        using key_type               = typename _LookupTable::key_type;
        using size_type              = typename _LookupTable::size_type;
        using cvar_type              = typename std::decay<decltype(
            std::declval<const _ValueExtract>()(
                std::declval<const element_type&>()))>::type;

        // -----------------------------------------------------------------
        //  constructors
        // -----------------------------------------------------------------

        // registry_table(lookup, value_extract)
        //   constructor: takes ownership of the lookup_table and value
        // extractor.
        D_CONSTEXPR explicit registry_table(
                _LookupTable _lookup,
                _ValueExtract _value_extract = _ValueExtract{}
            )
            : m_lookup(static_cast<_LookupTable&&>(_lookup)),
              m_value_extract(static_cast<_ValueExtract&&>(_value_extract))
        {
        }

        D_CONSTEXPR registry_table()                                 = default;
        D_CONSTEXPR registry_table(const registry_table&)            = default;
        D_CONSTEXPR registry_table(registry_table&&)                 = default;
        D_CONSTEXPR registry_table& operator=(const registry_table&) = default;
        D_CONSTEXPR registry_table& operator=(registry_table&&)      = default;


        // =================================================================
        //  cvar access (get / get_or / set / has)
        // =================================================================

        // get
        //   function: returns a const reference to the cvar value for
        // _key. The element must exist; behavior is undefined if the key
        // is absent (use has() or get_or() for safe access).
        D_CONSTEXPR const cvar_type& get(
                const key_type& _key
            ) const
        {
            return m_value_extract(*(m_lookup.find(_key)));
        }

        // get_or
        //   function: returns the cvar value for _key, or _fallback if
        // the key is not present.
        D_CONSTEXPR const cvar_type& get_or(
                const key_type& _key,
                const cvar_type& _fallback
            ) const
        {
            const element_type* p = m_lookup.find(_key);

            if (p)
            {
                return m_value_extract(*p);
            }

            return _fallback;
        }

        // set
        //   function: modifies the cvar value for _key in place. Only
        // available when _ValueExtract returns a mutable lvalue reference
        // from a non-const element.
        // Returns true if the key was found and the value was set.
        template<typename _V  = _ValueExtract,
                 typename _E  = element_type,
                 typename     = typename std::enable_if<
                     internal::is_mutable_extractor<_V, _E>::value>::type>
        D_CONSTEXPR bool set(
                const key_type& _key,
                const cvar_type& _value
            )
        {
            element_type* p = m_lookup.find(_key);

            if (p)
            {
                m_value_extract(*p) = _value;

                return true;
            }

            return false;
        }

        // has
        //   function: returns true if the key exists in the registry.
        // Delegates to the underlying lookup_table's contains().
        D_CONSTEXPR bool has(
                const key_type& _key
            ) const
        {
            return m_lookup.contains(_key);
        }


        // =================================================================
        //  underlying access
        // =================================================================

        // lookup
        //   function: returns a const reference to the underlying
        // lookup_table.
        D_CONSTEXPR const _LookupTable& lookup() const noexcept
        {
            return m_lookup;
        }

        // lookup
        //   function: returns a mutable reference to the underlying
        // lookup_table.
        D_CONSTEXPR _LookupTable& lookup() noexcept
        {
            return m_lookup;
        }

        // value_extract
        //   function: returns the value extractor.
        D_CONSTEXPR const _ValueExtract& value_extract() const noexcept
        {
            return m_value_extract;
        }

        // table
        //   function: returns a const reference to the innermost table
        // (two levels deep: registry_table → lookup_table → table).
        D_CONSTEXPR const auto& table() const noexcept
        {
            return m_lookup.table();
        }

        // table
        //   function: returns a mutable reference to the innermost table.
        D_CONSTEXPR auto& table() noexcept
        {
            return m_lookup.table();
        }


        // =================================================================
        //  forwarded capacity
        // =================================================================

        D_CONSTEXPR auto size() const
            -> decltype(std::declval<const _LookupTable&>().size())
        {
            return m_lookup.size();
        }

        D_CONSTEXPR auto empty() const
            -> decltype(std::declval<const _LookupTable&>().empty())
        {
            return m_lookup.empty();
        }


        // =================================================================
        //  forwarded iteration
        // =================================================================

        D_CONSTEXPR auto begin() const
            -> decltype(std::declval<const _LookupTable&>().begin())
        {
            return m_lookup.begin();
        }

        D_CONSTEXPR auto end() const
            -> decltype(std::declval<const _LookupTable&>().end())
        {
            return m_lookup.end();
        }

        D_CONSTEXPR auto cbegin() const
            -> decltype(std::declval<const _LookupTable&>().cbegin())
        {
            return m_lookup.cbegin();
        }

        D_CONSTEXPR auto cend() const
            -> decltype(std::declval<const _LookupTable&>().cend())
        {
            return m_lookup.cend();
        }


        // =================================================================
        //  forwarded lookup (passthrough for direct element access)
        // =================================================================

        // find
        //   function: forwards to the underlying lookup_table's find().
        D_CONSTEXPR const element_type* find(
                const key_type& _key
            ) const
        {
            return m_lookup.find(_key);
        }

        // find (mutable)
        //   function: mutable passthrough.
        D_CONSTEXPR element_type* find(
                const key_type& _key
            )
        {
            return m_lookup.find(_key);
        }

        // contains
        //   function: alias for has(). Provided for lookup_table
        // interface compatibility.
        D_CONSTEXPR bool contains(
                const key_type& _key
            ) const
        {
            return m_lookup.contains(_key);
        }

        // count
        //   function: forwards to the underlying lookup_table's count().
        D_CONSTEXPR size_type count(
                const key_type& _key
            ) const
        {
            return m_lookup.count(_key);
        }

    protected:
        _LookupTable  m_lookup;
        _ValueExtract m_value_extract;
    };


    // =========================================================================
    // III. FACTORY FUNCTIONS
    // =========================================================================

    // make_registry_table (from lookup_table + value extractor)
    //   function: wraps a pre-built lookup_table with cvar semantics.
    template<typename _LookupTable,
             typename _ValueExtract>
    D_CONSTEXPR auto
    make_registry_table(
            _LookupTable  _lookup,
            _ValueExtract _value_extract
        )
        -> registry_table<typename std::decay<_LookupTable>::type,
                          typename std::decay<_ValueExtract>::type>
    {
        using lt_t = typename std::decay<_LookupTable>::type;
        using ve_t = typename std::decay<_ValueExtract>::type;

        return registry_table<lt_t, ve_t>{
            static_cast<_LookupTable&&>(_lookup),
            static_cast<_ValueExtract&&>(_value_extract)
        };
    }

    // make_registry_table (from table + key extractor + value extractor)
    //   function: constructs a lookup_table from the given table and key
    // extractor, then wraps it with cvar semantics via the value extractor.
    // Convenience overload that builds the full chain in one call.
    template<typename _Strategy     = auto_strategy,
             typename _Table,
             typename _KeyExtract,
             typename _ValueExtract>
    D_CONSTEXPR auto
    make_registry_table(
            _Table        _table,
            _KeyExtract   _key_extract,
            _ValueExtract _value_extract
        )
        -> registry_table<
            lookup_table<typename std::decay<_Table>::type,
                         typename std::decay<_KeyExtract>::type,
                         _Strategy>,
            typename std::decay<_ValueExtract>::type>
    {
        using table_t = typename std::decay<_Table>::type;
        using ke_t    = typename std::decay<_KeyExtract>::type;
        using ve_t    = typename std::decay<_ValueExtract>::type;
        using lt_t    = lookup_table<table_t, ke_t, _Strategy>;

        return registry_table<lt_t, ve_t>{
            lt_t{static_cast<_Table&&>(_table),
                 static_cast<_KeyExtract&&>(_key_extract)},
            static_cast<_ValueExtract&&>(_value_extract)
        };
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_REGISTRY_TABLE_
