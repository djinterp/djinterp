/*******************************************************************************
* djinterp [c]                                                      mem_common.h
*
* Memory foundations -- the shared C core (tier 0):
*   The vocabulary layer of the memory subframework, declared once in C and
* compiled by both languages. Defines the counter type every allocator counts
* in, the status set every fallible operation reports through, the byte-block
* view every allocator hands back, the alignment arithmetic every allocator
* rounds with, the accounting block every allocator may embed, and the
* hardening primitives (poison, redzone) that turn a corrupt access into a
* recognisable one.
*
*   NOTHING HERE ALLOCATES. This header owns no storage and calls no upstream;
* it is the alphabet that mem_source.h, arena.h, pool.h and slab.h are all
* written in. That is why it is a _common: four modules read it, and two of
* them (arena, pool) would otherwise each grow a private copy of the same
* rounding and the same overflow guard.
*
*   This header carries NO dependency on the C++ face and NO dependency on an
* allocator. It is compilable at the C99 floor and is freestanding-safe apart
* from <string.h> (memset), which the poison and zero paths use and which a
* freestanding build can turn off by disabling those knobs.
*
* WHY THE ARITHMETIC IS SHARED RATHER THAN OPEN-CODED:
*   Alignment rounding is three tokens and is wrong in a specific way often
* enough to be worth naming: (n + a - 1) & ~(a - 1) overflows silently when n
* is within a-1 of the maximum, and the overflowed result is a SMALLER number,
* so the caller allocates less than it asked for and the bug surfaces as
* corruption somewhere else entirely. d_mem_align_up_checked is the same
* expression with the one comparison that makes the failure reportable, and
* every module in this subframework routes through it.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/memory/mem_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PORTABILITY LAYER
      -----------------
      a. D_MEM_FN / D_MEM_FN_HOT        (kernel qualifiers)
      b. D_MEM_LITERAL                  (aggregate construction)
      c. D_MEM_EXTERN_INLINE            (C99 out-of-line emission)
II.   COUNTER TYPE
      ------------
      a. d_mem_size / D_MEM_SIZE_MAX
      b. d_mem_max_align / D_MEM_MAX_ALIGN / D_MEM_ALIGN_DEFAULT
III.  STATUS
      ------
      a. enum d_mem_status
      b. d_mem_status_string
IV.   BLOCK VIEW
      ----------
      a. struct d_mem_block
      b. d_mem_block_make / _empty / _end / _contains / _is_empty
V.    ALIGNMENT AND OVERFLOW ARITHMETIC
      ---------------------------------
      a. d_mem_is_pow2 / d_mem_align_normalize
      b. d_mem_align_up / _align_up_checked / _align_padding
      c. d_mem_align_up_ptr / _is_aligned
      d. d_mem_add_checked / _mul_checked
VI.   ACCOUNTING
      ----------
      a. struct d_mem_stats
      b. d_mem_stats_clear / _on_reserve / _on_acquire / _on_release
      c. d_mem_stats_utilization
VII.  HARDENING
      ---------
      a. d_mem_poison / _poison_free / _zero
      b. d_mem_redzone_write / _redzone_check / _redzone_total
VIII. ARGUMENT POLICY
      ---------------
      a. D_MEM_REQUIRE / D_MEM_REQUIRE_STATUS
*/

#ifndef DJINTERP_C_MEMORY_MEM_COMMON_H
#define DJINTERP_C_MEMORY_MEM_COMMON_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "../../config/core/memory/cfg_mem_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t, uintptr_t,
                                              // UINT32_MAX, UINT64_MAX,
                                              // SIZE_MAX


