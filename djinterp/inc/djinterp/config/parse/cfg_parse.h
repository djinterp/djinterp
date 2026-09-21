/******************************************************************************
* djinterp [parse]                                                  cfg_parse.h
*
* Configuration for the parse subframework's execution substrate.
*   Owns every knob the substrate reads: whether the diagnostic sink and the
* operator registry may allocate, whether printf-style diagnostic formatting
* is compiled in, whether the machine carries a trace hook, and the three
* sizing constants. One file for the whole subframework rather than one per
* module, per the "group config per subframework" guidance.
*
* targets:  parse/diagnostic.h -> D_INTERNAL_PARSE_DIAG_HEAP /
*                                 D_INTERNAL_PARSE_DIAG_FORMAT /
*                                 D_PARSE_DIAG_DEFAULT_ITEMS /
*                                 D_PARSE_DIAG_DEFAULT_TEXT /
*                                 D_PARSE_DIAG_MESSAGE_MAX
*           parse/pool.h       -> D_INTERNAL_PARSE_POOL_HEAP /
*                                 D_PARSE_POOL_DEFAULT_BYTES /
*                                 D_PARSE_POOL_DEFAULT_ENTRIES
*           parse/program.h    -> D_INTERNAL_PARSE_PROGRAM_HEAP /
*                                 D_INTERNAL_PARSE_PROGRAM_TRANSPORT /
*                                 D_PARSE_PROGRAM_DEFAULT_CODE
*           parse/machine.h    -> D_INTERNAL_PARSE_MACHINE_TRACE /
*                                 D_INTERNAL_PARSE_OP_SET_HEAP /
*                                 D_PARSE_MACHINE_STEP_LIMIT
*           the parse sources  -> D_INTERNAL_PARSE_HEAP
* requires: cfg_common.h (helpers, D_CFG_TESTING)
*
* path:      /inc/djinterp/config/parse/cfg_parse.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_CFG_PARSE_
#define DJINTERP_CFG_PARSE_ 1

// (0) root first: helpers, user overrides, testing flag/preset.
#include "../cfg_common.h"


//==============================================================================
// 1.  FEATURE GATES
//==============================================================================
// Boolean knobs. Each falls back to the optional aggregate D_CFG_PARSE_ALL and
// then to its own hard default, so an individual override always wins.


// 1.1    Aggregate
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_PARSE_ALL
//   brief: optional aggregate for the parse subframework's boolean gates. NOT
// defaulted -- its absence is meaningful. Define it to 0 or 1 and every gate
// with no explicit value of its own falls back to it.

// 1.2    Allocation
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_PARSE_DIAG_HEAP
//   brief: 1 compiles the heap-backed diagnostic sink constructor and lets a
// full sink grow; 0 leaves only caller-supplied fixed storage, so the parse
// substrate performs no allocation at all. Default 1.
#ifndef D_CFG_PARSE_DIAG_HEAP
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_DIAG_HEAP        D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_DIAG_HEAP        1
#   endif
#endif

// 1.2.2
// D_CFG_PARSE_OP_SET_HEAP
//   brief: 1 compiles the heap-backed operator registry and lets a registry
// grow when an opcode lands past its table; 0 leaves only caller-supplied
// fixed storage. Default 1.
#ifndef D_CFG_PARSE_OP_SET_HEAP
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_OP_SET_HEAP      D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_OP_SET_HEAP      1
#   endif
#endif

// 1.2.3
// D_CFG_PARSE_POOL_HEAP
//   brief: 1 compiles the heap-backed operand pool and lets a full pool grow;
// 0 leaves only caller-supplied fixed storage. Default 1.
#ifndef D_CFG_PARSE_POOL_HEAP
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_POOL_HEAP        D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_POOL_HEAP        1
#   endif
#endif

// 1.2.4
// D_CFG_PARSE_PROGRAM_HEAP
//   brief: 1 compiles the heap-backed program and lets an instruction stream
// grow as a generator emits into it; 0 leaves only caller-supplied fixed
// storage, which suits a program known at build time. Default 1.
#ifndef D_CFG_PARSE_PROGRAM_HEAP
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_PROGRAM_HEAP     D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_PROGRAM_HEAP     1
#   endif
#endif


// 1.3    Optional facilities
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_PARSE_DIAG_FORMAT
//   brief: 1 compiles d_parse_diag_emitf, the printf-style emitter, which
// costs the <stdarg.h> / vsnprintf dependency in the source file; 0 leaves
// only the literal-message emitter. Default 1.
#ifndef D_CFG_PARSE_DIAG_FORMAT
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_DIAG_FORMAT      D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_DIAG_FORMAT      1
#   endif
#endif

// 1.3.2
// D_CFG_PARSE_MACHINE_TRACE
//   brief: 1 gives the machine a per-dispatch trace hook (two pointers of
// state, one predicted branch per step); 0 removes the fields entirely.
// Follows D_CFG_TESTING, since tracing is a debugging facility.
//   The default is TESTED, then written as a literal, never defined as an
// expression over D_CFG_TESTING: D_CFG_IS_BOOL validates a knob by pasting its
// fully-expanded value, so only the single token 0 or 1 passes, and an
// expression such as D_CFG_NORM(...) is a hard preprocessing error.
#ifndef D_CFG_PARSE_MACHINE_TRACE
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_MACHINE_TRACE    D_CFG_PARSE_ALL
#   elif D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_PARSE_MACHINE_TRACE    1
#   else
#       define D_CFG_PARSE_MACHINE_TRACE    0
#   endif
#endif

// 1.3.3
// D_CFG_PARSE_PROGRAM_TRANSPORT
//   brief: 1 compiles d_parse_program_write and _read, which move a compiled
// program between processes, runs, or machines in a defined little-endian
// layout; 0 leaves a program an in-memory artifact only. Default 1 -- this is
// the door to shipping a generated parser as a build product.
#ifndef D_CFG_PARSE_PROGRAM_TRANSPORT
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_PROGRAM_TRANSPORT D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_PROGRAM_TRANSPORT 1
#   endif
#endif


// 1.4    Validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_PARSE_DIAG_HEAP)
#   error "D_CFG_PARSE_DIAG_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_OP_SET_HEAP)
#   error "D_CFG_PARSE_OP_SET_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_DIAG_FORMAT)
#   error "D_CFG_PARSE_DIAG_FORMAT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_MACHINE_TRACE)
#   error "D_CFG_PARSE_MACHINE_TRACE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_POOL_HEAP)
#   error "D_CFG_PARSE_POOL_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_PROGRAM_HEAP)
#   error "D_CFG_PARSE_PROGRAM_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_PROGRAM_TRANSPORT)
#   error "D_CFG_PARSE_PROGRAM_TRANSPORT must be 0 or 1"
#endif


//==============================================================================
// 2.  SIZING CONSTANTS
//==============================================================================
// Non-boolean knobs. These are read by the module directly rather than through
// a D_INTERNAL_ alias, because there is nothing to derive: the value the user
// sets is the value the module uses.


// 2.1    Diagnostic sink
//------------------------------------------------------------------------------
// 2.1.1
// D_PARSE_DIAG_DEFAULT_ITEMS
//   brief: how many diagnostics a heap-backed sink reserves room to store when
// the caller does not say. Beyond this the sink keeps counting and stops
// storing, so a runaway stage cannot exhaust memory through diagnostics.
#ifndef D_PARSE_DIAG_DEFAULT_ITEMS
#   define D_PARSE_DIAG_DEFAULT_ITEMS       64
#endif

// 2.1.2
// D_PARSE_DIAG_DEFAULT_TEXT
//   brief: bytes of message arena a heap-backed sink reserves when the caller
// does not say.
#ifndef D_PARSE_DIAG_DEFAULT_TEXT
#   define D_PARSE_DIAG_DEFAULT_TEXT        4096
#endif

// 2.1.3
// D_PARSE_DIAG_MESSAGE_MAX
//   brief: the largest formatted message d_parse_diag_emitf will produce, and
// therefore the size of the stack buffer it formats into. Longer messages are
// truncated, never dropped.
#ifndef D_PARSE_DIAG_MESSAGE_MAX
#   define D_PARSE_DIAG_MESSAGE_MAX         512
#endif

// 2.2    Operand pool and program
//------------------------------------------------------------------------------
// 2.2.1
// D_PARSE_POOL_DEFAULT_BYTES
//   brief: blob bytes a heap-backed operand pool reserves when the caller does
// not say. Grows on demand; this only sets where it starts.
#ifndef D_PARSE_POOL_DEFAULT_BYTES
#   define D_PARSE_POOL_DEFAULT_BYTES       1024
#endif

// 2.2.2
// D_PARSE_POOL_DEFAULT_ENTRIES
//   brief: interned entries a heap-backed pool reserves room for by default.
#ifndef D_PARSE_POOL_DEFAULT_ENTRIES
#   define D_PARSE_POOL_DEFAULT_ENTRIES     32
#endif

// 2.2.3
// D_PARSE_PROGRAM_DEFAULT_CODE
//   brief: instructions a heap-backed program reserves room for by default.
#ifndef D_PARSE_PROGRAM_DEFAULT_CODE
#   define D_PARSE_PROGRAM_DEFAULT_CODE     64
#endif

// 2.3    Machine
//------------------------------------------------------------------------------
// 2.3.1
// D_PARSE_MACHINE_STEP_LIMIT
//   brief: the default runaway guard -- operators dispatched before a run is
// halted with D_PARSE_DIAG_STEP_LIMIT. A driver may raise or lower it per run
// by assigning machine.step_limit; 0 means no limit.
#ifndef D_PARSE_MACHINE_STEP_LIMIT
#   define D_PARSE_MACHINE_STEP_LIMIT       1000000L
#endif


//==============================================================================
// 3.  DERIVED VALUES
//==============================================================================
// The effective symbols the modules read. Nothing below is user-settable.


// 3.1    Effective gates
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_PARSE_DIAG_HEAP
//   brief: 1 when the diagnostic sink may allocate. Computed from
// D_CFG_PARSE_DIAG_HEAP; read by diagnostic.h and diagnostic.c.
#if D_CFG_IS_ON(D_CFG_PARSE_DIAG_HEAP)
#   define D_INTERNAL_PARSE_DIAG_HEAP       1
#else
#   define D_INTERNAL_PARSE_DIAG_HEAP       0
#endif

// 3.1.2
// D_INTERNAL_PARSE_OP_SET_HEAP
//   brief: 1 when the operator registry may allocate. Computed from
// D_CFG_PARSE_OP_SET_HEAP; read by machine.h and machine.c.
#if D_CFG_IS_ON(D_CFG_PARSE_OP_SET_HEAP)
#   define D_INTERNAL_PARSE_OP_SET_HEAP     1
#else
#   define D_INTERNAL_PARSE_OP_SET_HEAP     0
#endif

// 3.1.3
// D_INTERNAL_PARSE_DIAG_FORMAT
//   brief: 1 when the printf-style emitter is compiled. Computed from
// D_CFG_PARSE_DIAG_FORMAT; read by diagnostic.h and diagnostic.c.
#if D_CFG_IS_ON(D_CFG_PARSE_DIAG_FORMAT)
#   define D_INTERNAL_PARSE_DIAG_FORMAT     1
#else
#   define D_INTERNAL_PARSE_DIAG_FORMAT     0
#endif

// 3.1.4
// D_INTERNAL_PARSE_MACHINE_TRACE
//   brief: 1 when the machine carries its trace hook. Computed from
// D_CFG_PARSE_MACHINE_TRACE; read by machine.h. Changing it changes the layout
// of struct d_parse_machine, so it must agree across every translation unit --
// set it through the build, never per file.
#if D_CFG_IS_ON(D_CFG_PARSE_MACHINE_TRACE)
#   define D_INTERNAL_PARSE_MACHINE_TRACE   1
#else
#   define D_INTERNAL_PARSE_MACHINE_TRACE   0
#endif

// 3.1.5
// D_INTERNAL_PARSE_POOL_HEAP
//   brief: 1 when the operand pool may allocate. Computed from
// D_CFG_PARSE_POOL_HEAP; read by pool.h and pool.c.
#if D_CFG_IS_ON(D_CFG_PARSE_POOL_HEAP)
#   define D_INTERNAL_PARSE_POOL_HEAP       1
#else
#   define D_INTERNAL_PARSE_POOL_HEAP       0
#endif

// 3.1.6
// D_INTERNAL_PARSE_PROGRAM_HEAP
//   brief: 1 when a program may allocate. Computed from
// D_CFG_PARSE_PROGRAM_HEAP; read by program.h and program.c. Implies the pool
// gate, since the heap program form allocates its pool too.
#if ( D_CFG_IS_ON(D_CFG_PARSE_PROGRAM_HEAP) &&                                \
      (D_INTERNAL_PARSE_POOL_HEAP == 1) )
#   define D_INTERNAL_PARSE_PROGRAM_HEAP    1
#else
#   define D_INTERNAL_PARSE_PROGRAM_HEAP    0
#endif

// 3.1.7
// D_INTERNAL_PARSE_PROGRAM_TRANSPORT
//   brief: 1 when a program may be written and read back. Computed from
// D_CFG_PARSE_PROGRAM_TRANSPORT; read by program.h and program.c. Reading
// needs growable storage, so the write half stands alone but the read half
// reports a diagnostic and refuses when the heap gate is off.
#if D_CFG_IS_ON(D_CFG_PARSE_PROGRAM_TRANSPORT)
#   define D_INTERNAL_PARSE_PROGRAM_TRANSPORT 1
#else
#   define D_INTERNAL_PARSE_PROGRAM_TRANSPORT 0
#endif

// 3.2    Aggregate consequences
//------------------------------------------------------------------------------
// 3.2.1
// D_INTERNAL_PARSE_HEAP
//   brief: 1 when any part of the substrate may allocate, and therefore when
// the source files need <stdlib.h>. Computed from the two heap gates; read by
// diagnostic.c and machine.c so neither has to re-derive the disjunction.
#if ( (D_INTERNAL_PARSE_DIAG_HEAP == 1)    ||                                 \
      (D_INTERNAL_PARSE_OP_SET_HEAP == 1)  ||                                 \
      (D_INTERNAL_PARSE_POOL_HEAP == 1)    ||                                 \
      (D_INTERNAL_PARSE_PROGRAM_HEAP == 1) )
#   define D_INTERNAL_PARSE_HEAP            1
#else
#   define D_INTERNAL_PARSE_HEAP            0
#endif


#endif  // DJINTERP_CFG_PARSE_
