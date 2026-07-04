/******************************************************************************
* djinterp [core]                                            type_info_cpp.h
*
*  C++-specific extensions to the common type-information system.
*  Covers bits 32-47 of d_type_info64:
*      bit 32: lvalue reference (&)             (C++98)
*      bit 33: rvalue reference (&&)            (C++11)
*      bit 34: mutable                          (C++98)
*      bit 35: virtual                          (C++98)
*      bit 36: constexpr                        (C++11)
*      bit 37: noexcept                         (C++11)
*      bit 38: final                            (C++11)
*      bit 39: override                         (C++11)
*      bit 40: explicit                         (C++98)
*      bit 41: consteval                        (C++20)
*      bit 42: constinit                        (C++20)
*      bit 43: template                         (C++98)
*
*  Bit definitions (shifts/masks) are always visible — they define the
*  encoding format.  SET macros are gated behind env.h / env_cpp_features.h
*  feature detection so that a modifier cannot be applied unless the
*  compiler actually supports the corresponding keyword.
*
*  Bits 48-63 carry the user type ID when CUSTOM (bit 15) is set;
*  see type_info_common.h for the user type ID macros.
*
* 
* path:      /inc/c/type_info_cpp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.12.06
******************************************************************************/

#ifndef DJINTERP_C_TYPE_INFO_CPP_
#define DJINTERP_C_TYPE_INFO_CPP_ 1

#if !D_ENV_LANG_USING_CPP
    #error "type_info_cpp.h requires a C++ compiler"
#endif

#include "type_info_common.h"
#include "env_cpp_features.h"


/*============================================================================*
 *                  BIT LAYOUT — C++ MODIFIERS (bits 32-47)                   *
 *============================================================================*/

// D_TYPE_LVALREF_SHIFT / D_TYPE_LVALREF_BIT
//   macro: bit 32 — lvalue reference qualifier (&).  (C++98)
#define D_TYPE_LVALREF_SHIFT     32u
#define D_TYPE_LVALREF_BIT       (1ull << D_TYPE_LVALREF_SHIFT)

// D_TYPE_RVALREF_SHIFT / D_TYPE_RVALREF_BIT
//   macro: bit 33 — rvalue reference qualifier (&&).  (C++11)
#define D_TYPE_RVALREF_SHIFT     33u
#define D_TYPE_RVALREF_BIT       (1ull << D_TYPE_RVALREF_SHIFT)

// D_TYPE_MUTABLE_SHIFT / D_TYPE_MUTABLE_BIT
//   macro: bit 34 — `mutable` storage specifier.  (C++98)
#define D_TYPE_MUTABLE_SHIFT     34u
#define D_TYPE_MUTABLE_BIT       (1ull << D_TYPE_MUTABLE_SHIFT)

// D_TYPE_VIRTUAL_SHIFT / D_TYPE_VIRTUAL_BIT
//   macro: bit 35 — `virtual` member function specifier.  (C++98)
#define D_TYPE_VIRTUAL_SHIFT     35u
#define D_TYPE_VIRTUAL_BIT       (1ull << D_TYPE_VIRTUAL_SHIFT)

// D_TYPE_CONSTEXPR_SHIFT / D_TYPE_CONSTEXPR_BIT
//   macro: bit 36 — `constexpr` specifier.  (C++11)
#define D_TYPE_CONSTEXPR_SHIFT   36u
#define D_TYPE_CONSTEXPR_BIT     (1ull << D_TYPE_CONSTEXPR_SHIFT)

// D_TYPE_NOEXCEPT_SHIFT / D_TYPE_NOEXCEPT_BIT
//   macro: bit 37 — `noexcept` specifier.  (C++11)
#define D_TYPE_NOEXCEPT_SHIFT    37u
#define D_TYPE_NOEXCEPT_BIT      (1ull << D_TYPE_NOEXCEPT_SHIFT)

// D_TYPE_FINAL_SHIFT / D_TYPE_FINAL_BIT
//   macro: bit 38 — `final` class/virtual specifier.  (C++11)
#define D_TYPE_FINAL_SHIFT       38u
#define D_TYPE_FINAL_BIT         (1ull << D_TYPE_FINAL_SHIFT)

// D_TYPE_OVERRIDE_SHIFT / D_TYPE_OVERRIDE_BIT
//   macro: bit 39 — `override` virtual specifier.  (C++11)
#define D_TYPE_OVERRIDE_SHIFT    39u
#define D_TYPE_OVERRIDE_BIT      (1ull << D_TYPE_OVERRIDE_SHIFT)

