/******************************************************************************
* djinterp [container]                                lookup_table_concepts.hpp
*
* Lookup table concepts:
*   C++20 concepts layered over lookup_table_traits.hpp. These concepts
* provide readable constraints for existence-table-like and lookup-table-like
* types without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* lookup_table_traits.hpp:
*   - key and backing structure detection
*   - contains / find / find_or / count / table / extract capabilities
*   - existence-table, lookup-table, and searchable classification
*   - backing table storage classification (static / dynamic / any)
*   - aggregate shorthand concepts over lookup_table_class<T>
*
* path:      /inc/djinterp/container/table/lookup/lookup_table_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.24
******************************************************************************/

#ifndef DJINTERP_CONTAINER_LOOKUP_TABLE_CONCEPTS_
#define DJINTERP_CONTAINER_LOOKUP_TABLE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "lookup_table_concepts.hpp requires C++ compilation"
#endif

#include "lookup_table_traits.hpp"


NS_DJINTERP
NS_CONTAINER

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // =========================================================================
    // I.   PRIMITIVE STRUCTURAL CONCEPTS
    // =========================================================================

    // keyed_lookup_surface
    //   concept: constrains types exposing a key_type alias.
    template<typename _Type>
    concept keyed_lookup_surface =
        has_key_type<_Type>::value;

    // backed_lookup_surface
    //   concept: constrains types exposing backing_container_type.
    template<typename _Type>
    concept backed_lookup_surface =
        has_backing_container<_Type>::value;

    // contains_capable_lookup_surface
    //   concept: constrains types exposing contains(key).
    template<typename _Type>
    concept contains_capable_lookup_surface =
        has_contains_method<_Type>::value;

    // find_capable_lookup_surface
    //   concept: constrains types exposing find(key).
    template<typename _Type>
    concept find_capable_lookup_surface =
        has_find_method<_Type>::value;

    // find_or_capable_lookup_surface
    //   concept: constrains types exposing find_or(key, fallback).
    template<typename _Type>
    concept find_or_capable_lookup_surface =
        has_find_or_method<_Type>::value;

    // count_capable_lookup_surface
    //   concept: constrains types exposing count(key).
    template<typename _Type>
    concept count_capable_lookup_surface =
        has_count_method<_Type>::value;

    // table_access_lookup_surface
    //   concept: constrains types exposing table().
    template<typename _Type>
    concept table_access_lookup_surface =
        has_table_accessor<_Type>::value;

    // extract_access_lookup_surface
    //   concept: constrains types exposing extract().
    template<typename _Type>
    concept extract_access_lookup_surface =
        has_extract_accessor<_Type>::value;


    // =========================================================================
    // II.  AGGREGATE IDENTITY CONCEPTS
    // =========================================================================

    // existence_table_type
    //   concept: constrains types satisfying the existence table contract.
    template<typename _Type>
    concept existence_table_type =
        is_existence_table<_Type>::value;

    // lookup_table_type
    //   concept: constrains types satisfying the lookup table contract.
    template<typename _Type>
    concept lookup_table_type =
        is_lookup_table<_Type>::value;

    // searchable_lookup_surface
    //   concept: constrains types supporting keyed lookup regardless of
    // wrapper/backing classification.
    template<typename _Type>
    concept searchable_lookup_surface =
        is_searchable<_Type>::value;


    // =========================================================================
    // III. COMPOUND CAPABILITY CONCEPTS
    // =========================================================================

    // readable_existence_table_type
    //   concept: existence table with count-based query support.
    template<typename _Type>
    concept readable_existence_table_type =
        existence_table_type<_Type> &&
        count_capable_lookup_surface<_Type>;

    // retrievable_lookup_table_type
    //   concept: lookup table with both find() and find_or().
    template<typename _Type>
    concept retrievable_lookup_table_type =
        lookup_table_type<_Type> &&
        find_capable_lookup_surface<_Type> &&
        find_or_capable_lookup_surface<_Type>;

    // extracted_lookup_table_type
    //   concept: lookup surface with extractor access.
    template<typename _Type>
    concept extracted_lookup_table_type =
        searchable_lookup_surface<_Type> &&
        extract_access_lookup_surface<_Type>;

    // wrapped_lookup_table_type
    //   concept: backed lookup wrapper exposing both table() and extract().
    template<typename _Type>
    concept wrapped_lookup_table_type =
        backed_lookup_surface<_Type> &&
        table_access_lookup_surface<_Type> &&
        extract_access_lookup_surface<_Type>;


    // =========================================================================
    // IV.  CLASSIFICATION-BASED SHORTHAND CONCEPTS
    // =========================================================================

    // classified_lookup_table_type
    //   concept: shorthand for types recognized by lookup_table_class as lookup.
    template<typename _Type>
    concept classified_lookup_table_type =
        lookup_table_class<_Type>::is_lookup;

    // classified_existence_table_type
    //   concept: shorthand for types recognized by lookup_table_class as existence tables.
    template<typename _Type>
    concept classified_existence_table_type =
        lookup_table_class<_Type>::is_existence;

    // classified_searchable_lookup_surface
    //   concept: shorthand for types recognized by lookup_table_class as searchable.
    template<typename _Type>
    concept classified_searchable_lookup_surface =
        lookup_table_class<_Type>::is_searchable;

    // existence_only_table_type
    //   concept: existence table that does not satisfy full lookup-table requirements.
    template<typename _Type>
    concept existence_only_table_type =
        lookup_table_class<_Type>::is_existence_only;

    // fully_wrapped_lookup_table_type
    //   concept: lookup table with key, backing, table access, and extractor access.
    template<typename _Type>
    concept fully_wrapped_lookup_table_type =
        lookup_table_type<_Type> &&
        lookup_table_class<_Type>::has_key &&
        lookup_table_class<_Type>::has_backing &&
        lookup_table_class<_Type>::has_table &&
        lookup_table_class<_Type>::has_extract;

    // static_backed_lookup_table_type
    //   concept: lookup or existence table whose innermost backing storage
    // is a fixed-dimension table<>. D_CONSTEXPR paths on the wrapper are
    // valid in C++17 compile-time contexts.
    template<typename _Type>
    concept static_backed_lookup_table_type =
        lookup_table_class<_Type>::is_static_backing;

    // dynamic_backed_lookup_table_type
    //   concept: lookup or existence table whose backing storage is a
    // runtime-dimension table<> or database_table<>. The wrapper must be
    // used at runtime only.
    template<typename _Type>
    concept dynamic_backed_lookup_table_type =
        lookup_table_class<_Type>::is_dynamic_backing;

    // table_backed_lookup_table_type
    //   concept: lookup or existence table whose backing satisfies
    // is_table_type (static or dynamic djinterp table).
    template<typename _Type>
    concept table_backed_lookup_table_type =
        lookup_table_class<_Type>::is_table_backing;


    // =========================================================================
    // V.   BACKING TABLE STORAGE CONCEPTS
    // =========================================================================
    //
    // Individual structural concepts derived from the has_table_backing /
    // has_static_table_backing / has_dynamic_table_backing traits added in
    // lookup_table_traits.hpp section IIa. These fire on the presence of
    // backing_container_type alone and do not require full lookup-table
    // identity, making them usable on partially-classified types.
    //

    // table_backed_lookup_surface
    //   concept: constrains types whose backing_container_type satisfies
    // is_table_type (any djinterp table — static or dynamic).
    template<typename _Type>
    concept table_backed_lookup_surface =
        has_table_backing<_Type>::value;

    // static_table_backed_lookup_surface
    //   concept: constrains types whose backing_container_type satisfies
    // is_static_table_type. The backing is a fixed-dimension table<>;
    // constexpr use of the wrapper is valid in C++17.
    template<typename _Type>
    concept static_table_backed_lookup_surface =
        has_static_table_backing<_Type>::value;

    // dynamic_table_backed_lookup_surface
    //   concept: constrains types whose backing_container_type satisfies
    // is_dynamic_table_type. The backing is a runtime-dimension table<>
    // or database_table<>; the wrapper is runtime-only.
    template<typename _Type>
    concept dynamic_table_backed_lookup_surface =
        has_dynamic_table_backing<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LOOKUP_TABLE_CONCEPTS_