///////////////////////////////////////////////////////////////////////////////
///                     I.   PORTABILITY LAYER                              ///
///////////////////////////////////////////////////////////////////////////////
/*   The macros below are what let one body serve both languages. Each kernel
* is written once and resolves to the right qualifier set:
*
*     D_MEM_FN      -> constexpr inline (C++)  /  inline (C)
*     D_MEM_FN_HOT  -> inline           (C++)  /  inline (C)
*
*   D_MEM_FN marks arithmetic that is valid in a constant expression, so the
* C++ face can fold a slot geometry at translation time that C computes at run
* time. That is the framework's phase distinction working as designed: the
* SAME function, evaluated earlier. Every D_MEM_FN body is therefore a single
* return statement, which is the C++11 constexpr restriction and is the reason
* the bodies here look terser than the style guide would otherwise ask for.
*
*   D_MEM_FN_HOT marks a body that touches memory (poison fills, redzone
* checks, statistics) and so can never be constexpr, but is small enough that
* a call would cost more than the body.
*
*   C LINKAGE MODEL (D_CFG_MEM_HEADER_ONLY, cfg_mem_common.h section 7):
*     0 (default) - kernels are `inline`; their bodies live here, and each
*                   module's .c emits exactly one out-of-line definition via
*                   D_MEM_EXTERN_INLINE, so a non-inlined call and a taken
*                   address both resolve. Build and link the memory .c files.
*     1           - kernels are `static inline`; every TU gets its own copy
*                   and no .c file is required for them. The out-of-line
*                   module API (arena, pool, slab operations) still lives in
*                   the .c files either way -- this knob governs the header
*                   kernels only.
*/

// D_INTERNAL_MEM_INLINE_KW
//   macro (internal): the C spelling of `inline`. MSVC's C front-end accepts
// `__inline` in every mode it supports, where bare `inline` was a C++-only
// keyword there until recently; GCC and Clang accept `inline` under -std=c99
// with no diagnostic.
#ifndef D_INTERNAL_MEM_INLINE_KW
#   if defined(D_ENV_COMPILER_MSVC) && !defined(__cplusplus)
#       define D_INTERNAL_MEM_INLINE_KW __inline
#   else
#       define D_INTERNAL_MEM_INLINE_KW inline
#   endif
#endif

#ifdef __cplusplus
#   include "../../djinterp.hpp"

#   define D_MEM_FN                 D_CONSTEXPR_INLINE
#   define D_MEM_FN_HOT             inline
#   define D_MEM_EXTERN_INLINE      inline

#if D_ENV_PP_HAS_VARIADIC_MACROS
    // aggregate construction: T{ ... }
#   define D_MEM_LITERAL(T, ...)    T{ __VA_ARGS__ }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS
#else
#   if (D_INTERNAL_MEM_HEADER_ONLY == 1)
#       define D_MEM_FN             static D_INTERNAL_MEM_INLINE_KW
#       define D_MEM_FN_HOT         static D_INTERNAL_MEM_INLINE_KW
#       define D_MEM_EXTERN_INLINE  static D_INTERNAL_MEM_INLINE_KW
#   else
#       define D_MEM_FN             D_INTERNAL_MEM_INLINE_KW
#       define D_MEM_FN_HOT         D_INTERNAL_MEM_INLINE_KW
#       define D_MEM_EXTERN_INLINE  extern D_INTERNAL_MEM_INLINE_KW
#   endif

#if D_ENV_PP_HAS_VARIADIC_MACROS
    // aggregate construction: (struct T){ ... }  (C99 compound literal)
#   define D_MEM_LITERAL(T, ...)    (struct T){ __VA_ARGS__ }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS
#endif


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///                       II.   COUNTER TYPE                                ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_size
//   type: the one unsigned counter every allocator in this subframework
// counts bytes, slots and capacities in. Its width is chosen by
// D_CFG_MEM_SIZE_BITS, because it appears five to seven times per allocator
// struct and is therefore the largest single lever on how much an allocator
// costs to carry. NATIVE (the default) tracks size_t and can address whatever
// the platform can; 32 halves every counter on LP64 at the cost of capping one
// allocator at 4 GiB.
//   NOTE: this type is part of the layout. Two translation units compiled with
// different D_CFG_MEM_SIZE_BITS may not share an allocator, and the layout
// assertions below are what turn that into a compile error rather than a
// silent disagreement about where a field lives.
#if (D_INTERNAL_MEM_SIZE_BITS == 32)
    typedef uint32_t d_mem_size;
