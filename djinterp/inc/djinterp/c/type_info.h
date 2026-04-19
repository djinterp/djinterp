/******************************************************************************
* djinterp [core]                                                  type_info.h
*
*  Bit-efficient type information system with comprehensive macro support.
*  Layout:
*  - base (d_type_info16 = uint16_t):
*      bits 0-3:   Kind flags (PRIMITIVE, POINTER, ARRAY, TYPEDEF) - combinable
*      bits 4-7:   Subtype (primitive ID 0-15 or compound kind 0-5)
*      bits 8-10:  Context-specific (signed for primitives, compound data 
                   otherwise)
*      bit  11:    CONST qualifier
*      bit  12:    VOLATILE qualifier
*      bit  13:    Reserved
*      bit  14:    Extended info follows (EXT)
*      bit  15:    Custom/Framework-defined type (CUSTOM)
*  - pointer (d_type_info32 = uint32_t):
*      bits 0-15:  Base d_type_info16
*      bits 16-23: Pointer depth (1-255) when POINTER flag set
*      bits 24-31: Reserved / extended flags (or storage class in 
                   `d_type_info_ex`.)
*  - extended (d_type_info_ex = uint32_t):
*      bits 0-15:  Base d_type_info16
*      bits 16-23: Pointer depth
*      bits 24-31: C storage / qualifiers (restrict, atomic, static, extern, 
                   inline, noreturn)
*  - full (d_type_info64 = uint64_t):
*      bits 0-31:  d_type_info_ex
*      bits 32-47: C++ modifiers (references, constexpr, virtual, etc.)
*      bits 48-63: Reserved / user payload
*
* 
* path:      /inc/c/type_info.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.06
******************************************************************************/

#ifndef DJINTERP_C_TYPE_INFO_
#define DJINTERP_C_TYPE_INFO_ 1

#include <stddef.h>
#include <stdint.h>


/*============================================================================*
 *                         TYPE DEFINITIONS                                   *
 *============================================================================*/

typedef uint16_t d_type_info16;     // 16-bit base descriptor
typedef uint32_t d_type_info32;     // base + ptr depth byte 
typedef uint32_t d_type_info_ex;    // base + ptr depth + ext
typedef uint64_t d_type_info64;     // 64-bit (C++) modifiers

/*============================================================================*
 *                         BIT LAYOUT DEFINITIONS                             *
 *============================================================================*/

// kind flags (bits 0-3)
#define D_TYPE_KIND_SHIFT        0u
#define D_TYPE_KIND_BITS         4u
#define D_TYPE_KIND_MASK                                                    \
    ((d_type_info16)((1u << D_TYPE_KIND_BITS) - 1u))

// subtype (bits 4-7)
#define D_TYPE_SUB_SHIFT         4u
#define D_TYPE_SUB_BITS          4u
#define D_TYPE_SUB_MASK                                                     \
    ((d_type_info16)(((1u << D_TYPE_SUB_BITS) - 1u) << D_TYPE_SUB_SHIFT))

// context-specific (bits 8-10)
#define D_TYPE_SIGNED_SHIFT      8u
#define D_TYPE_SIGNED_BIT        ((d_type_info16)(1u << D_TYPE_SIGNED_SHIFT))
#define D_TYPE_CTX0_SHIFT        8u
#define D_TYPE_CTX0_BIT          ((d_type_info16)(1u << D_TYPE_CTX0_SHIFT))
#define D_TYPE_CTX1_SHIFT        9u
#define D_TYPE_CTX1_BIT          ((d_type_info16)(1u << D_TYPE_CTX1_SHIFT))
#define D_TYPE_CTX2_SHIFT        10u
#define D_TYPE_CTX2_BIT          ((d_type_info16)(1u << D_TYPE_CTX2_SHIFT))
#define D_TYPE_CTX_MASK          ((d_type_info16)(0x7u << D_TYPE_CTX0_SHIFT))

// qualifiers (bits 11-12)
#define D_TYPE_CONST_SHIFT       11u
#define D_TYPE_CONST_BIT         ((d_type_info16)(1u << D_TYPE_CONST_SHIFT))

#define D_TYPE_VOLATILE_SHIFT    12u
#define D_TYPE_VOLATILE_BIT      ((d_type_info16)(1u << D_TYPE_VOLATILE_SHIFT))

// reserved (bit 13)
#define D_TYPE_RSV_SHIFT         13u
#define D_TYPE_RSV_BIT           ((d_type_info16)(1u << D_TYPE_RSV_SHIFT))

// extended / Custom (bits 14-15)
#define D_TYPE_EXT_SHIFT         14u
#define D_TYPE_EXT_BIT           ((d_type_info16)(1u << D_TYPE_EXT_SHIFT))

#define D_TYPE_CUSTOM_SHIFT      15u
#define D_TYPE_CUSTOM_BIT        ((d_type_info16)(1u << D_TYPE_CUSTOM_SHIFT))

// pointer depth byte (bits 16-23 of d_type_info32 / d_type_info_ex) */
#define D_TYPE_PTR_DEPTH_SHIFT   16u
#define D_TYPE_PTR_DEPTH_BITS    8u
#define D_TYPE_PTR_DEPTH_MASK_U64 ((uint64_t)0xFFu << D_TYPE_PTR_DEPTH_SHIFT)
#define D_TYPE_PTR_DEPTH_MAX     255u

