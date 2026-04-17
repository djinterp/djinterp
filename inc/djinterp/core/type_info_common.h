/******************************************************************************
* djinterp [core]                                           type_info_common.h
*
*  Common type-information definitions shared by both the C and C++ modules.
*  Contains the base bit layout (bits 0-23), X-macro tables, constant IDs,
*  builder/modifier/accessor macros, predefined type constants, extended-info
*  structures, user type ID support, and utility helpers.
*
*  Bit layout covered here:
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
*      bits 24-31: Reserved / extended flags
*  - user type ID (d_type_info64, bits 48-63):
*      When bit 15 (CUSTOM) is set, bits 48-63 carry a 16-bit user-defined
*      type ID (0-65535).  The subtype nibble (bits 4-7) still independently
*      describes the compound shape (struct, union, enum, etc.), so a
*      descriptor can express "user struct #42" or "user enum #7".
*      When CUSTOM is clear, bits 48-63 must be zero.
*
* path:      /inc/c/type_info_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.06
******************************************************************************/

#ifndef DJINTERP_C_TYPE_INFO_COMMON_
#define DJINTERP_C_TYPE_INFO_COMMON_ 1

#include "env.h"

#include <stddef.h>
#include <stdint.h>

// in C++ `_Bool` is not a keyword; alias it to `bool` so that the
// X-macro table and size lookup compile in both languages.
#if D_ENV_LANG_USING_CPP
    typedef bool _Bool;
#endif


/*============================================================================*
 *                         TYPE DEFINITIONS                                   *
 *============================================================================*/

// d_type_info16
//   type: 16-bit base type descriptor encoding kind, subtype, signedness,
// cv-qualifiers, and control flags.
typedef uint16_t d_type_info16;

// d_type_info32
//   type: 32-bit descriptor extending the base with a pointer-depth byte
// in bits 16-23.
typedef uint32_t d_type_info32;

// d_type_info_ex
//   type: 32-bit extended descriptor; bits 24-31 carry C storage-class
// qualifiers (restrict, atomic, static, extern, inline, noreturn).
typedef uint32_t d_type_info_ex;

// d_type_info64
//   type: 64-bit full descriptor; bits 32-47 carry C++ modifiers
// (references, constexpr, virtual, etc.), bits 48-63 are reserved for
// user payload.
typedef uint64_t d_type_info64;

/*============================================================================*
 *                         BIT LAYOUT — BASE (bits 0-15)                      *
 *============================================================================*/

// D_TYPE_KIND_SHIFT / D_TYPE_KIND_BITS / D_TYPE_KIND_MASK
//   macro: position, width, and extraction mask for the kind-flag nibble
// (bits 0-3).
#define D_TYPE_KIND_SHIFT        0u
#define D_TYPE_KIND_BITS         4u
#define D_TYPE_KIND_MASK                                                    \
    ((d_type_info16)((1u << D_TYPE_KIND_BITS) - 1u))

// D_TYPE_SUB_SHIFT / D_TYPE_SUB_BITS / D_TYPE_SUB_MASK
//   macro: position, width, and extraction mask for the subtype nibble
// (bits 4-7).  Holds a primitive ID (0-15) or compound kind (0-5).
#define D_TYPE_SUB_SHIFT         4u
#define D_TYPE_SUB_BITS          4u
#define D_TYPE_SUB_MASK                                                     \
    ( (d_type_info16)(((1u << D_TYPE_SUB_BITS) - 1u) << D_TYPE_SUB_SHIFT) )

// D_TYPE_SIGNED_SHIFT / D_TYPE_SIGNED_BIT
//   macro: bit 8 — set when a primitive type is signed.  Aliases
// D_TYPE_CTX0 because bit 8 doubles as the first context bit for
// non-primitive types.
#define D_TYPE_SIGNED_SHIFT      8u
#define D_TYPE_SIGNED_BIT        ((d_type_info16)(1u << D_TYPE_SIGNED_SHIFT))

// D_TYPE_CTX0_SHIFT / D_TYPE_CTX0_BIT
//   macro: context-specific bit 0 (bit 8).
#define D_TYPE_CTX0_SHIFT        8u
#define D_TYPE_CTX0_BIT          ((d_type_info16)(1u << D_TYPE_CTX0_SHIFT))

// D_TYPE_CTX1_SHIFT / D_TYPE_CTX1_BIT
//   macro: context-specific bit 1 (bit 9).
#define D_TYPE_CTX1_SHIFT        9u
#define D_TYPE_CTX1_BIT          ((d_type_info16)(1u << D_TYPE_CTX1_SHIFT))

// D_TYPE_CTX2_SHIFT / D_TYPE_CTX2_BIT
//   macro: context-specific bit 2 (bit 10).
#define D_TYPE_CTX2_SHIFT        10u
#define D_TYPE_CTX2_BIT          ((d_type_info16)(1u << D_TYPE_CTX2_SHIFT))