#   define D_MEM_SIZE_MAX           ((d_mem_size)UINT32_MAX)
#elif (D_INTERNAL_MEM_SIZE_BITS == 64)
    typedef uint64_t d_mem_size;
#   define D_MEM_SIZE_MAX           ((d_mem_size)UINT64_MAX)
#else
    typedef size_t d_mem_size;
#   define D_MEM_SIZE_MAX           ((d_mem_size)SIZE_MAX)
#endif

// d_mem_max_align
//   union: the widest-aligned set of fundamental types. Exists so that
// D_MEM_MAX_ALIGN can be computed at the C99 floor, where _Alignof does not
// exist and <stddef.h> is not required to define max_align_t.
union d_mem_max_align
{
    long double         ld;
    void*               ptr;
    void              (*fn)(void);
    long                l;
    d_mem_size          sz;
};

// d_mem_align_probe
//   struct: the offsetof vehicle for D_MEM_MAX_ALIGN. A named struct rather
// than an anonymous one inside offsetof, because that spelling is a GNU
// extension in C and ill-formed in C++, and this header is compiled by both.
struct d_mem_align_probe
{
    char                  pad;
    union d_mem_max_align value;
};

// D_MEM_MAX_ALIGN
//   constant: the platform's strictest fundamental alignment. Derived by
// offsetof rather than alignof so that the value is identical at every
// language level this framework supports.
#define D_MEM_MAX_ALIGN                                                       \
    ((d_mem_size)offsetof(struct d_mem_align_probe, value))

// D_MEM_ALIGN_DEFAULT
//   constant: the alignment applied when a caller passes 0. Resolves the
// configured default, whose 0 means "whatever the platform demands of an
// allocation of unknown type" -- the only safe answer, and the reason 0 is
// what D_CFG_MEM_DEFAULT_ALIGN ships as.
#if (D_INTERNAL_MEM_DEFAULT_ALIGN_CFG == 0)
#   define D_MEM_ALIGN_DEFAULT      D_MEM_MAX_ALIGN
#else
#   define D_MEM_ALIGN_DEFAULT                                                \
        ((d_mem_size)D_INTERNAL_MEM_DEFAULT_ALIGN_CFG)
#endif

// D_MEM_ALIGN_MIN / D_MEM_ALIGN_MAX
//   constant: the configured alignment floor and ceiling, republished here in
// the module's own vocabulary so no consumer reads a D_INTERNAL_ symbol.
#define D_MEM_ALIGN_MIN             D_INTERNAL_MEM_MIN_ALIGN
#define D_MEM_ALIGN_MAX             D_INTERNAL_MEM_MAX_ALIGN

// d_mem_link_probe
//   struct: the offsetof vehicle for D_MEM_LINK_ALIGN, for the same reason
// d_mem_align_probe exists.
struct d_mem_link_probe
{
    char  pad;
    void* value;
};

// D_MEM_LINK_SIZE / D_MEM_LINK_ALIGN
//   constant: the size and alignment of an intrusive link stored inside a free
// slot. pool.h raises its slot geometry to these when its release policy
// threads a free list through unused slots, which is the one place a slot must
// be big enough to hold something other than the caller's object.
#define D_MEM_LINK_SIZE             ((d_mem_size)sizeof(void*))
#define D_MEM_LINK_ALIGN                                                      \
    ((d_mem_size)offsetof(struct d_mem_link_probe, value))


///////////////////////////////////////////////////////////////////////////////
///                         III.   STATUS                                   ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_status
//   enum: the outcome of a fallible memory operation. Every _ex form in this
// subframework returns one of these; the plain forms collapse them to a null
// pointer or a bool, which is the ergonomic spelling for a caller that will
// only ever branch on success.
//   THE DISTINCTION THAT MATTERS: D_MEM_ERR_EXHAUSTED means the allocator did
// what it was asked and there was no memory; D_MEM_ERR_INVALID means the
// request was malformed and no allocator could have satisfied it. Reporting
// them identically is the conformance bug that reads as correct behaviour, so
// they are separate values and stay separate through every layer.
enum d_mem_status
{
    // the operation completed
    D_MEM_OK              = 0,

