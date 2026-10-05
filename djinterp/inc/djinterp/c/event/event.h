/*******************************************************************************
* djinterp [c]                                                           event.h
*
* The event subframework umbrella:
*   One include for the whole module. Follows the shared-core convention set
* by c/meta/type_info.h -- the umbrella pulls in the language-neutral
* `_common` headers and then dispatches to the face for the language actually
* compiling it. Nothing in this file declares a type or a function.
*
* LAYERING:
*
*   event.h                        this umbrella
*     |
*     +-- event_common.h           tier 0: alphabet, key, verdict, payload
*     +-- event_handler_common.h   tier 0: the step and the seq/skip monoid
*     +-- event_table_common.h     tier 0: the erased store, mask, merge
*     +-- event_registry_common.h  tier 0: dispatch, run, the fused word
*     +-- event_dispatcher_common.h tier 0: the queue and the facade
*     |
*     +-- event_c.h                tier 1a: C ergonomics (C builds)
*     `-- event.hpp                tier 1b: the C++ face (C++ builds)
*
*   Tier 0 is ONE declaration of every layout, compiled by both languages and
* asserted in both. Tier 1a adds macros; tier 1b adds templates, traits, and
* concepts. Neither face declares a layout and neither reimplements an
* algorithm: a face that did would be a second implementation of one formal
* object, which is the failure this arrangement exists to prevent.
*
* WHAT MOVED, AND WHY IT MATTERS:
*   The pre-core C and C++ event modules implemented DIFFERENT formal objects,
* not one object in two notations. C bound at most one handler per event id
* and its callback returned nothing; C++ bound an ordered word of handlers,
* each returning a verdict, with a mask, a merge, and a staging operation. No
* amount of wrapping reconciles those, so the core is the C++ object -- the
* one the note defines -- lowered into C, and the C side gains the word, the
* verdict, the merge, and the fused path it did not have.
*
*   event_c.h section VI carries the symbol-by-symbol migration map.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_H
#define DJINTERP_C_EVENT_EVENT_H 1

// djinterp
// ---- tier -2: the environment the configuration detects on ----
//   These come FIRST, ahead of config, and the order is load-bearing.
// cfg_event_common.h resolves two knobs by DETECTION -- the iteration tier
// (probing dmacro's generated tables) and the C++ notation tier (probing the
// env's concepts feature) -- and a detection that runs before the thing it
// detects on has been loaded always reports "absent". That is the quiet kind
// of wrong: the build still compiles, it just silently takes the fallback
// tier. Every individual module header already loads its prerequisites ahead
// of its own config; the umbrella has to do the same.
#include "../djinterp.h"
#include "../dmacro.h"

#ifdef __cplusplus
    #include "../../djinterp.hpp"
#endif  // __cplusplus

// ---- tier -1: configuration, resolved before anything reads it ----
//   The umbrella is the ONE place the umbrella config belongs. A layer that
// needs one module's knobs includes that module's config; pulling this from
// event_table_common.h would load the queue's defaults for a caller who never
// included the queue, which is demand-loading running backwards.
#include "../../config/core/event/cfg_event.h"

// ---- tier 0: the shared core, in dependency order ----
#include "./event_common.h"
#include "./event_handler_common.h"
#include "./event_table_common.h"
#include "./event_registry_common.h"
#include "./event_dispatcher_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

// ---- tier 1: the face for the language compiling this header ----
// From C++11, decision 3.6's floor for event; below it a C++ caller has the
// C core.
#if ( (D_ENV_LANG_USING_CPP) &&                                           \
      (D_ENV_LANG_IS_CPP11_OR_HIGHER) )
    #include "../../core/event/event.hpp"
#endif  // C++11 and up


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_H