// extended 32-bit flags (bits 24-31)
#define D_TYPE_RESTRICT_SHIFT    24u
#define D_TYPE_RESTRICT_BIT      (1ull << D_TYPE_RESTRICT_SHIFT)

#define D_TYPE_ATOMIC_SHIFT      25u
#define D_TYPE_ATOMIC_BIT        (1ull << D_TYPE_ATOMIC_SHIFT)

#define D_TYPE_STATIC_SHIFT      26u
#define D_TYPE_STATIC_BIT        (1ull << D_TYPE_STATIC_SHIFT)

#define D_TYPE_EXTERN_SHIFT      27u
#define D_TYPE_EXTERN_BIT        (1ull << D_TYPE_EXTERN_SHIFT)

#define D_TYPE_INLINE_SHIFT      28u
#define D_TYPE_INLINE_BIT        (1ull << D_TYPE_INLINE_SHIFT)

#define D_TYPE_NORETURN_SHIFT    29u
#define D_TYPE_NORETURN_BIT      (1ull << D_TYPE_NORETURN_SHIFT)

// C++ modifier bits (32+)
#define D_TYPE_LVALREF_SHIFT     32u
#define D_TYPE_LVALREF_BIT       (1ull << D_TYPE_LVALREF_SHIFT)

#define D_TYPE_RVALREF_SHIFT     33u
#define D_TYPE_RVALREF_BIT       (1ull << D_TYPE_RVALREF_SHIFT)

#define D_TYPE_MUTABLE_SHIFT     34u
#define D_TYPE_MUTABLE_BIT       (1ull << D_TYPE_MUTABLE_SHIFT)

#define D_TYPE_VIRTUAL_SHIFT     35u
#define D_TYPE_VIRTUAL_BIT       (1ull << D_TYPE_VIRTUAL_SHIFT)

#define D_TYPE_CONSTEXPR_SHIFT   36u
#define D_TYPE_CONSTEXPR_BIT     (1ull << D_TYPE_CONSTEXPR_SHIFT)

#define D_TYPE_NOEXCEPT_SHIFT    37u
#define D_TYPE_NOEXCEPT_BIT      (1ull << D_TYPE_NOEXCEPT_SHIFT)

#define D_TYPE_FINAL_SHIFT       38u
#define D_TYPE_FINAL_BIT         (1ull << D_TYPE_FINAL_SHIFT)

#define D_TYPE_OVERRIDE_SHIFT    39u
#define D_TYPE_OVERRIDE_BIT      (1ull << D_TYPE_OVERRIDE_SHIFT)

#define D_TYPE_EXPLICIT_SHIFT    40u
#define D_TYPE_EXPLICIT_BIT      (1ull << D_TYPE_EXPLICIT_SHIFT)

#define D_TYPE_CONSTEVAL_SHIFT   41u
#define D_TYPE_CONSTEVAL_BIT     (1ull << D_TYPE_CONSTEVAL_SHIFT)

#define D_TYPE_CONSTINIT_SHIFT   42u
#define D_TYPE_CONSTINIT_BIT     (1ull << D_TYPE_CONSTINIT_SHIFT)

#define D_TYPE_TEMPLATE_SHIFT    43u
#define D_TYPE_TEMPLATE_BIT      (1ull << D_TYPE_TEMPLATE_SHIFT)

/*===========================================================================*
 *                         X-MACRO TABLES                                    *
 *===========================================================================*/

// kind flags are combinable (stored as bit values in low nibble).
#define D_TYPE_KIND_X(X)                                                    \
    X(PRIMITIVE,  0)                                                        \
    X(POINTER,    1)                                                        \
    X(ARRAY,      2)                                                        \
    X(TYPEDEF,    3)

/* Primitive ids 0..15 (stored in subtype field). */
#define D_TYPE_PRIMITIVE_X(X)                                               \
    X(VOID,     0,  void,               0,                   0)             \
    X(BOOL,     1,  _Bool,              sizeof(_Bool),       0)             \
    X(CHAR,     2,  char,               sizeof(char),        0)             \
    X(SCHAR,    3,  signed char,        sizeof(signed char), 1)             \
    X(UCHAR,    4,  unsigned char,      sizeof(unsigned char),0)            \
    X(SHORT,    5,  short,              sizeof(short),       1)             \
    X(USHORT,   6,  unsigned short,     sizeof(unsigned short),0)           \
    X(INT,      7,  int,                sizeof(int),         1)             \
    X(UINT,     8,  unsigned int,       sizeof(unsigned int),0)             \
    X(LONG,     9,  long,               sizeof(long),        1)             \
    X(ULONG,   10,  unsigned long,      sizeof(unsigned long),0)            \
    X(LLONG,   11,  long long,          sizeof(long long),   1)             \
    X(ULLONG,  12,  unsigned long long, sizeof(unsigned long long),0)       \
    X(FLOAT,   13,  float,              sizeof(float),       1)             \
    X(DOUBLE,  14,  double,             sizeof(double),      1)             \
    X(LDOUBLE, 15,  long double,        sizeof(long double), 1)