    // a required pointer argument was null
    D_MEM_ERR_NULL        = 1,

    // an argument was malformed: a zero size where one is required, an
    // alignment that is not a power of two, or one outside
    // [D_MEM_ALIGN_MIN, D_MEM_ALIGN_MAX]
    D_MEM_ERR_INVALID     = 2,

    // size arithmetic would exceed D_MEM_SIZE_MAX. Distinct from EXHAUSTED
    // because it is a property of the request, not of the machine
    D_MEM_ERR_OVERFLOW    = 3,

    // the upstream source refused, or a fixed-capacity allocator is full
    D_MEM_ERR_EXHAUSTED   = 4,

    // the allocator's configuration forbids this operation -- releasing an
    // individual block from a monotonic arena, or sweeping a generation in a
    // pool whose release policy carries no generations
    D_MEM_ERR_UNSUPPORTED = 5,

    // the pointer does not belong to this allocator
    D_MEM_ERR_FOREIGN     = 6,

    // a generational handle names a slot that has since been reused
    D_MEM_ERR_STALE       = 7,

    // a guard band was overwritten, or a slot was released twice
    D_MEM_ERR_CORRUPT     = 8
};

// D_MEM_STATUS_COUNT
//   constant: the number of status values, so a caller may size a table.
#define D_MEM_STATUS_COUNT          9


///////////////////////////////////////////////////////////////////////////////
///                       IV.   BLOCK VIEW                                  ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_block
//   struct: a borrowed view of a contiguous byte range. The universal currency
// of this subframework: every source hands one back, every arena region is
// one, and every bulk operation takes one. It OWNS NOTHING -- the allocator
// that produced it owns the bytes, and the view is valid only as long as that
// allocator says it is.
struct d_mem_block
{
    void*      ptr;     // first byte, or NULL when the block is empty
    d_mem_size size;    // length in bytes
};


///////////////////////////////////////////////////////////////////////////////
///              V.   ALIGNMENT AND OVERFLOW ARITHMETIC                     ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_is_pow2
//   kernel: true when _value is a power of two. Zero is NOT a power of two
// here, which is the useful answer: zero is the "unspecified" alignment and
// must take the default rather than pass validation.
D_MEM_FN bool
d_mem_is_pow2(
    d_mem_size _value
)
{
    return ( (_value != 0) && ((_value & (_value - 1)) == 0) );
}

// d_mem_align_normalize
//   kernel: the alignment a request will actually be given -- the caller's
// value, or the default when it passed 0, raised to the configured floor.
// Every module normalizes before validating, so that a caller passing 0 is
// always correct and never merely tolerated.
D_MEM_FN d_mem_size
d_mem_align_normalize(
    d_mem_size _align
)
{
    return ( (_align == 0)
                 ? ( (D_MEM_ALIGN_DEFAULT > D_MEM_ALIGN_MIN)
                         ? D_MEM_ALIGN_DEFAULT
                         : D_MEM_ALIGN_MIN )
                 : ( (_align > D_MEM_ALIGN_MIN)
                         ? _align
                         : D_MEM_ALIGN_MIN ) );
}

// d_mem_align_is_valid
//   kernel: true when a NORMALIZED alignment is one this subframework will
// honour -- a power of two within [D_MEM_ALIGN_MIN, D_MEM_ALIGN_MAX].
D_MEM_FN bool
d_mem_align_is_valid(
    d_mem_size _align
)
{
    return ( d_mem_is_pow2(_align) &&
             (_align >= D_MEM_ALIGN_MIN) &&
             (_align <= D_MEM_ALIGN_MAX) );
}

