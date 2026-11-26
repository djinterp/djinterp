/*******************************************************************************
* djinterp [c][parse]                                                cfg_parse.h
*
* Configuration for the parse subframework's execution substrate.
*   Owns every knob the substrate reads: whether the diagnostic sink and the
* operator registry may allocate, whether printf-style diagnostic formatting
* is compiled in, whether the machine carries a trace hook, and the three
* sizing constants. One file for the whole subframework rather than one per
* module, per the "group config per subframework" guidance.
*
* targets:  parse/c/diagnostic.h -> D_INTERNAL_PARSE_DIAG_HEAP /
*                                 D_INTERNAL_PARSE_DIAG_FORMAT /
*                                 D_INTERNAL_PARSE_DIAG_TEXT /
*                                 D_INTERNAL_PARSE_DIAG_FACTS /
*                                 D_PARSE_DIAG_DEFAULT_ITEMS /
*                                 D_PARSE_DIAG_DEFAULT_TEXT /
*                                 D_PARSE_DIAG_DEFAULT_FACTS /
*                                 D_PARSE_DIAG_MESSAGE_MAX
*           parse/c/parse_log.h  -> D_INTERNAL_PARSE_LOG /
*                                 D_CFG_PARSE_LOG_CODES
*           parse/c/pool.h       -> D_INTERNAL_PARSE_POOL_HEAP /
*                                 D_PARSE_POOL_DEFAULT_BYTES /
*                                 D_PARSE_POOL_DEFAULT_ENTRIES
*           parse/c/program.h    -> D_INTERNAL_PARSE_PROGRAM_HEAP /
*                                 D_INTERNAL_PARSE_PROGRAM_TRANSPORT /
*                                 D_PARSE_PROGRAM_DEFAULT_CODE
*           parse/c/machine.h    -> D_INTERNAL_PARSE_MACHINE_TRACE /
*                                 D_INTERNAL_PARSE_OP_SET_HEAP /
*                                 D_PARSE_MACHINE_STEP_LIMIT
*           parse/c/expr/...     -> D_INTERNAL_PARSE_EXPR_HEAP /
*                                 D_INTERNAL_PARSE_EXPR_RENDER /
*                                 D_INTERNAL_PARSE_EXPR_SCAN /
*                                 D_CFG_PARSE_EXPR_MAX_DEPTH /
*                                 D_CFG_PARSE_EXPR_MAX_ERRORS /
*                                 D_CFG_PARSE_EXPR_ERROR_ORDER
*           parse/c/settings.h -> D_INTERNAL_PARSE_SETTINGS
*           the parse sources  -> D_INTERNAL_PARSE_HEAP
*           parse/cpp/parsers/ -> D_INTERNAL_PARSE_LIBCLANG
* requires: cfg_common.h (helpers, and the testing preset it applies);
*           cfg_log.h, for two knobs' defaults
*
* HOW TO USE
* ----------
*   Define a knob before including any parse header, on the compiler's command
* line or in a project's configuration, to change what parse builds or how it
* sizes. Leave it undefined for its default.
*
* OPTIONS
* -------
*   None at run time: these are compile-time knobs. A run's choices are the rows
* of the djin `parse` (settings.h).
*
* CONFIGURATION
* -------------
*   D_CFG_PARSE_ALL sets every gate left unset, except D_CFG_PARSE_LIBCLANG.
* Each knob is validated here, and a value out of range stops the build with
* an error that names it.
*
*
* path:      /inc/djinterp/config/c/modules/parse/cfg_parse.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.11.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FEATURE GATES
    -------------
    1.  Aggregate
         1.  D_CFG_PARSE_ALL
    2.  Allocation
         1.  D_CFG_PARSE_DIAG_HEAP
         2.  D_CFG_PARSE_OP_SET_HEAP
         3.  D_CFG_PARSE_POOL_HEAP
         4.  D_CFG_PARSE_PROGRAM_HEAP
         5.  D_CFG_PARSE_EXPR_HEAP
    3.  Optional facilities
         1.  D_CFG_PARSE_DIAG_FORMAT
         2.  D_CFG_PARSE_MACHINE_TRACE
         3.  D_CFG_PARSE_PROGRAM_TRANSPORT
         4.  D_CFG_PARSE_EXPR_RENDER
         5.  D_CFG_PARSE_EXPR_SCAN
         6.  D_CFG_PARSE_SETTINGS
         7.  D_CFG_PARSE_DIAG_TEXT
         8.  D_CFG_PARSE_DIAG_FACTS
         9.  D_CFG_PARSE_LOG
         10. D_CFG_PARSE_LOG_CODES
         11. D_CFG_PARSE_LIBCLANG
    4.  Validation
2.  SIZING CONSTANTS
    ----------------
    1.  Diagnostic sink
         1.  D_PARSE_DIAG_DEFAULT_ITEMS
         2.  D_PARSE_DIAG_DEFAULT_TEXT
         3.  D_PARSE_DIAG_DEFAULT_FACTS
         4.  D_PARSE_DIAG_MESSAGE_MAX
    2.  Operand pool and program
         1.  D_PARSE_POOL_DEFAULT_BYTES
         2.  D_PARSE_POOL_DEFAULT_ENTRIES
         3.  D_PARSE_PROGRAM_DEFAULT_CODE
    3.  Machine
         1.  D_PARSE_MACHINE_STEP_LIMIT
    4.  Expression core v2
         1.  D_CFG_PARSE_EXPR_MAX_DEPTH
         2.  D_CFG_PARSE_EXPR_MAX_ERRORS
         3.  D_CFG_PARSE_EXPR_ERROR_ORDER
3.  DERIVED VALUES
    --------------
    1.  Effective gates
         1.  D_INTERNAL_PARSE_DIAG_HEAP
         2.  D_INTERNAL_PARSE_OP_SET_HEAP
         3.  D_INTERNAL_PARSE_DIAG_FORMAT
         4.  D_INTERNAL_PARSE_MACHINE_TRACE
         5.  D_INTERNAL_PARSE_POOL_HEAP
         6.  D_INTERNAL_PARSE_PROGRAM_HEAP
         7.  D_INTERNAL_PARSE_PROGRAM_TRANSPORT
         8.  D_INTERNAL_PARSE_EXPR_HEAP
         9.  D_INTERNAL_PARSE_EXPR_RENDER
         10. D_INTERNAL_PARSE_EXPR_SCAN
         11. D_INTERNAL_PARSE_SETTINGS
         12. D_INTERNAL_PARSE_LIBCLANG
         13. D_INTERNAL_PARSE_DIAG_TEXT
         14. D_INTERNAL_PARSE_DIAG_FACTS
         15. D_INTERNAL_PARSE_LOG
    2.  Aggregate consequences
         1.  D_INTERNAL_PARSE_HEAP
*/

