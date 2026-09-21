/******************************************************************************
* djinterp [parsegen]                                            cfg_parsegen.h
*
* Configuration for the parser-generator subsystem.
*   Owns the knobs parsegen's own containers read. They are separate from
* parse's because a build may want a growable grammar over a fixed-storage
* parse substrate, or the reverse.
*
*   Note the dependency the derived values encode: a grammar and a program both
* carry an operand pool, so neither can be heap-backed unless the pool is. That
* conjunction is resolved here, once, rather than re-derived in each module --
* which is what the localization rule is for.
*
* targets:  parsegen/grammar.h  -> D_INTERNAL_PARSEGEN_GRAMMAR_HEAP /
*                                  D_PARSEGEN_GRAMMAR_DEFAULT_NODES /
*                                  D_PARSEGEN_GRAMMAR_DEFAULT_RULES
*           parsegen/registry.h -> D_INTERNAL_PARSEGEN_REGISTRY_HEAP
* requires: cfg_common.h; parse/cfg_parse.h (the pool and heap gates these
*           derive from)
*
* path:      /inc/djinterp/config/parsegen/cfg_parsegen.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_CFG_PARSEGEN_
#define DJINTERP_CFG_PARSEGEN_ 1

// (0) root first: helpers, user overrides, testing flag/preset.
#include "../cfg_common.h"
// (0b) the dependency whose gates these derive from.
#include "../parse/cfg_parse.h"


//==============================================================================
// 1.  FEATURE GATES
//==============================================================================


// 1.1    Aggregate
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_PARSEGEN_ALL
//   brief: optional aggregate for this subsystem's boolean gates. NOT
// defaulted -- its absence is meaningful.

// 1.2    Allocation
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_PARSEGEN_GRAMMAR_HEAP
//   brief: 1 compiles the heap-backed grammar, whose node arena, rule table,
// and operand pool grow as a frontend reads; 0 leaves only caller-supplied
// fixed storage. Default 1.
#ifndef D_CFG_PARSEGEN_GRAMMAR_HEAP
#   if defined(D_CFG_PARSEGEN_ALL)
#       define D_CFG_PARSEGEN_GRAMMAR_HEAP      D_CFG_PARSEGEN_ALL
#   else
#       define D_CFG_PARSEGEN_GRAMMAR_HEAP      1
#   endif
#endif

// 1.2.2
// D_CFG_PARSEGEN_REGISTRY_HEAP
//   brief: 1 compiles the heap-backed stage registry; 0 leaves only
// caller-supplied fixed storage, which is the usual shape anyway since a build
// normally links in exactly the stages it wants. Default 1.
#ifndef D_CFG_PARSEGEN_REGISTRY_HEAP
#   if defined(D_CFG_PARSEGEN_ALL)
#       define D_CFG_PARSEGEN_REGISTRY_HEAP     D_CFG_PARSEGEN_ALL
#   else
#       define D_CFG_PARSEGEN_REGISTRY_HEAP     1
#   endif
#endif

// 1.3    Validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_PARSEGEN_GRAMMAR_HEAP)
#   error "D_CFG_PARSEGEN_GRAMMAR_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSEGEN_REGISTRY_HEAP)
#   error "D_CFG_PARSEGEN_REGISTRY_HEAP must be 0 or 1"
#endif


//==============================================================================
// 2.  SIZING CONSTANTS
//==============================================================================


// 2.1    Grammar
//------------------------------------------------------------------------------
// 2.1.1
// D_PARSEGEN_GRAMMAR_DEFAULT_NODES
//   brief: expression nodes a heap-backed grammar reserves when the caller
// does not say. Grows on demand; this only sets where it starts.
#ifndef D_PARSEGEN_GRAMMAR_DEFAULT_NODES
#   define D_PARSEGEN_GRAMMAR_DEFAULT_NODES     64
#endif

// 2.1.2
// D_PARSEGEN_GRAMMAR_DEFAULT_RULES
//   brief: rules a heap-backed grammar reserves room for by default.
#ifndef D_PARSEGEN_GRAMMAR_DEFAULT_RULES
#   define D_PARSEGEN_GRAMMAR_DEFAULT_RULES     16
#endif


//==============================================================================
// 3.  DERIVED VALUES
//==============================================================================


// 3.1    Effective gates
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_PARSEGEN_GRAMMAR_HEAP
//   brief: 1 when a grammar may allocate. A grammar carries an operand pool,
// so this requires the pool's own gate as well as its own -- the conjunction
// that would otherwise be re-derived, wrongly, in grammar.c.
#if ( D_CFG_IS_ON(D_CFG_PARSEGEN_GRAMMAR_HEAP) &&                             \
      (D_INTERNAL_PARSE_POOL_HEAP == 1) )
#   define D_INTERNAL_PARSEGEN_GRAMMAR_HEAP     1
#else
#   define D_INTERNAL_PARSEGEN_GRAMMAR_HEAP     0
#endif

// 3.1.2
// D_INTERNAL_PARSEGEN_REGISTRY_HEAP
//   brief: 1 when the stage registry may allocate. It carries no pool, so it
// needs only the general heap gate.
#if ( D_CFG_IS_ON(D_CFG_PARSEGEN_REGISTRY_HEAP) &&                            \
      (D_INTERNAL_PARSE_HEAP == 1) )
#   define D_INTERNAL_PARSEGEN_REGISTRY_HEAP    1
#else
#   define D_INTERNAL_PARSEGEN_REGISTRY_HEAP    0
#endif


#endif  // DJINTERP_CFG_PARSEGEN_
