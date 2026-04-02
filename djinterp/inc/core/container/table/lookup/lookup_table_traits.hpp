/******************************************************************************
* djinterp [container]                                  lookup_table_traits.hpp
*
* djinterp lookup table traits header:
*   SFINAE-based structural detection for existence tables and lookup tables.
* All detection is purely structural — no tags, no base-class checks. Any
* type exposing the right members is classified, not just the library's own
* existence_table and lookup_table.
*
*   TWO CLASSIFICATION GROUPS:
*   1. Existence table — has contains(key), backing_container_type, and a
*      key extraction mechanism. Set-like: answers "is this key present?"
*
*   2. Lookup table — everything an existence table has, plus find(key)
*      returning a pointer to the element. Map-like: answers "give me the
*      element for this key."
*
*   DETECTION PROBES:
*   - backing_container_type  → backed container axis
*   - key_type                → key detection
*   - contains(key)           → existence capability
*   - find(key)               → value-retrieval capability
*   - table()                 → underlying table access
*   - extract()               → key extractor access
*   - count(key)              → multiplicity query
*   - count_if(pred)          → predicate count
*
*   CLASSIFICATION STRUCT:
*   lookup_table_class<T> aggregates all detections into a single
*   compile-time struct with ~12 bool members.
*
* path:      \inc\container\lookup_table_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINER_LOOKUP_TABLE_TRAITS_
#define DJINTERP_CONTAINER_LOOKUP_TABLE_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "..\..\djinterp.h"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   DETECTION OPERATIONS
    // =========================================================================

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  type alias probes
        // -----------------------------------------------------------------

        // detect_key_type
        //   trait: probes for ::key_type alias.
        template<typename _T>
        using detect_key_type = typename _T::key_type;

        // detect_element_type
        //   trait: probes for ::element_type alias.
        template<typename _T>
        using detect_element_type = typename _T::element_type;

        // detect_extract_type
        //   trait: probes for ::extract_type alias.
        template<typename _T>
        using detect_extract_type = typename _T::extract_type;

        // detect_strategy_type
        //   trait: probes for ::strategy_type alias.
        template<typename _T>
        using detect_strategy_type = typename _T::strategy_type;

        // detect_backing_container_type
        //   trait: probes for ::backing_container_type alias.
        template<typename _T>
        using detect_backing_container_type =
            typename _T::backing_container_type;

        // detect_table_type
        //   trait: probes for ::table_type alias.
        template<typename _T>
        using detect_table_type = typename _T::table_type;

        // -----------------------------------------------------------------
        //  method probes
        // -----------------------------------------------------------------

        // detect_contains_method
        //   trait: probes for .contains(key) → bool.
        template<typename _T>
        using detect_contains_method = decltype(
            std::declval<const _T&>().contains(
                std::declval<const typename _T::key_type&>()));

        // detect_find_method
        //   trait: probes for .find(key) → const element*.
        template<typename _T>
        using detect_find_method = decltype(
            std::declval<const _T&>().find(
                std::declval<const typename _T::key_type&>()));

        // detect_find_or_method
        //   trait: probes for .find_or(key, fallback).
        template<typename _T>
        using detect_find_or_method = decltype(
            std::declval<const _T&>().find_or(
                std::declval<const typename _T::key_type&>(),
                std::declval<const typename _T::value_type&>()));

        // detect_count_method
        //   trait: probes for .count(key) → size_type.
        template<typename _T>
        using detect_count_method = decltype(
            std::declval<const _T&>().count(
                std::declval<const typename _T::key_type&>()));

        // detect_table_accessor
        //   trait: probes for .table() → const backing_type&.
        template<typename _T>
        using detect_table_accessor = decltype(
            std::declval<const _T&>().table());

        // detect_extract_accessor
        //   trait: probes for .extract() → const extract_type&.
        template<typename _T>
        using detect_extract_accessor = decltype(
            std::declval<const _T&>().extract());

    NS_END  // internal


    // =========================================================================
    // II.  INDIVIDUAL DETECTION TRAITS
    // =========================================================================

    // has_key_type
    //   trait: true if the type exposes a ::key_type alias.
    template<typename _Type,
             typename = void>
    struct has_key_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_key_type<_Type,
        void_t<internal::detect_key_type<_Type>>>
        : std::true_type
    {
    };

    // has_contains_method
    //   trait: true if the type exposes .contains(key) → bool.
    template<typename _Type,
             typename = void>
    struct has_contains_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_contains_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_contains_method<_Type>>>
        : std::true_type
    {
    };

    // has_find_method
    //   trait: true if the type exposes .find(key) returning a pointer.
    template<typename _Type,
             typename = void>
    struct has_find_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_find_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_find_method<_Type>>>
        : std::true_type
    {
    };

    // has_find_or_method
    //   trait: true if the type exposes .find_or(key, fallback).
    template<typename _Type,
             typename = void>
    struct has_find_or_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_find_or_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_find_or_method<_Type>>>
        : std::true_type
    {
    };

    // has_count_method
    //   trait: true if the type exposes .count(key).
    template<typename _Type,
             typename = void>
    struct has_count_method : std::false_type
    {
    };

    template<typename _Type>
    struct has_count_method<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_count_method<_Type>>>
        : std::true_type
    {
    };

    // has_table_accessor
    //   trait: true if the type exposes .table() returning the backing
    // container.
    template<typename _Type,
             typename = void>
    struct has_table_accessor : std::false_type
    {
    };

    template<typename _Type>
    struct has_table_accessor<_Type,
        void_t<internal::detect_table_accessor<_Type>>>
        : std::true_type
    {
    };

    // has_extract_accessor
    //   trait: true if the type exposes .extract() returning the key
    // extractor.
    template<typename _Type,
             typename = void>
    struct has_extract_accessor : std::false_type
    {
    };

    template<typename _Type>
    struct has_extract_accessor<_Type,
        void_t<internal::detect_extract_accessor<_Type>>>
        : std::true_type
    {
    };

    // has_backing_container
    //   trait: true if the type exposes ::backing_container_type.
    template<typename _Type,
             typename = void>
    struct has_backing_container : std::false_type
    {
    };

    template<typename _Type>
    struct has_backing_container<_Type,
        void_t<internal::detect_backing_container_type<_Type>>>
        : std::true_type
    {
    };


    // =========================================================================
    // III. COMPOSITE DETECTION TRAITS
    // =========================================================================

    // is_existence_table
    //   trait: true if the type satisfies the existence table contract:
    // has key_type, contains(key), table() accessor, and is a backed
    // container. Does NOT require find().
    template<typename _Type,
             typename = void>
    struct is_existence_table : std::false_type
    {
    };

    // is_existence_table (specialization)
    //   trait: SFINAE success — all existence table probes well-formed.
    template<typename _Type>
    struct is_existence_table<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_contains_method<_Type>,
               internal::detect_table_accessor<_Type>,
               internal::detect_backing_container_type<_Type>>>
        : std::true_type
    {
    };

    // is_lookup_table
    //   trait: true if the type satisfies the lookup table contract:
    // everything an existence table needs, plus find(key) returning an
    // element pointer.
    template<typename _Type,
             typename = void>
    struct is_lookup_table : std::false_type
    {
    };

    // is_lookup_table (specialization)
    //   trait: SFINAE success — all lookup table probes well-formed.
    template<typename _Type>
    struct is_lookup_table<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_contains_method<_Type>,
               internal::detect_find_method<_Type>,
               internal::detect_table_accessor<_Type>,
               internal::detect_backing_container_type<_Type>>>
        : std::true_type
    {
    };

    // is_searchable
    //   trait: true if the type supports keyed lookup via find() and
    // contains(), regardless of whether it is a backed/wrapper type.
    // More permissive than is_lookup_table.
    template<typename _Type,
             typename = void>
    struct is_searchable : std::false_type
    {
    };

    template<typename _Type>
    struct is_searchable<_Type,
        void_t<internal::detect_key_type<_Type>,
               internal::detect_find_method<_Type>,
               internal::detect_contains_method<_Type>>>
        : std::true_type
    {
    };


    // =========================================================================
    // IV.  VARIABLE TEMPLATES
    // =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    constexpr bool has_key_type_v =
        has_key_type<_Type>::value;

    template<typename _Type>
    constexpr bool has_contains_method_v =
        has_contains_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_find_method_v =
        has_find_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_find_or_method_v =
        has_find_or_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_count_method_v =
        has_count_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_table_accessor_v =
        has_table_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_extract_accessor_v =
        has_extract_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_backing_container_v =
        has_backing_container<_Type>::value;

    template<typename _Type>
    constexpr bool is_existence_table_v =
        is_existence_table<_Type>::value;

    template<typename _Type>
    constexpr bool is_lookup_table_v =
        is_lookup_table<_Type>::value;

    template<typename _Type>
    constexpr bool is_searchable_v =
        is_searchable<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


    // =========================================================================
    // V.   CLASSIFICATION STRUCT
    // =========================================================================

    // lookup_table_class
    //   struct: aggregates all lookup-related structural detections for a
    // type into a single compile-time classification. Query this instead
    // of individual traits when you need the full picture.
    template<typename _Type>
    struct lookup_table_class
    {
        // identity
        static constexpr bool is_existence   = is_existence_table<_Type>::value;
        static constexpr bool is_lookup      = is_lookup_table<_Type>::value;
        static constexpr bool is_searchable  = ::djinterp::container::is_searchable<_Type>::value;

        // capabilities
        static constexpr bool has_contains   = has_contains_method<_Type>::value;
        static constexpr bool has_find       = has_find_method<_Type>::value;
        static constexpr bool has_find_or    = has_find_or_method<_Type>::value;
        static constexpr bool has_count      = has_count_method<_Type>::value;

        // structure
        static constexpr bool has_backing    = has_backing_container<_Type>::value;
        static constexpr bool has_table      = has_table_accessor<_Type>::value;
        static constexpr bool has_extract    = has_extract_accessor<_Type>::value;
        static constexpr bool has_key        = has_key_type<_Type>::value;

        // group classification
        static constexpr bool is_existence_only =
            ( is_existence && !is_lookup );
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LOOKUP_TABLE_TRAITS_