#ifndef DJINTERP_CONFIG_C_PARSE_CFG_PARSE_H
#define DJINTERP_CONFIG_C_PARSE_CFG_PARSE_H 1

// (0) root first: helpers, user overrides, testing flag/preset.
#include "../../../cfg_common.h"  // D_CFG_ helpers, overrides, testing
// (1) the log's knobs, which two of parse's default to; no code
#include "../../log/cfg_log.h"    // D_CFG_LOG_ENABLED, D_CFG_LOG_CODES


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
// defaulted - its absence is meaningful. Define it to 0 or 1 and every gate
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

// 1.2.5
// D_CFG_PARSE_EXPR_HEAP
//   brief: 1 compiles expression core v2's heap forms: a term, a signature and
// the parser's scratch that allocate as they grow; 0 leaves only caller-
// supplied fixed storage, and the parse performs no allocation. Default 1.
#ifndef D_CFG_PARSE_EXPR_HEAP
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_EXPR_HEAP        D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_EXPR_HEAP        1
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
// Test builds turn it on from cfg_testing.h, the one home for what differs in
// a test build; the default here is for everything else.
//   Any default for it must be the literal token 0 or 1, never an expression:
// D_CFG_IS_BOOL validates a knob by pasting its fully-expanded value, so an
// expression such as D_CFG_NORM(...) is a hard preprocessing error.
#ifndef D_CFG_PARSE_MACHINE_TRACE
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_MACHINE_TRACE    D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_MACHINE_TRACE    0
#   endif
#endif

