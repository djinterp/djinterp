/******************************************************************************
* djinterp [container]                         container_database_concepts.hpp
*
* Database persistence concepts:
*   C++20 concepts layered over container_database_traits.hpp. These
* concepts provide readable constraints for database-persistable containers
* without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_database_traits.hpp:
*   - container-level serialize / deserialize
*   - element-level row / primitive mapping
*   - schema protocol and field typing
*   - serialize / deserialize strategy wrappers
*   - shorthand concepts over container_database_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_database_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_DATABASE_CONCEPTS_
#define DJINTERP_CONTAINER_DATABASE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_database_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_database_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Container-level concepts
// ===========================================================================

template<typename _Type>
concept db_serializable_container_surface =
    has_serialize_method_v<_Type>;

template<typename _Type>
concept db_deserializable_container_surface =
    has_deserialize_method_v<_Type>;


// ===========================================================================
// II.  Element-level concepts
// ===========================================================================

template<typename _Type>
concept row_mappable_db_elements_container =
    has_row_mappable_elements_v<_Type>;

template<typename _Type>
concept db_row_elements_container =
    has_db_row_elements_v<_Type>;

template<typename _Type>
concept db_primitive_elements_container =
    has_db_primitive_elements_v<_Type>;

template<typename _Type>
concept db_capable_elements_container =
    is_element_db_capable_v<_Type>;


// ===========================================================================
// III. Schema / field-type concepts
// ===========================================================================

template<typename _Type>
concept schema_aware_db_container =
    has_schema_v<_Type>;

template<typename _Type>
concept create_sql_db_container =
    has_create_table_sql_v<_Type>;

template<typename _Type>
concept table_named_db_container =
    has_table_name_v<_Type>;

template<typename _Type>
concept schema_named_db_container =
    has_schema_name_v<_Type>;


// ===========================================================================
// IV.  Strategy-oriented concepts
// ===========================================================================

template<typename _Type>
concept native_db_serialize_container =
    ( container_serialize_strategy_v<_Type> ==
      database_serialize_strategy::native );

template<typename _Type>
concept row_db_serialize_container =
    ( container_serialize_strategy_v<_Type> ==
      database_serialize_strategy::element_row );

template<typename _Type>
concept primitive_db_serialize_container =
    ( container_serialize_strategy_v<_Type> ==
      database_serialize_strategy::element_primitive );

template<typename _Type>
concept native_db_deserialize_container =
    ( container_deserialize_strategy_v<_Type> ==
      database_deserialize_strategy::native );

template<typename _Type>
concept row_db_deserialize_container =
    ( container_deserialize_strategy_v<_Type> ==
      database_deserialize_strategy::element_row );

template<typename _Type>
concept primitive_db_deserialize_container =
    ( container_deserialize_strategy_v<_Type> ==
      database_deserialize_strategy::element_primitive );


// ===========================================================================
// V.   Aggregate identity concepts
// ===========================================================================

template<typename _Type>
concept db_serializable_container =
    is_db_serializable_v<_Type>;

template<typename _Type>
concept db_deserializable_container =
    is_db_deserializable_v<_Type>;

template<typename _Type>
concept db_round_trip_container =
    is_db_round_trip_v<_Type>;

template<typename _Type>
concept classified_db_container =
    ( container_database_class<_Type>::is_serializable ||
      container_database_class<_Type>::is_deserializable );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_DATABASE_CONCEPTS_