/* Compound subtype ids (when PRIMITIVE flag is NOT set). */
#define D_TYPE_COMPOUND_X(X)                                                \
    X(NONE,      0)                                                         \
    X(STRUCT,    1)                                                         \
    X(UNION,     2)                                                         \
    X(ENUM,      3)                                                         \
    X(FUNCTION,  4)                                                         \
    X(CUSTOM,    5)

/*===========================================================================*
 *                         CONSTANTS (NO ENUMS)                              *
 *===========================================================================*/

// kind bit values (stored in bits 0-3)
#define D_TYPE_PRIMITIVE   ((d_type_info16)(1u << 0))
#define D_TYPE_POINTER     ((d_type_info16)(1u << 1))
#define D_TYPE_ARRAY       ((d_type_info16)(1u << 2))
#define D_TYPE_TYPEDEF     ((d_type_info16)(1u << 3))
#define D_TYPE_KIND_MASK_  D_TYPE_KIND_MASK  /* keep your old name */

// primitive ids
#define D_TYPE_PRIM_VOID     0u
#define D_TYPE_PRIM_BOOL     1u
#define D_TYPE_PRIM_CHAR     2u
#define D_TYPE_PRIM_SCHAR    3u
#define D_TYPE_PRIM_UCHAR    4u
#define D_TYPE_PRIM_SHORT    5u
#define D_TYPE_PRIM_USHORT   6u
#define D_TYPE_PRIM_INT      7u
#define D_TYPE_PRIM_UINT     8u
#define D_TYPE_PRIM_LONG     9u
#define D_TYPE_PRIM_ULONG    10u
#define D_TYPE_PRIM_LLONG    11u
#define D_TYPE_PRIM_ULLONG   12u
#define D_TYPE_PRIM_FLOAT    13u
#define D_TYPE_PRIM_DOUBLE   14u
#define D_TYPE_PRIM_LDOUBLE  15u
#define D_TYPE_PRIM_COUNT    16u

// compound ids
#define D_TYPE_NONE          0u
#define D_TYPE_STRUCT        1u
#define D_TYPE_UNION         2u
#define D_TYPE_ENUM          3u
#define D_TYPE_FUNCTION      4u
#define D_TYPE_CUSTOM        5u

/*===========================================================================*
 *                         PRIMITIVE SIZE/SIGNED TABLES                      *
 *===========================================================================*/

static const uint8_t d_type_prim_sizes[D_TYPE_PRIM_COUNT] = 
{
    0,                          // VOID 
    sizeof(_Bool),              // BOOL 
    sizeof(char),               // CHAR 
    sizeof(signed char),        // SCHAR
    sizeof(unsigned char),      // UCHAR
    sizeof(short),              // SHORT
    sizeof(unsigned short),     // USHORT
    sizeof(int),                // INT
    sizeof(unsigned int),       // UINT
    sizeof(long),               // LONg
    sizeof(unsigned long),      // ULONG 
    sizeof(long long),          // LLONG 
    sizeof(unsigned long long), // ULLONG 
    sizeof(float),              // FLOAT 
    sizeof(double),             // DOUBLE 
    sizeof(long double)         // LDOUBLE
};

#define D_TYPE_PRIM_SIGNED_MASK         \
    ( (uint16_t)                        \
      (                                 \
         (1u << D_TYPE_PRIM_SCHAR)   |  \
         (1u << D_TYPE_PRIM_SHORT)   |  \
         (1u << D_TYPE_PRIM_INT)     |  \
         (1u << D_TYPE_PRIM_LONG)    |  \
         (1u << D_TYPE_PRIM_LLONG)   |  \
         (1u << D_TYPE_PRIM_FLOAT)   |  \
         (1u << D_TYPE_PRIM_DOUBLE)  |  \
         (1u << D_TYPE_PRIM_LDOUBLE)    \
    ) )

/*===========================================================================*
 *                         BUILDER MACROS                                    *
 *===========================================================================*/

#define D_TYPE_MAKE(kind, subtype)                                              \
    ((d_type_info16)(                                                           \
        ((d_type_info16)((kind) & D_TYPE_KIND_MASK)) |                          \
        ((d_type_info16)(((subtype) & 0x0Fu) << D_TYPE_SUB_SHIFT))              \
    ))

#define D_TYPE_MAKE_PRIM(prim_id)                                               \
    ((d_type_info16)(                                                           \
        D_TYPE_PRIMITIVE |                                                      \
        (((prim_id) & 0x0Fu) << D_TYPE_SUB_SHIFT) |                             \
        (((D_TYPE_PRIM_SIGNED_MASK >> (prim_id)) & 1u) << D_TYPE_SIGNED_SHIFT)  \
    ))

#define D_TYPE_MAKE_COMPOUND(compound_kind)                                     \
    ((d_type_info16)(((compound_kind) & 0x0Fu) << D_TYPE_SUB_SHIFT))

