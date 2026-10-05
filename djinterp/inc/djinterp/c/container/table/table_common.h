/*******************************************************************************
* djinterp [c]                                                    table_common.h
*
*   The table module's PREAMBLE: what every part needs before any structure is
* declared -- the configuration knobs, D_INLINE_DEF, and the outcome
* vocabulary.
*
*   It is a preamble and NOT an aggregate, and that separation is
* load-bearing.
* A header that both carried the preamble and included the parts would be
* circular: a part including it back would find the later parts declared
* against
* structures whose own header had not finished executing. So the parts include
* this, and the FACES aggregate: table.h for C, table.hpp for C++.
*
*   THE PARTS, in dependency order:
*     table_domain.h the index space -- the multi-index, I_T, the order they
*                       share, and the refinement an atomic split needs.
*     table_carrier.h   the CELL-HOMOGENEOUS table: cells, storage strategy,
*                       cell access, and structural mutation.
*     table_layout.h    Gamma: regions, the owner function, anchors,
*                       layout-aware access, merge and split.
*     table_overlay.h   the restriction bundle a table wears.
*
*   Each part carries the layout assertions for the structures it declares, so
* drift is caught beside the declaration rather than in a distant block.
*
*   PORTABILITY:
*   C99 / C++11, the framework floors.
*
*
* path:      /inc/djinterp/c/container/table/table_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.01
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_TABLE_TABLE_COMMON_H
#define DJINTERP_C_CONTAINER_TABLE_TABLE_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t,
                                                 // UINT32_MAX, UINT64_MAX


// D_INLINE_DEF
//   macro: the specifier for a definition that lives in a .c BESIDE a plain
// declaration in the header. Distinct from D_INLINE, which is `static inline`
// in C and so is header-safe but purely local; D_INLINE_DEF is plain `inline`,
// and C11 6.7.4p7 makes the definition EXTERNAL precisely because a file-scope
// declaration without `inline` is visible. One symbol, linkable from both
// languages, free to be inlined within its own translation unit. THE C SOURCES
// MUST BE COMPILED AS C. Handed to a C++ compiler -- which is what `g++ foo.c`
// does -- `inline` takes its C++ meaning, no external
// definition is emitted, and the link fails on every function that moved out
// of a header.
// The trade this buys and costs: the header stays a pure declaration, and the
// body cannot be inlined into another TU without LTO. It is therefore applied
// only where that costs nothing -- a swept read through an out-of-lined cell
// accessor measured 4.2x slower, so the cell arithmetic keeps its body in the
// header and everything colder moves to a source. Belongs in the prelude
// beside D_INLINE; defined here, #ifndef-guarded, until it does.
#ifndef D_INLINE_DEF
#   if defined(_MSC_VER) && !defined(__clang__) && !defined(__cplusplus)
        // MSVC's C mode reached C11 inline semantics late; __inline is the
        // spelling that behaves across its versions.
#       define D_INLINE_DEF                 __inline
#   else
#       define D_INLINE_DEF                 inline
#   endif
#endif


D_EXTERN_C_BEGIN

// D_CFG_TABLE_EXTENT_64
//   brief: opt in to 64-bit stored extents. Off by default: 32-bit extents
// give a 24-byte d_table, a compact wire form, and a ceiling of 4294967295
// rows and columns. It does not affect the width of any index parameter, which
// is always size_t, so turning it on changes the representation and nothing
// else. Inherits D_CFG_CONTAINER_ALL, then off, per the cascade container.h
// states. THIS KNOB LIVES HERE, NOT IN config/. container.h records the
// migration -- "config/container/container_config.h is gone; its knobs live
// here, in the subframework's own common header" -- so a table knob belongs in
// the table's
// own common header on the same idiom, and an earlier note in this file
// promising to move it to cfg_table.h had the direction backwards.
#ifndef D_CFG_TABLE_EXTENT_64
#   if defined(D_CFG_CONTAINER_ALL)
#       define D_CFG_TABLE_EXTENT_64        D_CFG_CONTAINER_ALL
#   else
#       define D_CFG_TABLE_EXTENT_64        0
#   endif
#endif

#if !D_CFG_IS_BOOL(D_CFG_TABLE_EXTENT_64)
#   error "D_CFG_TABLE_EXTENT_64 must be 0 or 1"
#endif

// D_INTERNAL_TABLE_EXTENT
//   macro: the resolved spelling of a STORED extent -- a row count, a column
// count, or a cell size, as it sits in a struct and on the wire. A macro
// rather than a typedef because C must not hide what a type is, and because
// the choice must be visible to the preprocessor where the structs below are
// declared.
#ifndef D_INTERNAL_TABLE_EXTENT
#   if D_CFG_IS_ON(D_CFG_TABLE_EXTENT_64)
#       define D_INTERNAL_TABLE_EXTENT      uint64_t
#       define D_INTERNAL_TABLE_EXTENT_MAX  UINT64_MAX
#   else
#       define D_INTERNAL_TABLE_EXTENT      uint32_t
#       define D_INTERNAL_TABLE_EXTENT_MAX  UINT32_MAX
#   endif
#endif

// D_CFG_TABLE_MAX_RANK
//   brief: the greatest rank k this build carries. A domain, an index and a
// region each hold their coordinates INLINE, so the cap is what lets them be
// returned by value, compared by value, and serialized without an allocator --
// goals 3, "no hidden allocation", applied to the index space itself. Four
// covers the row-and-column table (k = 2) and the usual tensor shapes with
// room to spare; raise it where a build needs more and pay the bytes.
#ifndef D_CFG_TABLE_MAX_RANK
#   define D_CFG_TABLE_MAX_RANK         4
#endif

#if (D_CFG_TABLE_MAX_RANK < 1)
#   error "D_CFG_TABLE_MAX_RANK must be at least 1"
#endif

// D_TABLE_MAX_RANK
//   constant: the resolved cap. A rank beyond it is refused with
// D_TABLE_STATUS_RANK rather than truncated.
#define D_TABLE_MAX_RANK            D_CFG_TABLE_MAX_RANK

// D_TABLE_RANK_TABLE
//   constant: the rank of the row-and-column table, k = 2. Named because the
// leading case is worth naming, not because the module is limited to it.
#define D_TABLE_RANK_TABLE          2

// D_TABLE_EXTENT_FITS
//   macro: whether a size_t quantity may be stored as an extent without loss.
// The guard at every point where an index or a count becomes a stored field.
#define D_TABLE_EXTENT_FITS(_value) \
    ((size_t)(_value) <= (size_t)D_INTERNAL_TABLE_EXTENT_MAX)

// d_table_status
//   enum: the outcome of a table operation. The FORMAL failures (a domain
// violation, a rectangularity violation) and the MECHANICAL ones (out of room,
// allocation refused, null argument) are distinct codes because they are
// distinct things: "this index is not in I_T" is a claim the tex makes, and
// "the buffer is full" is a fact about a machine. Reporting them identically
// is a conformance bug that reads as correct behaviour. THE SPLIT IS A NUMERIC
// RANGE, not a comment, so that non-conflation is CHECKABLE. AGENT_README
// states the anti-pattern directly -- "these sets are in conflict" (the tex
// says the operation is undefined) and "the output buffer is full" are
// different things, and reporting them identically is a conformance bug that
// reads as correct behaviour. A range makes the difference a predicate a
// caller can branch on and a test can assert. A placeholder for the `result`
// carrier of functional_types (which exists in C++ at
// core/functional/result.hpp and has no C form yet), so the migration is
// deferred, not declined.
enum d_table_status
{
    // -- success ------------------------------------------------------------
    D_TABLE_STATUS_OK                = 0,

    // -- formal: the operation is not defined --------------------------------
    D_TABLE_STATUS_DOMAIN            = 0x001,  // index outside I_T
    D_TABLE_STATUS_SHAPE             = 0x002,  // would break rectangularity
    D_TABLE_STATUS_RANK              = 0x003,  // ranks disagree, or exceed the
                                               // build's cap
    D_TABLE_STATUS_COVER             = 0x004,  // the cells do not partition I_T

    // -- mechanical: the operation is defined, the machinery ran short -------
    // A null pointer is MECHANICAL, not formal: containers.tex models a table
    // as (T_, I_T, Gamma) and has no notion of a pointer at all, so "you
    // passed me nothing" is a fact about a machine and never a claim the tex
    // makes.
    D_TABLE_STATUS_INVALID_ARGUMENT  = 0x100,  // a required argument was null
    D_TABLE_STATUS_CAPACITY          = 0x101,  // no room, and no allocator
    D_TABLE_STATUS_NO_MEMORY         = 0x102,  // the strategy refused
    D_TABLE_STATUS_OVERFLOW          = 0x103   // extent or byte count overflow
};

// D_TABLE_STATUS_MECHANICAL_FLOOR
//   constant: the first mechanical status. A status at or above this value
// describes the machinery; below it, the request itself. The split is a
// NUMERIC RANGE rather than a comment so that "was this a formal failure?" is
// a predicate a caller can branch on and a test can assert.
#define D_TABLE_STATUS_MECHANICAL_FLOOR 0x100

// D_TABLE_STATUS_IS_FORMAL
//   macro: whether the status says the request itself was not defined -- a
// claim containers.tex makes, which no amount of memory would change.
#define D_TABLE_STATUS_IS_FORMAL(_s)      \
    ( ((int)(_s) != D_TABLE_STATUS_OK) && \
      ((int)(_s) <  D_TABLE_STATUS_MECHANICAL_FLOOR) )

// D_TABLE_STATUS_IS_MECHANICAL
//   macro: whether the status says the machine ran short -- the same request
// may succeed later, or with more room.
#define D_TABLE_STATUS_IS_MECHANICAL(_s) \
    ( (int)(_s) >= D_TABLE_STATUS_MECHANICAL_FLOOR )

const char* d_table_status_name(enum d_table_status _status);
const char* d_table_status_message(enum d_table_status _status);

D_EXTERN_C_END


#endif  // DJINTERP_C_CONTAINER_TABLE_TABLE_COMMON_H
