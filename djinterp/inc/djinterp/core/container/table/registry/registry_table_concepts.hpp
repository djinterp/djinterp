/******************************************************************************
* djinterp [container]                              registry_table_concepts.hpp
*
* Registry table concepts:
*   C++20 concepts layered over registry_table_traits.hpp. These concepts
* provide readable constraints for registry-table-like types without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* registry_table_traits.hpp:
*   - cvar access method detection
*   - lookup/backing structure detection
*   - inner table storage classification (static / dynamic / any)
*   - registry identity and mutability classification
*   - shorthand concepts over registry_table_class<T>
*
* 
* path:      /inc/djinterp/container/table/registry/registry_table_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.24
******************************************************************************/

#ifndef DJINTERP_CONTAINER_REGISTRY_TABLE_CONCEPTS_
#define DJINTERP_CONTAINER_REGISTRY_TABLE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "registry_table_concepts.hpp requires C++ compilation"
#endif

#include "registry_table_traits.hpp"


NS_DJINTERP
NS_CONTAINER

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // =========================================================================
    // I.   PRIMITIVE STRUCTURAL CONCEPTS
    // =========================================================================

    // cvar_typed_registry_surface
    //   concept: constrains types exposing a cvar_type alias.
    template<typename _Type>
    concept cvar_typed_registry_surface =
        has_cvar_type<_Type>::value;

    // get_capable_registry_surface
    //   concept: constrains types exposing get(key).
    template<typename _Type>
    concept get_capable_registry_surface =
        has_get_method<_Type>::value;

    // get_or_capable_registry_surface
    //   concept: constrains types exposing get_or(key, fallback).
    template<typename _Type>
    concept get_or_capable_registry_surface =
        has_get_or_method<_Type>::value;

    // set_capable_registry_surface
    //   concept: constrains types exposing set(key, value).
    template<typename _Type>
    concept set_capable_registry_surface =
        has_set_method<_Type>::value;

    // has_capable_registry_surface
    //   concept: constrains types exposing has(key).
    template<typename _Type>
    concept has_capable_registry_surface =
        has_has_method<_Type>::value;

    // lookup_access_registry_surface
    //   concept: constrains types exposing lookup().
    template<typename _Type>
    concept lookup_access_registry_surface =
        has_lookup_accessor<_Type>::value;


    // =========================================================================
    // II.  REGISTRY IDENTITY CONCEPTS
    // =========================================================================

    // registry_table_type
    //   concept: constrains types satisfying the registry table contract.
    template<typename _Type>
    concept registry_table_type =
        is_registry_table<_Type>::value;

    // mutable_registry_table_type
    //   concept: constrains registry tables supporting set(key, value).
    template<typename _Type>
    concept mutable_registry_table_type =
        has_mutable_cvars<_Type>::value;

    // readonly_registry_table_type
    //   concept: constrains registry tables supporting read access only.
    template<typename _Type>
    concept readonly_registry_table_type =
        is_readonly_registry<_Type>::value;


    // =========================================================================
    // III. COMPOUND CAPABILITY CONCEPTS
    // =========================================================================

    // readable_registry_table_type
    //   concept: registry table with get/get_or/has support.
    template<typename _Type>
    concept readable_registry_table_type =
        registry_table_type<_Type> &&
        has_get_method<_Type>::value &&
        has_get_or_method<_Type>::value &&
        has_has_method<_Type>::value;

    // lookup_backed_registry_table_type
    //   concept: registry table exposing lookup() and backing structure.
    template<typename _Type>
    concept lookup_backed_registry_table_type =
        registry_table_type<_Type> &&
        has_lookup_accessor<_Type>::value &&
        has_backing_container<_Type>::value;

    // readwrite_registry_table_type
    //   concept: registry table with both full read and write support.
    template<typename _Type>
    concept readwrite_registry_table_type =
        readable_registry_table_type<_Type> &&
        mutable_registry_table_type<_Type>;


    // =========================================================================
    // IV.  CLASSIFICATION-BASED SHORTHAND CONCEPTS
    // =========================================================================

    // classified_registry_table_type
    //   concept: shorthand for types recognized by registry_table_class as registries.
    template<typename _Type>
    concept classified_registry_table_type =
        registry_table_class<_Type>::is_registry;

    // classified_mutable_registry_table_type
    //   concept: shorthand for types recognized by registry_table_class as mutable.
    template<typename _Type>
    concept classified_mutable_registry_table_type =
        registry_table_class<_Type>::is_mutable;

    // classified_readonly_registry_table_type
    //   concept: shorthand for types recognized by registry_table_class as readonly.
    template<typename _Type>
    concept classified_readonly_registry_table_type =
        registry_table_class<_Type>::is_readonly;

    // fully_classified_registry_table_type
    //   concept: registry table with cvar typing, lookup access, and backing.
    template<typename _Type>
    concept fully_classified_registry_table_type =
        registry_table_class<_Type>::is_registry &&
        registry_table_class<_Type>::has_cvar &&
        registry_table_class<_Type>::has_lookup &&
        registry_table_class<_Type>::has_backing;

    // static_inner_backed_registry_table_type
    //   concept: registry table whose innermost storage (two levels down:
    // registry → lookup → table) is a fixed-dimension table<>. The full
    // wrapper chain is valid in C++17 compile-time contexts.
    template<typename _Type>
    concept static_inner_backed_registry_table_type =
        registry_table_class<_Type>::is_static_backing;

    // dynamic_inner_backed_registry_table_type
    //   concept: registry table whose innermost storage is a runtime-
    // dimension table<> or database_table<>. The full chain is runtime-only.
    template<typename _Type>
    concept dynamic_inner_backed_registry_table_type =
        registry_table_class<_Type>::is_dynamic_backing;

    // table_inner_backed_registry_table_type
    //   concept: registry table whose innermost storage satisfies
    // is_table_type (any djinterp table, static or dynamic).
    template<typename _Type>
    concept table_inner_backed_registry_table_type =
        registry_table_class<_Type>::is_table_backing;


    // =========================================================================
    // V.   INNER TABLE STORAGE CONCEPTS
    // =========================================================================
    //
    // Individual structural concepts derived from the has_table_inner_backing /
    // has_static_inner_table / has_dynamic_inner_table traits added in
    // registry_table_traits.hpp section IIa. These fire on the presence of
    // lookup_table_type alone and do not require full registry-table identity,
    // making them usable on partially-classified types.
    //
    // The "inner table" is two levels below the registry:
    //   registry_table → lookup_table → table<>
    //

    // table_inner_backed_registry_surface
    //   concept: constrains types whose inner backing (via lookup_table_type)
    // satisfies is_table_type (any djinterp table — static or dynamic).
    template<typename _Type>
    concept table_inner_backed_registry_surface =
        has_table_inner_backing<_Type>::value;

    // static_inner_backed_registry_surface
    //   concept: constrains types whose inner backing satisfies
    // is_static_table_type. The innermost storage is a fixed-dimension
    // table<>; the full chain is constexpr-valid in C++17.
    template<typename _Type>
    concept static_inner_backed_registry_surface =
        has_static_inner_table<_Type>::value;

    // dynamic_inner_backed_registry_surface
    //   concept: constrains types whose inner backing satisfies
    // is_dynamic_table_type. The innermost storage is a runtime-dimension
    // table<> or database_table<>; the full chain is runtime-only.
    template<typename _Type>
    concept dynamic_inner_backed_registry_surface =
        has_dynamic_inner_table<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_REGISTRY_TABLE_CONCEPTS_
