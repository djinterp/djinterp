/*******************************************************************************
* djinterp [config]                                           cfg_event_common.h
*
*   Root configuration for the event subframework. Owns the aggregate and the
* presets, the foundation knobs every event module reads, and the derived
* symbols that select the iteration tier, the erasure policy, the assertion
* level, and the C++ notation tier.
*
*   Every other cfg_event_*.h includes this file first, so a preset resolved
* here is in force before any child's defaults run. That ordering is what lets
* the event subframework carry presets at all under demand-loading: a
* cross-cutting rule normally has to live in dconfig.h, but a rule confined to
* one subframework can live at that subframework's config root, because the
* root is on every child's include path by construction.
*
*   A note on parity (framework_goals.md section 2): every knob here is read by
* BOTH languages, because both compile this same header. A knob therefore
* cannot make C and C++ disagree -- it can only move both together. Config
* that lived in one face would be a parity hazard; config that lives here is
* not.
*
*   targets:  core/event/event.h, event_common.h, event_common.hpp,
*             event_c.h  ->  D_INTERNAL_EVENT_ITERATION_DMACRO,
*             D_INTERNAL_EVENT_ARITY_MAX, D_INTERNAL_EVENT_ASSERT_LAYOUT,
*             D_INTERNAL_EVENT_ASSERT_SIZES, D_INTERNAL_EVENT_USE_CONCEPTS,
*             D_INTERNAL_EVENT_KEY_FROM_NAME, D_INTERNAL_EVENT_VALIDATE
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*
* path:      /inc/djinterp/config/core/event/cfg_event_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_COMMON_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_COMMON_H 1

// djinterp
// (0) root first: helpers, user overrides, testing flag and preset.
#include "../../cfg_common.h"

//   PREREQUISITE. Two knobs below resolve by DETECTION -- the iteration tier
// probes dmacro's generated tables, the notation tier probes the environment's
// concepts feature. Whoever includes this file must already have loaded
// djinterp.h, dmacro.h, and (in C++) djinterp.hpp, or both detections report
// "absent" and the build silently takes a fallback tier it did not need.
// core/event/event.h and every core/event module header load them first; if
// you include this config directly, do the same.


// ---------------------------------------------------------------------------
//  contents
// ---------------------------------------------------------------------------
//    1.  aggregate and presets
//    2.  erasure policy (the kappa question)
//    3.  iteration tier and arity ceiling
//    4.  assertion level
//    5.  argument and payload validation
//    6.  C++ notation tier
//    7.  validation
//    8.  derived values
// ---------------------------------------------------------------------------


// ===========================================================================
//  1.  AGGREGATE AND PRESETS
// ===========================================================================

// D_CFG_EVENT_ALL
//   brief: aggregate fallback for every optional event feature. Not defined by
// default -- when the user defines it, each child knob whose own value is
// unset falls back to it. Set it to 0 for the smallest build that still
// dispatches, or 1 to opt every optional feature in. Individual knobs always
// outrank it.

// D_CFG_EVENT_PRESET_MINIMAL
//   brief: 1 selects the freestanding profile -- no allocation, no deferral,
// no staging, no statistics, no merge. Intended for a build with no allocator
// and a fixed handler budget. Implemented as guarded #defines below rather
// than as a forced block, so an explicit knob still wins over the preset.
#ifndef D_CFG_EVENT_PRESET_MINIMAL
#   define D_CFG_EVENT_PRESET_MINIMAL 0
#endif

// D_CFG_EVENT_PRESET_FULL
//   brief: 1 opts every optional event feature in, including the ones that
// default off. Useful as a conformance-build switch: the widest surface is
// also the widest test target.
#ifndef D_CFG_EVENT_PRESET_FULL
#   define D_CFG_EVENT_PRESET_FULL 0
#endif

#if D_CFG_IS_ON(D_CFG_EVENT_PRESET_MINIMAL)
#   ifndef D_CFG_EVENT_TABLE_ALLOC
#       define D_CFG_EVENT_TABLE_ALLOC 0
#   endif
#   ifndef D_CFG_EVENT_TABLE_STATS
#       define D_CFG_EVENT_TABLE_STATS 0
#   endif
#   ifndef D_CFG_EVENT_TABLE_MERGE
#       define D_CFG_EVENT_TABLE_MERGE 0
#   endif
#   ifndef D_CFG_EVENT_REGISTRY_STAGING
#       define D_CFG_EVENT_REGISTRY_STAGING 0
#   endif
#   ifndef D_CFG_EVENT_REGISTRY_RUN
#       define D_CFG_EVENT_REGISTRY_RUN 0
#   endif
#   ifndef D_CFG_EVENT_QUEUE
#       define D_CFG_EVENT_QUEUE 0
#   endif
#   ifndef D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER
#       define D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER 0
#   endif
#endif  // D_CFG_EVENT_PRESET_MINIMAL

#if D_CFG_IS_ON(D_CFG_EVENT_PRESET_FULL)
#   ifndef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_ALL 1
#   endif
#   ifndef D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS
#       define D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS 1
#   endif
#   ifndef D_CFG_EVENT_VALIDATE_PAYLOAD
#       define D_CFG_EVENT_VALIDATE_PAYLOAD 1
#   endif
#endif  // D_CFG_EVENT_PRESET_FULL


// ===========================================================================
//  2.  ERASURE POLICY (the kappa question)
// ===========================================================================
//   framework_goals.md section 13 lists "serialization identity stability" as
// an OPEN decision: whether interned handles are session-local (bytes on the
// wire, re-interned on load) or content-derived (stable but collision-
// bearing). The event subframework cannot proceed without an answer, but it
// is not the event subframework's answer to give. So the answer is a knob,
// with the content-derived form as the shipped default and the alternative
// reachable without editing a header.

// D_CFG_EVENT_KEY_NAME_HASH / D_CFG_EVENT_KEY_EXPLICIT
//   brief: the two admissible values of D_CFG_EVENT_KEY_POLICY.
#define D_CFG_EVENT_KEY_NAME_HASH   0
#define D_CFG_EVENT_KEY_EXPLICIT    1

// D_CFG_EVENT_KEY_POLICY
//   brief: how D_EVENT_DECLARE derives kappa(e). NAME_HASH (0, the default)
// takes FNV-1a-64 over the declared name: identical in C and C++, across
// translation units, runs, and byte orders, at the cost of admitting
// collisions. EXPLICIT (1) makes D_EVENT_DECLARE_KEYED the house form and the
// declared name advisory, for a project that maintains its own id registry.
//   Neither value removes a macro: both declaration forms compile under both
// policies, so this selects a convention rather than a capability.
#ifndef D_CFG_EVENT_KEY_POLICY
#   define D_CFG_EVENT_KEY_POLICY   D_CFG_EVENT_KEY_NAME_HASH
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_EVENT_KEY_POLICY)
    #error "D_CFG_EVENT_KEY_POLICY must name one of its values; a misspelled name would read as 0"
#endif


// ===========================================================================
//  3.  ITERATION TIER AND ARITY CEILING
// ===========================================================================

// D_CFG_EVENT_ITERATION_DMACRO
//   brief: 1 expands event payload blocks with dmacro.h's indexed iteration
// (arity up to D_VARG_COUNT_MAX), 0 with the event subframework's own
// positional expander (arity up to 12). Left unset it is DETECTED, which is
// the intent -- set it only to force a tier for a bisect or a bug report.
//
//   The detection probes the GENERATED TABLE, not the macro that consumes it.
// dmacro.h always defines D_FOR_EACH_INDEXED and D_INC, but D_INC expands to
// D_CONCAT(D_INTERNAL_INC_, x) and that table arrives from a variant header
// selected by D_DMACRO_VARIANT and gated by D_CFG_DMACRO_INCLUDE_*. So the
// macro can be defined and still not expand. Presence of the macro is a
// version fact; presence of its table is the feature (goals section 5,
// "detect features, not versions").
//
//   Both tiers generate the same members in the same order, so the payload
// block has one layout either way. Only the ceiling moves -- which is the
// tier law working as intended.
#ifndef D_CFG_EVENT_ITERATION_DMACRO
#   if defined(D_VARG_COUNT) && defined(D_INTERNAL_INC_0)
#       define D_CFG_EVENT_ITERATION_DMACRO 1
#   else
#       define D_CFG_EVENT_ITERATION_DMACRO 0
#   endif
#endif

// D_CFG_EVENT_ARITY_MAX_FALLBACK
//   brief: the arity ceiling of the built-in expander. Raising it requires
// writing the corresponding D_INTERNAL_EVENT_FIELDS_n macros, so it is a knob
// for reading, not for setting; the dmacro tier is the way to go higher.
#ifndef D_CFG_EVENT_ARITY_MAX_FALLBACK
#   define D_CFG_EVENT_ARITY_MAX_FALLBACK 12
#endif


// ===========================================================================
//  4.  ASSERTION LEVEL
// ===========================================================================

// D_CFG_EVENT_ASSERT_LAYOUT
//   brief: 1 emits the sizeof/offsetof assertions that make layout drift a
// compile error rather than a wire-format bug. Defaults ON because goals
// section 4 requires layout be asserted rather than assumed; 0 exists for a
// tier whose D_STATIC_ASSERT fallback is itself unavailable, not as a
// convenience.
#ifndef D_CFG_EVENT_ASSERT_LAYOUT
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_ASSERT_LAYOUT D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_ASSERT_LAYOUT 1
#   endif
#endif

// D_CFG_EVENT_ASSERT_SIZES_OFF_LP64
//   brief: 1 asserts the exact struct sizes even where the data model is not
// LP64. Defaults OFF: on a model with a different pointer or size_t width the
// sizes legitimately differ, and a hard failure there would breach
// "degrade, never error". Offsets are asserted unconditionally, since they are
// model independent.
#ifndef D_CFG_EVENT_ASSERT_SIZES_OFF_LP64
#   define D_CFG_EVENT_ASSERT_SIZES_OFF_LP64 0
#endif


// ===========================================================================
//  5.  ARGUMENT AND PAYLOAD VALIDATION
// ===========================================================================

// D_CFG_EVENT_STRICT_ARGS
//   brief: 0 (default) makes the core tolerate a null argument and report it
// through the return value -- D_EVENT_ERR_NULL, or the empty dispatch result.
// 1 makes it a hard assertion instead, for a build that would rather find the
// caller than absorb the mistake.
#ifndef D_CFG_EVENT_STRICT_ARGS
#   define D_CFG_EVENT_STRICT_ARGS 0
#endif

// D_CFG_EVENT_VALIDATE_PAYLOAD
//   brief: 1 checks a payload view's size and arity against the declaration
// before the payload is enqueued or dispatched. This is the run-time tier of
// an invariant the C++ face checks with static_assert -- goals section 2,
// "same invariant, three enforcement tiers". Follows D_CFG_TESTING, so a test
// build gets the check and a release build does not pay for it.
#ifndef D_CFG_EVENT_VALIDATE_PAYLOAD
#   if D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_EVENT_VALIDATE_PAYLOAD 1
#   else
#       define D_CFG_EVENT_VALIDATE_PAYLOAD 0
#   endif
#endif


// ===========================================================================
//  6.  C++ NOTATION TIER
// ===========================================================================

// D_CFG_EVENT_USE_CONCEPTS
//   brief: 1 compiles the event subframework's concept constraints. Left unset
// it follows env detection of C++20 concepts. Setting it to 0 on a tier that
// has them keeps the traits and drops the concepts, which is a legitimate
// choice for a codebase standardising on the trait spelling -- the concepts
// are notation over the traits, and dropping notation changes nothing about
// what is computed (goals section 2).
#ifndef D_CFG_EVENT_USE_CONCEPTS
#   if defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                            \
       (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1)
#       define D_CFG_EVENT_USE_CONCEPTS 1
#   else
#       define D_CFG_EVENT_USE_CONCEPTS 0
#   endif
#endif

// D_CFG_EVENT_TRAIT_DETECTORS
//   brief: 1 compiles the structural detection traits (event_table_traits,
// event_dispatcher_traits) that let a caller substitute its own store or
// facade. Defaults ON; 0 trims a large amount of template instantiation from
// a build that never substitutes either.
#ifndef D_CFG_EVENT_TRAIT_DETECTORS
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_TRAIT_DETECTORS D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_TRAIT_DETECTORS 1
#   endif
#endif


// ===========================================================================
//  7.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_EVENT_PRESET_MINIMAL) &&                               \
    !D_CFG_IS_OFF(D_CFG_EVENT_PRESET_MINIMAL)
#   error "D_CFG_EVENT_PRESET_MINIMAL must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_PRESET_FULL) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_PRESET_FULL)
#   error "D_CFG_EVENT_PRESET_FULL must be 0 or 1"
#endif
#if D_CFG_IS_ON(D_CFG_EVENT_PRESET_MINIMAL) &&                                \
    D_CFG_IS_ON(D_CFG_EVENT_PRESET_FULL)
#   error "D_CFG_EVENT_PRESET_MINIMAL and _FULL are mutually exclusive"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_ITERATION_DMACRO) &&                             \
    !D_CFG_IS_OFF(D_CFG_EVENT_ITERATION_DMACRO)
#   error "D_CFG_EVENT_ITERATION_DMACRO must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_ASSERT_LAYOUT) &&                                \
    !D_CFG_IS_OFF(D_CFG_EVENT_ASSERT_LAYOUT)
#   error "D_CFG_EVENT_ASSERT_LAYOUT must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_ASSERT_SIZES_OFF_LP64) &&                        \
    !D_CFG_IS_OFF(D_CFG_EVENT_ASSERT_SIZES_OFF_LP64)
#   error "D_CFG_EVENT_ASSERT_SIZES_OFF_LP64 must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_STRICT_ARGS) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_STRICT_ARGS)
#   error "D_CFG_EVENT_STRICT_ARGS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_VALIDATE_PAYLOAD) &&                             \
    !D_CFG_IS_OFF(D_CFG_EVENT_VALIDATE_PAYLOAD)
#   error "D_CFG_EVENT_VALIDATE_PAYLOAD must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_USE_CONCEPTS) &&                                 \
    !D_CFG_IS_OFF(D_CFG_EVENT_USE_CONCEPTS)
#   error "D_CFG_EVENT_USE_CONCEPTS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_TRAIT_DETECTORS) &&                              \
    !D_CFG_IS_OFF(D_CFG_EVENT_TRAIT_DETECTORS)
#   error "D_CFG_EVENT_TRAIT_DETECTORS must be 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_KEY_POLICY) != D_CFG_EVENT_KEY_NAME_HASH) &&      \
    (D_CFG_NORM(D_CFG_EVENT_KEY_POLICY) != D_CFG_EVENT_KEY_EXPLICIT)
#   error "D_CFG_EVENT_KEY_POLICY must be _NAME_HASH or _EXPLICIT"
#endif


// ===========================================================================
//  8.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_ITERATION_DMACRO
//   brief: 1 when event_common.h should expand payload blocks through
// dmacro.h's indexed iteration. Read by event_common.h.
#define D_INTERNAL_EVENT_ITERATION_DMACRO                                     \
    D_CFG_NORM(D_CFG_EVENT_ITERATION_DMACRO)

// D_INTERNAL_EVENT_ARITY_MAX
//   brief: the greatest declarable event arity on this tier. Read by
// event_common.h and reported to users as D_EVENT_ARITY_MAX.
#if D_CFG_IS_ON(D_CFG_EVENT_ITERATION_DMACRO)
#   define D_INTERNAL_EVENT_ARITY_MAX D_VARG_COUNT_MAX
#else
#   define D_INTERNAL_EVENT_ARITY_MAX D_CFG_EVENT_ARITY_MAX_FALLBACK
#endif

// D_INTERNAL_EVENT_ASSERT_LAYOUT
//   brief: 1 when the shared headers should emit their layout assertions.
#define D_INTERNAL_EVENT_ASSERT_LAYOUT                                        \
    D_CFG_NORM(D_CFG_EVENT_ASSERT_LAYOUT)

// D_INTERNAL_EVENT_ASSERT_SIZES
//   brief: 1 when exact struct sizes should be asserted -- on LP64, or
// anywhere if the user has forced it. Offsets are asserted regardless.
#if D_CFG_IS_ON(D_CFG_EVENT_ASSERT_LAYOUT)
#   if D_CFG_IS_ON(D_CFG_EVENT_ASSERT_SIZES_OFF_LP64)
#       define D_INTERNAL_EVENT_ASSERT_SIZES 1
#   elif defined(UINTPTR_MAX) && (UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu)
#       define D_INTERNAL_EVENT_ASSERT_SIZES 1
#   else
#       define D_INTERNAL_EVENT_ASSERT_SIZES 0
#   endif
#else
#   define D_INTERNAL_EVENT_ASSERT_SIZES 0
#endif

// D_INTERNAL_EVENT_KEY_FROM_NAME
//   brief: 1 when D_EVENT_DECLARE should derive kappa from the declared name.
// Read by event_common.h; both declaration macros exist either way.
#if (D_CFG_NORM(D_CFG_EVENT_KEY_POLICY) == D_CFG_EVENT_KEY_NAME_HASH)
#   define D_INTERNAL_EVENT_KEY_FROM_NAME 1
#else
#   define D_INTERNAL_EVENT_KEY_FROM_NAME 0
#endif

// D_INTERNAL_EVENT_VALIDATE
//   brief: 1 when the core should check payload views against declarations at
// run time. Read by the queue and by dispatch.
#define D_INTERNAL_EVENT_VALIDATE                                             \
    D_CFG_NORM(D_CFG_EVENT_VALIDATE_PAYLOAD)

// D_INTERNAL_EVENT_STRICT_ARGS
//   brief: 1 when a null argument should assert rather than be reported.
#define D_INTERNAL_EVENT_STRICT_ARGS                                          \
    D_CFG_NORM(D_CFG_EVENT_STRICT_ARGS)

// D_INTERNAL_EVENT_USE_CONCEPTS
//   brief: 1 when the C++ faces should compile their concept constraints.
#define D_INTERNAL_EVENT_USE_CONCEPTS                                         \
    D_CFG_NORM(D_CFG_EVENT_USE_CONCEPTS)

// D_INTERNAL_EVENT_TRAIT_DETECTORS
//   brief: 1 when the C++ faces should compile their structural detectors.
#define D_INTERNAL_EVENT_TRAIT_DETECTORS                                      \
    D_CFG_NORM(D_CFG_EVENT_TRAIT_DETECTORS)


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_COMMON_H