// 1.3.3
// D_CFG_PARSE_PROGRAM_TRANSPORT
//   brief: 1 compiles d_parse_program_write and _read, which move a compiled
// program between processes, runs, or machines in a defined little-endian
// layout; 0 leaves a program an in-memory artifact only. Default 1 - this is
// the door to shipping a generated parser as a build product.
#ifndef D_CFG_PARSE_PROGRAM_TRANSPORT
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_PROGRAM_TRANSPORT D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_PROGRAM_TRANSPORT 1
#   endif
#endif

// 1.3.4
// D_CFG_PARSE_EXPR_RENDER
//   brief: 1 compiles expression core v2's renderer, which writes a term back
// as text; 0 leaves the term without it. Default 1.
#ifndef D_CFG_PARSE_EXPR_RENDER
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_EXPR_RENDER      D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_EXPR_RENDER      1
#   endif
#endif

// 1.3.5
// D_CFG_PARSE_EXPR_SCAN
//   brief: 1 compiles expression core v2's default scanner, a source that
// reads names, numbers, strings and the signature's spellings from a text;
// 0 leaves a client to bring its own source. Default 1.
#ifndef D_CFG_PARSE_EXPR_SCAN
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_EXPR_SCAN        D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_EXPR_SCAN        1
#   endif
#endif

// 1.3.6
// D_CFG_PARSE_SETTINGS
//   brief: 1 compiles parse's settings group, the djin `parse` (settings.h):
// the rows `expr_max_depth`, `expr_max_errors` and `expr_error_order`, which
// an option of 0 then reads once d_parse_register has run; 0 leaves an
// option of 0 meaning its knob. Default 1.
#ifndef D_CFG_PARSE_SETTINGS
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_SETTINGS         D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_SETTINGS         1
#   endif
#endif


// 1.3.7
// D_CFG_PARSE_DIAG_TEXT
//   brief: 1 lets a diagnostic carry its text: a sink stores it, and the
// messages parse words itself are written from their code tables'
// templates; 0 leaves every diagnostic without text, and compiles out the
// template writer and the templates. The row parse.diag_text turns it off
// for a run. Default 1.
#ifndef D_CFG_PARSE_DIAG_TEXT
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_DIAG_TEXT        D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_DIAG_TEXT        1
#   endif
#endif

// 1.3.8
// D_CFG_PARSE_DIAG_FACTS
//   brief: 1 lets a diagnostic carry its facts, the values its template
// names: a sink with room for them stores them, and a streaming hook that
// asks is given them; 0 leaves every diagnostic with its code and span
// alone. The row parse.diag_facts turns it off for a run. Default 1.
#ifndef D_CFG_PARSE_DIAG_FACTS
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_DIAG_FACTS       D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_DIAG_FACTS       1
#   endif
#endif

// 1.3.9
// D_CFG_PARSE_LOG
//   brief: 1 compiles parse_log.h and parse_log.c, which write parse's
// diagnostics to the log as its records; 0 leaves parse without them, and
// nothing of parse names the log. The row parse.log turns it off for a run.
// Default: the log's own D_CFG_LOG_ENABLED, as for any module the log is
// not built on; with the log off, the unit builds and writes nothing.
#ifndef D_CFG_PARSE_LOG
#   if defined(D_CFG_PARSE_ALL)
#       define D_CFG_PARSE_LOG              D_CFG_PARSE_ALL
#   else
#       define D_CFG_PARSE_LOG              D_CFG_LOG_ENABLED
#   endif
#endif