// d_mem_align_accept
//   kernel: the alignment a request resolves to, or 0 when the request is
// inadmissible. THE ONE FUNCTION EVERY ALLOCATOR HERE VALIDATES THROUGH.
//   It rejects a non-zero value that is not a power of two BEFORE the floor is
// applied. That order matters: with D_CFG_MEM_MIN_ALIGN raised to 16, a
// caller asking for alignment 3 would otherwise be silently given 16 and
// never learn that 3 was meaningless. Stronger-than-requested is a fine thing
// for a FLOOR to do to a valid request; it is not a fine thing to do to an
// invalid one, because it hides the caller's mistake behind a correct-looking
// result.
//   Zero still means "the configured default", which is a request rather than
// a mistake and is resolved rather than refused.
D_MEM_FN d_mem_size
d_mem_align_accept(
    d_mem_size _align
)
{
    return ( ( (_align != 0) && (!d_mem_is_pow2(_align)) )
                 ? (d_mem_size)0
                 : ( d_mem_align_is_valid(d_mem_align_normalize(_align))
                         ? d_mem_align_normalize(_align)
                         : (d_mem_size)0 ) );
}

// d_mem_align_up
//   kernel: _value rounded up to the next multiple of _align, which MUST be a
// power of two. UNCHECKED: when _value is within _align-1 of D_MEM_SIZE_MAX
// the sum wraps and the result is smaller than _value. Use it only where the
// magnitude is already known to be bounded -- a slot size, a header size --
// and use d_mem_align_up_checked everywhere a caller's number is involved.
D_MEM_FN d_mem_size
d_mem_align_up(
    d_mem_size _value,
    d_mem_size _align
)
{
    return ( (_value + (_align - 1)) & ~(_align - 1) );
}

// d_mem_align_padding
//   kernel: the bytes that must be skipped from _value to reach the next
// multiple of _align. Cannot overflow: the result is always < _align.
D_MEM_FN d_mem_size
d_mem_align_padding(
    d_mem_size _value,
    d_mem_size _align
)
{
    return ( (_align - (_value & (_align - 1))) & (_align - 1) );
}

// d_mem_align_would_overflow
//   kernel: true when d_mem_align_up(_value, _align) would wrap.
D_MEM_FN bool
d_mem_align_would_overflow(
    d_mem_size _value,
    d_mem_size _align
)
{
    return (_value > (D_MEM_SIZE_MAX - (_align - 1)));
}

// d_mem_is_aligned
//   kernel: true when the address held in _ptr is a multiple of _align.
D_MEM_FN_HOT bool
d_mem_is_aligned(
    const void* _ptr,
    d_mem_size  _align
)
{
    return ( ((uintptr_t)_ptr & (uintptr_t)(_align - 1)) == 0 );
}

// d_mem_add_would_overflow / d_mem_mul_would_overflow
//   kernel: true when the sum / product would exceed D_MEM_SIZE_MAX. The
// multiplication guard divides rather than multiplying, so it never produces
// the wrapped value it is testing for.
D_MEM_FN bool
d_mem_add_would_overflow(
    d_mem_size _a,
    d_mem_size _b
)
{
    return (_a > (D_MEM_SIZE_MAX - _b));
}

D_MEM_FN bool
d_mem_mul_would_overflow(
    d_mem_size _a,
    d_mem_size _b
)
{
    return ( (_a != 0) && (_b > (D_MEM_SIZE_MAX / _a)) );
}


///////////////////////////////////////////////////////////////////////////////
///                       VI.   ACCOUNTING                                  ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_stats
//   struct: byte and call accounting for one allocator. Whether an allocator
// EMBEDS one is D_CFG_MEM_STATS; the type itself is always declared, because a
// caller may want to ask for one and receive a zeroed answer rather than fail
// to compile against a build that turned accounting off.
//   The peak fields are separately gated: a running total costs an add on the
// allocate path, a high-water mark costs a compare and a conditional store,
// and a caller that samples the totals periodically has no use for the second.
struct d_mem_stats
{
    d_mem_size bytes_reserved;      // obtained from the upstream source
    d_mem_size bytes_committed;     // handed out, including padding and guards
    d_mem_size bytes_live;          // committed and not yet released
    d_mem_size acquire_count;       // successful allocate calls
    d_mem_size release_count;       // successful release calls
    d_mem_size upstream_count;      // requests made of the upstream source
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
    d_mem_size peak_bytes_live;     // high-water of bytes_live
    d_mem_size peak_live_count;     // high-water of (acquire - release)
#endif
};