// D_TYPE_EXPLICIT_SHIFT / D_TYPE_EXPLICIT_BIT
//   macro: bit 40 — `explicit` constructor/conversion specifier.  (C++98;
// conditional explicit is C++20.)
#define D_TYPE_EXPLICIT_SHIFT    40u
#define D_TYPE_EXPLICIT_BIT      (1ull << D_TYPE_EXPLICIT_SHIFT)

// D_TYPE_CONSTEVAL_SHIFT / D_TYPE_CONSTEVAL_BIT
//   macro: bit 41 — C++20 `consteval` (immediate function) specifier.
#define D_TYPE_CONSTEVAL_SHIFT   41u
#define D_TYPE_CONSTEVAL_BIT     (1ull << D_TYPE_CONSTEVAL_SHIFT)

// D_TYPE_CONSTINIT_SHIFT / D_TYPE_CONSTINIT_BIT
//   macro: bit 42 — C++20 `constinit` specifier.
#define D_TYPE_CONSTINIT_SHIFT   42u
#define D_TYPE_CONSTINIT_BIT     (1ull << D_TYPE_CONSTINIT_SHIFT)

// D_TYPE_TEMPLATE_SHIFT / D_TYPE_TEMPLATE_BIT
//   macro: bit 43 — indicates a template entity.  (C++98)
#define D_TYPE_TEMPLATE_SHIFT    43u
#define D_TYPE_TEMPLATE_BIT      (1ull << D_TYPE_TEMPLATE_SHIFT)

/*============================================================================*
 *                  MODIFIER MACROS — C++98                                   *
 *============================================================================*/

// D_TYPE_SET_LVALREF / D_TYPE_SET_MUTABLE / D_TYPE_SET_VIRTUAL /
// D_TYPE_SET_EXPLICIT / D_TYPE_SET_TEMPLATE
//   macro: modifiers available in any conforming C++ implementation.
#define D_TYPE_SET_LVALREF(info)    ((d_type_info64)((info) | D_TYPE_LVALREF_BIT))
#define D_TYPE_SET_MUTABLE(info)    ((d_type_info64)((info) | D_TYPE_MUTABLE_BIT))
#define D_TYPE_SET_VIRTUAL(info)    ((d_type_info64)((info) | D_TYPE_VIRTUAL_BIT))
#define D_TYPE_SET_EXPLICIT(info)   ((d_type_info64)((info) | D_TYPE_EXPLICIT_BIT))
#define D_TYPE_SET_TEMPLATE(info)   ((d_type_info64)((info) | D_TYPE_TEMPLATE_BIT))

/*============================================================================*
 *                  MODIFIER MACROS — C++11                                   *
 *============================================================================*/

// D_TYPE_SET_RVALREF
//   macro: requires rvalue reference support (__cpp_rvalue_references).
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #define D_TYPE_SET_RVALREF(info)    ((d_type_info64)((info) | D_TYPE_RVALREF_BIT))
#endif

// D_TYPE_SET_CONSTEXPR
//   macro: requires constexpr support (__cpp_constexpr).
#if D_ENV_CPP_FEATURE_LANG_CONSTEXPR
    #define D_TYPE_SET_CONSTEXPR(info)  ((d_type_info64)((info) | D_TYPE_CONSTEXPR_BIT))
#endif

// D_TYPE_SET_NOEXCEPT / D_TYPE_SET_FINAL / D_TYPE_SET_OVERRIDE
//   macro: C++11 core keywords with no individual __cpp_* feature macro;
// gated on the detected C++ standard version.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_TYPE_SET_NOEXCEPT(info)   ((d_type_info64)((info) | D_TYPE_NOEXCEPT_BIT))
    #define D_TYPE_SET_FINAL(info)      ((d_type_info64)((info) | D_TYPE_FINAL_BIT))
    #define D_TYPE_SET_OVERRIDE(info)   ((d_type_info64)((info) | D_TYPE_OVERRIDE_BIT))
#endif

/*============================================================================*
 *                  MODIFIER MACROS — C++20                                   *
 *============================================================================*/

// D_TYPE_SET_CONSTEVAL
//   macro: requires consteval support (__cpp_consteval).
#if D_ENV_CPP_FEATURE_LANG_CONSTEVAL
    #define D_TYPE_SET_CONSTEVAL(info)  ((d_type_info64)((info) | D_TYPE_CONSTEVAL_BIT))
#endif

// D_TYPE_SET_CONSTINIT
//   macro: requires constinit support (__cpp_constinit).
#if D_ENV_CPP_FEATURE_LANG_CONSTINIT
    #define D_TYPE_SET_CONSTINIT(info)  ((d_type_info64)((info) | D_TYPE_CONSTINIT_BIT))
#endif


#endif  // DJINTERP_C_TYPE_INFO_CPP_
