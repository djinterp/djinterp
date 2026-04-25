/******************************************************************************
* djinterp [container]                               registry_table_traits.hpp
*
* djinterp registry table traits header:
*   SFINAE-based structural detection for registry tables (cvar containers).
* All detection is purely structural — any type exposing the right members
* is classified, not just the library's own registry_table.
*
*   A registry table is detected by the presence of cvar access methods
* (get, get_or, has) combined with a backing lookup_table accessible via
* lookup(). The set() method is optional and detected separately to
* distinguish read-only from read-write registries.
*
*   DETECTION PROBES:
*   - get(key)                → cvar read access
*   - get_or(key, fallback)   → safe cvar read access
*   - set(key, value)         → cvar write access (optional)
*   - has(key)                → existence check
*   - lookup()                → underlying lookup_table access
*   - value_extract()         → value extractor access
*   - cvar_type               → the cvar value type
*   - lookup_table_type       → the underlying lookup_table type
*   - backing_container_type  → backed container detection
*
*
* path:      /inc/djinterp/container/table/registry/registry_table_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_CONTAINER_REGISTRY_TABLE_TRAITS_
#define DJINTERP_CONTAINER_REGISTRY_TABLE_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../../../djinterp.h"
#include "../lookup/lookup_table_traits.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   DETECTION OPERATIONS
    // =========================================================================

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  type alias probes
        // -----------------------------------------------------------------

        // detect_cvar_type
        //   trait: probes for ::cvar_type alias.
        template<typename _T>
        using detect_cvar_type = typename _T::cvar_type;

        // detect_lookup_table_type
        //   trait: probes for ::lookup_table_type alias.
        template<typename _T>
        using detect_lookup_table_type = typename _T::lookup_table_type;

        // detect_value_extract_type
        //   trait: probes for ::value_extract_type alias.
        template<typename _T>
        using detect_value_extract_type = typename _T::value_extract_type;

        // -----------------------------------------------------------------
        //  cvar method probes
        // -----------------------------------------------------------------

        // detect_get_method
        //   trait: probes for .get(key) → const cvar_type&.
        template<typename _T>
        using detect_get_method = decltype(
            std::declval<const _T&>().get(
                std::declval<const typename _T::key_type&>()));

        // detect_get_or_method
        //   trait: probes for .get_or(key, fallback) → const cvar_type&.
        template<typename _T>
        using detect_get_or_method = decltype(
            std::declval<const _T&>().get_or(
                std::declval<const typename _T::key_type&>(),
                std::declval<const typename _T::cvar_type&>()));

        // detect_set_method
        //   trait: probes for .set(key, value) → bool.
        template<typename _T>
        using detect_set_method = decltype(
            std::declval<_T&>().set(
                std::declval<const typename _T::key_type&>(),
                std::declval<const typename _T::cvar_type&>()));

        // detect_has_method
        //   trait: probes for .has(key) → bool.
        template<typename _T>
        using detect_has_method = decltype(
            std::declval<const _T&>().has(
                std::declval<const typename _T::key_type&>()));

        // -----------------------------------------------------------------
        //  accessor probes
        // -----------------------------------------------------------------

        // detect_lookup_accessor
        //   trait: probes for .lookup() → const lookup_table_type&.
        template<typename _T>
        using detect_lookup_accessor = decltype(
            std::declval<const _T&>().lookup());

        // detect_value_extract_accessor
        //   trait: probes for .value_extract() → const extract_type&.
        template<typename _T>
        using detect_value_extract_accessor = decltype(
            std::declval<const _T&>().value_extract());

    NS_END  // internal


    // =========================================================================
    // II.  INDIVIDUAL DETECTION TRAITS
    // =========================================================================

    // has_get_method
    //   trait: true if the type exposes .get(key).
    template<typename _Type,
             typename = void>
    struct has_get_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_get_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_get_method<_Type>>>
        : std::true_type
    {
    };

    // has_get_or_method
    //   trait: true if the type exposes .get_or(key, fallback).
    template<typename _Type,
             typename = void>
    struct has_get_or_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_get_or_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_cvar_type<_Type>,
               internal::detect_get_or_method<_Type>>>
        : std::true_type
    {
    };

    // has_set_method
    //   trait: true if the type exposes .set(key, value).
    template<typename _Type,
             typename = void>
    struct has_set_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_set_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_cvar_type<_Type>,
               internal::detect_set_method<_Type>>>
        : std::true_type
    {
    };

    // has_has_method
    //   trait: true if the type exposes .has(key).
    template<typename _Type,
             typename = void>
    struct has_has_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_has_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_has_method<_Type>>>
        : std::true_type
    {
    };

    // has_lookup_accessor
    //   trait: true if the type exposes .lookup().
    template<typename _Type,
             typename = void>
    struct has_lookup_accessor : std::false_type
    {
    };

    template<typename _Type>
    struct has_lookup_accessor<_Type,
        void_t<internal::detect_lookup_accessor<_Type>>>
        : std::true_type
    {
    };

    // has_cvar_type
    //   trait: true if the type exposes ::cvar_type alias.
    template<typename _Type,
             typename = void>
    struct has_cvar_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_cvar_type<_Type,
        void_t<internal::detect_cvar_type<_Type>>>
        : std::true_type
    {
    };


    // =========================================================================
    // III. COMPOSITE DETECTION TRAITS
    // =========================================================================

    // is_registry_table
    //   trait: true if the type satisfies the registry table contract:
    // has get(key), has(key), lookup() accessor, cvar_type, key_type,
    // and backing_container_type. The set() method is NOT required.
    template<typename _Type,
             typename = void>
    struct is_registry_table : std::false_type
    {
    };

    // is_registry_table (specialization)
    //   trait: SFINAE success — all registry table probes well-formed.
    template<typename _Type>
    struct is_registry_table<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_cvar_type<_Type>,
               internal::detect_get_method<_Type>,
               internal::detect_has_method<_Type>,
               internal::detect_lookup_accessor<_Type>,
               internal::detect_backing_container_type<_Type>>>
        : std::true_type
    {
    };

    // has_mutable_cvars
    //   trait: true if the registry table supports cvar modification
    // via set(key, value).
    template<typename _Type>
    struct has_mutable_cvars
        : std::integral_constant<bool,
            ( is_registry_table<_Type>::value &&
              has_set_method<_Type>::value )>
    {
    };

    // is_readonly_registry
    //   trait: true if the registry table supports get but NOT set.
    template<typename _Type>
    struct is_readonly_registry
        : std::integral_constant<bool,
            ( is_registry_table<_Type>::value &&
              !has_set_method<_Type>::value )>
    {
    };


    // =========================================================================
    // IV.  VARIABLE TEMPLATES
    // =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    constexpr bool has_get_method_v =
        has_get_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_get_or_method_v =
        has_get_or_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_set_method_v =
        has_set_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_has_method_v =
        has_has_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_lookup_accessor_v =
        has_lookup_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_cvar_type_v =
        has_cvar_type<_Type>::value;

    template<typename _Type>
    constexpr bool is_registry_table_v =
        is_registry_table<_Type>::value;

    template<typename _Type>
    constexpr bool has_mutable_cvars_v =
        has_mutable_cvars<_Type>::value;

    template<typename _Type>
    constexpr bool is_readonly_registry_v =
        is_readonly_registry<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


    // =========================================================================
    // V.   CLASSIFICATION STRUCT
    // =========================================================================

    // registry_table_class
    //   struct: aggregates all registry-table-related structural detections
    // into a single compile-time classification. Query this instead of
    // individual traits when you need the full picture.
    template<typename _Type>
    struct registry_table_class
    {
        // identity
        static constexpr bool is_registry = is_registry_table<_Type>::value;
        static constexpr bool is_readonly = is_readonly_registry<_Type>::value;
        static constexpr bool is_mutable  = has_mutable_cvars<_Type>::value;

        // cvar capabilities
        static constexpr bool has_get     = has_get_method<_Type>::value;
        static constexpr bool has_get_or  = has_get_or_method<_Type>::value;
        static constexpr bool has_set     = has_set_method<_Type>::value;
        static constexpr bool has_has     = has_has_method<_Type>::value;

        // structure
        static constexpr bool has_cvar    = has_cvar_type<_Type>::value;
        static constexpr bool has_lookup  = has_lookup_accessor<_Type>::value;
        static constexpr bool has_backing = has_backing_container<_Type>::value;
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_REGISTRY_TABLE_TRAITS_