// D_TYPE_CTX_MASK
//   macro: 3-bit mask spanning the context-specific field (bits 8-10).
#define D_TYPE_CTX_MASK          ((d_type_info16)(0x7u << D_TYPE_CTX0_SHIFT))

// D_TYPE_CONST_SHIFT / D_TYPE_CONST_BIT
//   macro: bit 11 — const qualifier.
#define D_TYPE_CONST_SHIFT       11u
#define D_TYPE_CONST_BIT         ((d_type_info16)(1u << D_TYPE_CONST_SHIFT))

// D_TYPE_VOLATILE_SHIFT / D_TYPE_VOLATILE_BIT
//   macro: bit 12 — volatile qualifier.
#define D_TYPE_VOLATILE_SHIFT    12u
#define D_TYPE_VOLATILE_BIT      ((d_type_info16)(1u << D_TYPE_VOLATILE_SHIFT))

// D_TYPE_RSV_SHIFT / D_TYPE_RSV_BIT
//   macro: bit 13 — reserved for future use.
#define D_TYPE_RSV_SHIFT         13u
#define D_TYPE_RSV_BIT           ((d_type_info16)(1u << D_TYPE_RSV_SHIFT))

// D_TYPE_EXT_SHIFT / D_TYPE_EXT_BIT
//   macro: bit 14 — signals that extended info follows.
#define D_TYPE_EXT_SHIFT         14u
#define D_TYPE_EXT_BIT           ((d_type_info16)(1u << D_TYPE_EXT_SHIFT))

// D_TYPE_CUSTOM_SHIFT / D_TYPE_CUSTOM_BIT
//   macro: bit 15 — marks a custom or framework-defined type.
#define D_TYPE_CUSTOM_SHIFT      15u
#define D_TYPE_CUSTOM_BIT        ((d_type_info16)(1u << D_TYPE_CUSTOM_SHIFT))

/*============================================================================*
 *                         BIT LAYOUT — POINTER DEPTH (bits 16-23)            *
 *============================================================================*/

// D_TYPE_PTR_DEPTH_SHIFT / D_TYPE_PTR_DEPTH_BITS
//   macro: position and width of the pointer-depth byte inside a 32-bit
// or 64-bit descriptor.
#define D_TYPE_PTR_DEPTH_SHIFT   16u
#define D_TYPE_PTR_DEPTH_BITS    8u

// D_TYPE_PTR_DEPTH_MASK_U64
//   macro: 64-bit mask isolating the pointer-depth byte so the same mask
// works on both d_type_info32 and d_type_info64.
#define D_TYPE_PTR_DEPTH_MASK_U64 ((uint64_t)0xFFu << D_TYPE_PTR_DEPTH_SHIFT)

// D_TYPE_PTR_DEPTH_MAX
//   constant: maximum encodable pointer depth (8-bit field).
#define D_TYPE_PTR_DEPTH_MAX     255u

/*============================================================================*
 *                         BIT LAYOUT — USER TYPE ID (bits 48-63)             *
 *============================================================================*/

// D_TYPE_USER_ID_SHIFT / D_TYPE_USER_ID_BITS
//   macro: position and width of the user type ID field inside a
// d_type_info64.  Only meaningful when the CUSTOM bit (15) is set.
#define D_TYPE_USER_ID_SHIFT     48u
#define D_TYPE_USER_ID_BITS      16u

// D_TYPE_USER_ID_MASK
//   macro: 64-bit mask isolating the user type ID field (bits 48-63).
#define D_TYPE_USER_ID_MASK      ((uint64_t)0xFFFFull << D_TYPE_USER_ID_SHIFT)

// D_TYPE_USER_ID_MAX
//   constant: maximum encodable user type ID (16-bit field).
#define D_TYPE_USER_ID_MAX       65535u

/*============================================================================*
 *                         X-MACRO TABLES                                     *
 *============================================================================*/

// D_TYPE_KIND_X
//   macro: X-macro table of kind flags.  Each entry is X(name, bit_index);
// the corresponding bit value is (1 << bit_index), stored in the low nibble.
// Flags are combinable (e.g. POINTER | ARRAY).
#define D_TYPE_KIND_X(X)                                                    \
    X(PRIMITIVE,  0)                                                        \
    X(POINTER,    1)                                                        \
    X(ARRAY,      2)                                                        \
    X(TYPEDEF,    3)

// D_TYPE_PRIMITIVE_X
//   macro: X-macro table of primitive types.  Each entry is
// X(name, id, c_type, sizeof_expr, is_signed).  IDs 0-15 fit the 4-bit
// subtype field.
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

// D_TYPE_COMPOUND_X
//   macro: X-macro table of compound subtype IDs (used when the PRIMITIVE
// flag is NOT set).  Each entry is X(name, id).
#define D_TYPE_COMPOUND_X(X)                                                \
    X(NONE,      0)                                                         \
    X(STRUCT,    1)                                                         \
    X(UNION,     2)                                                         \
    X(ENUM,      3)                                                         \
    X(FUNCTION,  4)                                                         \
    X(CUSTOM,    5)

