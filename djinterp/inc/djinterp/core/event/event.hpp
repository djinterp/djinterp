/******************************************************************************
* djinterp [event]                                                   event.hpp
*
* C++ event system — umbrella include:
*   Includes the complete event system module hierarchy. Including this
* single header provides access to all event system components.
*
* MODULE HIERARCHY:
*   event_traits.hpp           - event tag detection, event_traits,
*                                event_context, D_EVENT macros,
*                                is_event concept
*   event_listener_traits.hpp  - listener_id, event_listener_traits,
*                                is_event_listener / is_nothrow_event_listener
*                                concepts
*   event_table.hpp            - listener_entry, type_id_value,
*                                event_table_stats, event_table
*   event_table_traits.hpp     - event_table_traits structural detection,
*                                is_event_table_type concept
*   event_listener.hpp         - listener_registry
*   event_handler.hpp          - dispatch_result, event_queue, event_handler
*   event_handler_traits.hpp   - event_handler_traits structural detection,
*                                event_handler_has_bind / _fire / _queue,
*                                is_event_handler_type concept
*
* INCLUDE CHAIN:
*   event_traits.hpp
*       ↓
*   event_listener_traits.hpp
*       ↓
*   event_table.hpp
*       ↓                ↘
*   event_listener.hpp    event_table_traits.hpp
*       ↓
*   event_handler.hpp
*       ↓                ↘
*   event.hpp (this)      event_handler_traits.hpp
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
* 
* 
* path:      /inc/djinterp/core/event/event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_
#define DJINTERP_EVENT_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event.hpp requires C++11 or higher"
#endif

// core implementation chain
// (event_handler.hpp transitively includes event_listener.hpp,
//  event_table.hpp, event_listener_traits.hpp, event_traits.hpp)
#include "event_handler.hpp"

// structural detection traits (independent of implementations;
// depend only on event_listener_traits.hpp for listener_id)
#include "event_table_traits.hpp"
#include "event_handler_traits.hpp"


#endif  // DJINTERP_EVENT_