/* Primitive builders (kept for your API) */
#define D_TYPE_VOID_()      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_VOID)
#define D_TYPE_BOOL_()      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_BOOL)
#define D_TYPE_CHAR_()      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_CHAR)
#define D_TYPE_SCHAR_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_SCHAR)
#define D_TYPE_UCHAR_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_UCHAR)
#define D_TYPE_SHORT_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_SHORT)
#define D_TYPE_USHORT_()    D_TYPE_MAKE_PRIM(D_TYPE_PRIM_USHORT)
#define D_TYPE_INT_()       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_INT)
#define D_TYPE_UINT_()      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_UINT)
#define D_TYPE_LONG_()      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LONG)
#define D_TYPE_ULONG_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_ULONG)
#define D_TYPE_LLONG_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LLONG)
#define D_TYPE_ULLONG_()    D_TYPE_MAKE_PRIM(D_TYPE_PRIM_ULLONG)
#define D_TYPE_FLOAT_()     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_FLOAT)
#define D_TYPE_DOUBLE_()    D_TYPE_MAKE_PRIM(D_TYPE_PRIM_DOUBLE)
#define D_TYPE_LDOUBLE_()   D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LDOUBLE)

/* Compound builders */
#define D_TYPE_STRUCT_()    D_TYPE_MAKE_COMPOUND(D_TYPE_STRUCT)
#define D_TYPE_UNION_()     D_TYPE_MAKE_COMPOUND(D_TYPE_UNION)
#define D_TYPE_ENUM_()      D_TYPE_MAKE_COMPOUND(D_TYPE_ENUM)
#define D_TYPE_FUNCTION_()  D_TYPE_MAKE_COMPOUND(D_TYPE_FUNCTION)
#define D_TYPE_CUSTOM_()    D_TYPE_MAKE_COMPOUND(D_TYPE_CUSTOM)

/*============================================================================*
 *                         MODIFIER MACROS                                    *
 *============================================================================*/

#define D_TYPE_SET_ARRAY(info16)        ((d_type_info16)((info16) | D_TYPE_ARRAY))
#define D_TYPE_SET_TYPEDEF(info16)      ((d_type_info16)((info16) | D_TYPE_TYPEDEF))
#define D_TYPE_SET_EXT(info16)          ((d_type_info16)((info16) | D_TYPE_EXT_BIT))
#define D_TYPE_SET_CUSTOM(info16)       ((d_type_info16)((info16) | D_TYPE_CUSTOM_BIT))
#define D_TYPE_SET_FRAMEWORK(info16)    D_TYPE_SET_CUSTOM(info16)

#define D_TYPE_SET_CONST(info16)        ((d_type_info16)((info16) | D_TYPE_CONST_BIT))
#define D_TYPE_SET_VOLATILE(info16)     ((d_type_info16)((info16) | D_TYPE_VOLATILE_BIT))

#define D_TYPE_MAKE_PTR(base16, depth)                                      \
    ((d_type_info32)(                                                       \
        ((uint32_t)((base16) & 0xFFFFu)) |                                  \
        (uint32_t)D_TYPE_POINTER |                                          \
        (((uint32_t)((depth) & 0xFFu)) << D_TYPE_PTR_DEPTH_SHIFT)           \
    ))

#define D_TYPE_GET_BASE(info)                                               \
    ((d_type_info16)((info) & 0xFFFFu))

#define D_TYPE_GET_PTR_DEPTH(info)                                          \
    ( (( (info) & D_TYPE_POINTER ) == 0)                                    \
        ? 0u                                                                \
        : ( (unsigned)(((uint64_t)(info) & D_TYPE_PTR_DEPTH_MASK_U64) >> D_TYPE_PTR_DEPTH_SHIFT) \
            ? (unsigned)(((uint64_t)(info) & D_TYPE_PTR_DEPTH_MASK_U64) >> D_TYPE_PTR_DEPTH_SHIFT) \
            : 1u ) )

#define D_TYPE_SET_PTR_DEPTH(info, depth)                                          \
    ( ((depth) == 0u)                                                              \
        ? (d_type_info32)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER)  \
        : D_TYPE_MAKE_PTR(D_TYPE_GET_BASE(info), (depth)) )

#define D_TYPE_ADD_PTR(info)                                                       \
    ( D_TYPE_IS_POINTER(info)                                                      \
        ? D_TYPE_SET_PTR_DEPTH((info),                                             \
              (D_TYPE_GET_PTR_DEPTH(info) >= D_TYPE_PTR_DEPTH_MAX                  \
                  ? D_TYPE_PTR_DEPTH_MAX                                           \
                  : (D_TYPE_GET_PTR_DEPTH(info) + 1u)))                            \
        : D_TYPE_MAKE_PTR(D_TYPE_GET_BASE(info), 1u) )

#define D_TYPE_SUB_PTR(info)                                                       \
    ( (D_TYPE_GET_PTR_DEPTH(info) > 1u)                                            \
        ? D_TYPE_SET_PTR_DEPTH((info), (D_TYPE_GET_PTR_DEPTH(info) - 1u))          \
        : (d_type_info16)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER) )

/* Extended (32-bit) modifiers */
#define D_TYPE_SET_RESTRICT(info)   ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_RESTRICT_BIT))
#define D_TYPE_SET_ATOMIC(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_ATOMIC_BIT))
#define D_TYPE_SET_STATIC(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_STATIC_BIT))
#define D_TYPE_SET_EXTERN(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_EXTERN_BIT))
#define D_TYPE_SET_INLINE(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_INLINE_BIT))
#define D_TYPE_SET_NORETURN(info)   ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_NORETURN_BIT))