/*============================================================================*
 *                         CONSTANTS (NO ENUMS)                               *
 *============================================================================*/

// kind bit values (bits 0-3) — each is a single-bit flag
#define D_TYPE_PRIMITIVE   ((d_type_info16)(1u << 0))
#define D_TYPE_POINTER     ((d_type_info16)(1u << 1))
#define D_TYPE_ARRAY       ((d_type_info16)(1u << 2))
#define D_TYPE_TYPEDEF     ((d_type_info16)(1u << 3))
#define D_TYPE_KIND_MASK_  D_TYPE_KIND_MASK

// D_TYPE_PRIM_VOID .. D_TYPE_PRIM_LDOUBLE
//   constant: numeric IDs for each primitive type (0-15).
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

// D_TYPE_PRIM_COUNT
//   constant: total number of primitive type IDs.
#define D_TYPE_PRIM_COUNT    16u

// compound IDs — stored in the subtype nibble when PRIMITIVE is clear
#define D_TYPE_NONE          0u
#define D_TYPE_STRUCT        1u
#define D_TYPE_UNION         2u
#define D_TYPE_ENUM          3u
#define D_TYPE_FUNCTION      4u
#define D_TYPE_CUSTOM        5u

/*============================================================================*
 *                         PRIMITIVE SIZE / SIGNED TABLES                     *
 *============================================================================*/

// d_type_prim_sizes
//   constant: byte sizes indexed by primitive ID.
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
    sizeof(long),               // LONG
    sizeof(unsigned long),      // ULONG
    sizeof(long long),          // LLONG
    sizeof(unsigned long long), // ULLONG
    sizeof(float),              // FLOAT
    sizeof(double),             // DOUBLE
    sizeof(long double)         // LDOUBLE
};

// D_TYPE_PRIM_SIGNED_MASK
//   macro: bitmask with one bit per primitive ID; bit N is set when
// primitive N is signed.
#define D_TYPE_PRIM_SIGNED_MASK                                             \
    ( (uint16_t)                                                            \
      (                                                                     \
         (1u << D_TYPE_PRIM_SCHAR)   |                                      \
         (1u << D_TYPE_PRIM_SHORT)   |                                      \
         (1u << D_TYPE_PRIM_INT)     |                                      \
         (1u << D_TYPE_PRIM_LONG)    |                                      \
         (1u << D_TYPE_PRIM_LLONG)   |                                      \
         (1u << D_TYPE_PRIM_FLOAT)   |                                      \
         (1u << D_TYPE_PRIM_DOUBLE)  |                                      \
         (1u << D_TYPE_PRIM_LDOUBLE)                                        \
    ) )

/*============================================================================*
 *                         BUILDER MACROS                                     *
 *============================================================================*/

// D_TYPE_MAKE
//   macro: build a raw d_type_info16 from a kind flag set and a 4-bit
// subtype value.
#define D_TYPE_MAKE(kind, subtype)                                          \
    ((d_type_info16)(                                                       \
        ((d_type_info16)((kind) & D_TYPE_KIND_MASK)) |                      \
        ((d_type_info16)(((subtype) & 0x0Fu) << D_TYPE_SUB_SHIFT))          \
    ))

// D_TYPE_MAKE_PRIM
//   macro: build a d_type_info16 for a primitive, automatically setting
// the PRIMITIVE kind flag, the subtype nibble, and the signed bit from
// D_TYPE_PRIM_SIGNED_MASK.
#define D_TYPE_MAKE_PRIM(prim_id)                                               \
    ( (d_type_info16)(                                                          \
        D_TYPE_PRIMITIVE |                                                      \
        (((prim_id) & 0x0Fu) << D_TYPE_SUB_SHIFT) |                             \
        (((D_TYPE_PRIM_SIGNED_MASK >> (prim_id)) & 1u) << D_TYPE_SIGNED_SHIFT)  \
    ) )

// D_TYPE_MAKE_COMPOUND
//   macro: build a d_type_info16 for a compound type; PRIMITIVE flag is
// NOT set.
#define D_TYPE_MAKE_COMPOUND(compound_kind)                                     \
    ((d_type_info16)(((compound_kind) & 0x0Fu) << D_TYPE_SUB_SHIFT))

// D_TYPE_VOID_() .. D_TYPE_LDOUBLE_()
//   macro: convenience builders for each primitive type.
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

// D_TYPE_STRUCT_() .. D_TYPE_CUSTOM_()
//   macro: convenience builders for each compound kind.
#define D_TYPE_STRUCT_()    D_TYPE_MAKE_COMPOUND(D_TYPE_STRUCT)
#define D_TYPE_UNION_()     D_TYPE_MAKE_COMPOUND(D_TYPE_UNION)
#define D_TYPE_ENUM_()      D_TYPE_MAKE_COMPOUND(D_TYPE_ENUM)
#define D_TYPE_FUNCTION_()  D_TYPE_MAKE_COMPOUND(D_TYPE_FUNCTION)
#define D_TYPE_CUSTOM_()    D_TYPE_MAKE_COMPOUND(D_TYPE_CUSTOM)