// 1.3.10
// D_CFG_PARSE_LOG_CODES
//   brief: 1 builds the names, templates and facts of parse's two tables of
// codes into the log's records (parse_log.c); 0 writes each code as its
// module and number, "parse.expr:4". Parse's own messages do not change
// with it. Default: the log's D_CFG_LOG_CODES.
#ifndef D_CFG_PARSE_LOG_CODES
#   define D_CFG_PARSE_LOG_CODES            D_CFG_LOG_CODES
#endif

// 1.3.11
// D_CFG_PARSE_LIBCLANG
//   brief: 1 compiles the libclang parsers - parsers/c/c_parser.hpp,
// parsers/cpp/cpp_parser.hpp, cpp_def_parser.hpp and clang_util.hpp - where
// <clang-c/Index.h> is found; 0 compiles them to nothing, wherever libclang
// is installed. Default 0, and not set by D_CFG_PARSE_ALL: the parsers still
// use the arena's API from before its rename, so they do not compile yet,
// and an aggregate turning everything on must not turn on what cannot build.
// The LLVM adapter ports them, and then decides the default.
#ifndef D_CFG_PARSE_LIBCLANG
#   define D_CFG_PARSE_LIBCLANG             0
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
#if !D_CFG_IS_BOOL(D_CFG_PARSE_EXPR_HEAP)
#   error "D_CFG_PARSE_EXPR_HEAP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_EXPR_RENDER)
#   error "D_CFG_PARSE_EXPR_RENDER must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_EXPR_SCAN)
#   error "D_CFG_PARSE_EXPR_SCAN must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_SETTINGS)
#   error "D_CFG_PARSE_SETTINGS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_LIBCLANG)
#   error "D_CFG_PARSE_LIBCLANG must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_DIAG_TEXT)
#   error "D_CFG_PARSE_DIAG_TEXT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_DIAG_FACTS)
#   error "D_CFG_PARSE_DIAG_FACTS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_LOG)
#   error "D_CFG_PARSE_LOG must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_PARSE_LOG_CODES)
#   error "D_CFG_PARSE_LOG_CODES must be 0 or 1"
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
// D_PARSE_DIAG_DEFAULT_FACTS
//   brief: bytes of facts arena a heap-backed sink reserves when the caller
// does not say: each fact takes 16 bytes, and its name and text after that.
#ifndef D_PARSE_DIAG_DEFAULT_FACTS
#   define D_PARSE_DIAG_DEFAULT_FACTS       4096
#endif

// 2.1.4
// D_PARSE_DIAG_MESSAGE_MAX
//   brief: the largest message written on the stack: by d_parse_diag_emitf,
// and from a template for a streaming hook when the sink does not store the
// text. Longer messages are truncated there, never dropped.
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
//   brief: the default runaway guard - operators dispatched before a run is
// halted with D_PARSE_DIAG_STEP_LIMIT. A driver may raise or lower it per run
// by assigning machine.step_limit; 0 means no limit.
#ifndef D_PARSE_MACHINE_STEP_LIMIT
#   define D_PARSE_MACHINE_STEP_LIMIT       1000000L
#endif

// 2.4    Expression core v2
//------------------------------------------------------------------------------
//   Each sets the default of the run-time option of the same name in
// struct dj_parse_expr_options; an option of 0 takes the knob's value.

// 2.4.1
// D_CFG_PARSE_EXPR_MAX_DEPTH
//   brief: the depth guard: the deepest term a parse builds before it stops
// with D_PARSE_EXPR_DIAG_TOO_DEEP. The core recurses nowhere and needs no
// guard; the bound protects what consumes a term recursively (v1's cata
// through the bridge, a simplifier), and 1000 of those frames fit in a 1 MB
// stack, Windows' default, with room to spare. It is at most 65535, the
// deepest a node records, or 0xFFFFFFFF, which leaves memory as the only
// bound.
#ifndef D_CFG_PARSE_EXPR_MAX_DEPTH
#   define D_CFG_PARSE_EXPR_MAX_DEPTH       1000
#endif