///////////////////////////////////////////////////////////////////////////////
///                       VII.   HARDENING                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_MEM_POISON_ALLOC / D_MEM_POISON_FREE / D_MEM_REDZONE_FILL
//   constant: the three fill bytes, republished in the module's vocabulary.
#define D_MEM_POISON_ALLOC          ((int)(D_CFG_MEM_POISON_ALLOC_BYTE))
#define D_MEM_POISON_FREE           ((int)(D_CFG_MEM_POISON_FREE_BYTE))
#define D_MEM_REDZONE_FILL          ((int)(D_CFG_MEM_REDZONE_BYTE))

// D_MEM_REDZONE_BYTES
//   constant: the width of ONE guard band, or 0 when redzones are off, so a
// module can add it unconditionally instead of branching on the knob.
#define D_MEM_REDZONE_BYTES         D_INTERNAL_MEM_REDZONE_BYTES

// D_MEM_REDZONE_TOTAL
//   constant: what a guarded block costs over an unguarded one -- two bands.
#define D_MEM_REDZONE_TOTAL         (D_MEM_REDZONE_BYTES * (d_mem_size)2)

// d_mem_redzone_lead
//   kernel: the bytes reserved BEFORE a payload for its leading guard band,
// rounded up to the payload's alignment.
//   THE ROUNDING IS THE WHOLE POINT. Every allocator here aligns a cursor and
// then skips the lead to reach the payload; if the lead were not a multiple of
// the alignment, that skip would land the payload off its boundary and each
// allocator would need its own correction. Rounded, the redzone interacts with
// alignment in exactly one place -- this function -- and nowhere else.
//   Zero when redzones are off, so a caller adds it unconditionally rather
// than branching on the knob.
D_MEM_FN d_mem_size
d_mem_redzone_lead(
    d_mem_size _align
)
{
    return ( (D_MEM_REDZONE_BYTES == 0)
                 ? (d_mem_size)0
                 : d_mem_align_up(D_MEM_REDZONE_BYTES, _align) );
}


///////////////////////////////////////////////////////////////////////////////
///                    VIII.   ARGUMENT POLICY                              ///
///////////////////////////////////////////////////////////////////////////////

// D_MEM_REQUIRE
//   macro: the guard every entry point opens with. Under
// D_CFG_MEM_STRICT_ARGS it asserts, so a malformed call stops at the caller;
// otherwise it returns _fail, so the same call is reported rather than fatal.
// One spelling, two policies, no per-function #if.
#if (D_INTERNAL_MEM_STRICT_ARGS == 1)
#   include <assert.h>
#   define D_MEM_REQUIRE(cond, fail)                                          \
        do                                                                    \
        {                                                                     \
            assert(cond);                                                     \
            if (!(cond))                                                      \
            {                                                                 \
                return (fail);                                                \
            }                                                                 \
        } while (0)
#else
#   define D_MEM_REQUIRE(cond, fail)                                          \
        do                                                                    \
        {                                                                     \
            if (!(cond))                                                      \
            {                                                                 \
                return (fail);                                                \
            }                                                                 \
        } while (0)
#endif

// D_MEM_UNUSED
//   macro: marks a parameter that a given configuration does not read, so the
// "unused parameter" diagnostic stays on for the ones that are a real mistake.
#define D_MEM_UNUSED(x)             ((void)(x))


// I.    block view
struct d_mem_block d_mem_block_make(void*      _ptr,
                                    d_mem_size _size);
struct d_mem_block d_mem_block_empty(void);
bool               d_mem_block_is_empty(const struct d_mem_block* _block);
void*              d_mem_block_end(const struct d_mem_block* _block);
bool               d_mem_block_contains(const struct d_mem_block* _block,
                                        const void*               _ptr);

