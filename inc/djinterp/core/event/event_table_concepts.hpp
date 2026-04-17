/****************************************************************************
* djinterp [event]                                  event_table_concepts.hpp
*
*  djinterp event table classification concepts
*   C++20 concepts layered on top of event_table_traits.hpp.  These
* concepts provide readable `requires` constraints for event table types,
* including core table identity, count/query capability, and optional
* extended table features such as stats, selective clearing, and type-key
* accounting.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable, or concept from the event table trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Table Concepts
* 3.   Count and Query Concepts
* 4.   Extended Feature Concepts
*
* 
* path:      /inc/djinterp/core/event/event_table_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                        date: 2026.04.07
****************************************************************************/

#ifndef DJINTERP_EVENT_TABLE_CONCEPTS_
#define DJINTERP_EVENT_TABLE_CONCEPTS_ 1

#include "event_handler_concepts.hpp"
#include "event_table_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "event_table_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// =========================================================================
// I.   CORE TABLE CONCEPTS
// =========================================================================

// event_table_type
//   concept: constrains types satisfying the structural event table
// protocol.
template<typename _Type>
concept event_table_type =
    is_event_table_type<clean_t<_Type>>;

// non_event_table_type
//   concept: constrains types that do not satisfy the structural event
// table protocol.
template<typename _Type>
concept non_event_table_type =
    !event_table_type<_Type>;

// clearable_event_table_type
//   concept: constrains event tables supporting clear().
template<typename _Type>
concept clearable_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_clear;


// =========================================================================
// II.  COUNT AND QUERY CONCEPTS
// =========================================================================

// counting_event_table_type
//   concept: constrains event tables supporting the full required count
// and query interface.
template<typename _Type>
concept counting_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_count_for &&
    event_table_traits<clean_t<_Type>>::has_has_entries_for &&
    event_table_traits<clean_t<_Type>>::has_total_count &&
    event_table_traits<clean_t<_Type>>::has_enabled_count;

// type_key_counting_event_table_type
//   concept: constrains event tables exposing type_key_count().
template<typename _Type>
concept type_key_counting_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_type_key_count;


// =========================================================================
// III. EXTENDED FEATURE CONCEPTS
// =========================================================================

// stats_event_table_type
//   concept: constrains event tables exposing get_stats().
template<typename _Type>
concept stats_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_stats;

// selectively_clearable_event_table_type
//   concept: constrains event tables supporting clear_for(type-key).
template<typename _Type>
concept selectively_clearable_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_clear_for;

// extended_event_table_type
//   concept: constrains event tables exposing all optional extension
// points currently tracked by event_table_traits.
template<typename _Type>
concept extended_event_table_type =
    event_table_type<_Type> &&
    event_table_traits<clean_t<_Type>>::has_type_key_count &&
    event_table_traits<clean_t<_Type>>::has_stats &&
    event_table_traits<clean_t<_Type>>::has_clear_for;


NS_END  // djinterp


#endif  // DJINTERP_EVENT_TABLE_CONCEPTS_