/*============================================================================*
 *                         MODIFIER MACROS — BASE (16-bit)                    *
 *============================================================================*/

// D_TYPE_SET_ARRAY .. D_TYPE_SET_FRAMEWORK
//   macro: set individual flag bits on a d_type_info16.
#define D_TYPE_SET_ARRAY(info16)        ((d_type_info16)((info16) | D_TYPE_ARRAY))
#define D_TYPE_SET_TYPEDEF(info16)      ((d_type_info16)((info16) | D_TYPE_TYPEDEF))
#define D_TYPE_SET_EXT(info16)          ((d_type_info16)((info16) | D_TYPE_EXT_BIT))
#define D_TYPE_SET_CUSTOM(info16)       ((d_type_info16)((info16) | D_TYPE_CUSTOM_BIT))
#define D_TYPE_SET_FRAMEWORK(info16)    D_TYPE_SET_CUSTOM(info16)

// D_TYPE_SET_CONST / D_TYPE_SET_VOLATILE
//   macro: apply cv-qualifiers.
#define D_TYPE_SET_CONST(info16)        ((d_type_info16)((info16) | D_TYPE_CONST_BIT))
#define D_TYPE_SET_VOLATILE(info16)     ((d_type_info16)((info16) | D_TYPE_VOLATILE_BIT))

/*============================================================================*
 *                         MODIFIER MACROS — POINTER (32-bit)                 *
 *============================================================================*/

// D_TYPE_MAKE_PTR
//   macro: combine a 16-bit base descriptor with a pointer depth (1-255)
// into a d_type_info32, setting the POINTER kind flag.
#define D_TYPE_MAKE_PTR(base16, depth)                                      \
    ((d_type_info32)(                                                       \
        ((uint32_t)((base16) & 0xFFFFu)) |                                  \
        (uint32_t)D_TYPE_POINTER |                                          \
        (((uint32_t)((depth) & 0xFFu)) << D_TYPE_PTR_DEPTH_SHIFT)           \
    ))

// D_TYPE_GET_BASE
//   macro: extract the low 16 bits (the base descriptor) from any wider
// descriptor.
#define D_TYPE_GET_BASE(info)                                               \
    ((d_type_info16)((info) & 0xFFFFu))

// D_TYPE_GET_PTR_DEPTH
//   macro: return the pointer depth.  Yields 0 when the POINTER flag is
// clear; yields at least 1 when POINTER is set (treats a zero depth byte
// as depth 1 for backward compatibility).
#define D_TYPE_GET_PTR_DEPTH(info)                                          \
    ( (( (info) & D_TYPE_POINTER ) == 0)                                    \
        ? 0u                                                                \
        : ( (unsigned)(((uint64_t)(info) & D_TYPE_PTR_DEPTH_MASK_U64) >> D_TYPE_PTR_DEPTH_SHIFT) \
            ? (unsigned)(((uint64_t)(info) & D_TYPE_PTR_DEPTH_MASK_U64) >> D_TYPE_PTR_DEPTH_SHIFT) \
            : 1u ) )

// D_TYPE_SET_PTR_DEPTH
//   macro: replace the pointer depth of an existing descriptor.  A depth
// of 0 clears the POINTER flag entirely.
#define D_TYPE_SET_PTR_DEPTH(info, depth)                                          \
    ( ((depth) == 0u)                                                              \
        ? (d_type_info32)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER)  \
        : D_TYPE_MAKE_PTR(D_TYPE_GET_BASE(info), (depth)) )

// D_TYPE_ADD_PTR
//   macro: increment the pointer depth by one, clamping at
// D_TYPE_PTR_DEPTH_MAX.  If the descriptor is not yet a pointer, sets
// depth to 1.
#define D_TYPE_ADD_PTR(info)                                                       \
    ( D_TYPE_IS_POINTER(info)                                                      \
        ? D_TYPE_SET_PTR_DEPTH((info),                                             \
              (D_TYPE_GET_PTR_DEPTH(info) >= D_TYPE_PTR_DEPTH_MAX                  \
                  ? D_TYPE_PTR_DEPTH_MAX                                           \
                  : (D_TYPE_GET_PTR_DEPTH(info) + 1u)))                            \
        : D_TYPE_MAKE_PTR(D_TYPE_GET_BASE(info), 1u) )

// D_TYPE_SUB_PTR
//   macro: decrement the pointer depth by one.  When depth reaches zero
// the POINTER flag is cleared.
#define D_TYPE_SUB_PTR(info)                                                       \
    ( (D_TYPE_GET_PTR_DEPTH(info) > 1u)                                            \
        ? D_TYPE_SET_PTR_DEPTH((info), (D_TYPE_GET_PTR_DEPTH(info) - 1u))          \
        : (d_type_info16)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER) )

/*============================================================================*
 *                         ACCESSORS / TESTS                                  *
 *============================================================================*/