#ifdef __cplusplus
#define D_TYPE_SET_LVALREF(info)    ((d_type_info64)((info) | D_TYPE_LVALREF_BIT))
#define D_TYPE_SET_RVALREF(info)    ((d_type_info64)((info) | D_TYPE_RVALREF_BIT))
#define D_TYPE_SET_MUTABLE(info)    ((d_type_info64)((info) | D_TYPE_MUTABLE_BIT))
#define D_TYPE_SET_VIRTUAL(info)    ((d_type_info64)((info) | D_TYPE_VIRTUAL_BIT))
#define D_TYPE_SET_CONSTEXPR(info)  ((d_type_info64)((info) | D_TYPE_CONSTEXPR_BIT))
#define D_TYPE_SET_NOEXCEPT(info)   ((d_type_info64)((info) | D_TYPE_NOEXCEPT_BIT))
#define D_TYPE_SET_FINAL(info)      ((d_type_info64)((info) | D_TYPE_FINAL_BIT))
#define D_TYPE_SET_OVERRIDE(info)   ((d_type_info64)((info) | D_TYPE_OVERRIDE_BIT))
#define D_TYPE_SET_EXPLICIT(info)   ((d_type_info64)((info) | D_TYPE_EXPLICIT_BIT))
#define D_TYPE_SET_CONSTEVAL(info)  ((d_type_info64)((info) | D_TYPE_CONSTEVAL_BIT))
#define D_TYPE_SET_CONSTINIT(info)  ((d_type_info64)((info) | D_TYPE_CONSTINIT_BIT))
#define D_TYPE_SET_TEMPLATE(info)   ((d_type_info64)((info) | D_TYPE_TEMPLATE_BIT))
#endif

/*============================================================================*
 *                         ACCESSORS / TESTS                                  *
 *============================================================================*/

#define D_TYPE_GET_KIND(info)       ((d_type_info16)((info) & D_TYPE_KIND_MASK))
#define D_TYPE_GET_SUB(info)        ((unsigned)(((info) & D_TYPE_SUB_MASK) >> D_TYPE_SUB_SHIFT))
#define D_TYPE_GET_PRIM(info)       D_TYPE_GET_SUB(info)
#define D_TYPE_GET_COMPOUND(info)   D_TYPE_GET_SUB(info) /* (no enum cast) */

#define D_TYPE_IS_PRIMITIVE(info)   (((info) & D_TYPE_PRIMITIVE) != 0)
#define D_TYPE_IS_POINTER(info)     (((info) & D_TYPE_POINTER) != 0)
#define D_TYPE_IS_ARRAY(info)       (((info) & D_TYPE_ARRAY) != 0)
#define D_TYPE_IS_TYPEDEF(info)     (((info) & D_TYPE_TYPEDEF) != 0)

#define D_TYPE_HAS_EXT(info)        (((info) & D_TYPE_EXT_BIT) != 0)
#define D_TYPE_IS_CUSTOM(info)      (((info) & D_TYPE_CUSTOM_BIT) != 0)
#define D_TYPE_IS_FRAMEWORK(info)   D_TYPE_IS_CUSTOM(info)

#define D_TYPE_IS_CONST(info)       (((info) & D_TYPE_CONST_BIT) != 0)
#define D_TYPE_IS_VOLATILE(info)    (((info) & D_TYPE_VOLATILE_BIT) != 0)
#define D_TYPE_IS_CV(info)          (((info) & (D_TYPE_CONST_BIT | D_TYPE_VOLATILE_BIT)) != 0)

#define D_TYPE_IS_COMPOUND(info)    (!D_TYPE_IS_PRIMITIVE(info) && (D_TYPE_GET_SUB(info) != D_TYPE_NONE))

#define D_TYPE_IS_VOID(info)        (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_VOID)
#define D_TYPE_IS_BOOL(info)        (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_BOOL)

#define D_TYPE_IS_CHAR(info)        (D_TYPE_IS_PRIMITIVE(info) && (D_TYPE_GET_PRIM(info) >= D_TYPE_PRIM_CHAR && D_TYPE_GET_PRIM(info) <= D_TYPE_PRIM_UCHAR))
#define D_TYPE_IS_CHAR_PLAIN(info)  (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_CHAR)
#define D_TYPE_IS_SCHAR(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_SCHAR)
#define D_TYPE_IS_UCHAR(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_UCHAR)

#define D_TYPE_IS_SHORT(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_SHORT)
#define D_TYPE_IS_USHORT(info)      (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_USHORT)
#define D_TYPE_IS_INT(info)         (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_INT)
#define D_TYPE_IS_UINT(info)        (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_UINT)
#define D_TYPE_IS_LONG(info)        (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_LONG)
#define D_TYPE_IS_ULONG(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_ULONG)
#define D_TYPE_IS_LLONG(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_LLONG)
#define D_TYPE_IS_ULLONG(info)      (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_ULLONG)

#define D_TYPE_IS_FLOAT(info)       (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_FLOAT)
#define D_TYPE_IS_DOUBLE(info)      (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_DOUBLE)
#define D_TYPE_IS_LDOUBLE(info)     (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) == D_TYPE_PRIM_LDOUBLE)

