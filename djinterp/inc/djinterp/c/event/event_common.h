/*******************************************************************************
* djinterp [c]                                                    event_common.h
*
* Event foundations -- the shared C core (tier 0):
*   The alphabet layer of the event system, declared once in C and compiled by
* both languages. Defines the erasure key set K, the verdict set
* P = {pass, consume}, the payload block A_e, the occurrence pair (e, a), and
* the two identity scalars (event key kappa-image, handler id). Every layout in
* this file is asserted; the C++ face wraps these declarations and adds no
* members.
*
*   This header carries NO dependency on the C++ face and NO dependency on an
* allocator. It is compilable at the C99 floor.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   alphabet   Sigma            -- the open set of declared event summands
*   event type e                -- one summand, named by a d_event_key
*   erasure    kappa : T_e -> K -- d_event_key_of_name / the declared key
*   key set    K                -- d_event_key (uint64_t)
*   payload    A_e              -- struct d_event_payload (bytes + arity)
*   occurrence (e, a)           -- struct d_event_occurrence
*   verdict    P = {pass,consume} -- enum d_verdict; consume is the left zero
*                                    of handler sequencing (event_handler_
*                                    common.h)
*
* OPEN (AGENT_README.md section 9, questions 1 and 3) -- do not treat as
* settled:
*   The C++ face derived kappa from the address of a per-type static
* (event_table.hpp internal::type_key). That value is session-local: it differs
* between runs, between translation units, and between languages, so any output
* that names a key breaks the parity law for environmental reasons. This header
* offers a deterministic content-derived alternative (d_event_key_of_name, an
* FNV-1a-64 over the declared event name) so that C and C++ can agree, but the
* choice between session-local handles and content-derived hashes is listed as
* an open question and belongs in the note, not here.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_COMMON_H
#define DJINTERP_C_EVENT_EVENT_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../dmacro.h"
#include "../../config/core/event/cfg_event_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///        I.    IDENTITY SCALARS                                           ///
///////////////////////////////////////////////////////////////////////////////

// d_event_key
//   type: the image of the erasure kappa : T_e -> K. Names one summand of the
// alphabet Sigma. Fixed at 64 bits for byte-level determinacy: the width is
// part of the wire format, so it is not configurable.
typedef uint64_t d_event_key;

// d_handler_id
//   type: the named letter of a registry word -- the handle returned by bind
// and consumed by unbind, enable, and disable. Zero is the null sentinel.
typedef uint64_t d_handler_id;

// d_event_id
//   type: retired spelling of d_event_key, retained so existing call sites
// compile. NOTE: the pre-core spelling was `D_EVENT_ID_TYPE` defaulting to
// `int`; the key is now unconditionally 64-bit and D_EVENT_ID_TYPE is no
// longer honoured. Narrowing conversions at existing call sites are the
// migration cost.
typedef d_event_key d_event_id;

// D_HANDLER_ID_NULL
//   constant: the invalid handler id. bind never returns it; every query
// against it fails.
#define D_HANDLER_ID_NULL           ((d_handler_id)0)

// D_EVENT_KEY_NONE
//   constant: the key that names no summand of Sigma. Reserved; never
// produced by d_event_key_of_name.
#define D_EVENT_KEY_NONE            ((d_event_key)0)

// D_EVENT_INDEX_NONE
//   constant: the null entry index in the table's chains. Chains link by
// index rather than by pointer so that a table is relocatable, memcpy-able,
// and fwrite-able (AGENT_README.md section 8, the offsets-not-pointers
// decision).
#define D_EVENT_INDEX_NONE          ((uint32_t)0xFFFFFFFFu)


///////////////////////////////////////////////////////////////////////////////
///        II.   THE VERDICT SET P                                          ///
///////////////////////////////////////////////////////////////////////////////
// Values are pinned and members that store a verdict are declared int32_t,
// following the pattern established by the archive/compress core: an enum's
// underlying type is implementation-defined in C, and a byte-determinate
// record may not depend on that choice.

// d_verdict
//   enum: the two-point verdict set P returned by every handler step. `pass`
// lets dispatch continue to the next letter of the effective word; `consume`
// cuts off the remainder for the current occurrence. In the handler monoid
// `consume` is the left zero and the unit `skip` always yields `pass`.
enum d_verdict
{
    D_VERDICT_PASS    = 0,
    D_VERDICT_CONSUME = 1
};

// D_VERDICT_IS_CONSUMED
//   macro: true if a verdict code halts propagation. Takes the pinned code
// (int32_t) rather than the enum, so it reads a stored member directly.
#define D_VERDICT_IS_CONSUMED(_v)                                              \
    ( (int32_t)(_v) == (int32_t)D_VERDICT_CONSUME )

// d_verdict_consumed
//   function: true if the verdict code halts propagation. The function form
// exists for use as a value; the macro exists for use in a hot loop.
D_STATIC_INLINE bool
d_verdict_consumed(int32_t _verdict)
{
    return D_VERDICT_IS_CONSUMED(_verdict);
}


///////////////////////////////////////////////////////////////////////////////
///        III.  THE PAYLOAD A_e                                            ///
///////////////////////////////////////////////////////////////////////////////
// The core never interprets payload bytes. Its only obligation is the fibre
// condition: an erased payload is touched only by code selected by its own
// key, which holds because bind and dispatch key by the same kappa. The size
// is carried because the queue must copy the payload by value in order to
// defer it, and a byte copy is the only copy available in C.

// d_event_payload
//   struct: a view of one occurrence's payload block. `data` is borrowed and
// caller-owned for immediate dispatch; the queue copies the bytes.
//   INVARIANT: the block pointed to by `data` must be a standard-layout
// aggregate whose representation both languages agree on. std::tuple is NOT
// an admissible block: libstdc++ stores its elements in reverse declaration
// order, so a C++ payload lowered as a tuple cannot be read by C. See
// AGENT_README.md section 5, "things that are defects, not tradeoffs".
struct d_event_payload
{
    void*    data;   // borrowed pointer to the packed payload block
    size_t   size;   // size of the block in bytes; 0 for an empty payload
    uint32_t arity;  // number of value domains the block carries
    uint32_t flags;  // reserved; must be 0
};

// d_event_occurrence
//   struct: the pair (e, a) -- one occurrence of one event type, carrying its
// payload. This is what a queue slot rehydrates to and what dispatch consumes.
struct d_event_occurrence
{
    d_event_key            key;
    struct d_event_payload payload;
};

// D_EVENT_PAYLOAD_EMPTY
//   macro: initializer for the empty payload (arity 0, no bytes). The correct
// argument for an event declared with no value domains.
#define D_EVENT_PAYLOAD_EMPTY                                                 \
    {                                                                         \
        NULL, (size_t)0, (uint32_t)0, (uint32_t)0                             \
    }

// d_event_payload_make
//   function: builds a payload view over a caller-owned block.
D_STATIC_INLINE struct d_event_payload
d_event_payload_make(void*    _data,
                     size_t   _size,
                     uint32_t _arity)
{
    struct d_event_payload payload;

    payload.data  = _data;
    payload.size  = _size;
    payload.arity = _arity;
    payload.flags = 0u;

    return payload;
}

// d_event_occurrence_make
//   function: builds an occurrence from a key and a payload view.
D_STATIC_INLINE struct d_event_occurrence
d_event_occurrence_make(d_event_key            _key,
                        struct d_event_payload _payload)
{
    struct d_event_occurrence occurrence;

    occurrence.key     = _key;
    occurrence.payload = _payload;

    return occurrence;
}


///////////////////////////////////////////////////////////////////////////////
///        IV.   THE ERASURE kappa                                          ///
///////////////////////////////////////////////////////////////////////////////
// A content-derived key: FNV-1a-64 over the declared event name. Deterministic
// across runs, translation units, languages, and byte orders, which the
// address-of-static form is not. Collision-bearing, which is the acknowledged
// cost of question 3 in AGENT_README.md section 9.

// D_INTERNAL_EVENT_U64
//   macro (internal): the uint64_t whose high and low 32-bit halves are _hi
// and _lo. A 64-bit literal is `long long` wherever `long` has 32 bits, and
// ISO C++98 has no `long long`; two unsigned long halves are the same value
// at every level.
#define D_INTERNAL_EVENT_U64(_hi, _lo)                                        \
    ( ((uint64_t)(_hi) << 32) | (uint64_t)(_lo) )

// D_EVENT_FNV1A64_OFFSET / D_EVENT_FNV1A64_PRIME
//   constant: the FNV-1a 64-bit basis (14695981039346656037) and prime
// (1099511628211).
#define D_EVENT_FNV1A64_OFFSET                                                \
    D_INTERNAL_EVENT_U64(0xcbf29ce4UL, 0x84222325UL)
#define D_EVENT_FNV1A64_PRIME                                                 \
    D_INTERNAL_EVENT_U64(0x00000100UL, 0x000001b3UL)

// d_event_key_of_name
//   function: kappa for a named event summand -- FNV-1a-64 over the name's
// bytes. Never returns D_EVENT_KEY_NONE: a name that hashes to zero is
// remapped to 1, so the reserved value stays reserved.
D_STATIC_INLINE d_event_key
d_event_key_of_name(const char* _name)
{
    uint64_t hash;
    size_t   i;

    if (!_name)
    {
        return D_EVENT_KEY_NONE;
    }

    hash = D_EVENT_FNV1A64_OFFSET;
    i    = 0u;

    while (_name[i] != '\0')
    {
        hash ^= (uint64_t)(unsigned char)_name[i];
        hash *= D_EVENT_FNV1A64_PRIME;

        ++i;
    }

    return (d_event_key)((hash == 0u) ? 1u : hash);
}

// d_event_key_hash
//   function: maps a key onto a bucket index. Mixes the key VALUE rather than
// its bytes, so the bucket assignment is identical on big- and little-endian
// hosts -- a byte-wise hash makes cross-key iteration order platform
// dependent, and iteration order is observable in a statistics dump.
D_STATIC_INLINE size_t
d_event_key_hash(d_event_key _key,
                 size_t      _bucket_count)
{
    uint64_t mixed;

    if (_bucket_count == 0u)
    {
        return 0u;
    }

    // splitmix64 finalizer: full avalanche, endian independent, no table
    mixed = _key + D_INTERNAL_EVENT_U64(0x9E3779B9UL, 0x7F4A7C15UL);
    mixed = (mixed ^ (mixed >> 30))
            * D_INTERNAL_EVENT_U64(0xBF58476DUL, 0x1CE4E5B9UL);
    mixed = (mixed ^ (mixed >> 27))
            * D_INTERNAL_EVENT_U64(0x94D049BBUL, 0x133111EBUL);
    mixed =  mixed ^ (mixed >> 31);

    return (size_t)(mixed % (uint64_t)_bucket_count);
}

// d_event_next_prime
//   function: the smallest prime greater than or equal to _n, used to size a
// bucket array. Carried over from the pre-core table unchanged.
size_t d_event_next_prime(size_t _n);


///////////////////////////////////////////////////////////////////////////////
///        V.    DECLARING A SUMMAND OF Sigma                               ///
///////////////////////////////////////////////////////////////////////////////
//   These macros live in tier 0, not in either face, because what they
// generate IS A LAYOUT -- the packed block that carries A_e -- and a layout is
// declared once and compiled by both languages. The C face and the C++ face
// both expand the same macro, so an event declared in one language has the
// same block, the same field offsets, the same arity, and the same key in the
// other.
//   This is the direct fix for the sharpest conformance defect in the
// pre-core module: the C++ face used std::tuple as the lowered payload and
// passed a pointer to it across the erasure boundary. libstdc++ stores tuple
// elements in REVERSE declaration order, so a C reader of those bytes gets the
// fields backwards -- and the standard fixes no order at all, so there is no
// correct C reader to write. A generated aggregate has none of that freedom.

//   The tier below is SELECTED IN CONFIG, not here. cfg_event_common.h owns
// the detection (it probes dmacro's generated D_INTERNAL_INC_ table rather
// than the macro that consumes it) and publishes the answer as
// D_INTERNAL_EVENT_ITERATION_DMACRO. This header only reads it -- config
// resolution in a module header is the localization rule's one prohibition.
//
//   The two tiers generate THE SAME MEMBERS -- _field_0, _field_1, ... in
// declaration order -- so a payload block has one layout regardless of which
// is in force. Only the arity ceiling differs, which is the tier law working
// as intended: a symbol either compiles on a tier or is absent from it, and
// no tier changes what a shared type IS.

// both tiers take the payload types as a variadic list, so without variadic
// macros (ISO strict C++98) neither exists, and D_EVENT_DECLARE with them
#if D_ENV_PP_HAS_VARIADIC_MACROS

#if (D_INTERNAL_EVENT_ITERATION_DMACRO == 1)

    // D_INTERNAL_EVENT_ARITY
    //   macro: number of value domains, via dmacro's counter.
    #define D_INTERNAL_EVENT_ARITY(...)                                       \
        D_VARG_COUNT(__VA_ARGS__)


    // D_INTERNAL_EVENT_FIELD
    //   macro: expands one value domain into a member of the packed block.
    #define D_INTERNAL_EVENT_FIELD(_index, _type)                             \
        _type D_CONCAT(_field_, _index);

    // D_INTERNAL_EVENT_FIELDS
    //   macro: expands every value domain, via dmacro's indexed iteration.
    #define D_INTERNAL_EVENT_FIELDS(...)                                      \
        D_FOR_EACH_INDEXED(D_INTERNAL_EVENT_FIELD, __VA_ARGS__)

#else

    // D_INTERNAL_EVENT_ARITY_PICK
    //   macro: positional selector for the fallback counter and expander.
    #define D_INTERNAL_EVENT_ARITY_PICK(                                      \
        _1,  _2,  _3,  _4,  _5,  _6,                                          \
        _7,  _8,  _9, _10, _11, _12, _N, ...) _N

    // D_INTERNAL_EVENT_ARITY
    //   macro: number of value domains, counted locally.
    #define D_INTERNAL_EVENT_ARITY(...)                                       \
        D_INTERNAL_EVENT_ARITY_PICK(__VA_ARGS__,                              \
            12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)


    // D_INTERNAL_EVENT_FIELDS_n
    //   macro: expands n value domains into n members. Members are named
    // _field_0, _field_1, ... in declaration order -- positionally, because
    // A_e IS a position-indexed product, and naming the positions after their
    // meaning would put meaning into the notation.
    #define D_INTERNAL_EVENT_FIELDS_1(_t0)                                    \
        _t0 _field_0;
    #define D_INTERNAL_EVENT_FIELDS_2(_t0, _t1)                               \
        D_INTERNAL_EVENT_FIELDS_1(_t0)   _t1  _field_1;
    #define D_INTERNAL_EVENT_FIELDS_3(_t0, _t1, _t2)                          \
        D_INTERNAL_EVENT_FIELDS_2(_t0, _t1)   _t2  _field_2;
    #define D_INTERNAL_EVENT_FIELDS_4(_t0, _t1, _t2, _t3)                     \
        D_INTERNAL_EVENT_FIELDS_3(_t0, _t1, _t2)   _t3  _field_3;
    #define D_INTERNAL_EVENT_FIELDS_5(_t0, _t1, _t2, _t3, _t4)                \
        D_INTERNAL_EVENT_FIELDS_4(_t0, _t1, _t2, _t3)   _t4  _field_4;
    #define D_INTERNAL_EVENT_FIELDS_6(_t0, _t1, _t2, _t3, _t4, _t5)           \
        D_INTERNAL_EVENT_FIELDS_5(_t0, _t1, _t2, _t3, _t4)   _t5  _field_5;
    #define D_INTERNAL_EVENT_FIELDS_7(_t0, _t1, _t2, _t3, _t4, _t5, _t6)      \
        D_INTERNAL_EVENT_FIELDS_6(_t0, _t1, _t2, _t3, _t4, _t5)  _t6 _field_6;
    #define D_INTERNAL_EVENT_FIELDS_8(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7) \
        D_INTERNAL_EVENT_FIELDS_7(_t0, _t1, _t2, _t3, _t4, _t5, _t6)          \
        _t7 _field_7;
    #define D_INTERNAL_EVENT_FIELDS_9(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7, \
                                      _t8)                                    \
        D_INTERNAL_EVENT_FIELDS_8(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7)     \
        _t8 _field_8;
    #define D_INTERNAL_EVENT_FIELDS_10(_t0, _t1, _t2, _t3, _t4, _t5, _t6,     \
                                       _t7, _t8, _t9)                         \
        D_INTERNAL_EVENT_FIELDS_9(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7,     \
                                  _t8)                                        \
        _t9 _field_9;
    #define D_INTERNAL_EVENT_FIELDS_11(_t0, _t1, _t2, _t3, _t4, _t5, _t6,     \
                                       _t7, _t8, _t9, _t10)                   \
        D_INTERNAL_EVENT_FIELDS_10(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7,    \
                                   _t8, _t9)                                  \
        _t10 _field_10;
    #define D_INTERNAL_EVENT_FIELDS_12(_t0, _t1, _t2, _t3, _t4, _t5, _t6,     \
                                       _t7, _t8, _t9, _t10, _t11)             \
        D_INTERNAL_EVENT_FIELDS_11(_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7,    \
                                   _t8, _t9, _t10)                            \
        _t11 _field_11;

    // D_INTERNAL_EVENT_FIELDS
    //   macro: selects the expander for the given number of value domains.
    // The selector is positional, so no arithmetic and no token pasting on a
    // computed value is involved.
    #define D_INTERNAL_EVENT_FIELDS(...)                                      \
        D_INTERNAL_EVENT_ARITY_PICK(__VA_ARGS__,                              \
            D_INTERNAL_EVENT_FIELDS_12, D_INTERNAL_EVENT_FIELDS_11,           \
            D_INTERNAL_EVENT_FIELDS_10, D_INTERNAL_EVENT_FIELDS_9,            \
            D_INTERNAL_EVENT_FIELDS_8,  D_INTERNAL_EVENT_FIELDS_7,            \
            D_INTERNAL_EVENT_FIELDS_6,  D_INTERNAL_EVENT_FIELDS_5,            \
            D_INTERNAL_EVENT_FIELDS_4,  D_INTERNAL_EVENT_FIELDS_3,            \
            D_INTERNAL_EVENT_FIELDS_2,  D_INTERNAL_EVENT_FIELDS_1)(__VA_ARGS__)

#endif  // D_INTERNAL_EVENT_ITERATION_DMACRO

#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_EVENT_ARITY_MAX
//   constant: the greatest declarable event arity on this tier, as resolved
// by cfg_event_common.h. Declaring past it fails to compile rather than
// silently dropping members, which is the right failure: a truncated block is
// a different layout, not a smaller one.
#define D_EVENT_ARITY_MAX           D_INTERNAL_EVENT_ARITY_MAX


// D_INTERNAL_EVENT_KEY_EXPR
//   macro: how D_EVENT_DECLARE derives kappa, as selected by
// D_CFG_EVENT_KEY_POLICY and resolved into D_INTERNAL_EVENT_KEY_FROM_NAME by
// cfg_event_common.h.
//   Under the name-hash policy (the default) the key is FNV-1a-64 over the
// declared name -- identical in C and C++, across translation units, runs, and
// byte orders, at the cost of admitting collisions. Under the explicit policy
// D_EVENT_DECLARE still compiles and still derives from the name, but the
// house form becomes D_EVENT_DECLARE_KEYED and the project supplies its own
// ids. Neither policy removes a macro; framework_goals.md section 13 lists
// this as OPEN, and an open question is answered with a knob, not a fait
// accompli in a header.
#if (D_INTERNAL_EVENT_KEY_FROM_NAME == 1)
    #define D_INTERNAL_EVENT_KEY_EXPR(_name)                                  \
        d_event_key_of_name(D_STRINGIFY(_name))
#else
    #define D_INTERNAL_EVENT_KEY_EXPR(_name)                                  \
        d_event_key_of_name(D_STRINGIFY(_name))
#endif

// D_EVENT_PAYLOAD_TYPE
//   macro: the packed payload block type of a declared summand. Spelled
// `struct ...` in full, so that being a struct stays visible in C.
#define D_EVENT_PAYLOAD_TYPE(_name)                                           \
    struct D_CONCAT(d_event_payload_, _name)

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_EVENT_DECLARE
//   macro: declares one summand of the alphabet -- its packed block, its
// arity, its name, its key, and a view constructor. Usage:
//     D_EVENT_DECLARE(on_resize, int, int);
//     D_EVENT_PAYLOAD_TYPE(on_resize) block = { 800, 600 };
//     D_EVENT_FIRE(&dispatcher, on_resize, &block);
#define D_EVENT_DECLARE(_name, ...)                                           \
    D_EVENT_PAYLOAD_TYPE(_name)                                               \
    {                                                                         \
        D_INTERNAL_EVENT_FIELDS(__VA_ARGS__)                                  \
    };                                                                        \
                                                                              \
    enum                                                                      \
    {                                                                         \
        D_CONCAT(d_event_arity_, _name) =                                     \
            (int)D_INTERNAL_EVENT_ARITY(__VA_ARGS__)                          \
    };                                                                        \
                                                                              \
    D_STATIC_INLINE const char*                                               \
    D_CONCAT(d_event_name_, _name)(void)                                      \
    {                                                                         \
        return D_STRINGIFY(_name);                                            \
    }                                                                         \
                                                                              \
    D_STATIC_INLINE d_event_key                                               \
    D_CONCAT(d_event_key_, _name)(void)                                       \
    {                                                                         \
        return D_INTERNAL_EVENT_KEY_EXPR(_name);                              \
    }                                                                         \
                                                                              \
    D_STATIC_INLINE struct d_event_payload                                    \
    D_CONCAT(d_event_view_, _name)(D_EVENT_PAYLOAD_TYPE(_name)* _block)       \
    {                                                                         \
        return d_event_payload_make(                                          \
            (void*)_block,                                                    \
            sizeof(D_EVENT_PAYLOAD_TYPE(_name)),                              \
            (uint32_t)D_CONCAT(d_event_arity_, _name));                       \
    }                                                                         \
                                                                              \
    typedef int D_CONCAT(d_event_declared_, _name)
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_EVENT_DECLARE_EMPTY
//   macro: declares a summand carrying no payload. No block type is
// generated: C has no zero-sized object, and a one-byte placeholder would
// make an empty payload cost a byte on the wire. An empty occurrence is
// spelled with a NULL data pointer and size 0.
#define D_EVENT_DECLARE_EMPTY(_name)                                          \
    enum                                                                      \
    {                                                                         \
        D_CONCAT(d_event_arity_, _name) = 0                                   \
    };                                                                        \
                                                                              \
    D_STATIC_INLINE const char*                                               \
    D_CONCAT(d_event_name_, _name)(void)                                      \
    {                                                                         \
        return D_STRINGIFY(_name);                                            \
    }                                                                         \
                                                                              \
    D_STATIC_INLINE d_event_key                                               \
    D_CONCAT(d_event_key_, _name)(void)                                       \
    {                                                                         \
        return D_INTERNAL_EVENT_KEY_EXPR(_name);                              \
    }                                                                         \
                                                                              \
    typedef int D_CONCAT(d_event_declared_, _name)

// D_EVENT_KEY / D_EVENT_NAME / D_EVENT_ARITY / D_EVENT_PAYLOAD
//   macro: accessors for a declared summand.
#define D_EVENT_KEY(_name)          D_CONCAT(d_event_key_, _name)()
#define D_EVENT_NAME(_name)         D_CONCAT(d_event_name_, _name)()
#define D_EVENT_ARITY(_name)                                                  \
    ((uint32_t)D_CONCAT(d_event_arity_, _name))
#define D_EVENT_PAYLOAD(_name, _block)                                        \
    D_CONCAT(d_event_view_, _name)(_block)


///////////////////////////////////////////////////////////////////////////////
///        VI.   SIZING CONSTANTS -- moved                                  ///
///////////////////////////////////////////////////////////////////////////////
//   The table and queue capacities, the rehash threshold, and the growth
// factor used to be literal constants here. They are now knobs, and they live
// with the module that owns them:
//     D_EVENT_TABLE_DEFAULT_CAPACITY, _DEFAULT_BUCKETS, _LOAD_FACTOR_NUM,
//     _LOAD_FACTOR_DEN, _GROWTH_FACTOR   -> event_table_common.h
//     D_EVENT_QUEUE_DEFAULT_CAPACITY, _DEFAULT_BYTES
//                                        -> event_dispatcher_common.h
//   Each is now a read of a D_INTERNAL_* value resolved in config. A default
// living in a module header is a default nobody can override without editing
// the tree, which is the situation the dconfig cascade exists to end.

///////////////////////////////////////////////////////////////////////////////
///        VII.  STATUS CODES                                               ///
///////////////////////////////////////////////////////////////////////////////
// A formal failure and a mechanical failure are different things and are
// reported differently (AGENT_README.md section 10). D_EVENT_ERR_CAPACITY is
// mechanical: the arena is full. D_EVENT_ERR_KEY is formal: the key names no
// summand of the alphabet in force.

// d_event_status
//   enum: outcome of a table or queue operation. Pinned values; stored as
// int32_t wherever a record carries one.
enum d_event_status
{
    D_EVENT_OK           =  0,
    D_EVENT_ERR_NULL     = -1,  // a required argument was NULL
    D_EVENT_ERR_CAPACITY = -2,  // mechanical: arena or bucket array full
    D_EVENT_ERR_ALLOC    = -3,  // mechanical: allocation failed
    D_EVENT_ERR_NOT_FOUND= -4,  // no entry with the given handler id
    D_EVENT_ERR_KEY      = -5,  // formal: key names no summand of Sigma
    D_EVENT_ERR_STATE    = -6   // formal: operation undefined in this state
};


///////////////////////////////////////////////////////////////////////////////
///        VIII. LAYOUT ASSERTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////
// One declaration, asserted in both dialects. The assertions are gated on the
// 64-bit data model rather than being unconditional: on a tier where the model
// differs the sizes differ legitimately, and a hard failure there would breach
// "degrade, never error". The offsets are asserted unconditionally because
// they are model independent.

#if (D_INTERNAL_EVENT_ASSERT_SIZES == 1)

    D_STATIC_ASSERT(sizeof(struct d_event_payload) == 24,
                    "d_event_payload layout drift");
    D_STATIC_ASSERT(sizeof(struct d_event_occurrence) == 32,
                    "d_event_occurrence layout drift");

#endif  // D_INTERNAL_EVENT_ASSERT_SIZES

#if (D_INTERNAL_EVENT_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(sizeof(d_event_key) == 8,
                "d_event_key must be exactly 64 bits (wire format)");
D_STATIC_ASSERT(sizeof(d_handler_id) == 8,
                "d_handler_id must be exactly 64 bits (wire format)");
D_STATIC_ASSERT(offsetof(struct d_event_payload, data) == 0,
                "d_event_payload field drift");
D_STATIC_ASSERT(offsetof(struct d_event_occurrence, key) == 0,
                "d_event_occurrence field drift");
D_STATIC_ASSERT((int)D_VERDICT_PASS == 0,
                "verdict pass must be 0 (pinned; wire format)");
D_STATIC_ASSERT((int)D_VERDICT_CONSUME == 1,
                "verdict consume must be 1 (pinned; wire format)");

#endif  // D_INTERNAL_EVENT_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_COMMON_H