// 2.4.2
// D_CFG_PARSE_EXPR_MAX_ERRORS
//   brief: the errors a parse reports before it stops. The parse goes on after
// an error so that it can find one further left, a bracket never closed, that
// it only learns of later; 1 stops it at the first error met, and 65535 is
// the most, the bound of lane E's setting that will override it.
#ifndef D_CFG_PARSE_EXPR_MAX_ERRORS
#   define D_CFG_PARSE_EXPR_MAX_ERRORS      16
#endif

// 2.4.3
// D_CFG_PARSE_EXPR_ERROR_ORDER
//   brief: which of a parse's errors is the error: 1, the leftmost (S209's
// rule, every compiler's); 2, the leftmost bracket error where there is one,
// as math's parser layer orders them today. The values are parse.h's
// D_PARSE_EXPR_ORDER_LEFTMOST and _STRUCTURE_FIRST.
#ifndef D_CFG_PARSE_EXPR_ERROR_ORDER
#   define D_CFG_PARSE_EXPR_ERROR_ORDER     1
#endif

#if ( (D_CFG_PARSE_EXPR_ERROR_ORDER != 1) &&                                  \
      (D_CFG_PARSE_EXPR_ERROR_ORDER != 2) )
#   error "D_CFG_PARSE_EXPR_ERROR_ORDER must be 1 or 2"
#endif
#if ( (D_CFG_PARSE_EXPR_MAX_ERRORS < 1) ||                                     \
      (D_CFG_PARSE_EXPR_MAX_ERRORS > 65535) )
#   error "D_CFG_PARSE_EXPR_MAX_ERRORS must be 1 to 65535"
#endif
#if (D_CFG_PARSE_EXPR_MAX_DEPTH < 1)
#   error "D_CFG_PARSE_EXPR_MAX_DEPTH must be at least 1"
#endif
#if ( (D_CFG_PARSE_EXPR_MAX_DEPTH > 65535) &&                                  \
      (D_CFG_PARSE_EXPR_MAX_DEPTH != 0xFFFFFFFF) )
#   error "D_CFG_PARSE_EXPR_MAX_DEPTH must be at most 65535, or 0xFFFFFFFF"
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
// of struct dj_parse_machine, so it must agree across every translation unit --
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

// 3.1.8
// D_INTERNAL_PARSE_EXPR_HEAP
//   brief: 1 when expression core v2 may allocate. Computed from
// D_CFG_PARSE_EXPR_HEAP; read by the expr/ headers and sources.
#if D_CFG_IS_ON(D_CFG_PARSE_EXPR_HEAP)
#   define D_INTERNAL_PARSE_EXPR_HEAP       1
#else
#   define D_INTERNAL_PARSE_EXPR_HEAP       0
#endif

// 3.1.9
// D_INTERNAL_PARSE_EXPR_RENDER
//   brief: 1 when expression core v2's renderer is compiled. Computed from
// D_CFG_PARSE_EXPR_RENDER; read by expr/render.h and render.c.
#if D_CFG_IS_ON(D_CFG_PARSE_EXPR_RENDER)
#   define D_INTERNAL_PARSE_EXPR_RENDER     1
#else
#   define D_INTERNAL_PARSE_EXPR_RENDER     0
#endif

// 3.1.10
// D_INTERNAL_PARSE_EXPR_SCAN
//   brief: 1 when expression core v2's default scanner is compiled. Computed
// from D_CFG_PARSE_EXPR_SCAN; read by expr/scan.h and scan.c.
#if D_CFG_IS_ON(D_CFG_PARSE_EXPR_SCAN)
#   define D_INTERNAL_PARSE_EXPR_SCAN       1
#else
#   define D_INTERNAL_PARSE_EXPR_SCAN       0
#endif