#define D_TYPE_IS_INTEGER(info)     (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) >= D_TYPE_PRIM_SCHAR && D_TYPE_GET_PRIM(info) <= D_TYPE_PRIM_ULLONG)
#define D_TYPE_IS_FLOATING(info)    (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) >= D_TYPE_PRIM_FLOAT)

#define D_TYPE_IS_SIGNED(info)      (D_TYPE_IS_PRIMITIVE(info) && (((info) & D_TYPE_SIGNED_BIT) != 0))
#define D_TYPE_IS_UNSIGNED(info)    (D_TYPE_IS_PRIMITIVE(info) && (((info) & D_TYPE_SIGNED_BIT) == 0))

#define D_TYPE_IS_INTEGRAL(info)    (D_TYPE_IS_BOOL(info) || D_TYPE_IS_CHAR(info) || D_TYPE_IS_INTEGER(info))
#define D_TYPE_IS_ARITHMETIC(info)  (D_TYPE_IS_INTEGRAL(info) || D_TYPE_IS_FLOATING(info))

#define D_TYPE_IS_STRUCT(info)      (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_STRUCT)
#define D_TYPE_IS_UNION(info)       (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_UNION)
#define D_TYPE_IS_ENUM(info)        (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_ENUM)
#define D_TYPE_IS_FUNCTION(info)    (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_FUNCTION)
#define D_TYPE_IS_CUSTOM_KIND(info) (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_CUSTOM)

#define D_TYPE_IS_SCALAR(info)      (D_TYPE_IS_ARITHMETIC(info) || D_TYPE_IS_POINTER(info) || D_TYPE_IS_ENUM(info))
#define D_TYPE_IS_AGGREGATE(info)   (D_TYPE_IS_ARRAY(info) || D_TYPE_IS_STRUCT(info) || D_TYPE_IS_UNION(info))

#define D_TYPE_GET_SIZE(info)                                               \
    (D_TYPE_IS_POINTER(info)                                                \
        ? sizeof(void*)                                                     \
        : (D_TYPE_IS_PRIMITIVE(info)                                        \
            ? d_type_prim_sizes[D_TYPE_GET_PRIM(info)]                      \
            : sizeof(void*)))

/*============================================================================*
 *                         PREDEFINED D_TYPE_INFO_*                            *
 *============================================================================*/

// base primitives
#define D_TYPE_INFO_VOID        D_TYPE_MAKE_PRIM(D_TYPE_PRIM_VOID)
#define D_TYPE_INFO_BOOL        D_TYPE_MAKE_PRIM(D_TYPE_PRIM_BOOL)
#define D_TYPE_INFO_CHAR        D_TYPE_MAKE_PRIM(D_TYPE_PRIM_CHAR)
#define D_TYPE_INFO_SCHAR       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_SCHAR)
#define D_TYPE_INFO_UCHAR       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_UCHAR)
#define D_TYPE_INFO_SHORT       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_SHORT)
#define D_TYPE_INFO_USHORT      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_USHORT)
#define D_TYPE_INFO_INT         D_TYPE_MAKE_PRIM(D_TYPE_PRIM_INT)
#define D_TYPE_INFO_UINT        D_TYPE_MAKE_PRIM(D_TYPE_PRIM_UINT)
#define D_TYPE_INFO_LONG        D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LONG)
#define D_TYPE_INFO_ULONG       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_ULONG)
#define D_TYPE_INFO_LLONG       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LLONG)
#define D_TYPE_INFO_ULLONG      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_ULLONG)
#define D_TYPE_INFO_FLOAT       D_TYPE_MAKE_PRIM(D_TYPE_PRIM_FLOAT)
#define D_TYPE_INFO_DOUBLE      D_TYPE_MAKE_PRIM(D_TYPE_PRIM_DOUBLE)
#define D_TYPE_INFO_LDOUBLE     D_TYPE_MAKE_PRIM(D_TYPE_PRIM_LDOUBLE)

/* CV variants */
#define D_TYPE_INFO_CONST_CHAR           D_TYPE_SET_CONST(D_TYPE_INFO_CHAR)
#define D_TYPE_INFO_CONST_STRING         D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1)
#define D_TYPE_INFO_CONST_STRING_ARRAY   D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 2)

/* Arrays */
#define D_TYPE_INFO_CHAR_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_CHAR))
#define D_TYPE_INFO_INT_ARRAY            D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT))
#define D_TYPE_INFO_BOOL_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_BOOL))

/* Pointer (depth=1) */
#define D_TYPE_INFO_VOID_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_VOID, 1)
#define D_TYPE_INFO_BOOL_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_BOOL, 1)
#define D_TYPE_INFO_CHAR_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_CHAR, 1)
#define D_TYPE_INFO_INT_PTR              D_TYPE_MAKE_PTR(D_TYPE_INFO_INT, 1)

/* “array of pointers” */
#define D_TYPE_INFO_CHAR_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_CHAR, 1)))
#define D_TYPE_INFO_INT_PTR_ARRAY        D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_INT, 1)))
#define D_TYPE_INFO_BOOL_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_BOOL, 1)))
#define D_TYPE_INFO_VOID_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_VOID, 1)))

/* Fixed-width aliases */
#define D_TYPE_INFO_INT8                 D_TYPE_INFO_SCHAR
#define D_TYPE_INFO_INT16                D_TYPE_INFO_SHORT
#define D_TYPE_INFO_INT32                D_TYPE_INFO_INT
#define D_TYPE_INFO_INT64                D_TYPE_INFO_LLONG