// field extractors
#define D_TYPE_GET_KIND(info)       ((d_type_info16)((info) & D_TYPE_KIND_MASK))
#define D_TYPE_GET_SUB(info)        ((unsigned)(((info) & D_TYPE_SUB_MASK) >> D_TYPE_SUB_SHIFT))
#define D_TYPE_GET_PRIM(info)       D_TYPE_GET_SUB(info)
#define D_TYPE_GET_COMPOUND(info)   D_TYPE_GET_SUB(info)

// kind tests
#define D_TYPE_IS_PRIMITIVE(info)   (((info) & D_TYPE_PRIMITIVE) != 0)
#define D_TYPE_IS_POINTER(info)     (((info) & D_TYPE_POINTER) != 0)
#define D_TYPE_IS_ARRAY(info)       (((info) & D_TYPE_ARRAY) != 0)
#define D_TYPE_IS_TYPEDEF(info)     (((info) & D_TYPE_TYPEDEF) != 0)

// flag tests
#define D_TYPE_HAS_EXT(info)        (((info) & D_TYPE_EXT_BIT) != 0)
#define D_TYPE_IS_CUSTOM(info)      (((info) & D_TYPE_CUSTOM_BIT) != 0)
#define D_TYPE_IS_FRAMEWORK(info)   D_TYPE_IS_CUSTOM(info)

// cv-qualifier tests
#define D_TYPE_IS_CONST(info)       (((info) & D_TYPE_CONST_BIT) != 0)
#define D_TYPE_IS_VOLATILE(info)    (((info) & D_TYPE_VOLATILE_BIT) != 0)
#define D_TYPE_IS_CV(info)          (((info) & (D_TYPE_CONST_BIT | D_TYPE_VOLATILE_BIT)) != 0)

// compound test — subtype is nonzero and PRIMITIVE flag is clear
#define D_TYPE_IS_COMPOUND(info)    (!D_TYPE_IS_PRIMITIVE(info) && (D_TYPE_GET_SUB(info) != D_TYPE_NONE))

// individual primitive tests
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

// category tests
#define D_TYPE_IS_INTEGER(info)     (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) >= D_TYPE_PRIM_SCHAR && D_TYPE_GET_PRIM(info) <= D_TYPE_PRIM_ULLONG)
#define D_TYPE_IS_FLOATING(info)    (D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_PRIM(info) >= D_TYPE_PRIM_FLOAT)
#define D_TYPE_IS_SIGNED(info)      (D_TYPE_IS_PRIMITIVE(info) && (((info) & D_TYPE_SIGNED_BIT) != 0))
#define D_TYPE_IS_UNSIGNED(info)    (D_TYPE_IS_PRIMITIVE(info) && (((info) & D_TYPE_SIGNED_BIT) == 0))
#define D_TYPE_IS_INTEGRAL(info)    (D_TYPE_IS_BOOL(info) || D_TYPE_IS_CHAR(info) || D_TYPE_IS_INTEGER(info))
#define D_TYPE_IS_ARITHMETIC(info)  (D_TYPE_IS_INTEGRAL(info) || D_TYPE_IS_FLOATING(info))

// compound kind tests
#define D_TYPE_IS_STRUCT(info)      (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_STRUCT)
#define D_TYPE_IS_UNION(info)       (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_UNION)
#define D_TYPE_IS_ENUM(info)        (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_ENUM)
#define D_TYPE_IS_FUNCTION(info)    (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_FUNCTION)
#define D_TYPE_IS_CUSTOM_KIND(info) (!D_TYPE_IS_PRIMITIVE(info) && D_TYPE_GET_COMPOUND(info) == D_TYPE_CUSTOM)

// C standard classification
#define D_TYPE_IS_SCALAR(info)      (D_TYPE_IS_ARITHMETIC(info) || D_TYPE_IS_POINTER(info) || D_TYPE_IS_ENUM(info))
#define D_TYPE_IS_AGGREGATE(info)   (D_TYPE_IS_ARRAY(info) || D_TYPE_IS_STRUCT(info) || D_TYPE_IS_UNION(info))

// D_TYPE_GET_SIZE
//   macro: return the byte size of the described type.  Pointers always
// yield sizeof(void*); primitives use the lookup table; compounds fall
// back to sizeof(void*).
#define D_TYPE_GET_SIZE(info)                                               \
    (D_TYPE_IS_POINTER(info)                                                \
        ? sizeof(void*)                                                     \
        : (D_TYPE_IS_PRIMITIVE(info)                                        \
            ? d_type_prim_sizes[D_TYPE_GET_PRIM(info)]                      \
            : sizeof(void*)))

