/******************************************************************************
* djinterp [core]                                                   cpp.hpp
*
* C++-specific language constants:
*   This header defines the symbol kinds, qualifier flags, and other
* classification constants specific to C++.  It extends common.hpp
* with C++-only declaration forms (namespaces, classes, methods,
* constructors, templates) and C++-only qualifier bits (virtual,
* constexpr, noexcept, final, override, etc.).
*
* Qualifier bit alignment:
*   The C++ qualifier flags in this header occupy bits 32-47 of a
* uint64_t, matching the bit positions defined in type_info_cpp.h.
* This means a symbol's qualifier field can be passed directly to
* the D_TYPE_IS_VIRTUAL / D_TYPE_SET_CONSTEXPR / etc. macros with
* no translation.  The correspondence is exact:
*
*     qualifier::lvalue_ref   = D_TYPE_LVALREF_BIT     (bit 32)
*     qualifier::rvalue_ref   = D_TYPE_RVALREF_BIT     (bit 33)
*     qualifier::mutable_     = D_TYPE_MUTABLE_BIT     (bit 34)
*     qualifier::virtual_     = D_TYPE_VIRTUAL_BIT     (bit 35)
*     qualifier::constexpr_   = D_TYPE_CONSTEXPR_BIT   (bit 36)
*     qualifier::noexcept_    = D_TYPE_NOEXCEPT_BIT    (bit 37)
*     qualifier::final_       = D_TYPE_FINAL_BIT       (bit 38)
*     qualifier::override_    = D_TYPE_OVERRIDE_BIT    (bit 39)
*     qualifier::explicit_    = D_TYPE_EXPLICIT_BIT    (bit 40)
*     qualifier::consteval_   = D_TYPE_CONSTEVAL_BIT   (bit 41)
*     qualifier::constinit_   = D_TYPE_CONSTINIT_BIT   (bit 42)
*     qualifier::template_    = D_TYPE_TEMPLATE_BIT    (bit 43)
*
* Dependencies:
*   Includes common.hpp (shared symbol kinds and qualifier flags).
*
*
* path:      /inc/cpp/lang/cpp.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_LANG_CPP_
#define DJINTERP_LANG_CPP_ 1

#include <cstdint>
#include "../core/djinterp.hpp"
#include "./common.hpp"


NS_DJINTERP
NS_LANG


// ================================================================
//  symbol_kind  (C++-only extensions, 0x00A0-0x00FF)
// ================================================================

// symbol_kind (continued)
//   C++-only declaration kinds.
namespace symbol_kind
{
    // namespaces
    constexpr std::uint16_t namespace_decl     = 0x00A0;

    // classes
    constexpr std::uint16_t class_decl         = 0x00A1;

    // methods
    constexpr std::uint16_t method_decl        = 0x00A2;
    constexpr std::uint16_t constructor_decl   = 0x00A3;
    constexpr std::uint16_t destructor_decl    = 0x00A4;

    // type aliasing (C++ uses 'using', distinct from C typedef)
    constexpr std::uint16_t type_alias_decl    = 0x00A5;

    // templates
    constexpr std::uint16_t template_decl      = 0x00A6;
};


// ================================================================
//  qualifier  (C++-only extensions, bits 32-47)
// ================================================================

// qualifier (continued)
//   C++ modifier bits occupying bits 32-47.  Each value matches
// the corresponding D_TYPE_*_BIT from type_info_cpp.h exactly.
namespace qualifier
{
    // --------------------------------------------------------
    //  references (bits 32-33)
    // --------------------------------------------------------
    constexpr std::uint64_t lvalue_ref   = (1ULL << 32);
    constexpr std::uint64_t rvalue_ref   = (1ULL << 33);

    // --------------------------------------------------------
    //  C++ specifiers (bits 34-43)
    // --------------------------------------------------------
    constexpr std::uint64_t mutable_     = (1ULL << 34);
    constexpr std::uint64_t virtual_     = (1ULL << 35);
    constexpr std::uint64_t constexpr_   = (1ULL << 36);
    constexpr std::uint64_t noexcept_    = (1ULL << 37);
    constexpr std::uint64_t final_       = (1ULL << 38);
    constexpr std::uint64_t override_    = (1ULL << 39);
    constexpr std::uint64_t explicit_    = (1ULL << 40);
    constexpr std::uint64_t consteval_   = (1ULL << 41);
    constexpr std::uint64_t constinit_   = (1ULL << 42);
    constexpr std::uint64_t template_    = (1ULL << 43);

    // pure virtual is virtual + a semantic marker;
    // we encode it as virtual | bit 44 (no type_info
    // counterpart — this is parser-internal)
    constexpr std::uint64_t pure_virtual = (1ULL << 44);

    // --------------------------------------------------------
    //  masks
    // --------------------------------------------------------
    constexpr std::uint64_t ref_mask     = (lvalue_ref | rvalue_ref);

    constexpr std::uint64_t cpp_modifier_mask =
        ( lvalue_ref | rvalue_ref  | mutable_    | virtual_   |
          constexpr_ | noexcept_   | final_      | override_  |
          explicit_  | consteval_  | constinit_  | template_  |
          pure_virtual );

    constexpr std::uint64_t all_mask =
        (cv_mask | c_storage_mask | cpp_modifier_mask);
};


// ================================================================
//  def_symbol_kind  (definition-internal, 0x0100-0x01FF)
// ================================================================

// def_symbol_kind
//   constants: extend symbol_kind for nodes that appear only
// inside function/method definition bodies (produced by
// cpp_def_parser).
namespace def_symbol_kind
{
    constexpr std::uint16_t function_def    = 0x0100;
    constexpr std::uint16_t call_expr       = 0x0101;
    constexpr std::uint16_t local_var       = 0x0102;
    constexpr std::uint16_t return_stmt     = 0x0103;
    constexpr std::uint16_t member_ref      = 0x0104;
    constexpr std::uint16_t decl_ref        = 0x0105;
};

// is_def_symbol_kind
//   returns true if a uint16_t kind value falls in the
// definition-internal range.
inline constexpr bool
is_def_symbol_kind
(
    std::uint16_t _kind
)
{
    return ( (_kind >= 0x0100) &&
             (_kind <= 0x01FF) );
}


// ================================================================
//  change_kind
// ================================================================

// change_kind
//   constants: classifies the nature of a source change.
// Used by the diff pipeline.
namespace change_kind
{
    constexpr std::uint8_t added     = 0x01;
    constexpr std::uint8_t removed   = 0x02;
    constexpr std::uint8_t modified  = 0x03;
    constexpr std::uint8_t moved     = 0x04;
    constexpr std::uint8_t renamed   = 0x05;
};


// ================================================================
//  C++ language version
// ================================================================

// cpp_standard
//   constants: C++ standard version identifiers.
namespace cpp_standard
{
    constexpr std::uint8_t cpp98  = 0x01;
    constexpr std::uint8_t cpp03  = 0x02;
    constexpr std::uint8_t cpp11  = 0x03;
    constexpr std::uint8_t cpp14  = 0x04;
    constexpr std::uint8_t cpp17  = 0x05;
    constexpr std::uint8_t cpp20  = 0x06;
    constexpr std::uint8_t cpp23  = 0x07;
};


NS_END  // lang
NS_END  // djinterp


#endif  // DJINTERP_LANG_CPP_