#define D_TYPE_INFO_UINT8                D_TYPE_INFO_UCHAR
#define D_TYPE_INFO_UINT16               D_TYPE_INFO_USHORT
#define D_TYPE_INFO_UINT32               D_TYPE_INFO_UINT
#define D_TYPE_INFO_UINT64               D_TYPE_INFO_ULLONG

#define D_TYPE_INFO_INT8_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT8))
#define D_TYPE_INFO_INT16_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT16))
#define D_TYPE_INFO_INT32_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT32))
#define D_TYPE_INFO_INT64_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT64))

#define D_TYPE_INFO_UINT8_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT8))
#define D_TYPE_INFO_UINT16_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT16))
#define D_TYPE_INFO_UINT32_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT32))
#define D_TYPE_INFO_UINT64_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT64))

#define D_TYPE_INFO_INT8_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_INT8, 1)
#define D_TYPE_INFO_INT16_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT16, 1)
#define D_TYPE_INFO_INT32_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT32, 1)
#define D_TYPE_INFO_INT64_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT64, 1)

#define D_TYPE_INFO_UINT8_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT8, 1)
#define D_TYPE_INFO_UINT16_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT16, 1)
#define D_TYPE_INFO_UINT32_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT32, 1)
#define D_TYPE_INFO_UINT64_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT64, 1)

#define D_TYPE_INFO_INT8_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT8_PTR))
#define D_TYPE_INFO_INT16_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT16_PTR))
#define D_TYPE_INFO_INT32_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT32_PTR))
#define D_TYPE_INFO_INT64_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT64_PTR))

#define D_TYPE_INFO_UINT8_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT8_PTR))
#define D_TYPE_INFO_UINT16_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT16_PTR))
#define D_TYPE_INFO_UINT32_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT32_PTR))
#define D_TYPE_INFO_UINT64_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT64_PTR))

/* string / string array (your semantics: char const* and char const**) */
#define D_TYPE_INFO_STRING               D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1) /* const char * */
#define D_TYPE_INFO_STRING_ARRAY         D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 2) /* const char ** */
#define D_TYPE_INFO_STRING_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1))) /* const char*[] */

/* size_t (correct width) */
#if defined(SIZE_MAX) && defined(UINT64_MAX) && (SIZE_MAX == UINT64_MAX)
    #define D_TYPE_INFO_SIZE_T           D_TYPE_INFO_UINT64
    #define D_TYPE_INFO_SIZE_T_PTR       D_TYPE_INFO_UINT64_PTR
    #define D_TYPE_INFO_SIZE_T_ARRAY     D_TYPE_INFO_UINT64_ARRAY
    #define D_TYPE_INFO_SIZE_T_PTR_ARRAY D_TYPE_INFO_UINT64_PTR_ARRAY
#elif defined(SIZE_MAX) && defined(UINT32_MAX) && (SIZE_MAX == UINT32_MAX)
    #define D_TYPE_INFO_SIZE_T           D_TYPE_INFO_UINT32
    #define D_TYPE_INFO_SIZE_T_PTR       D_TYPE_INFO_UINT32_PTR
    #define D_TYPE_INFO_SIZE_T_ARRAY     D_TYPE_INFO_UINT32_ARRAY
    #define D_TYPE_INFO_SIZE_T_PTR_ARRAY D_TYPE_INFO_UINT32_PTR_ARRAY
#else
    /* Fallback: treat as uintptr_t-ish */
    #define D_TYPE_INFO_SIZE_T           D_TYPE_INFO_UINT64
    #define D_TYPE_INFO_SIZE_T_PTR       D_TYPE_INFO_UINT64_PTR
    #define D_TYPE_INFO_SIZE_T_ARRAY     D_TYPE_INFO_UINT64_ARRAY
    #define D_TYPE_INFO_SIZE_T_PTR_ARRAY D_TYPE_INFO_UINT64_PTR_ARRAY
#endif

/*============================================================================*
 *                         PREDEFINED TYPE CONSTANTS (FOR _Generic)           *
 *============================================================================*/

#define D_INTERNAL_PRIM_CONST(name, id, ctype, sz, sgn) \
    static const d_type_info16 D_TYPE_C_##name = D_TYPE_MAKE_PRIM(id);

D_TYPE_PRIMITIVE_X(D_INTERNAL_PRIM_CONST)

#undef D_INTERNAL_PRIM_CONST

static const d_type_info16 D_TYPE_C_STRUCT   = D_TYPE_STRUCT_();
static const d_type_info16 D_TYPE_C_UNION    = D_TYPE_UNION_();
static const d_type_info16 D_TYPE_C_ENUM     = D_TYPE_ENUM_();
static const d_type_info16 D_TYPE_C_FUNCTION = D_TYPE_FUNCTION_();
static const d_type_info16 D_TYPE_C_CUSTOM   = D_TYPE_CUSTOM_();

/*============================================================================*
 *                         _GENERIC TYPE DETECTION (C11)                      *
 *============================================================================*/

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