/*============================================================================*
 *                         PREDEFINED D_TYPE_INFO_*                           *
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

// CV variants
#define D_TYPE_INFO_CONST_CHAR           D_TYPE_SET_CONST(D_TYPE_INFO_CHAR)
#define D_TYPE_INFO_CONST_STRING         D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1)
#define D_TYPE_INFO_CONST_STRING_ARRAY   D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 2)

// arrays (base element, EXT flag set)
#define D_TYPE_INFO_CHAR_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_CHAR))
#define D_TYPE_INFO_INT_ARRAY            D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT))
#define D_TYPE_INFO_BOOL_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_BOOL))

// pointers (depth=1)
#define D_TYPE_INFO_VOID_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_VOID, 1)
#define D_TYPE_INFO_BOOL_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_BOOL, 1)
#define D_TYPE_INFO_CHAR_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_CHAR, 1)
#define D_TYPE_INFO_INT_PTR              D_TYPE_MAKE_PTR(D_TYPE_INFO_INT, 1)

// arrays of pointers
#define D_TYPE_INFO_CHAR_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_CHAR, 1)))
#define D_TYPE_INFO_INT_PTR_ARRAY        D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_INT, 1)))
#define D_TYPE_INFO_BOOL_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_BOOL, 1)))
#define D_TYPE_INFO_VOID_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_VOID, 1)))

// fixed-width aliases — map stdint names to the underlying primitive
#define D_TYPE_INFO_INT8                 D_TYPE_INFO_SCHAR
#define D_TYPE_INFO_INT16                D_TYPE_INFO_SHORT
#define D_TYPE_INFO_INT32                D_TYPE_INFO_INT
#define D_TYPE_INFO_INT64                D_TYPE_INFO_LLONG
#define D_TYPE_INFO_UINT8                D_TYPE_INFO_UCHAR
#define D_TYPE_INFO_UINT16               D_TYPE_INFO_USHORT
#define D_TYPE_INFO_UINT32               D_TYPE_INFO_UINT
#define D_TYPE_INFO_UINT64               D_TYPE_INFO_ULLONG

// fixed-width arrays
#define D_TYPE_INFO_INT8_ARRAY           D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT8))
#define D_TYPE_INFO_INT16_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT16))
#define D_TYPE_INFO_INT32_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT32))
#define D_TYPE_INFO_INT64_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT64))
#define D_TYPE_INFO_UINT8_ARRAY          D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT8))
#define D_TYPE_INFO_UINT16_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT16))
#define D_TYPE_INFO_UINT32_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT32))
#define D_TYPE_INFO_UINT64_ARRAY         D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT64))

// fixed-width pointers (depth=1)
#define D_TYPE_INFO_INT8_PTR             D_TYPE_MAKE_PTR(D_TYPE_INFO_INT8, 1)
#define D_TYPE_INFO_INT16_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT16, 1)
#define D_TYPE_INFO_INT32_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT32, 1)
#define D_TYPE_INFO_INT64_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_INT64, 1)
#define D_TYPE_INFO_UINT8_PTR            D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT8, 1)
#define D_TYPE_INFO_UINT16_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT16, 1)
#define D_TYPE_INFO_UINT32_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT32, 1)
#define D_TYPE_INFO_UINT64_PTR           D_TYPE_MAKE_PTR(D_TYPE_INFO_UINT64, 1)

// fixed-width pointer arrays
#define D_TYPE_INFO_INT8_PTR_ARRAY       D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT8_PTR))
#define D_TYPE_INFO_INT16_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT16_PTR))
#define D_TYPE_INFO_INT32_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT32_PTR))
#define D_TYPE_INFO_INT64_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_INT64_PTR))
#define D_TYPE_INFO_UINT8_PTR_ARRAY      D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT8_PTR))
#define D_TYPE_INFO_UINT16_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT16_PTR))
#define D_TYPE_INFO_UINT32_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT32_PTR))
#define D_TYPE_INFO_UINT64_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_INFO_UINT64_PTR))

// string / string array (const char*, const char**)
#define D_TYPE_INFO_STRING               D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1)
#define D_TYPE_INFO_STRING_ARRAY         D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 2)
#define D_TYPE_INFO_STRING_PTR_ARRAY     D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR(D_TYPE_INFO_CONST_CHAR, 1)))

// size_t — resolved to the correct width at compile time
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
    // fallback: treat as uintptr_t-ish
    #define D_TYPE_INFO_SIZE_T           D_TYPE_INFO_UINT64
    #define D_TYPE_INFO_SIZE_T_PTR       D_TYPE_INFO_UINT64_PTR
    #define D_TYPE_INFO_SIZE_T_ARRAY     D_TYPE_INFO_UINT64_ARRAY
    #define D_TYPE_INFO_SIZE_T_PTR_ARRAY D_TYPE_INFO_UINT64_PTR_ARRAY
#endif

/*============================================================================*
 *                         COMPOSITE TYPE BUILDERS                            *
 *============================================================================*/

// D_TYPE_PTR_TO .. D_TYPE_FW
//   macro: high-level helpers that compose the lower-level modifier
// macros into common patterns.
#define D_TYPE_PTR_TO(base_info)                                              \
    D_TYPE_ADD_PTR(base_info)

#define D_TYPE_ARRAY_OF(elem_info)                                            \
    D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(elem_info))

#define D_TYPE_CONST_OF(info)                                                 \
    D_TYPE_SET_CONST(info)

