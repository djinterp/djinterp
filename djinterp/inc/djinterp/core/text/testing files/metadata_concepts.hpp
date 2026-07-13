/******************************************************************************
* djinterp [util]                                        metadata_concepts.hpp
*
* djinterp metadata concepts module:
*   C++20 concept-based companions to `metadata_traits.hpp`. Provides
* `requires`-expression checks for the same three canonical metadata names
* — `metadata`, `metadata_type`, and `metadata_container_type` — across
* their three natural class-member shapes (data member, member function,
* nested type alias).
*
*   NAMESPACE
*   =========
*   To allow this header and `metadata_traits.hpp` to coexist without
* collision (a concept and a class template sharing a name in the same
* namespace would clash), all concepts live in the `djinterp::concepts`
* sub-namespace. Idiomatic use:
*     using namespace djinterp::concepts;
*     template<has_metadata _T> void foo(_T const&);
*
*   CONCEPTS
*   ========
*     has_metadata_data_member<T>
*     has_metadata_method<T>
*     has_metadata_nested_type<T>
*     has_metadata<T>                                  — any of the three above
*     has_metadata_type_data_member<T>
*     has_metadata_type_method<T>
*     has_metadata_type_nested_type<T>
*     has_metadata_type<T>                             — any of the three above
*     has_metadata_container_type_data_member<T>
*     has_metadata_container_type_method<T>
*     has_metadata_container_type_nested_type<T>
*     has_metadata_container_type<T>                   — any of the three above
*
*   EXTRACTORS
*   ==========
*   The `_t` type-extraction aliases (`metadata_t`,
* `metadata_type_t`, `metadata_container_type_t`) live in
* `metadata_traits.hpp` and apply equally under C++20. This header
* re-exports them inside the `concepts` sub-namespace via using
* declarations for ergonomics.
*
*   PORTABILITY
*   ===========
*     version: C++20 or higher (header is empty under earlier standards).
*     dependencies:
*       - djinterp.hpp           : NS_DJINTERP
*       - core/meta/type_traits.hpp : feature detection macros
*       - metadata_traits.hpp    : `_t` aliases re-exported here
*
*
* path:      /inc/djinterp/core/util/metadata/metadata_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_UTILITY_METADATA_CONCEPTS_
#define DJINTERP_UTILITY_METADATA_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_traits.hpp"
#include "./metadata_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

namespace concepts {


// =========================================================================
// I.   `metadata` CONCEPTS
// =========================================================================
//   Concept-form detection of the `metadata` name across the
// three natural class-member shapes. The unifying `has_metadata`
// concept is satisfied if any of the shapes is well-formed.

// has_metadata_data_member
//   concept: satisfied when `_Type` exposes `metadata` as a
// non-static data member accessible on a const-qualified
// instance.
template<typename _Type>
concept has_metadata_data_member = requires(const _Type& _t)
{
    _t.metadata;
};

// has_metadata_method
//   concept: satisfied when `_Type` exposes a no-argument
// member function `metadata()` callable on a const-qualified
// instance.
template<typename _Type>
concept has_metadata_method = requires(const _Type& _t)
{
    _t.metadata();
};

// has_metadata_nested_type
//   concept: satisfied when `_Type` exposes a nested type
// alias named `metadata`.
template<typename _Type>
concept has_metadata_nested_type = requires
{
    typename _Type::metadata;
};

// has_metadata
//   concept: satisfied iff `_Type` exposes `metadata` as a
// data member, a no-argument member function, or a nested type
// alias.
template<typename _Type>
concept has_metadata =
    ( has_metadata_data_member<_Type>  ||
      has_metadata_method<_Type>       ||
      has_metadata_nested_type<_Type> );


// =========================================================================
// II.  `metadata_type` CONCEPTS
// =========================================================================
//   Mirror of section I for the `metadata_type` name. In typical
// usage `metadata_type` will be a nested type alias, but member
// and method forms are detected for completeness.

// has_metadata_type_data_member
//   concept: satisfied when `_Type` exposes `metadata_type` as
// a non-static data member.
template<typename _Type>
concept has_metadata_type_data_member = requires(const _Type& _t)
{
    _t.metadata_type;
};

// has_metadata_type_method
//   concept: satisfied when `_Type` exposes a no-argument
// member function `metadata_type()`.
template<typename _Type>
concept has_metadata_type_method = requires(const _Type& _t)
{
    _t.metadata_type();
};

// has_metadata_type_nested_type
//   concept: satisfied when `_Type` exposes a nested type
// alias named `metadata_type`.
template<typename _Type>
concept has_metadata_type_nested_type = requires
{
    typename _Type::metadata_type;
};

// has_metadata_type
//   concept: satisfied iff `_Type` exposes `metadata_type` as
// a data member, a no-argument member function, or a nested
// type alias.
template<typename _Type>
concept has_metadata_type =
    ( has_metadata_type_data_member<_Type>  ||
      has_metadata_type_method<_Type>       ||
      has_metadata_type_nested_type<_Type> );


// =========================================================================
// III. `metadata_container_type` CONCEPTS
// =========================================================================
//   Mirror of section I for the `metadata_container_type` name.

// has_metadata_container_type_data_member
//   concept: satisfied when `_Type` exposes
// `metadata_container_type` as a non-static data member.
template<typename _Type>
concept has_metadata_container_type_data_member = requires(const _Type& _t)
{
    _t.metadata_container_type;
};

// has_metadata_container_type_method
//   concept: satisfied when `_Type` exposes a no-argument
// member function `metadata_container_type()`.
template<typename _Type>
concept has_metadata_container_type_method = requires(const _Type& _t)
{
    _t.metadata_container_type();
};

// has_metadata_container_type_nested_type
//   concept: satisfied when `_Type` exposes a nested type
// alias named `metadata_container_type`.
template<typename _Type>
concept has_metadata_container_type_nested_type = requires
{
    typename _Type::metadata_container_type;
};

// has_metadata_container_type
//   concept: satisfied iff `_Type` exposes
// `metadata_container_type` as a data member, a no-argument
// member function, or a nested type alias.
template<typename _Type>
concept has_metadata_container_type =
    ( has_metadata_container_type_data_member<_Type>  ||
      has_metadata_container_type_method<_Type>       ||
      has_metadata_container_type_nested_type<_Type> );


// =========================================================================
// IV.  TYPE-EXTRACTION RE-EXPORTS
// =========================================================================
//   The `_t` aliases live in `metadata_traits.hpp` and are
// re-exported here so callers using the `concepts` namespace
// don't need to switch namespaces for type extraction.

// metadata_t
//   alias: re-export of `djinterp::metadata_t`.
using djinterp::metadata_t;

// metadata_type_t
//   alias: re-export of `djinterp::metadata_type_t`.
using djinterp::metadata_type_t;

// metadata_container_type_t
//   alias: re-export of `djinterp::metadata_container_type_t`.
using djinterp::metadata_container_type_t;


}  // namespace concepts

NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_UTILITY_METADATA_CONCEPTS_
