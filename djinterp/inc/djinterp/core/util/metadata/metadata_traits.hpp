/******************************************************************************
* djinterp [util]                                          metadata_traits.hpp
*
* djinterp metadata traits module:
*   Structural classification of types by their exposure of three canonical
* metadata-related names — `metadata`, `metadata_type`, and
* `metadata_container_type` — without any inheritance, tags, or fixed
* `metadata`-class hierarchy. The previous `metadata.hpp` (which defined
* `metadata_value`, `metadata_entry`, and `metadata_collection`) is
* deliberately superseded: there is no longer a "metadata type" in
* djinterp — only the question of whether a given user type carries
* metadata, what type that metadata has, and what container holds it.
*
*   SHAPES
*   ======
*   For each canonical name `X`, three member shapes are detected:
*     - data member       : `t.X` is a well-formed expression
*     - member function   : `t.X()` is a well-formed expression
*     - nested type alias : `typename T::X` names a type
*   These three shapes are mutually exclusive within a single class (the
* C++ class member name space forbids overlap), so detection across all
* three is a structural OR over disjoint cases.
*
*   TRAITS
*   ======
*     has_metadata_data_member                      / _v
*     has_metadata_method                           / _v
*     has_metadata_nested_type                      / _v
*     has_metadata                                  / _v
*     has_metadata_type_data_member                 / _v
*     has_metadata_type_method                      / _v
*     has_metadata_type_nested_type                 / _v
*     has_metadata_type                             / _v
*     has_metadata_container_type_data_member       / _v
*     has_metadata_container_type_method            / _v
*     has_metadata_container_type_nested_type       / _v
*     has_metadata_container_type                   / _v
*
*   EXTRACTORS
*   ==========
*   `_t` aliases yield the type associated with the detected shape:
*     metadata_t<T>                                 — type of `T::metadata`
*     metadata_type_t<T>                            — type of `T::metadata_type`
*     metadata_container_type_t<T>                  — type of
*                                                     `T::metadata_container_type`
*   Dispatch order on access: nested type alias > data member > member
* function. Cleaned of cv/reference qualifiers via `clean_t`. Ill-formed
* if no shape is present.
*
*   PORTABILITY
*   ===========
*     version: C++11 or higher; `_v` companions C++17+.
*     dependencies:
*       - djinterp.hpp           : NS_DJINTERP, NS_INTERNAL, clean_t
*       - core/meta/type_traits.hpp : D_TRAIT_IS_DETECTED, D_TRAIT_HAS_TYPE,
*                                    D_VOID_T
*
*
* path:      /inc/djinterp/core/util/metadata/metadata_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_UTILITY_METADATA_TRAITS_
#define DJINTERP_UTILITY_METADATA_TRAITS_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   `metadata` DETECTION
// =========================================================================
//   Structural detection of the `metadata` name across the three
// natural class-member shapes. The unifying `has_metadata` trait
// returns true if any of the shapes is well-formed for the
// queried type.

// has_metadata_data_member
//   trait: detects a non-static data member named `metadata`
// accessible on a const-qualified instance.
D_TRAIT_IS_DETECTED(has_metadata_data_member,
    decltype(std::declval<const _Type&>().metadata))

// has_metadata_method
//   trait: detects a no-argument member function `metadata()`
// callable on a const-qualified instance.
D_TRAIT_IS_DETECTED(has_metadata_method,
    decltype(std::declval<const _Type&>().metadata()))

// has_metadata_nested_type
//   trait: detects a nested type alias named `metadata`.
D_TRAIT_HAS_TYPE(has_metadata_nested_type, metadata)

// has_metadata
//   trait: true iff `_Type` exposes `metadata` as a data
// member, a no-argument member function, or a nested type alias.
template<typename _Type>
struct has_metadata
    : std::integral_constant<bool,
        ( has_metadata_data_member<_Type>::value  ||
          has_metadata_method<_Type>::value       ||
          has_metadata_nested_type<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    // has_metadata_v
    //   value: variable-template companion to `has_metadata`.
    template<typename _Type>
    inline constexpr bool has_metadata_v = has_metadata<_Type>::value;
#endif


// =========================================================================
// II.  `metadata_type` DETECTION
// =========================================================================
//   Mirror of section I for the `metadata_type` name. In typical
// usage `metadata_type` will be a nested type alias, but member
// and method forms are detected for completeness.

// has_metadata_type_data_member
//   trait: detects a non-static data member named `metadata_type`
// accessible on a const-qualified instance.
D_TRAIT_IS_DETECTED(has_metadata_type_data_member,
    decltype(std::declval<const _Type&>().metadata_type))

// has_metadata_type_method
//   trait: detects a no-argument member function `metadata_type()`
// callable on a const-qualified instance.
D_TRAIT_IS_DETECTED(has_metadata_type_method,
    decltype(std::declval<const _Type&>().metadata_type()))

// has_metadata_type_nested_type
//   trait: detects a nested type alias named `metadata_type`.
D_TRAIT_HAS_TYPE(has_metadata_type_nested_type, metadata_type)

// has_metadata_type
//   trait: true iff `_Type` exposes `metadata_type` as a data
// member, a no-argument member function, or a nested type alias.
template<typename _Type>
struct has_metadata_type
    : std::integral_constant<bool,
        ( has_metadata_type_data_member<_Type>::value  ||
          has_metadata_type_method<_Type>::value       ||
          has_metadata_type_nested_type<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    // has_metadata_type_v
    //   value: variable-template companion to `has_metadata_type`.
    template<typename _Type>
    inline constexpr bool has_metadata_type_v =
        has_metadata_type<_Type>::value;
#endif


// =========================================================================
// III. `metadata_container_type` DETECTION
// =========================================================================
//   Mirror of section I for the `metadata_container_type` name.

// has_metadata_container_type_data_member
//   trait: detects a non-static data member named
// `metadata_container_type` accessible on a const-qualified
// instance.
D_TRAIT_IS_DETECTED(has_metadata_container_type_data_member,
    decltype(std::declval<const _Type&>().metadata_container_type))

// has_metadata_container_type_method
//   trait: detects a no-argument member function
// `metadata_container_type()` callable on a const-qualified
// instance.
D_TRAIT_IS_DETECTED(has_metadata_container_type_method,
    decltype(std::declval<const _Type&>().metadata_container_type()))

// has_metadata_container_type_nested_type
//   trait: detects a nested type alias named
// `metadata_container_type`.
D_TRAIT_HAS_TYPE(has_metadata_container_type_nested_type,
                 metadata_container_type)

// has_metadata_container_type
//   trait: true iff `_Type` exposes `metadata_container_type`
// as a data member, a no-argument member function, or a nested
// type alias.
template<typename _Type>
struct has_metadata_container_type
    : std::integral_constant<bool,
        ( has_metadata_container_type_data_member<_Type>::value  ||
          has_metadata_container_type_method<_Type>::value       ||
          has_metadata_container_type_nested_type<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    // has_metadata_container_type_v
    //   value: variable-template companion to
    // `has_metadata_container_type`.
    template<typename _Type>
    inline constexpr bool has_metadata_container_type_v =
        has_metadata_container_type<_Type>::value;
#endif


// =========================================================================
// IV.  TYPE EXTRACTION ( _t aliases )
// =========================================================================
//   For each canonical name, an `_t` alias yields the type
// associated with whichever shape the queried type exposes.
// Dispatch order on access: nested type alias first, then data
// member, then member function return. The result is cleaned of
// cv/reference qualifiers via `clean_t`. Substitution failure if
// no shape is detected.

NS_INTERNAL

    // metadata_extractor_helper
    //   trait: dispatcher between the three protocol shapes
    // for `metadata_t`. Primary template — failure case (no
    // shape detected).
    template<typename _Type,
             bool     _HasNested,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_extractor_helper
    {};

    // metadata_extractor_helper (nested-alias case)
    //   trait: yields `typename _Type::metadata` when present
    // as a nested type alias.
    template<typename _Type,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_extractor_helper<_Type, true, _HasMember, _HasMethod>
    {
        using type = typename clean_t<_Type>::metadata;
    };

    // metadata_extractor_helper (data-member case)
    //   trait: yields the cleaned type of `_Type::metadata`
    // when present only as a data member.
    template<typename _Type,
             bool     _HasMethod>
    struct metadata_extractor_helper<_Type, false, true, _HasMethod>
    {
        using type =
            clean_t<decltype(std::declval<const _Type&>().metadata)>;
    };

    // metadata_extractor_helper (method-only case)
    //   trait: yields the cleaned return type of
    // `_Type::metadata()` when present only as a member function.
    template<typename _Type>
    struct metadata_extractor_helper<_Type, false, false, true>
    {
        using type =
            clean_t<decltype(std::declval<const _Type&>().metadata())>;
    };


    // metadata_type_extractor_helper
    //   trait: dispatcher between the three protocol shapes
    // for `metadata_type_t`. Primary template — failure case.
    template<typename _Type,
             bool     _HasNested,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_type_extractor_helper
    {};

    // metadata_type_extractor_helper (nested-alias case)
    template<typename _Type,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_type_extractor_helper<_Type, true, _HasMember, _HasMethod>
    {
        using type = typename clean_t<_Type>::metadata_type;
    };

    // metadata_type_extractor_helper (data-member case)
    template<typename _Type,
             bool     _HasMethod>
    struct metadata_type_extractor_helper<_Type, false, true, _HasMethod>
    {
        using type =
            clean_t<decltype(std::declval<const _Type&>().metadata_type)>;
    };

    // metadata_type_extractor_helper (method-only case)
    template<typename _Type>
    struct metadata_type_extractor_helper<_Type, false, false, true>
    {
        using type =
            clean_t<decltype(std::declval<const _Type&>().metadata_type())>;
    };


    // metadata_container_type_extractor_helper
    //   trait: dispatcher between the three protocol shapes
    // for `metadata_container_type_t`. Primary template —
    // failure case.
    template<typename _Type,
             bool     _HasNested,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_container_type_extractor_helper
    {};

    // metadata_container_type_extractor_helper (nested-alias case)
    template<typename _Type,
             bool     _HasMember,
             bool     _HasMethod>
    struct metadata_container_type_extractor_helper<_Type,
                                                    true,
                                                    _HasMember,
                                                    _HasMethod>
    {
        using type = typename clean_t<_Type>::metadata_container_type;
    };

    // metadata_container_type_extractor_helper (data-member case)
    template<typename _Type,
             bool     _HasMethod>
    struct metadata_container_type_extractor_helper<_Type,
                                                    false,
                                                    true,
                                                    _HasMethod>
    {
        using type =
            clean_t<decltype(
                std::declval<const _Type&>().metadata_container_type)>;
    };

    // metadata_container_type_extractor_helper (method-only case)
    template<typename _Type>
    struct metadata_container_type_extractor_helper<_Type,
                                                    false,
                                                    false,
                                                    true>
    {
        using type =
            clean_t<decltype(
                std::declval<const _Type&>().metadata_container_type())>;
    };

NS_END  // internal


// metadata_t
//   alias: yields the type associated with `_Type::metadata`,
// dispatched across nested type alias, data member, and member
// function shapes. Ill-formed if no shape is present.
template<typename _Type>
using metadata_t = typename internal::metadata_extractor_helper<
        _Type,
        has_metadata_nested_type<_Type>::value,
        has_metadata_data_member<_Type>::value,
        has_metadata_method<_Type>::value
    >::type;

// metadata_type_t
//   alias: yields the type associated with `_Type::metadata_type`,
// dispatched across nested type alias, data member, and member
// function shapes. Ill-formed if no shape is present.
template<typename _Type>
using metadata_type_t = typename internal::metadata_type_extractor_helper<
        _Type,
        has_metadata_type_nested_type<_Type>::value,
        has_metadata_type_data_member<_Type>::value,
        has_metadata_type_method<_Type>::value
    >::type;

// metadata_container_type_t
//   alias: yields the type associated with
// `_Type::metadata_container_type`, dispatched across nested
// type alias, data member, and member function shapes.
// Ill-formed if no shape is present.
template<typename _Type>
using metadata_container_type_t =
    typename internal::metadata_container_type_extractor_helper<
        _Type,
        has_metadata_container_type_nested_type<_Type>::value,
        has_metadata_container_type_data_member<_Type>::value,
        has_metadata_container_type_method<_Type>::value
    >::type;


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_METADATA_TRAITS_