#define D_TYPE_VOLATILE_OF(info)                                              \
    D_TYPE_SET_VOLATILE(info)

#define D_TYPE_CV_OF(info)                                                    \
    D_TYPE_SET_CONST(D_TYPE_SET_VOLATILE(info))

#define D_TYPE_CUSTOM_OF(info)                                                \
    D_TYPE_SET_CUSTOM(info)

#define D_TYPE_FW(info)                                                       \
    D_TYPE_SET_CUSTOM(info)

// D_TYPE_PTR_ARRAY
//   macro: build an "array of pointers" descriptor.
#define D_TYPE_PTR_ARRAY(elem_info, depth) \
    D_TYPE_SET_EXT(D_TYPE_SET_ARRAY(D_TYPE_MAKE_PTR((elem_info), (depth))))

// D_TYPE_CONST_PTR
//   macro: pointer to const T  (const T *).
#define D_TYPE_CONST_PTR(elem_info)                                           \
    D_TYPE_ADD_PTR(D_TYPE_SET_CONST(elem_info))

// D_TYPE_PTR_CONST
//   macro: const pointer to T  (T * const).
#define D_TYPE_PTR_CONST(elem_info)                                           \
    D_TYPE_SET_CONST(D_TYPE_ADD_PTR(elem_info))

// D_TYPE_TYPEDEF_OF
//   macro: mark a descriptor as a typedef.
#define D_TYPE_TYPEDEF_OF(info)                                               \
    D_TYPE_SET_TYPEDEF(info)

/*============================================================================*
 *                         USER TYPE ID (bits 48-63)                          *
 *============================================================================*/

// D_TYPE_HAS_USER_ID
//   macro: true when the descriptor carries a user type ID (i.e. the
// CUSTOM bit is set).
#define D_TYPE_HAS_USER_ID(info)                                              \
    (D_TYPE_IS_CUSTOM(D_TYPE_GET_BASE(info)))

// D_TYPE_GET_USER_ID
//   macro: extract the 16-bit user type ID from a d_type_info64.
// Returns 0 when CUSTOM is not set.
#define D_TYPE_GET_USER_ID(info)                                              \
    ( D_TYPE_HAS_USER_ID(info)                                                \
        ? (unsigned)(((uint64_t)(info) & D_TYPE_USER_ID_MASK)                 \
                     >> D_TYPE_USER_ID_SHIFT)                                 \
        : 0u )

// D_TYPE_SET_USER_ID
//   macro: set or replace the user type ID on an existing d_type_info64,
// automatically setting the CUSTOM bit.
#define D_TYPE_SET_USER_ID(info, id)                                          \
    ((d_type_info64)(                                                         \
        ((uint64_t)(info) & ~D_TYPE_USER_ID_MASK)                             \
        | ((uint64_t)D_TYPE_CUSTOM_BIT)                                       \
        | (((uint64_t)((id) & 0xFFFFu)) << D_TYPE_USER_ID_SHIFT)             \
    ))

// D_TYPE_MAKE_USER
//   macro: build a complete d_type_info64 for a user-defined compound
// type from scratch.  `compound_kind` is one of D_TYPE_STRUCT,
// D_TYPE_UNION, D_TYPE_ENUM, etc.  `user_id` is the caller's type ID
// (0-65535).
//   Example:
//     d_type_info64 my_struct = D_TYPE_MAKE_USER(D_TYPE_STRUCT, 42);
//     d_type_info64 my_enum   = D_TYPE_MAKE_USER(D_TYPE_ENUM, 7);
#define D_TYPE_MAKE_USER(compound_kind, user_id)                              \
    ((d_type_info64)(                                                         \
        (uint64_t)D_TYPE_MAKE_COMPOUND(compound_kind)                         \
        | (uint64_t)D_TYPE_CUSTOM_BIT                                         \
        | (((uint64_t)((user_id) & 0xFFFFu)) << D_TYPE_USER_ID_SHIFT)        \
    ))

// D_TYPE_MAKE_USER_PTR
//   macro: build a d_type_info64 for a pointer to a user-defined compound
// type.  Combines user type ID with pointer depth.
//   Example:
//     d_type_info64 p = D_TYPE_MAKE_USER_PTR(D_TYPE_STRUCT, 42, 1);
//     // struct #42 *
#define D_TYPE_MAKE_USER_PTR(compound_kind, user_id, depth)                   \
    ((d_type_info64)(                                                         \
        (uint64_t)D_TYPE_MAKE_COMPOUND(compound_kind)                         \
        | (uint64_t)D_TYPE_CUSTOM_BIT                                         \
        | (uint64_t)D_TYPE_POINTER                                            \
        | (((uint64_t)((depth) & 0xFFu)) << D_TYPE_PTR_DEPTH_SHIFT)          \
        | (((uint64_t)((user_id) & 0xFFFFu)) << D_TYPE_USER_ID_SHIFT)        \
    ))

