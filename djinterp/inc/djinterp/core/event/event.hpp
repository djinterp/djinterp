/******************************************************************************
* djinterp [event]                                                  events.hpp
*
* The event submodule umbrella:
*   Single include for the entire event system. The submodule was refactored
* to coincide with the formal "Definition of an Event": each module now ships
* as one header carrying its own traits and concepts (the former *_traits.hpp
* and *_concepts.hpp companions were absorbed), so this umbrella reduces to a
* single include at the top of the dependency chain.
*
* INCLUDE CHAIN (each header includes the one below it):
*
*   events.hpp
*     `- event_dispatcher.hpp   facade: registry + queue + fused bridge
*                               (<- event_handler[_traits|_concepts].hpp)
*          `- registry.hpp      typed bind / dispatch / run, merge, compile
*                               (<- event_listener.hpp)
*               `- event_table.hpp   erased storage: kappa, mask, merge
*                               (<- event_table[_traits|_concepts].hpp)
*                    `- handler.hpp  the step: verdict, traits, seq/skip
*                               (<- event_listener_traits|_concepts.hpp)
*                         `- event.hpp   foundations: the alphabet
*                               (<- event[_traits|_concepts].hpp)
*
* WHAT EACH LAYER PROVIDES:
*   event.hpp            verdict; event_traits; D_EVENT macros; event concepts
*   handler.hpp          handler_id; handler_traits; the seq/skip monoid;
*                        handler concepts
*   event_table.hpp      event_table (type-erased store); event_table_traits;
*                        event_table concepts
*   registry.hpp         event_registry; dispatch_result; run_result;
*                        fused_step
*   event_dispatcher.hpp event_dispatcher; event_queue; drive;
*                        event_dispatcher_traits; dispatcher concepts
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   alphabet / occurrence / payload       -- event.hpp
*   verdict P = {pass, consume}           -- event.hpp (verdict)
*   handler h : S x A_e -> S x P          -- handler.hpp
*   sequencing monoid (seq, skip)         -- handler.hpp
*   erasure kappa; mask m; merge (+)      -- event_table.hpp / registry.hpp
*   dispatch delta_rho; run; staging      -- registry.hpp
*   queue q; process; deferral coherence  -- event_dispatcher.hpp
*   fused vs erased coherence law         -- event_dispatcher.hpp (drive)
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/events.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_ALL_
#define DJINTERP_EVENT_ALL_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "events.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "events.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "events.hpp requires C++11 or higher"
#endif

// the whole submodule, pulled via the top of the dependency chain
#include "../djinterp.hpp"
#include "./event_common.hpp"
#include "./event_listener.hpp"
#include "./event_registry.hpp"
#include "./event_table.hpp"
#include "./event_handler.hpp"
#include "./event_dispatcher.hpp"


#endif  // DJINTERP_EVENT_ALL_