// 3.1.11
// D_INTERNAL_PARSE_SETTINGS
//   brief: 1 when parse's settings group is compiled. Computed from
// D_CFG_PARSE_SETTINGS; read by settings.h and settings.c.
#if D_CFG_IS_ON(D_CFG_PARSE_SETTINGS)
#   define D_INTERNAL_PARSE_SETTINGS        1
#else
#   define D_INTERNAL_PARSE_SETTINGS        0
#endif

// 3.1.12
// D_INTERNAL_PARSE_LIBCLANG
//   brief: 1 when the libclang parsers are compiled: D_CFG_PARSE_LIBCLANG is
// on and the compiler finds <clang-c/Index.h>. Read by the four parsers/
// headers. Without __has_include the header cannot be looked for, and the
// parsers are left out.
#if D_CFG_IS_ON(D_CFG_PARSE_LIBCLANG) && defined(__has_include)
#   if __has_include(<clang-c/Index.h>)
#       define D_INTERNAL_PARSE_LIBCLANG    1
#   endif
#endif
#ifndef D_INTERNAL_PARSE_LIBCLANG
#   define D_INTERNAL_PARSE_LIBCLANG        0
#endif

// 3.1.13
// D_INTERNAL_PARSE_DIAG_TEXT
//   brief: 1 when a diagnostic may carry its text. Computed from
// D_CFG_PARSE_DIAG_TEXT; read by diagnostic.h, diagnostic.c and the emitters
// that word their messages from templates.
#if D_CFG_IS_ON(D_CFG_PARSE_DIAG_TEXT)
#   define D_INTERNAL_PARSE_DIAG_TEXT       1
#else
#   define D_INTERNAL_PARSE_DIAG_TEXT       0
#endif

// 3.1.14
// D_INTERNAL_PARSE_DIAG_FACTS
//   brief: 1 when a diagnostic may carry its facts. Computed from
// D_CFG_PARSE_DIAG_FACTS; read by diagnostic.h and diagnostic.c.
#if D_CFG_IS_ON(D_CFG_PARSE_DIAG_FACTS)
#   define D_INTERNAL_PARSE_DIAG_FACTS      1
#else
#   define D_INTERNAL_PARSE_DIAG_FACTS      0
#endif

// 3.1.15
// D_INTERNAL_PARSE_LOG
//   brief: 1 when parse_log.h and parse_log.c are compiled. Computed from
// D_CFG_PARSE_LOG; read by those two, which also need the log's 64-bit
// floor (RE_STD_HAS_INT64), as log.h does.
#if D_CFG_IS_ON(D_CFG_PARSE_LOG)
#   define D_INTERNAL_PARSE_LOG             1
#else
#   define D_INTERNAL_PARSE_LOG             0
#endif

// 3.2    Aggregate consequences
//------------------------------------------------------------------------------
// 3.2.1
// D_INTERNAL_PARSE_HEAP
//   brief: 1 when any part of the substrate may allocate, and therefore when
// the source files need <stdlib.h> and storage.h's d_parse_grow is compiled.
// Computed from the heap gates; read by diagnostic.c, machine.c, storage.h and
// the expr/ sources so none has to re-derive the disjunction.
#if ( (D_INTERNAL_PARSE_DIAG_HEAP == 1)    ||                                 \
      (D_INTERNAL_PARSE_OP_SET_HEAP == 1)  ||                                 \
      (D_INTERNAL_PARSE_POOL_HEAP == 1)    ||                                 \
      (D_INTERNAL_PARSE_PROGRAM_HEAP == 1) ||                                 \
      (D_INTERNAL_PARSE_EXPR_HEAP == 1) )
#   define D_INTERNAL_PARSE_HEAP            1
#else
#   define D_INTERNAL_PARSE_HEAP            0
#endif


#endif  // DJINTERP_CONFIG_C_PARSE_CFG_PARSE_H