// II.   checked arithmetic
enum d_mem_status  d_mem_align_up_checked(d_mem_size  _value,
                                          d_mem_size  _align,
                                          d_mem_size* _out);
enum d_mem_status  d_mem_add_checked(d_mem_size  _a,
                                     d_mem_size  _b,
                                     d_mem_size* _out);
enum d_mem_status  d_mem_mul_checked(d_mem_size  _a,
                                     d_mem_size  _b,
                                     d_mem_size* _out);

// III.  slot geometry
d_mem_size         d_mem_slot_align(d_mem_size _align,
                                    d_mem_size _link_align);
d_mem_size         d_mem_slot_size(d_mem_size _object_size,
                                   d_mem_size _slot_align,
                                   d_mem_size _link_size);

// IV.   accounting
void               d_mem_stats_clear(struct d_mem_stats* _stats);
void               d_mem_stats_on_reserve(struct d_mem_stats* _stats,
                                          d_mem_size          _bytes);
void               d_mem_stats_on_unreserve(struct d_mem_stats* _stats,
                                            d_mem_size          _bytes);
void               d_mem_stats_on_acquire(struct d_mem_stats* _stats,
                                          d_mem_size          _bytes);
void               d_mem_stats_on_release(struct d_mem_stats* _stats,
                                          d_mem_size          _bytes);
void               d_mem_stats_on_reset(struct d_mem_stats* _stats);
double             d_mem_stats_utilization(const struct d_mem_stats* _stats);

// V.    hardening
void               d_mem_poison(void*      _ptr,
                                d_mem_size _size);
void               d_mem_poison_free(void*      _ptr,
                                     d_mem_size _size);
void               d_mem_zero(void*      _ptr,
                              d_mem_size _size);
void               d_mem_prepare(void*      _ptr,
                                 d_mem_size _size);
void               d_mem_redzone_write(void*      _payload,
                                       d_mem_size _payload_size);
bool               d_mem_redzone_verify(const void* _payload,
                                        d_mem_size  _payload_size);

// VI.   diagnostics
const char*        d_mem_status_string(enum d_mem_status _status);
bool               d_mem_status_is_ok(enum d_mem_status _status);


///////////////////////////////////////////////////////////////////////////////
///                     IX.   LAYOUT ASSERTIONS                             ///
///////////////////////////////////////////////////////////////////////////////
//   Goals section 4: one declaration, size and offset asserted in both
// dialects. Offsets are asserted whenever assertions are on, since they are
// model independent; sizes only where the data model and the counter width
// are both the ones the numbers were written for.

#if (D_INTERNAL_MEM_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_mem_block, ptr) == 0,
                "d_mem_block.ptr must lead the struct");

D_STATIC_ASSERT(offsetof(struct d_mem_stats, bytes_reserved) == 0,
                "d_mem_stats.bytes_reserved must lead the struct");

D_STATIC_ASSERT(sizeof(struct d_mem_stats) >= (6 * sizeof(d_mem_size)),
                "d_mem_stats must carry its six running counters");

#endif  // D_INTERNAL_MEM_ASSERT_LAYOUT

#if (D_INTERNAL_MEM_ASSERT_SIZES == 1)

D_STATIC_ASSERT(sizeof(struct d_mem_block) == 16,
                "d_mem_block layout drift (LP64, native counter)");

#   if (D_INTERNAL_MEM_STATS_PEAK == 1)
D_STATIC_ASSERT(sizeof(struct d_mem_stats) == 64,
                "d_mem_stats layout drift (LP64, native counter, peaks on)");
#   else
D_STATIC_ASSERT(sizeof(struct d_mem_stats) == 48,
                "d_mem_stats layout drift (LP64, native counter, peaks off)");
#   endif

#endif  // D_INTERNAL_MEM_ASSERT_SIZES


D_EXTERN_C_END


#endif  // DJINTERP_C_MEMORY_MEM_COMMON_H
