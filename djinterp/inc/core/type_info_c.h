/******************************************************************************
* djinterp [core]                                              type_info_c.h
*
*  C-specific extensions to the common type-information system.
*  Covers:
*  - extended (d_type_info_ex) storage-class / qualifier bits (bits 24-31):
*      bit 24: restrict
*      bit 25: _Atomic
*      bit 26: static
*      bit 27: extern
*      bit 28: inline
*      bit 29: _Noreturn
*  - SET macros for each storage-class bit
*  - predefined `static const` type constants for use with _Generic
*  - C11 _Generic-based type detection (D_TYPE_OF_EXPR / D_TYPE_OF_TYPE)
*
* path:      /inc/c/type_info_c.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.06
******************************************************************************/

#ifndef DJINTERP_C_TYPE_INFO_C_
#define DJINTERP_C_TYPE_INFO_C_ 1

#include "type_info_common.h"


/*============================================================================*
 *                  BIT LAYOUT — C STORAGE CLASS (bits 24-31)                 *
 *============================================================================*/

// D_TYPE_RESTRICT_SHIFT / D_TYPE_RESTRICT_BIT
//   macro: bit 24 — C99 `restrict` qualifier.
#define D_TYPE_RESTRICT_SHIFT    24u
#define D_TYPE_RESTRICT_BIT      (1ull << D_TYPE_RESTRICT_SHIFT)

// D_TYPE_ATOMIC_SHIFT / D_TYPE_ATOMIC_BIT
//   macro: bit 25 — C11 `_Atomic` qualifier.
#define D_TYPE_ATOMIC_SHIFT      25u
#define D_TYPE_ATOMIC_BIT        (1ull << D_TYPE_ATOMIC_SHIFT)

// D_TYPE_STATIC_SHIFT / D_TYPE_STATIC_BIT
//   macro: bit 26 — `static` storage class.
#define D_TYPE_STATIC_SHIFT      26u
#define D_TYPE_STATIC_BIT        (1ull << D_TYPE_STATIC_SHIFT)

// D_TYPE_EXTERN_SHIFT / D_TYPE_EXTERN_BIT
//   macro: bit 27 — `extern` storage class.
#define D_TYPE_EXTERN_SHIFT      27u
#define D_TYPE_EXTERN_BIT        (1ull << D_TYPE_EXTERN_SHIFT)

// D_TYPE_INLINE_SHIFT / D_TYPE_INLINE_BIT
//   macro: bit 28 — `inline` function specifier.
#define D_TYPE_INLINE_SHIFT      28u
#define D_TYPE_INLINE_BIT        (1ull << D_TYPE_INLINE_SHIFT)

// D_TYPE_NORETURN_SHIFT / D_TYPE_NORETURN_BIT
//   macro: bit 29 — C11 `_Noreturn` function specifier.
#define D_TYPE_NORETURN_SHIFT    29u
#define D_TYPE_NORETURN_BIT      (1ull << D_TYPE_NORETURN_SHIFT)

/*============================================================================*
 *                  MODIFIER MACROS — C STORAGE CLASS                         *
 *============================================================================*/

// D_TYPE_SET_RESTRICT .. D_TYPE_SET_NORETURN
//   macro: apply the corresponding storage-class / qualifier bit to a
// d_type_info_ex descriptor.
#define D_TYPE_SET_RESTRICT(info)   ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_RESTRICT_BIT))
#define D_TYPE_SET_ATOMIC(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_ATOMIC_BIT))
#define D_TYPE_SET_STATIC(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_STATIC_BIT))
#define D_TYPE_SET_EXTERN(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_EXTERN_BIT))
#define D_TYPE_SET_INLINE(info)     ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_INLINE_BIT))
#define D_TYPE_SET_NORETURN(info)   ((d_type_info_ex)((info) | (d_type_info_ex)D_TYPE_NORETURN_BIT))

/*============================================================================*
 *                  PREDEFINED TYPE CONSTANTS (FOR _Generic)                  *
 *============================================================================*/

// D_INTERNAL_PRIM_CONST
//   macro: X-macro callback that expands each primitive entry into a
// `static const d_type_info16` constant named D_TYPE_C_<name>.
#define D_INTERNAL_PRIM_CONST(name, id, ctype, sz, sgn)                       \
    static const d_type_info16 D_TYPE_C_##name = D_TYPE_MAKE_PRIM(id);

D_TYPE_PRIMITIVE_X(D_INTERNAL_PRIM_CONST)

#undef D_INTERNAL_PRIM_CONST

// compound constants for _Generic default paths
static const d_type_info16 D_TYPE_C_STRUCT   = D_TYPE_STRUCT_();
static const d_type_info16 D_TYPE_C_UNION    = D_TYPE_UNION_();
static const d_type_info16 D_TYPE_C_ENUM     = D_TYPE_ENUM_();
static const d_type_info16 D_TYPE_C_FUNCTION = D_TYPE_FUNCTION_();
static const d_type_info16 D_TYPE_C_CUSTOM   = D_TYPE_CUSTOM_();

/*============================================================================*
 *                  _GENERIC TYPE DETECTION (C11)                             *
 *============================================================================*/

#if D_ENV_LANG_IS_C11_OR_HIGHER

// D_TYPE_OF_EXPR
//   macro: use C11 _Generic to map an expression's type to the matching
// d_type_info16 constant at compile time.  Returns 0 for unrecognised
// types.
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

// D_TYPE_OF_TYPE
//   macro: deduce the d_type_info16 for a type name by casting a zero
// literal and passing it through D_TYPE_OF_EXPR.
#define D_TYPE_OF_TYPE(T)   D_TYPE_OF_EXPR((T)0)

#endif // D_ENV_LANG_IS_C11_OR_HIGHER


#endif  // DJINTERP_C_TYPE_INFO_C_