// D_TYPE_STRIP_USER_ID
//   macro: clear the user type ID field and the CUSTOM bit, yielding the
// underlying compound descriptor.
#define D_TYPE_STRIP_USER_ID(info)                                            \
    ((d_type_info64)(                                                         \
        (uint64_t)(info) & ~(D_TYPE_USER_ID_MASK | (uint64_t)D_TYPE_CUSTOM_BIT) \
    ))

/*============================================================================*
 *                         EXTENDED INFO STRUCTURES                           *
 *============================================================================*/

// d_type_array_ext
//   type: variable-length descriptor for multi-dimensional array types.
// The base field carries the ARRAY flag; ndims and dims[] describe the
// shape.
struct d_type_array_ext
{
    d_type_info16 base;     // base info (with ARRAY flag)
    uint8_t       ndims;    // number of dimensions
    uint32_t      dims[1];  // flexible dims (actual length = ndims)
};

// d_type_struct_field
//   type: per-field record inside a struct descriptor.
struct d_type_struct_field
{
    uint16_t      offset;   // byte offset within the struct
    uint16_t      size;     // byte size of this field
    d_type_info16 type;     // type descriptor for the field
};

// d_type_struct_ext
//   type: variable-length descriptor for struct/union types, carrying
// field layout information.
struct d_type_struct_ext
{
    d_type_info16              base;        // base info
    uint16_t                   total_size;  // sizeof the struct
    uint8_t                    nfields;     // number of fields
    struct d_type_struct_field fields[1];   // flexible (length = nfields)
};

// d_type_func_ext
//   type: variable-length descriptor for function types, carrying return
// type and parameter list.
struct d_type_func_ext
{
    d_type_info16 base;      // base info (compound FUNCTION)
    d_type_info16 ret_type;  // return type descriptor
    uint8_t       nparam;    // number of parameters
    d_type_info16 params[1]; // flexible (length = nparam)
};

/*============================================================================*
 *                         UTILITY / DEBUG                                    *
 *============================================================================*/

// D_TYPE_STRIP_CV
//   macro: clear both const and volatile bits.
#define D_TYPE_STRIP_CV(info)                                                 \
    ((d_type_info16)((info) & ~(D_TYPE_CONST_BIT | D_TYPE_VOLATILE_BIT)))

// D_TYPE_STRIP_PTR
//   macro: decrement pointer depth by one (alias for D_TYPE_SUB_PTR).
#define D_TYPE_STRIP_PTR(info)        D_TYPE_SUB_PTR(info)

// D_TYPE_STRIP_ALL_PTR
//   macro: remove the pointer flag entirely, yielding the base non-pointer
// descriptor.
#define D_TYPE_STRIP_ALL_PTR(info)    ((d_type_info16)(D_TYPE_GET_BASE(info) & (d_type_info16)~D_TYPE_POINTER))

// D_TYPE_BASE
//   macro: extract only the core identity (kind + subtype + signed bit),
// stripping cv-qualifiers and upper flags.
#define D_TYPE_BASE(info)                                                     \
    ((d_type_info16)((info) & (D_TYPE_PRIMITIVE | D_TYPE_SUB_MASK | D_TYPE_SIGNED_BIT)))

// D_TYPE_EQ / D_TYPE_EQ_BASE / D_TYPE_COMPAT
//   macro: comparison helpers.  EQ is exact; EQ_BASE compares only the low
// 16 bits; COMPAT compares the core identity (ignoring qualifiers).
#define D_TYPE_EQ(a, b)          ((a) == (b))
#define D_TYPE_EQ_BASE(a, b)     (((a) & 0xFFFFu) == ((b) & 0xFFFFu))
#define D_TYPE_COMPAT(a, b)      (D_TYPE_BASE(a) == D_TYPE_BASE(b))

// D_TYPE_TO_BASE / D_TYPE_TO_PTR
//   macro: narrowing / widening casts between descriptor widths.
#define D_TYPE_TO_BASE(info)     ((d_type_info16)((info) & 0xFFFFu))
#define D_TYPE_TO_PTR(info)      ((d_type_info32)(info))

// D_TYPE_DUMP_BITS
//   macro: initializer list suitable for printf debugging; expands to
// { kind, sub, ptr_depth, signed, const, volatile, ext, custom }.
#define D_TYPE_DUMP_BITS(info)                                                \
{                                                                             \
    (unsigned)D_TYPE_GET_KIND(info),                                          \
    (unsigned)D_TYPE_GET_SUB(info),                                           \
    (unsigned)D_TYPE_GET_PTR_DEPTH(info),                                     \
    (unsigned)(D_TYPE_IS_PRIMITIVE(info) && D_TYPE_IS_SIGNED(info)),          \
    (unsigned)D_TYPE_IS_CONST(info),                                          \
    (unsigned)D_TYPE_IS_VOLATILE(info),                                       \
    (unsigned)D_TYPE_HAS_EXT(info),                                           \
    (unsigned)D_TYPE_IS_CUSTOM(info)                                          \
}


#endif  // DJINTERP_C_TYPE_INFO_COMMON_