#define D_TYPE_OF_EXPR(x) _Generic((x),                                     \
    _Bool:              D_TYPE_C_BOOL,                                      \
    char:               D_TYPE_C_CHAR,                                      \
    signed char:        D_TYPE_C_SCHAR,                                     \
    unsigned char:      D_TYPE_C_UCHAR,                                     \
    short:              D_TYPE_C_SHORT,                                     \
    unsigned short:     D_TYPE_C_USHORT,                                    \
    int:                D_TYPE_C_INT,                                       \
    unsigned int:       D_TYPE_C_UINT,                                      \
    long:               D_TYPE_C_LONG,                                      \
    unsigned long:      D_TYPE_C_ULONG,                                     \
    long long:          D_TYPE_C_LLONG,                                     \
    unsigned long long: D_TYPE_C_ULLONG,                                    \
    float:              D_TYPE_C_FLOAT,                                     \
    double:             D_TYPE_C_DOUBLE,                                    \
    long double:        D_TYPE_C_LDOUBLE,                                   \
    default:            (d_type_info16)0)

#define D_TYPE_OF_TYPE(T)   D_TYPE_OF_EXPR((T)0)

#endif /* C11 */

/*============================================================================*
 *                         COMPOSITE TYPE BUILDERS                            *
 *============================================================================*/

#define D_TYPE_PTR_TO(base_info)      D_TYPE_ADD_PTR(base_info)
#define D_TYPE_ARRAY_OF(elem_info)   D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(elem_info))
#define D_TYPE_CONST_OF(info)        D_TYPE_SET_CONST(info)
#define D_TYPE_VOLATILE_OF(info)     D_TYPE_SET_VOLATILE(info)
#define D_TYPE_CV_OF(info)           D_TYPE_SET_CONST(D_TYPE_SET_VOLATILE(info))
#define D_TYPE_CUSTOM_OF(info)       D_TYPE_SET_CUSTOM(info)
#define D_TYPE_FW(info)              D_TYPE_SET_CUSTOM(info)

/* Builders you already used */
#define D_TYPE_PTR_ARRAY(elem_info, depth) \
    D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR((elem_info), (depth))))

#define D_TYPE_CONST_PTR(elem_info)  D_TYPE_ADD_PTR(D_TYPE_SET_CONST(elem_info)) /* const T * */
#define D_TYPE_PTR_CONST(elem_info)  D_TYPE_SET_CONST(D_TYPE_ADD_PTR(elem_info)) /* T * const */
#define D_TYPE_TYPEDEF_OF(info)      D_TYPE_SET_TYPEDEF(info)

/*============================================================================*
 *                         EXTENDED INFO STRUCTURES                           *
 *============================================================================*/

struct d_type_array_ext
{
    d_type_info16 base;     // base info (with ARRAY flag)`
    uint8_t       ndims;    // number of dimensions
    uint32_t      dims[1];  // flexible dims
};

struct d_type_struct_field
{
    uint16_t      offset;
    uint16_t      size;
    d_type_info16 type;
};

struct d_type_struct_ext
{
    d_type_info16              base;
    uint16_t                   total_size;
    uint8_t                    nfields;
    struct d_type_struct_field fields[1];
};

struct d_type_func_ext
{
    d_type_info16 base;
    d_type_info16 ret_type;
    uint8_t       nparam;
    d_type_info16 params[1];
};

/*============================================================================*
 *                         UTILITY / DEBUG                                    *
 *============================================================================*/

#define D_TYPE_STRIP_CV(info)  \
    ((d_type_info16)((info) & ~(D_TYPE_CONST_BIT | D_TYPE_VOLATILE_BIT)))

#define D_TYPE_STRIP_PTR(info)        D_TYPE_SUB_PTR(info)
#define D_TYPE_STRIP_ALL_PTR(info)    ((d_type_info16)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER))

#define D_TYPE_BASE(info)  \
    ((d_type_info16)((info) & (D_TYPE_PRIMITIVE | D_TYPE_SUB_MASK | D_TYPE_SIGNED_BIT)))

#define D_TYPE_EQ(a, b)          ((a) == (b))
#define D_TYPE_EQ_BASE(a, b)     (((a) & 0xFFFFu) == ((b) & 0xFFFFu))
#define D_TYPE_COMPAT(a, b)      (D_TYPE_BASE(a) == D_TYPE_BASE(b))

#define D_TYPE_TO_BASE(info)     ((d_type_info16)((info) & 0xFFFFu))
#define D_TYPE_TO_PTR(info)      ((d_type_info32)(info))

#define D_TYPE_DUMP_BITS(info)                                       \
{                                                                    \
    (unsigned)D_TYPE_GET_KIND(info),                                 \
    (unsigned)D_TYPE_GET_SUB(info),                                  \
    (unsigned)D_TYPE_GET_PTR_DEPTH(info),                            \
    (unsigned)(D_TYPE_IS_PRIMITIVE(info) && D_TYPE_IS_SIGNED(info)), \
    (unsigned)D_TYPE_IS_CONST(info),                                 \
    (unsigned)D_TYPE_IS_VOLATILE(info),                              \
    (unsigned)D_TYPE_HAS_EXT(info),                                  \
    (unsigned)D_TYPE_IS_CUSTOM(info)                                 \
}


#endif  // DJINTERP_C_TYPE_INFO_
